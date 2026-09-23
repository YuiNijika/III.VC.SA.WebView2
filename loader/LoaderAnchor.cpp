// The loader DLL owns no logic of its own. This anchor keeps one compile item
// so MSVC runs Link and pulls the XBaseBootstrap entry object with WHOLEARCHIVE.
extern "C" void WebView2LoaderAnchor() {
}

// XBase bootstrap 用它决定载荷目录与文件名：<游戏根目录>\XBase\Mods\WebView2\WebView2<游戏>.dll
extern "C" __declspec(dllexport) const char* XBasePayloadBaseName() {
    return "WebView2";
}
