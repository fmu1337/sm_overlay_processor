#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <overlay_processor>

public Plugin myinfo =
{
	name        = "Overlay Processor",
	author      = "sm_overlay_processor",
	description = "Cvars and admin helpers for the overlay_processor extension",
	version     = "1.0.0",
	url         = "https://github.com/sm_overlay_processor"
};

ConVar g_CvarLegacyPriority;
ConVar g_CvarDebug;

public void OnPluginStart()
{
	g_CvarLegacyPriority = CreateConVar(
		"sm_overlay_legacy_priority",
		"0",
		"Priority assigned to intercepted ClientCommand(\"r_screenoverlay\") calls.",
		_,
		true, -100000.0,
		true, 100000.0);

	g_CvarDebug = CreateConVar(
		"sm_overlay_debug",
		"0",
		"Log overlay intercepts and applies (0/1).",
		_,
		true, 0.0,
		true, 1.0);

	AutoExecConfig(true, "overlay_processor");

	g_CvarLegacyPriority.AddChangeHook(OnLegacyPriorityChanged);
	g_CvarDebug.AddChangeHook(OnDebugChanged);

	RegAdminCmd("sm_overlay_status", Command_Status, ADMFLAG_GENERIC,
		"Show active overlay claims for a target (or all players).");

	SyncExtensionSettings();
}

public void OnConfigsExecuted()
{
	SyncExtensionSettings();
}

void SyncExtensionSettings()
{
	Overlay_SetLegacyPriority(g_CvarLegacyPriority.IntValue);
	Overlay_SetDebug(g_CvarDebug.BoolValue);
}

public void OnLegacyPriorityChanged(ConVar convar, const char[] oldValue, const char[] newValue)
{
	Overlay_SetLegacyPriority(convar.IntValue);
}

public void OnDebugChanged(ConVar convar, const char[] oldValue, const char[] newValue)
{
	Overlay_SetDebug(convar.BoolValue);
}

public Action Command_Status(int client, int args)
{
	int target = -1;

	if (args >= 1)
	{
		char arg[64];
		GetCmdArg(1, arg, sizeof(arg));
		target = FindTarget(client, arg, true, false);
		if (target <= 0)
			return Plugin_Handled;
	}

	if (target > 0)
	{
		PrintStatus(client, target);
		return Plugin_Handled;
	}

	int shown = 0;
	for (int i = 1; i <= MaxClients; i++)
	{
		if (!IsClientInGame(i))
			continue;

		char path[256];
		if (!Overlay_GetActive(i, path, sizeof(path)) && Overlay_GetClaimCount(i) == 0 && Overlay_GetStackDepth(i) == 0)
			continue;

		PrintStatus(client, i);
		shown++;
	}

	if (shown == 0)
		ReplyToCommand(client, "[Overlay] No active overlays.");

	return Plugin_Handled;
}

void PrintStatus(int admin, int target)
{
	char path[256];
	bool active = Overlay_GetActive(target, path, sizeof(path));
	int prio = Overlay_GetActivePriority(target);
	int claims = Overlay_GetClaimCount(target);
	int stack = Overlay_GetStackDepth(target);

	ReplyToCommand(admin, "[Overlay] %N (#%d): active=\"%s\" prio=%d claims=%d stack=%d",
		target, target, active ? path : "", prio, claims, stack);

	for (int i = 0; i < claims; i++)
	{
		char claimPath[256];
		int claimPrio, token, owner;
		if (!Overlay_GetClaimInfo(target, i, claimPath, sizeof(claimPath), claimPrio, token, owner))
			continue;

		ReplyToCommand(admin, "  claim[%d] token=%d prio=%d owner=%d path=\"%s\"",
			i, token, claimPrio, owner, claimPath);
	}
}

public void Overlay_OnChanged(int client, const char[] path, int priority)
{
	if (!g_CvarDebug.BoolValue)
		return;

	LogMessage("Overlay changed for %N: \"%s\" (prio %d)", client, path, priority);
}
