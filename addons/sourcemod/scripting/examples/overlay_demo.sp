#pragma semicolon 1
#pragma newdecls required

#include <sourcemod>
#include <overlay_processor>

public Plugin myinfo =
{
	name        = "Overlay Processor Demo",
	author      = "sm_overlay_processor",
	description = "Example: priority claims + push/pop overrides",
	version     = "1.0.0",
	url         = "https://github.com/sm_overlay_processor"
};

int g_LowToken[MAXPLAYERS + 1];
int g_HighToken[MAXPLAYERS + 1];
int g_PushToken[MAXPLAYERS + 1];

public void OnPluginStart()
{
	RegAdminCmd("sm_ov_low", Command_Low, ADMFLAG_GENERIC, "Set low-priority overlay claim");
	RegAdminCmd("sm_ov_high", Command_High, ADMFLAG_GENERIC, "Set high-priority overlay claim");
	RegAdminCmd("sm_ov_clear", Command_Clear, ADMFLAG_GENERIC, "Clear this plugin's claims");
	RegAdminCmd("sm_ov_push", Command_Push, ADMFLAG_GENERIC, "Push temporary override");
	RegAdminCmd("sm_ov_pop", Command_Pop, ADMFLAG_GENERIC, "Pop temporary override");
	RegAdminCmd("sm_ov_legacy", Command_Legacy, ADMFLAG_GENERIC, "Fire raw ClientCommand (intercepted)");
}

public void OnClientDisconnect(int client)
{
	g_LowToken[client] = 0;
	g_HighToken[client] = 0;
	g_PushToken[client] = 0;
}

public Action Command_Low(int client, int args)
{
	if (client <= 0)
		return Plugin_Handled;

	if (g_LowToken[client])
		Overlay_Clear(client, g_LowToken[client]);

	g_LowToken[client] = Overlay_Set(client, "debug/yuv", 10);
	ReplyToCommand(client, "Low claim token=%d", g_LowToken[client]);
	return Plugin_Handled;
}

public Action Command_High(int client, int args)
{
	if (client <= 0)
		return Plugin_Handled;

	if (g_HighToken[client])
		Overlay_Clear(client, g_HighToken[client]);

	g_HighToken[client] = Overlay_Set(client, "effects/combine_binocoverlay", 100);
	ReplyToCommand(client, "High claim token=%d", g_HighToken[client]);
	return Plugin_Handled;
}

public Action Command_Clear(int client, int args)
{
	if (client <= 0)
		return Plugin_Handled;

	Overlay_ClearMine(client);
	g_LowToken[client] = 0;
	g_HighToken[client] = 0;
	g_PushToken[client] = 0;
	ReplyToCommand(client, "Cleared own claims/stack.");
	return Plugin_Handled;
}

public Action Command_Push(int client, int args)
{
	if (client <= 0)
		return Plugin_Handled;

	if (g_PushToken[client])
		Overlay_Pop(client, g_PushToken[client]);

	g_PushToken[client] = Overlay_Push(client, "effects/tp_eyefx/tpeye");
	ReplyToCommand(client, "Pushed override token=%d", g_PushToken[client]);
	return Plugin_Handled;
}

public Action Command_Pop(int client, int args)
{
	if (client <= 0)
		return Plugin_Handled;

	if (!g_PushToken[client])
	{
		ReplyToCommand(client, "No push token.");
		return Plugin_Handled;
	}

	Overlay_Pop(client, g_PushToken[client]);
	g_PushToken[client] = 0;
	ReplyToCommand(client, "Popped override.");
	return Plugin_Handled;
}

public Action Command_Legacy(int client, int args)
{
	if (client <= 0)
		return Plugin_Handled;

	// Intentionally uses ClientCommand — extension should intercept this.
	ClientCommand(client, "r_screenoverlay \"effects/tvscreen_noise003a\"");
	ReplyToCommand(client, "Sent legacy ClientCommand (should be intercepted).");
	return Plugin_Handled;
}
