#ifdef NDEBUG
#undef NDEBUG
#endif
#define NOMINMAX
#include <windows.h>
#include <cassert>
#include <iostream>
#include "WindowPlacement.h"
static RECT rect(HWND h){RECT r{};assert(GetWindowRect(h,&r));return r;}
int main(){
    assert(!positionCurveWindowLeft(nullptr));
    auto owner=CreateWindowExW(0,L"STATIC",L"Test owner",WS_OVERLAPPEDWINDOW,350,200,300,200,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);assert(owner);
    // Even an owned main frame whose document name contains Curve Mini stays put.
    auto host=CreateWindowExW(WS_EX_TOOLWINDOW,L"STATIC",L"Curve Mini project - Ableton Live",WS_OVERLAPPEDWINDOW,450,300,300,200,owner,nullptr,GetModuleHandleW(nullptr),nullptr);assert(host);
    auto before=rect(host);assert(!positionCurveWindowLeft(host));auto after=rect(host);assert(before.left==after.left&&before.top==after.top);
    auto unrelated=CreateWindowExW(WS_EX_TOOLWINDOW,L"STATIC",L"Other plugin",WS_OVERLAPPEDWINDOW,450,300,300,200,owner,nullptr,GetModuleHandleW(nullptr),nullptr);assert(unrelated);assert(!positionCurveWindowLeft(unrelated));
    auto plugin=CreateWindowExW(WS_EX_TOOLWINDOW,L"STATIC",L"Curve Mini",WS_OVERLAPPEDWINDOW,450,300,300,200,owner,nullptr,GetModuleHandleW(nullptr),nullptr);assert(plugin);
    assert(positionCurveWindowLeft(plugin));MONITORINFO info{};info.cbSize=sizeof(info);assert(GetMonitorInfo(MonitorFromWindow(plugin,MONITOR_DEFAULTTONEAREST),&info));auto moved=rect(plugin);assert(moved.left==info.rcWork.left+12&&moved.top==info.rcWork.top+12);
    DestroyWindow(plugin);DestroyWindow(unrelated);DestroyWindow(host);DestroyWindow(owner);
    std::cout<<"PASS: only an identified Mini floating window moves; host and unrelated windows stay unchanged\n";
}
