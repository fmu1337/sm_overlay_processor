/**
 * Overlay Processor Extension
 */

#ifndef _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
#define _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_

#include "smsdk_ext.h"
#include "overlay_manager.h"

class OverlayProcessor :
	public SDKExtension,
	public IClientListener,
	public IPluginsListener
{
public:
	virtual bool SDK_OnLoad(char *error, size_t maxlen, bool late);
	virtual void SDK_OnUnload();
	virtual void SDK_OnAllLoaded();
	virtual bool QueryRunning(char *error, size_t maxlen);

public:
#if defined SMEXT_CONF_METAMOD
	virtual bool SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late);
	virtual bool SDK_OnMetamodUnload(char *error, size_t maxlen);
#endif

public: // IClientListener
	virtual void OnClientDisconnecting(int client);

public: // IPluginsListener
	virtual void OnPluginUnloaded(IPlugin *plugin);

public:
	void FireOverlayChanged(int client, const char *path, int priority);
	void OnSendClientCommand(edict_t *pPlayer, const char *szFormat);

private:
	bool HookClientCommand();
	void UnhookClientCommand();

	IForward *m_pOnChanged;
	bool m_bHooked;
};

extern OverlayProcessor g_OverlayProcessor;
extern IVEngineServer *engine;

int Overlay_PluginSerial(IPluginContext *pContext);
bool Overlay_ParseScreenOverlay(const char *cmd, char *outPath, int outLen);

const sp_nativeinfo_t overlay_natives[];

#endif // _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
