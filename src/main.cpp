#include <XBase/Config.h>
#include <XBase/Core.h>
#include <XBase/Hooks.h>
#include <XBase/Host.h>
#include <XBase/Input.h>
#include <XBase/Log.h>
#include <XBase/Platform.h>
#include <XBase/Runtime.h>
#include <XBase/WebBridge.h>
#include <XBase/WebView.h>

#include "Panel.h"

#include <string>

namespace {

const char* PLUGIN_NAME = "III.VC.SA.WebView2";
const char* PLUGIN_VERSION = "v0.0.1";
constexpr const char* DefaultUrl = "https://gtamodx.com/";
constexpr const char* DefaultHotkey = "F8";
constexpr const char* ConfigFileName = "config.json";

int s_bootstrapStage = -1;
bool s_pluginActive = false;
XBase::Hooks::DrawCallbackId s_drawCallbackId{};
XBase::Input::Hotkey s_hotkey{};
bool s_hotkeyValid = false;

XBase::Hooks::WindowMode ToEngineWindowMode(int mode) {
    switch (mode) {
    case 1: return XBase::Hooks::WindowMode::Windowed;
    case 2: return XBase::Hooks::WindowMode::Borderless;
    default: return XBase::Hooks::WindowMode::Fullscreen;
    }
}

// 配置文件与载荷同目录，首次运行写出默认值方便直接改
void InitConfig() {
    XBase::Config::InitForMod("WebView2");
    if (XBase::Config::HasKey("webview.url")) {
        return;
    }

    XBase::Config::SetString("webview.url", DefaultUrl);
    XBase::Config::SetString("webview.hotkey", DefaultHotkey);
    XBase::Config::SetFloat("webview.zoom", 1.0f);
    XBase::Config::SetFloat("webview.width", 1000.0f);
    XBase::Config::SetFloat("webview.height", 640.0f);
    XBase::Config::SetInt("webview.windowMode", 0);
    XBase::Config::Save();
}

void LoadConfig() {
    Panel::SetUrl(XBase::Config::GetString("webview.url", DefaultUrl));
    Panel::SetZoom(XBase::Config::GetFloat("webview.zoom", 1.0f));
    Panel::SetSize(
        XBase::Config::GetFloat("webview.width", 1000.0f),
        XBase::Config::GetFloat("webview.height", 640.0f));

    const std::string hotkey = XBase::Config::GetString("webview.hotkey", DefaultHotkey);
    s_hotkeyValid = XBase::Input::ParseHotkey(hotkey, s_hotkey);
    if (!s_hotkeyValid) {
        XBase::Log::Warn("WebView2: 热键解析失败，回退到默认热键");
        s_hotkeyValid = XBase::Input::ParseHotkey(DefaultHotkey, s_hotkey);
    }
}

void AdvanceBootstrap() {
    if (s_bootstrapStage < 0 || s_bootstrapStage >= 3) return;

    if (s_bootstrapStage == 0) {
        XBase::Log::Info(std::string(PLUGIN_NAME) + " " + PLUGIN_VERSION + " 初始化中");
        LoadConfig();
        XBase::Core::Init(XBase::Core::DomainBit(XBase::Core::Domain::WebView));
        s_bootstrapStage = 1;
        return;
    }

    if (s_bootstrapStage == 1) {
        if (!XBase::Core::IsWorldReady()) return;
        Panel::Init();
        s_bootstrapStage = 2;
        return;
    }

    s_drawCallbackId = XBase::Hooks::RegisterDrawCallback([]() {
        if (!s_pluginActive) {
            return;
        }

        // 面板占住焦点后游戏可能进入暂停，脚本事件停摆，热键只能在渲染回调里补
        XBase::Input::PollSystemKeys();
        if (s_hotkeyValid && XBase::Input::WasPressed(s_hotkey)) {
            Panel::Toggle();
        }

        Panel::Draw();
    });
    if (!static_cast<bool>(s_drawCallbackId) || !XBase::Hooks::Init()) {
        if (s_drawCallbackId) {
            XBase::Hooks::UnregisterDrawCallback(s_drawCallbackId);
            s_drawCallbackId = {};
        }
        XBase::Log::Error("WebView2: 渲染后端初始化失败，插件停止");
        s_bootstrapStage = -1;
        return;
    }

    s_pluginActive = true;
    s_bootstrapStage = 3;
    XBase::Log::Info("WebView2: 渲染后端就绪");
}

void OnGameInit() {
    XBase::Core::NotifyGameInit();
    s_bootstrapStage = 0;
}

void OnProcess() {
    if (!s_pluginActive) {
        if (s_bootstrapStage >= 0) {
            AdvanceBootstrap();
        }
        return;
    }

    Panel::Process();
    XBase::Core::Process();
    XBase::Hooks::MaintainInputState();

    if (s_hotkeyValid && XBase::Input::WasPressed(s_hotkey)) {
        Panel::Toggle();
    }
}

} // namespace

extern "C" void XBasePayloadAttach() {
    XBase::Log::InitForMod("WebView2");
    XBase::Log::Info("WebView2: 载荷已加载，开始启动校验");

    const XBase::Runtime::ValidationResult validation = XBase::Runtime::ValidateEnvironment();
    if (!validation.success) {
        XBase::Log::Error("WebView2: 游戏版本不受支持，插件停止初始化");
        return;
    }

    // 网页面板需要 DWM 合成，窗口模式必须在游戏创建设备之前决定
    InitConfig();
    const int windowMode = XBase::Config::GetInt("webview.windowMode", 0);
    if (windowMode == 1 || windowMode == 2) {
        XBase::Hooks::PrepareStartupWindowMode(ToEngineWindowMode(windowMode));
    }

    if (!XBase::Host::Install({OnGameInit, OnProcess})) {
        XBase::Log::Error("WebView2: Host 注册失败，插件停止初始化");
        return;
    }

    // 网页可以直接调用 XBase 公共 API，前端框架因此可以替代 ImGui 写界面
    XBase::WebBridge::Install();

    s_bootstrapStage = 0;
    XBase::Log::Info("WebView2: Host 注册通过");
}

extern "C" void XBasePayloadDetach() {
    if (s_drawCallbackId) {
        XBase::Hooks::UnregisterDrawCallback(s_drawCallbackId);
        s_drawCallbackId = {};
    }
    Panel::SetVisible(false);
    XBase::WebBridge::Shutdown();
    XBase::Host::Shutdown();
    XBase::Core::Shutdown();
    XBase::Hooks::Shutdown();
    XBase::Log::Shutdown();
}
