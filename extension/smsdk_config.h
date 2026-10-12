/**
 * Overlay Processor Extension
 * Copyright (C) 2026 sm_overlay_processor contributors
 */

#ifndef _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_

#define SMEXT_CONF_NAME			"Overlay Processor"
#define SMEXT_CONF_DESCRIPTION	"Central r_screenoverlay manager with priority and intercept"
#define SMEXT_CONF_VERSION		"1.0.0"
#define SMEXT_CONF_AUTHOR		"sm_overlay_processor"
#define SMEXT_CONF_URL			"https://github.com/sm_overlay_processor"
#define SMEXT_CONF_LOGTAG		"OVERLAY"
#define SMEXT_CONF_LICENSE		"GPL"
#define SMEXT_CONF_DATESTRING	__DATE__

#define SMEXT_LINK(name) SDKExtension *g_pExtensionIface = name;

#define SMEXT_CONF_METAMOD

#define SMEXT_ENABLE_FORWARDSYS
#define SMEXT_ENABLE_PLAYERHELPERS
#define SMEXT_ENABLE_GAMEHELPERS
#define SMEXT_ENABLE_PLUGINSYS
#define SMEXT_ENABLE_LIBSYS

#endif // _INCLUDE_SOURCEMOD_EXTENSION_CONFIG_H_
