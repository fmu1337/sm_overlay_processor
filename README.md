# sm_overlay_processor

SourceMod extension + plugin that owns `r_screenoverlay` for the whole server.

Other plugins either call the natives, or keep using `ClientCommand(client, "r_screenoverlay ...")` — the extension intercepts those calls, stores them as a legacy claim, and applies only the winning overlay.

## Features

- **Priority claims** — `Overlay_Set` / `Overlay_Clear` / `Overlay_ClearMine`
- **Override stack** — `Overlay_Push` / `Overlay_Pop` (always wins over claims)
- **Legacy intercept** — SourceHook on `IVEngineServer::ClientCommand` (vafmt)
- **Restore** — clearing/popping recomputes the previous winner
- **Cleanup** — client disconnect and plugin unload drop owned claims/stack entries

## Layout

```
addons/sourcemod/
  extensions/overlay_processor.autoload
  gamedata/overlay_processor.games.txt
  scripting/overlay_processor.sp
  scripting/include/overlay_processor.inc
  scripting/examples/overlay_demo.sp
extension/          # C++ SourceMod extension sources (AMBuild)
```

## Build (extension)

Requires SourceMod, Metamod:Source, HL2SDK(s), AMBuild, and [hl2sdk-manifests](https://github.com/alliedmodders/hl2sdk-manifests).

```bash
cd extension
mkdir build && cd build
python3 ../configure.py --enable-optimize \
  --sm-path /path/to/sourcemod \
  --mms-path /path/to/metamod-source \
  --hl2sdk-root /path/to/hl2sdks \
  --hl2sdk-manifest-path /path/to/hl2sdk-manifests \
  -s css,csgo,tf2
ambuild
```

Copy `package/addons/sourcemod/extensions/overlay_processor.ext.*.so` (or `.dll`) into the server `addons/sourcemod/extensions/` folder. Use the `x64/` subfolder for 64-bit builds.

Compile the SourcePawn plugins with `spcomp`:

```bash
spcomp addons/sourcemod/scripting/overlay_processor.sp -o overlay_processor.smx -i addons/sourcemod/scripting/include
spcomp addons/sourcemod/scripting/examples/overlay_demo.sp -o overlay_demo.smx -i addons/sourcemod/scripting/include
```

## Install

1. Drop the extension binary + `overlay_processor.autoload` into `addons/sourcemod/extensions/`
2. Drop `overlay_processor.games.txt` into `addons/sourcemod/gamedata/`
3. Drop `overlay_processor.inc` into `addons/sourcemod/scripting/include/`
4. Drop `overlay_processor.smx` into `addons/sourcemod/plugins/`
5. (Optional) install `overlay_demo.smx` for smoke commands

## Migration

Before:

```sourcepawn
ClientCommand(client, "r_screenoverlay \"%s\"", path);
ClientCommand(client, "r_screenoverlay \"\"");
```

After:

```sourcepawn
#include <overlay_processor>

int g_Token;

// claim
g_Token = Overlay_Set(client, path, 50);

// clear this claim
Overlay_Clear(client, g_Token);

// temporary override
int push = Overlay_Push(client, "effects/something");
// ... later
Overlay_Pop(client, push);
```

Legacy `ClientCommand` still works without edits: it becomes one legacy claim per client at `sm_overlay_legacy_priority` (default `0`). Native claims with higher priority will not be overwritten by leftover plugins.

## Cvars / commands

| Name | Default | Meaning |
|------|---------|---------|
| `sm_overlay_legacy_priority` | `0` | Priority for intercepted ClientCommand overlays |
| `sm_overlay_debug` | `0` | Log intercepts / applies |
| `sm_overlay_status [target]` | — | Dump active path, claims, stack depth |

Demo (`overlay_demo`): `sm_ov_low`, `sm_ov_high`, `sm_ov_push`, `sm_ov_pop`, `sm_ov_clear`, `sm_ov_legacy`.

## Limits

- Source1 only (CS:S, CS:GO, TF2, L4D2, …). Not CS2 / Source2.
- Clients with `cl_restrict_server_commands 1` may ignore server overlays.
- Overlays sent via `env_screenoverlay` entities are out of scope for v1.
- Intercept requires the Metamod-backed extension build; without the hook, natives still work.

## API sketch

```
claims: max(priority), tie → newest
stack:  top overrides all claims
empty:  r_screenoverlay ""
apply:  trampoline ClientCommand with re-entry guard
```
