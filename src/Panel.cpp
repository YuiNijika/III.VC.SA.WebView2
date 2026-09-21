#include "Panel.h"

#include <XBase/Hooks.h>
#include <XBase/Host.h>
#include <XBase/Log.h>

#include <algorithm>

namespace {
    bool s_visible = false;
    std::string s_url = "https://gtamodx.com/";
    float s_zoom = 1.0f;
    float s_width = 1000.0f;
    float s_height = 640.0f;
    bool s_fullscreenNoticeShown = false;

    constexpr const char* kWindowId = "WebView2Panel";
    constexpr const char* kTitle = "MOD Download";

    XBase::Rect ViewportRect(const XBase::Rect& windowRect) {
        const float padding = 8.0f;
        XBase::Rect rect{};
        rect.left = windowRect.left + padding;
        rect.top = windowRect.top + padding;
        rect.right = windowRect.right - padding;
        rect.bottom = windowRect.bottom - padding;
        if (rect.right - rect.left < 24.0f || rect.bottom - rect.top < 24.0f) {
            rect.right = rect.left + 1.0f;
            rect.bottom = rect.top + 1.0f;
        }
        return rect;
    }

    void DrawOverlayMessage(const XBase::Rect& area, const char* message, bool retry) {
        const XBase::Vec2 center{
            (area.left + area.right) * 0.5f,
            (area.top + area.bottom) * 0.5f};
        XBase::UI::SetCursorScreenPos({center.x - 160.0f, center.y - 40.0f});
        XBase::UI::TextWrapped(message);
        XBase::UI::SetCursorScreenPos({center.x - 70.0f, center.y + 8.0f});
        if (retry && XBase::UI::Button("Reload", {140.0f, 32.0f})) {
            XBase::WebView::Reload();
        }
    }
}

namespace Panel {

void Init() {
    XBase::WebView::SetZoom(s_zoom);
}

void Process() {
    if (s_visible != XBase::WebView::IsVisible()) {
        s_visible = XBase::WebView::IsVisible();
    }
    if (s_visible && !XBase::Hooks::IsMenuVisible()) {
        SetVisible(false);
    }
}

void Draw() {
    if (!XBase::WebView::IsRuntimeAvailable()) {
        return;
    }

    XBase::UI::SetNextWindowSize({s_width, s_height}, true);
    bool open = true;
    XBase::UI::Window(kWindowId, kTitle, [&] {
        const XBase::Rect windowRect = XBase::UI::GetCurrentWindowRect();
        const XBase::Rect area = ViewportRect(windowRect);
        const XBase::Vec2 size{area.right - area.left, area.bottom - area.top};
        XBase::UI::SetCursorScreenPos({area.left, area.top});
        XBase::UI::InvisibleButton("##WebArea", size);

        if (!s_visible) {
            DrawOverlayMessage(area, "Panel is closed. Press the hotkey to open it.", true);
            return;
        }

        XBase::WebView::SetBounds(area);
        if (XBase::WebView::UsesCaptureMode()) {
            XBase::WebView::DrawPanel(area);
            if (XBase::UI::IsLastItemHovered()) {
                XBase::WebView::ForwardPanelInput(
                    area,
                    XBase::UI::GetMousePosition(),
                    XBase::UI::IsMouseDown(XBase::UI::MouseButton::Left),
                    XBase::Hooks::ConsumeWheelDelta());
            }
        }

        const XBase::WebView::State state = XBase::WebView::GetState();
        if (state.lastError != 0) {
            DrawOverlayMessage(area, "The page failed to load. Check the network and retry.", true);
        }
    }, &open);

    if (!open) {
        SetVisible(false);
    }
}

void Toggle() {
    SetVisible(!s_visible);
}

void SetVisible(bool visible) {
    if (visible == s_visible) {
        return;
    }
    s_visible = visible;
    XBase::Hooks::SetMenuVisible(visible);
    if (!visible) {
        XBase::WebView::SetVisible(false);
        s_fullscreenNoticeShown = false;
        return;
    }

    XBase::WebView::Init();
    XBase::WebView::Navigate(s_url);
    XBase::WebView::SetZoom(s_zoom);
    XBase::WebView::SetVisible(true);

    if (!s_fullscreenNoticeShown && XBase::Hooks::IsGameWindowFullscreen()) {
        s_fullscreenNoticeShown = true;
        XBase::Host::QueueMessage("Exclusive fullscreen only shows a captured preview; use borderless windowed mode for the native page.");
    }
}

bool IsVisible() {
    return s_visible;
}

void SetUrl(const std::string& url) {
    if (url.empty()) {
        return;
    }
    s_url = url;
}

const std::string& GetUrl() {
    return s_url;
}

void SetZoom(float zoom) {
    s_zoom = std::clamp(zoom, 0.5f, 2.0f);
    XBase::WebView::SetZoom(s_zoom);
}

float GetZoom() {
    return s_zoom;
}

void SetSize(float width, float height) {
    s_width = std::clamp(width, 480.0f, 3840.0f);
    s_height = std::clamp(height, 320.0f, 2160.0f);
}

} // namespace Panel
