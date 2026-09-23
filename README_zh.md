# III.VC.SA.WebView2

> 在 GTA III / Vice City / San Andreas 游戏内显示一个由系统 WebView2 渲染的网页面板，热键开关，页面地址与窗口模式由配置决定。

[English](README.md) | **简体中文**

## 这个项目做什么

- 游戏内显示网页：面板是一个独立窗口，页面由系统 WebView2 渲染，不依赖任何菜单
- 独占全屏下改用抓帧贴图呈现，并提示切换到无边框窗口以获得原生速度与文字输入
- 页面加载失败时给出重新加载入口

不属于本项目的内容：

- 不提供菜单、作弊、传送等游戏功能，这些属于 XMenu
- 不内置网页内容，页面由配置里的地址决定

## 面板

- 面板是一个带标题栏的普通窗口，标题为 `III.VC.SA.WebView2 | Author@鼠子(YuiNijika)`，右上角是关闭按钮
- `webview.hotkey` 开关面板；游戏暂停时热键依然有效，因为插件同时在渲染回调里轮询热键
- 用关闭按钮或热键关掉面板后，输入与焦点都会交还游戏

## 安装

1. 安装 [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases)（若已装 Widescreen Fix 则自带）
2. 把 `WebView2.asi` 放进游戏目录或 `scripts\` 目录，载荷与配置由 XBase 统一放在 `<游戏根目录>\XBase\Mods\WebView2\`
3. 目录结构保持如下，载荷在 `Mods\WebView2` 下，加载器与 XBase 数据同级

```txt
游戏目录\
├─ WebView2.asi
└─ XBase\
   ├─ debug.log
   ├─ WebView2Loader.dll
   └─ Mods\WebView2\
      ├─ WebView2SA.dll / WebView2VC.dll / WebView2III.dll
      ├─ config.json
      └─ debug.log
```

运行时要求：Windows 10/11 自带 WebView2 运行时；缺失时面板不会出现，日志会记录原因。

首次运行会在 `<游戏根目录>\XBase\Mods\WebView2\config.json` 生成配置，字段见下一节。

## 支持的游戏

| 游戏 | 版本 |
|---|---|
| GTA III | 1.0 |
| GTA Vice City | 1.0 |
| GTA San Andreas | 1.0 US |

## 配置

首次运行会在 `<游戏根目录>\XBase\Mods\WebView2\config.json` 生成配置，改动后重启游戏生效。

| 键 | 类型 | 默认值 | 说明 |
|---|---|---|---|
| `webview.url` | string | `https://gtamodx.com/` | 面板加载的地址 |
| `webview.hotkey` | string | `F8` | 开关面板的热键，写法见 `XBase::Input::ParseHotkey`，支持 `Ctrl+` / `Alt+` / `Shift+` 前缀 |
| `webview.zoom` | number | `1.0` | 页面缩放，范围 0.5 到 2.0 |
| `webview.width` | number | `1000` | 面板宽度，像素 |
| `webview.height` | number | `640` | 面板高度，像素 |
| `webview.windowMode` | number | `0` | `0` 独占全屏，`1` 窗口，`2` 无边框；窗口与无边框需要在游戏启动前决定，因此修改后重启游戏生效 |

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

窗口模式说明：独占全屏时 DWM 不合成游戏窗口，面板只能以抓帧预览显示，帧率受限且不支持文字输入；把 `windowMode` 设为 `2` 并重启游戏后，游戏以无边框窗口运行，面板恢复原生渲染。无边框窗口是普通层级，其他程序仍可覆盖游戏，任务栏也在其之上。

## 构建

依赖 XBase SDK。先构建 XBase，它会把头文件与静态库暂存到本项目的 `include\XBase` 与 `lib`：

```bash
XBase\Build.bat Release --no-pause
III.VC.SA.WebView2\Build.bat Release --no-pause
```

产物：

```txt
build\bin\WebView2.asi
build\bin\XBase\Mods\WebView2\WebView2SA.dll
build\bin\XBase\Mods\WebView2\WebView2VC.dll
build\bin\XBase\Mods\WebView2\WebView2III.dll
XBase\lib\WebView2Loader.dll（由 XBase 构建统一分发到 <游戏根目录>\XBase\）
```

## 代码结构

| 路径 | 职责 |
|---|---|
| `loader/LoaderAnchor.cpp` | 只保留一个编译单元，让 MSVC 链接时用 WHOLEARCHIVE 拉入 XBase 的 bootstrap 入口；导出 `XBasePayloadBaseName` 声明载荷基名 |
| `src/main.cpp` | 启动校验、配置读取、Host 注册、热键处理；导出 `XBasePayloadAttach` / `XBasePayloadDetach` |
| `src/Panel.cpp` | 面板窗口、视口提交、抓帧回显与输入转发、加载失败处理 |
| `lib/` `include/` | XBase 构建时暂存的 SDK，不要手工修改 |

加载链路：

```txt
WebView2.asi
└─ XBase Bootstrap 检测游戏版本
   └─ 载入 XBase\Mods\WebView2\WebView2<游戏>.dll
      └─ XBasePayloadAttach
         ├─ Runtime::ValidateEnvironment
         ├─ Config::Init 并准备窗口模式
         └─ Host::Install
```

面板每帧流程：

```txt
OnProcess
├─ Panel::Process   同步可见性、菜单关闭时隐藏
├─ Core::Process    WebView 域创建/抓帧/光标
└─ Hooks::MaintainInputState
Draw 回调
├─ Input::PollSystemKeys 与热键切换   暂停时依然生效
└─ Panel::Draw
   ├─ WebView::SetBounds
   ├─ WebView::DrawPanel        仅抓帧模式
   └─ WebView::ForwardPanelInput 仅抓帧模式
```

## 与 XBase 的关系

本项目不包含任何游戏地址，全部能力来自 XBase 公共 API：

| 用途 | 入口 |
|---|---|
| 生命周期 | `XBase::Host::Install`、`XBase::Core::Init/Process/Shutdown` |
| 渲染与输入 | `XBase::Hooks::RegisterDrawCallback`、`MaintainInputState`、`SetMenuVisible` |
| 窗口模式 | `XBase::Hooks::PrepareStartupWindowMode` |
| 网页视图 | `XBase::WebView::Init/Navigate/SetVisible/SetBounds/DrawPanel/ForwardPanelInput` |
| 界面与配置 | `XBase::UI`、`XBase::Config`、`XBase::Input` |

完整 API、能力矩阵与边界说明见 XBase 文档：<https://blog.miomoe.cn/docs/xbase/>

## 排查路径

1. 游戏目录下确认 `WebView2.asi` 存在，且 `<游戏根目录>\XBase\Mods\WebView2\` 内有与游戏匹配的载荷 DLL
2. 查看 `<游戏根目录>\XBase\Mods\WebView2\debug.log`，正常启动应出现 `WebView2: Host 注册通过` 与 `WebView2: 渲染后端就绪`
3. 提示 `Failed to detect supported GTA runtime` 表示游戏版本不在支持列表
4. 提示找不到载荷文件时，按日志里的期望路径核对目录与文件名
5. 面板出现但没有画面：确认 `<游戏根目录>\XBase\WebView2Loader.dll` 存在，且系统已安装 WebView2 运行时
6. 面板帧率低或不支持文字输入：当前是独占全屏抓帧预览，按配置章节切换到无边框窗口模式
7. 热键无效：确认 `webview.hotkey` 写法正确，日志会记录解析失败并回退到默认值
