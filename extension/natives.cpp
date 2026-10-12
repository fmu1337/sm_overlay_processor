/**
 * Overlay Processor natives
 */

#include "extension.h"

#include <cstring>

static cell_t Native_Overlay_Set(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	char *path = nullptr;
	pContext->LocalToString(params[2], &path);
	int priority = params[3];
	int owner = Overlay_PluginSerial(pContext);

	if (client <= 0 || client >= OVERLAY_MAX_CLIENTS)
	{
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	return g_Overlays.Set(client, path ? path : "", priority, owner);
}

static cell_t Native_Overlay_Clear(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	int token = params[2];

	if (client <= 0 || client >= OVERLAY_MAX_CLIENTS)
	{
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	return g_Overlays.Clear(client, token) ? 1 : 0;
}

static cell_t Native_Overlay_ClearMine(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	int owner = Overlay_PluginSerial(pContext);

	if (client <= 0 || client >= OVERLAY_MAX_CLIENTS)
	{
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	return g_Overlays.ClearMine(client, owner) ? 1 : 0;
}

static cell_t Native_Overlay_Push(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	char *path = nullptr;
	pContext->LocalToString(params[2], &path);
	int owner = Overlay_PluginSerial(pContext);

	if (client <= 0 || client >= OVERLAY_MAX_CLIENTS)
	{
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	return g_Overlays.Push(client, path ? path : "", owner);
}

static cell_t Native_Overlay_Pop(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	int token = params[2];

	if (client <= 0 || client >= OVERLAY_MAX_CLIENTS)
	{
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	return g_Overlays.Pop(client, token) ? 1 : 0;
}

static cell_t Native_Overlay_GetActive(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	int maxlen = params[3];

	if (client <= 0 || client >= OVERLAY_MAX_CLIENTS)
	{
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	char path[OVERLAY_MAX_PATH];
	int priority = 0;
	bool ok = g_Overlays.GetActive(client, path, sizeof(path), &priority);

	pContext->StringToLocalUTF8(params[2], maxlen, ok ? path : "", nullptr);
	return ok ? 1 : 0;
}

static cell_t Native_Overlay_GetActivePriority(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];

	if (client <= 0 || client >= OVERLAY_MAX_CLIENTS)
	{
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	return g_Overlays.GetActivePriority(client);
}

static cell_t Native_Overlay_SetLegacyPriority(IPluginContext *pContext, const cell_t *params)
{
	(void)pContext;
	g_Overlays.SetLegacyPriority(params[1]);
	return 1;
}

static cell_t Native_Overlay_GetLegacyPriority(IPluginContext *pContext, const cell_t *params)
{
	(void)pContext;
	(void)params;
	return g_Overlays.GetLegacyPriority();
}

static cell_t Native_Overlay_SetDebug(IPluginContext *pContext, const cell_t *params)
{
	(void)pContext;
	g_Overlays.SetDebug(params[1] != 0);
	return 1;
}

static cell_t Native_Overlay_GetClaimCount(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	if (client <= 0 || client >= OVERLAY_MAX_CLIENTS)
	{
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}
	return g_Overlays.GetClaimCount(client);
}

static cell_t Native_Overlay_GetClaimInfo(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	int index = params[2];
	int maxlen = params[4];

	if (client <= 0 || client >= OVERLAY_MAX_CLIENTS)
	{
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}

	char path[OVERLAY_MAX_PATH];
	int priority = 0;
	int token = 0;
	int owner = 0;

	if (!g_Overlays.GetClaimInfo(client, index, path, sizeof(path), &priority, &token, &owner))
		return 0;

	pContext->StringToLocalUTF8(params[3], maxlen, path, nullptr);

	cell_t *pPriority = nullptr;
	cell_t *pToken = nullptr;
	cell_t *pOwner = nullptr;
	pContext->LocalToPhysAddr(params[5], &pPriority);
	pContext->LocalToPhysAddr(params[6], &pToken);
	pContext->LocalToPhysAddr(params[7], &pOwner);

	*pPriority = priority;
	*pToken = token;
	*pOwner = owner;
	return 1;
}

static cell_t Native_Overlay_GetStackDepth(IPluginContext *pContext, const cell_t *params)
{
	int client = params[1];
	if (client <= 0 || client >= OVERLAY_MAX_CLIENTS)
	{
		return pContext->ThrowNativeError("Invalid client index %d", client);
	}
	return g_Overlays.GetStackDepth(client);
}

const sp_nativeinfo_t overlay_natives[] =
{
	{"Overlay_Set",               Native_Overlay_Set},
	{"Overlay_Clear",             Native_Overlay_Clear},
	{"Overlay_ClearMine",         Native_Overlay_ClearMine},
	{"Overlay_Push",              Native_Overlay_Push},
	{"Overlay_Pop",               Native_Overlay_Pop},
	{"Overlay_GetActive",         Native_Overlay_GetActive},
	{"Overlay_GetActivePriority", Native_Overlay_GetActivePriority},
	{"Overlay_SetLegacyPriority", Native_Overlay_SetLegacyPriority},
	{"Overlay_GetLegacyPriority", Native_Overlay_GetLegacyPriority},
	{"Overlay_SetDebug",          Native_Overlay_SetDebug},
	{"Overlay_GetClaimCount",     Native_Overlay_GetClaimCount},
	{"Overlay_GetClaimInfo",      Native_Overlay_GetClaimInfo},
	{"Overlay_GetStackDepth",     Native_Overlay_GetStackDepth},
	{nullptr,                     nullptr},
};
