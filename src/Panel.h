#pragma once

#include <XBase/UI.h>
#include <XBase/WebView.h>

#include <string>

// 独立网页面板，不依赖菜单，热键开关，位置与尺寸由配置决定
namespace Panel {

void Init();
void Process();
void Draw();

void Toggle();
void ClosePanel();
void SetVisible(bool visible);
bool IsVisible();

void SetUrl(const std::string& url);
const std::string& GetUrl();

void SetZoom(float zoom);
float GetZoom();

void SetSize(float width, float height);

} // namespace Panel
