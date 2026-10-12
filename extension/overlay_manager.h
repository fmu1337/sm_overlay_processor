/**
 * Overlay Processor — per-client claim + override stack manager
 */

#ifndef _INCLUDE_OVERLAY_MANAGER_H_
#define _INCLUDE_OVERLAY_MANAGER_H_

#include <vector>

static const int OVERLAY_MAX_CLIENTS = 65;
static const int OVERLAY_MAX_PATH = 256;
static const int OVERLAY_LEGACY_OWNER = -1;

struct OverlayClaim
{
	int token;
	int ownerPluginSerial; // IPlugin serial, or OVERLAY_LEGACY_OWNER
	int priority;
	unsigned int sequence;
	char path[OVERLAY_MAX_PATH];
};

struct OverlayStackEntry
{
	int token;
	int ownerPluginSerial;
	char path[OVERLAY_MAX_PATH];
};

class OverlayManager
{
public:
	OverlayManager();

	void ResetClient(int client);
	void ResetAll();
	void OnPluginUnloaded(int pluginSerial);

	int Set(int client, const char *path, int priority, int ownerPluginSerial);
	bool Clear(int client, int token);
	bool ClearMine(int client, int ownerPluginSerial);

	int Push(int client, const char *path, int ownerPluginSerial);
	bool Pop(int client, int token);

	bool SetLegacy(int client, const char *path);
	void ClearLegacy(int client);

	bool GetActive(int client, char *buf, int maxlen, int *outPriority) const;
	int GetActivePriority(int client) const;

	int GetClaimCount(int client) const;
	bool GetClaimInfo(int client, int index, char *path, int maxlen,
	                  int *priority, int *token, int *ownerSerial) const;
	int GetStackDepth(int client) const;

	void SetLegacyPriority(int priority);
	int GetLegacyPriority() const { return m_legacyPriority; }

	void SetDebug(bool enabled) { m_debug = enabled; }
	bool IsDebug() const { return m_debug; }

	/** Recompute winner and send via trampoline if changed. */
	void Recompute(int client);

private:
	bool IsValidClient(int client) const;
	int AllocToken();
	bool Resolve(int client, char *buf, int maxlen, int *outPriority) const;
	void Apply(int client, const char *path, int priority);

	std::vector<OverlayClaim> m_claims[OVERLAY_MAX_CLIENTS];
	std::vector<OverlayStackEntry> m_stack[OVERLAY_MAX_CLIENTS];
	char m_applied[OVERLAY_MAX_CLIENTS][OVERLAY_MAX_PATH];
	bool m_hasApplied[OVERLAY_MAX_CLIENTS];

	int m_nextToken;
	unsigned int m_sequence;
	int m_legacyPriority;
	bool m_debug;
};

extern OverlayManager g_Overlays;
extern bool g_bApplyingOverlay;

void Overlay_ApplyToClient(int client, const char *path);

#endif // _INCLUDE_OVERLAY_MANAGER_H_
