/**
 * Overlay Processor — per-client claim + override stack manager
 */

#include "extension.h"
#include "overlay_manager.h"

#include <cstring>

OverlayManager g_Overlays;
bool g_bApplyingOverlay = false;

static void CopyString(char *dest, size_t destLen, const char *src)
{
	if (dest == nullptr || destLen == 0)
		return;
	if (src == nullptr)
	{
		dest[0] = '\0';
		return;
	}

	size_t i = 0;
	for (; i + 1 < destLen && src[i] != '\0'; i++)
		dest[i] = src[i];
	dest[i] = '\0';
}

static bool IsOffToken(const char *path)
{
	static const char kOff[] = "off";
	for (size_t i = 0; ; i++)
	{
		char a = path[i];
		char b = kOff[i];
		if (a >= 'A' && a <= 'Z')
			a = static_cast<char>(a - 'A' + 'a');
		if (a != b)
			return false;
		if (b == '\0')
			return true;
	}
}

OverlayManager::OverlayManager()
	: m_nextToken(1),
	  m_sequence(1),
	  m_legacyPriority(0),
	  m_debug(false)
{
	ResetAll();
}

void OverlayManager::ResetAll()
{
	for (int i = 0; i < OVERLAY_MAX_CLIENTS; i++)
	{
		ResetClient(i);
	}
}

void OverlayManager::ResetClient(int client)
{
	if (client < 0 || client >= OVERLAY_MAX_CLIENTS)
		return;

	m_claims[client].clear();
	m_stack[client].clear();
	m_applied[client][0] = '\0';
	m_hasApplied[client] = false;
}

bool OverlayManager::IsValidClient(int client) const
{
	return client > 0 && client < OVERLAY_MAX_CLIENTS;
}

int OverlayManager::AllocToken()
{
	int token = m_nextToken++;
	if (m_nextToken <= 0)
		m_nextToken = 1;
	return token;
}

void OverlayManager::SetLegacyPriority(int priority)
{
	m_legacyPriority = priority;
}

void OverlayManager::OnPluginUnloaded(int pluginSerial)
{
	for (int client = 1; client < OVERLAY_MAX_CLIENTS; client++)
	{
		bool changed = false;

		auto &claims = m_claims[client];
		for (size_t i = 0; i < claims.size(); )
		{
			if (claims[i].ownerPluginSerial == pluginSerial)
			{
				claims.erase(claims.begin() + static_cast<std::ptrdiff_t>(i));
				changed = true;
			}
			else
			{
				i++;
			}
		}

		auto &stack = m_stack[client];
		for (size_t i = 0; i < stack.size(); )
		{
			if (stack[i].ownerPluginSerial == pluginSerial)
			{
				stack.erase(stack.begin() + static_cast<std::ptrdiff_t>(i));
				changed = true;
			}
			else
			{
				i++;
			}
		}

		if (changed)
			Recompute(client);
	}
}

int OverlayManager::Set(int client, const char *path, int priority, int ownerPluginSerial)
{
	if (!IsValidClient(client) || path == nullptr)
		return 0;

	OverlayClaim claim;
	claim.token = AllocToken();
	claim.ownerPluginSerial = ownerPluginSerial;
	claim.priority = priority;
	claim.sequence = m_sequence++;
	CopyString(claim.path, sizeof(claim.path), path);

	m_claims[client].push_back(claim);
	Recompute(client);
	return claim.token;
}

bool OverlayManager::Clear(int client, int token)
{
	if (!IsValidClient(client) || token <= 0)
		return false;

	auto &claims = m_claims[client];
	for (size_t i = 0; i < claims.size(); i++)
	{
		if (claims[i].token == token)
		{
			claims.erase(claims.begin() + static_cast<std::ptrdiff_t>(i));
			Recompute(client);
			return true;
		}
	}

	auto &stack = m_stack[client];
	for (size_t i = 0; i < stack.size(); i++)
	{
		if (stack[i].token == token)
		{
			stack.erase(stack.begin() + static_cast<std::ptrdiff_t>(i));
			Recompute(client);
			return true;
		}
	}

	return false;
}

bool OverlayManager::ClearMine(int client, int ownerPluginSerial)
{
	if (!IsValidClient(client))
		return false;

	bool changed = false;
	auto &claims = m_claims[client];
	for (size_t i = 0; i < claims.size(); )
	{
		if (claims[i].ownerPluginSerial == ownerPluginSerial)
		{
			claims.erase(claims.begin() + static_cast<std::ptrdiff_t>(i));
			changed = true;
		}
		else
		{
			i++;
		}
	}

	auto &stack = m_stack[client];
	for (size_t i = 0; i < stack.size(); )
	{
		if (stack[i].ownerPluginSerial == ownerPluginSerial)
		{
			stack.erase(stack.begin() + static_cast<std::ptrdiff_t>(i));
			changed = true;
		}
		else
		{
			i++;
		}
	}

	if (changed)
		Recompute(client);

	return changed;
}

int OverlayManager::Push(int client, const char *path, int ownerPluginSerial)
{
	if (!IsValidClient(client) || path == nullptr)
		return 0;

	OverlayStackEntry entry;
	entry.token = AllocToken();
	entry.ownerPluginSerial = ownerPluginSerial;
	CopyString(entry.path, sizeof(entry.path), path);

	m_stack[client].push_back(entry);
	Recompute(client);
	return entry.token;
}

bool OverlayManager::Pop(int client, int token)
{
	if (!IsValidClient(client) || token <= 0)
		return false;

	auto &stack = m_stack[client];
	if (stack.empty())
		return false;

	for (size_t i = stack.size(); i-- > 0; )
	{
		if (stack[i].token == token)
		{
			stack.erase(stack.begin() + static_cast<std::ptrdiff_t>(i));
			Recompute(client);
			return true;
		}
	}

	return false;
}

bool OverlayManager::SetLegacy(int client, const char *path)
{
	if (!IsValidClient(client) || path == nullptr)
		return false;

	if (path[0] == '\0' || !strcmp(path, "0") || IsOffToken(path))
	{
		ClearLegacy(client);
		return true;
	}

	auto &claims = m_claims[client];
	for (size_t i = 0; i < claims.size(); i++)
	{
		if (claims[i].ownerPluginSerial == OVERLAY_LEGACY_OWNER)
		{
			CopyString(claims[i].path, sizeof(claims[i].path), path);
			claims[i].priority = m_legacyPriority;
			claims[i].sequence = m_sequence++;
			Recompute(client);
			return true;
		}
	}

	OverlayClaim claim;
	claim.token = AllocToken();
	claim.ownerPluginSerial = OVERLAY_LEGACY_OWNER;
	claim.priority = m_legacyPriority;
	claim.sequence = m_sequence++;
	CopyString(claim.path, sizeof(claim.path), path);
	claims.push_back(claim);
	Recompute(client);
	return true;
}

void OverlayManager::ClearLegacy(int client)
{
	if (!IsValidClient(client))
		return;

	auto &claims = m_claims[client];
	for (size_t i = 0; i < claims.size(); i++)
	{
		if (claims[i].ownerPluginSerial == OVERLAY_LEGACY_OWNER)
		{
			claims.erase(claims.begin() + static_cast<std::ptrdiff_t>(i));
			Recompute(client);
			return;
		}
	}

	Recompute(client);
}

bool OverlayManager::Resolve(int client, char *buf, int maxlen, int *outPriority) const
{
	if (!IsValidClient(client) || buf == nullptr || maxlen <= 0)
		return false;

	buf[0] = '\0';
	if (outPriority)
		*outPriority = 0;

	const auto &stack = m_stack[client];
	if (!stack.empty())
	{
		CopyString(buf, static_cast<size_t>(maxlen), stack.back().path);
		if (outPriority)
			*outPriority = 0x7fffffff;
		return true;
	}

	const auto &claims = m_claims[client];
	if (claims.empty())
		return false;

	size_t best = 0;
	for (size_t i = 1; i < claims.size(); i++)
	{
		if (claims[i].priority > claims[best].priority)
			best = i;
		else if (claims[i].priority == claims[best].priority
		         && claims[i].sequence > claims[best].sequence)
			best = i;
	}

	CopyString(buf, static_cast<size_t>(maxlen), claims[best].path);
	if (outPriority)
		*outPriority = claims[best].priority;
	return true;
}

bool OverlayManager::GetActive(int client, char *buf, int maxlen, int *outPriority) const
{
	return Resolve(client, buf, maxlen, outPriority);
}

int OverlayManager::GetActivePriority(int client) const
{
	char path[OVERLAY_MAX_PATH];
	int priority = 0;
	if (!Resolve(client, path, sizeof(path), &priority))
		return 0;
	return priority;
}

int OverlayManager::GetClaimCount(int client) const
{
	if (!IsValidClient(client))
		return 0;
	return static_cast<int>(m_claims[client].size());
}

bool OverlayManager::GetClaimInfo(int client, int index, char *path, int maxlen,
                                  int *priority, int *token, int *ownerSerial) const
{
	if (!IsValidClient(client) || index < 0
	    || index >= static_cast<int>(m_claims[client].size()))
		return false;

	const OverlayClaim &c = m_claims[client][static_cast<size_t>(index)];
	if (path && maxlen > 0)
		CopyString(path, static_cast<size_t>(maxlen), c.path);
	if (priority)
		*priority = c.priority;
	if (token)
		*token = c.token;
	if (ownerSerial)
		*ownerSerial = c.ownerPluginSerial;
	return true;
}

int OverlayManager::GetStackDepth(int client) const
{
	if (!IsValidClient(client))
		return 0;
	return static_cast<int>(m_stack[client].size());
}

void OverlayManager::Recompute(int client)
{
	if (!IsValidClient(client))
		return;

	char path[OVERLAY_MAX_PATH];
	int priority = 0;
	bool has = Resolve(client, path, sizeof(path), &priority);

	if (!has)
		path[0] = '\0';

	if (m_hasApplied[client] && !strcmp(m_applied[client], path))
		return;

	Apply(client, path, priority);
}

void OverlayManager::Apply(int client, const char *path, int priority)
{
	CopyString(m_applied[client], sizeof(m_applied[client]), path ? path : "");
	m_hasApplied[client] = true;

	if (m_debug)
	{
		g_pSM->LogMessage(myself, "client %d apply overlay \"%s\" (prio %d)",
		                  client, m_applied[client], priority);
	}

	Overlay_ApplyToClient(client, m_applied[client]);
	g_OverlayProcessor.FireOverlayChanged(client, m_applied[client], priority);
}
