# III.VC.SA.WebView2

> An in-game web panel for GTA III / Vice City / San Andreas, rendered by the system WebView2, toggled by a hotkey, with the page URL and window mode driven by config.

**English** | [简体中文](README_zh.md)

## What this project does

- Shows a web page inside the game: the panel is its own window, the page is rendered by the system WebView2, and no menu is involved
- In exclusive fullscreen it switches to a captured preview and tells you to move to borderless windowed mode for native speed and text input
- Shows a reload entry when the page fails to load

Out of scope:

- No menu, cheats, teleport, or other game features; those belong to XMenu
- No bundled web content; the page comes from the configured URL

## The panel

- The panel is a regular window with a title bar; the title reads `III.VC.SA.WebView2 | Author@鼠子(YuiNijika)` and the close button sits at the top right
- `webview.hotkey` toggles the panel, and the hotkey keeps working while the game is paused because the plugin also polls it from the render callback
- Closing the panel, by the close button or the hotkey, returns input and focus to the game

## Installation

1. Install [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases) (Widescreen Fix ships it as well)
2. Copy `WebView2.asi` into the game root or the `scripts\` folder; payload and config live under `<game root>\XBase\Mods\WebView2\`r
3. Keep the layout below; the payload sits under `Mods\WebView2` and the loader next to the XBase data

```txt
GameRoot\
├─ WebView2.asi
└─ XBase\
   ├─ debug.log
   ├─ WebView2Loader.dll
   └─ Mods\WebView2\
      ├─ WebView2SA.dll / WebView2VC.dll / WebView2III.dll
      ├─ config.json
      └─ debug.log
```

Runtime requirement: Windows 10/11 ships the WebView2 runtime; when it is missing the panel never appears and the log records the reason.

The first run writes `<game root>\XBase\Mods\WebView2\config.json`; the next section covers its keys.

## Supported games

| Game | Version |
|---|---|
| GTA III | 1.0 |
| GTA Vice City | 1.0 |
| GTA San Andreas | 1.0 US |

## Configuration

After the first run the config lives at `<game root>\XBase\Mods\WebView2\config.json`. Changes apply after a game restart.

| Key | Type | Default | Meaning |
|---|---|---|---|
| `webview.url` | string | `https://gtamodx.com/` | Page loaded in the panel |
| `webview.hotkey` | string | `F8` | Panel toggle hotkey; syntax follows `XBase::Input::ParseHotkey` and supports `Ctrl+` / `Alt+` / `Shift+` prefixes |
| `webview.zoom` | number | `1.0` | Page zoom, 0.5 to 2.0 |
| `webview.width` | number | `1000` | Panel width in pixels |
| `webview.height` | number | `640` | Panel height in pixels |
| `webview.windowMode` | number | `0` | `0` exclusive fullscreen, `1` windowed, `2` borderless; windowed and borderless must be chosen before the game creates its device, so restart after changing it |

```json
{
  "webview": {
    "url": "https://gtamodx.com/",
    "hotkey": "F8",
    "zoom": 1.0,
    "width": 1000,
    "height": 640,
    "windowMode": 2
  }
}
```

Window mode notes: in exclusive fullscreen DWM does not compose the game window, so the panel can only be shown as a captured preview with a capped frame rate and no text input. Setting `windowMode` to `2` and restarting makes the game run as a borderless window, and the panel returns to native rendering. The borderless window stays at normal window level, so other applications can still cover the game and the taskbar keeps its own layer.

## Build

Requires the XBase SDK. Build XBase first; it stages headers and static libraries into this project's `include\XBase` and `lib`:

```bash
XBase\Build.bat Release --no-pause
III.VC.SA.WebView2\Build.bat Release --no-pause
```

Outputs:

```txt
build\bin\WebView2.asi
build\bin\XBase\Mods\WebView2\WebView2SA.dll
build\bin\XBase\Mods\WebView2\WebView2VC.dll
build\bin\XBase\Mods\WebView2\WebView2III.dll
XBase\lib\WebView2Loader.dll (shipped to <game root>\XBase\ by the XBase build)
```

## Source layout

| Path | Responsibility |
|---|---|
| `loader/LoaderAnchor.cpp` | Keeps one compile item so MSVC runs Link and pulls the XBase bootstrap entry with WHOLEARCHIVE; exports `XBasePayloadBaseName` to declare the payload base name |
| `src/main.cpp` | Startup validation, config load, Host registration, hotkey handling; exports `XBasePayloadAttach` / `XBasePayloadDetach` |
| `src/Panel.cpp` | Panel window, viewport submission, captured-frame display and input forwarding, load failure handling |
| `lib/` `include/` | SDK staged by the XBase build; never edit by hand |

Load chain:

```txt
WebView2.asi
└─ XBase Bootstrap detects the game
   └─ loads XBase\Mods\WebView2\WebView2<game>.dll
      └─ XBasePayloadAttach
         ├─ Runtime::ValidateEnvironment
         ├─ Config::Init and startup window mode
         └─ Host::Install
```

Per-frame flow:

```txt
OnProcess
├─ Panel::Process   sync visibility, hide when the menu closes
├─ Core::Process    WebView domain: create, capture, cursor
└─ Hooks::MaintainInputState
Draw callback
├─ Input::PollSystemKeys + hotkey toggle   keeps working while paused
└─ Panel::Draw
   ├─ WebView::SetBounds
   ├─ WebView::DrawPanel          captured-frame mode only
   └─ WebView::ForwardPanelInput  captured-frame mode only
```

## Relation to XBase

This project contains no game addresses; every capability comes from the XBase public API:

| Purpose | Entry |
|---|---|
| Lifecycle | `XBase::Host::Install`, `XBase::Core::Init/Process/Shutdown` |
| Rendering and input | `XBase::Hooks::RegisterDrawCallback`, `MaintainInputState`, `SetMenuVisible` |
| Window mode | `XBase::Hooks::PrepareStartupWindowMode` |
| Web view | `XBase::WebView::Init/Navigate/SetVisible/SetBounds/DrawPanel/ForwardPanelInput` |
| UI and config | `XBase::UI`, `XBase::Config`, `XBase::Input` |

Full API, capability matrix, and boundaries: <https://blog.miomoe.cn/docs/xbase/>

## Troubleshooting

1. Confirm `WebView2.asi` exists and `<game root>\XBase\Mods\WebView2\` holds the payload DLL matches the game
2. Check `<game root>\XBase\Mods\WebView2\debug.log`; a healthy start logs `WebView2: Host 注册通过` and `WebView2: 渲染后端就绪`
3. `Failed to detect supported GTA runtime` means the game version is not supported
4. When the payload file is missing, compare the expected path printed in the log with the real folder and file names
5. Panel appears but stays blank: verify `<game root>\XBase\WebView2Loader.dll` exists and the WebView2 runtime is installed
6. Low frame rate or no text input: the panel is in exclusive-fullscreen captured preview; switch to borderless windowed mode as described above
7. Hotkey does nothing: check the `webview.hotkey` syntax; the log records a parse failure and falls back to the default
