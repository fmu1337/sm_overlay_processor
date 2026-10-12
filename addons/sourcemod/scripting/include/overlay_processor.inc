#if defined _overlay_processor_included
 #endinput
#endif
#define _overlay_processor_included

/**
 * Overlay Processor
 *
 * Central manager for r_screenoverlay. Prefer these natives over raw
 * ClientCommand(client, "r_screenoverlay ..."). Legacy ClientCommand calls are
 * intercepted by the extension and stored as a single low-priority claim.
 */

/**
 * Called when the active overlay for a client changes.
 *
 * @param client    Client index.
 * @param path      Active material path (empty if cleared).
 * @param priority  Winning priority (stack overrides use INT_MAX).
 */
forward void Overlay_OnChanged(int client, const char[] path, int priority);

/**
 * Register an overlay claim. Higher priority wins; equal priority → newest wins.
 *
 * @param client    Client index.
 * @param path      Material path (e.g. "effects/combine_binocoverlay").
 * @param priority  Claim priority (default 0).
 * @return          Token used with Overlay_Clear, or 0 on failure.
 */
native int Overlay_Set(int client, const char[] path, int priority = 0);

/**
 * Remove a claim or stack entry by token.
 *
 * @param client    Client index.
 * @param token     Token from Overlay_Set / Overlay_Push.
 * @return          True if removed.
 */
native bool Overlay_Clear(int client, int token);

/**
 * Remove all claims and stack entries owned by the calling plugin for a client.
 *
 * @param client    Client index.
 * @return          True if anything was removed.
 */
native bool Overlay_ClearMine(int client);

/**
 * Push a temporary override above all priority claims.
 *
 * @param client    Client index.
 * @param path      Material path.
 * @return          Token used with Overlay_Pop, or 0 on failure.
 */
native int Overlay_Push(int client, const char[] path);

/**
 * Pop a temporary override by token.
 *
 * @param client    Client index.
 * @param token     Token from Overlay_Push.
 * @return          True if removed.
 */
native bool Overlay_Pop(int client, int token);

/**
 * Read the currently applied overlay path.
 *
 * @param client    Client index.
 * @param buffer    Output buffer.
 * @param maxlen    Buffer size.
 * @return          True if an overlay is active (including blank claim).
 */
native bool Overlay_GetActive(int client, char[] buffer, int maxlen);

/**
 * Priority of the active overlay (stack → 2147483647).
 *
 * @param client    Client index.
 * @return          Priority value, or 0 if none.
 */
native int Overlay_GetActivePriority(int client);

/**
 * Priority used for intercepted legacy ClientCommand overlays.
 *
 * @param priority  New legacy priority.
 */
native void Overlay_SetLegacyPriority(int priority);

/**
 * @return Current legacy intercept priority.
 */
native int Overlay_GetLegacyPriority();

/**
 * Enable/disable extension debug logging.
 *
 * @param enabled   True to log intercepts and applies.
 */
native void Overlay_SetDebug(bool enabled);

/**
 * @param client    Client index.
 * @return          Number of priority claims (including legacy).
 */
native int Overlay_GetClaimCount(int client);

/**
 * Read claim info by index (0 .. GetClaimCount-1).
 *
 * @param client      Client index.
 * @param index       Claim index.
 * @param path        Output path.
 * @param maxlen      Path buffer size.
 * @param priority    Output priority.
 * @param token       Output token.
 * @param ownerSerial Plugin serial, or -1 for legacy.
 * @return            True on success.
 */
native bool Overlay_GetClaimInfo(int client, int index, char[] path, int maxlen, int &priority, int &token, int &ownerSerial);

/**
 * @param client    Client index.
 * @return          Override stack depth.
 */
native int Overlay_GetStackDepth(int client);

public SharedPlugin __pl_overlay_processor =
{
	name = "overlay_processor",
	file = "overlay_processor.ext",
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};

#if !defined REQUIRE_EXTENSIONS
public void __ext_overlay_processor_SetNTVOptional()
{
	MarkNativeAsOptional("Overlay_Set");
	MarkNativeAsOptional("Overlay_Clear");
	MarkNativeAsOptional("Overlay_ClearMine");
	MarkNativeAsOptional("Overlay_Push");
	MarkNativeAsOptional("Overlay_Pop");
	MarkNativeAsOptional("Overlay_GetActive");
	MarkNativeAsOptional("Overlay_GetActivePriority");
	MarkNativeAsOptional("Overlay_SetLegacyPriority");
	MarkNativeAsOptional("Overlay_GetLegacyPriority");
	MarkNativeAsOptional("Overlay_SetDebug");
	MarkNativeAsOptional("Overlay_GetClaimCount");
	MarkNativeAsOptional("Overlay_GetClaimInfo");
	MarkNativeAsOptional("Overlay_GetStackDepth");
}
#endif
