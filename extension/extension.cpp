/**
 * Overlay Processor Extension
 */

#include "extension.h"

#include <cctype>
#include <cstring>

OverlayProcessor g_OverlayProcessor;
SMEXT_LINK(&g_OverlayProcessor);

IVEngineServer *engine = nullptr;

// Source1 IVEngineServer::ClientCommand is printf-style; SourceHook vafmt wraps it.
#define OVERLAY_HOOK_CLIENTCOMMAND 1

#ifdef OVERLAY_HOOK_CLIENTCOMMAND
SH_DECL_HOOK1_void_vafmt(IVEngineServer, ClientCommand, SH_NOATTRIB, 0, edict_t *);
#endif

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

void Overlay_ApplyToClient(int client, const char *path)
{
	if (engine == nullptr || gamehelpers == nullptr)
		return;

	edict_t *pEdict = gamehelpers->EdictOfIndex(client);
	if (pEdict == nullptr || pEdict->IsFree())
		return;

	const char *safePath = path ? path : "";

	g_bApplyingOverlay = true;
#ifdef OVERLAY_HOOK_CLIENTCOMMAND
	SH_CALL(engine, &IVEngineServer::ClientCommand)(pEdict, "r_screenoverlay \"%s\"", safePath);
#else
	engine->ClientCommand(pEdict, "r_screenoverlay \"%s\"", safePath);
#endif
	g_bApplyingOverlay = false;
}

bool Overlay_ParseScreenOverlay(const char *cmd, char *outPath, int outLen)
{
	if (cmd == nullptr || outPath == nullptr || outLen <= 0)
		return false;

	outPath[0] = '\0';

	// Skip leading whitespace
	while (*cmd && isspace(static_cast<unsigned char>(*cmd)))
		cmd++;

	static const char kPrefix[] = "r_screenoverlay";
	for (size_t i = 0; kPrefix[i] != '\0'; i++)
	{
		char a = static_cast<char>(tolower(static_cast<unsigned char>(cmd[i])));
		char b = kPrefix[i];
		if (a != b)
			return false;
	}

	cmd += sizeof(kPrefix) - 1;
	if (*cmd != '\0' && !isspace(static_cast<unsigned char>(*cmd)))
		return false;

	while (*cmd && isspace(static_cast<unsigned char>(*cmd)))
		cmd++;

	// Strip wrapping quotes
	if (*cmd == '\"')
	{
		cmd++;
		const char *end = strchr(cmd, '\"');
		if (end == nullptr)
		{
			CopyString(outPath, static_cast<size_t>(outLen), cmd);
		}
		else
		{
			size_t n = static_cast<size_t>(end - cmd);
			if (n >= static_cast<size_t>(outLen))
				n = static_cast<size_t>(outLen) - 1;
			memcpy(outPath, cmd, n);
			outPath[n] = '\0';
		}
		return true;
	}

	CopyString(outPath, static_cast<size_t>(outLen), cmd);

	// Trim trailing whitespace
	size_t len = strlen(outPath);
	while (len > 0 && isspace(static_cast<unsigned char>(outPath[len - 1])))
	{
		outPath[len - 1] = '\0';
		len--;
	}

	return true;
}

int Overlay_PluginSerial(IPluginContext *pContext)
{
	if (pContext == nullptr || plsys == nullptr)
		return 0;

	IPlugin *plugin = plsys->FindPluginByContext(pContext->GetContext());
	if (plugin == nullptr)
		return 0;

	return static_cast<int>(plugin->GetSerial());
}

bool OverlayProcessor::SDK_OnLoad(char *error, size_t maxlen, bool late)
{
	m_pOnChanged = nullptr;
	m_bHooked = false;

	sharesys->AddNatives(myself, overlay_natives);
	sharesys->RegisterLibrary(myself, "overlay_processor");

	playerhelpers->AddClientListener(this);
	plsys->AddPluginsListener(this);

	return true;
}

void OverlayProcessor::SDK_OnAllLoaded()
{
	m_pOnChanged = forwards->CreateForward("Overlay_OnChanged", ET_Ignore, 3, nullptr,
	                                       Param_Cell, Param_String, Param_Cell);

	if (!HookClientCommand())
	{
		g_pSM->LogError(myself, "Failed to hook IVEngineServer::ClientCommand; "
		                        "legacy ClientCommand intercept disabled");
	}
}

void OverlayProcessor::SDK_OnUnload()
{
	UnhookClientCommand();

	if (m_pOnChanged != nullptr)
	{
		forwards->ReleaseForward(m_pOnChanged);
		m_pOnChanged = nullptr;
	}

	if (playerhelpers != nullptr)
		playerhelpers->RemoveClientListener(this);
	if (plsys != nullptr)
		plsys->RemovePluginsListener(this);

	g_Overlays.ResetAll();
}

bool OverlayProcessor::QueryRunning(char *error, size_t maxlen)
{
	return true;
}

#if defined SMEXT_CONF_METAMOD
bool OverlayProcessor::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
	return true;
}

bool OverlayProcessor::SDK_OnMetamodUnload(char *error, size_t maxlen)
{
	UnhookClientCommand();
	return true;
}
#endif

bool OverlayProcessor::HookClientCommand()
{
#ifdef OVERLAY_HOOK_CLIENTCOMMAND
	if (engine == nullptr || m_bHooked)
		return m_bHooked;

	SH_ADD_HOOK(IVEngineServer, ClientCommand, engine,
	            SH_MEMBER(this, &OverlayProcessor::OnSendClientCommand), false);
	m_bHooked = true;
	return true;
#else
	return false;
#endif
}

void OverlayProcessor::UnhookClientCommand()
{
#ifdef OVERLAY_HOOK_CLIENTCOMMAND
	if (!m_bHooked || engine == nullptr)
		return;

	SH_REMOVE_HOOK(IVEngineServer, ClientCommand, engine,
	               SH_MEMBER(this, &OverlayProcessor::OnSendClientCommand), false);
	m_bHooked = false;
#endif
}

void OverlayProcessor::OnSendClientCommand(edict_t *pPlayer, const char *szFormat)
{
	if (g_bApplyingOverlay || szFormat == nullptr)
	{
		RETURN_META(MRES_IGNORED);
	}

	char path[OVERLAY_MAX_PATH];
	if (!Overlay_ParseScreenOverlay(szFormat, path, sizeof(path)))
	{
		RETURN_META(MRES_IGNORED);
	}

	int client = gamehelpers->IndexOfEdict(pPlayer);
	if (client <= 0 || client >= OVERLAY_MAX_CLIENTS)
	{
		RETURN_META(MRES_IGNORED);
	}

	if (g_Overlays.IsDebug())
	{
		g_pSM->LogMessage(myself, "intercept client %d: r_screenoverlay \"%s\"", client, path);
	}

	g_Overlays.SetLegacy(client, path);
	RETURN_META(MRES_SUPERCEDE);
}

void OverlayProcessor::OnClientDisconnecting(int client)
{
	g_Overlays.ResetClient(client);
}

void OverlayProcessor::OnPluginUnloaded(IPlugin *plugin)
{
	if (plugin == nullptr)
		return;
	g_Overlays.OnPluginUnloaded(static_cast<int>(plugin->GetSerial()));
}

void OverlayProcessor::FireOverlayChanged(int client, const char *path, int priority)
{
	if (m_pOnChanged == nullptr)
		return;

	m_pOnChanged->PushCell(client);
	m_pOnChanged->PushString(path ? path : "");
	m_pOnChanged->PushCell(priority);
	m_pOnChanged->Execute();
}
