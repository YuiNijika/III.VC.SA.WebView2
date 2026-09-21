// The loader DLL owns no logic of its own. This anchor keeps one compile item
// so MSVC runs Link and pulls the XBaseBootstrap entry object with WHOLEARCHIVE.
extern "C" void WebView2LoaderAnchor() {
}

// XBase bootstrap 用它决定载荷目录与文件名：WebView2\WebView2SA.dll，
// 主 ASI 自身仍叫 III.VC.SA.WebView2.asi
extern "C" const char* XBasePayloadBaseName() {
    return "WebView2";
}
