#include "WindowPlacement.h"
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
bool positionCurveWindowLeft(void* childHandle){
    auto child=static_cast<HWND>(childHandle);if(!child||!IsWindow(child))return false;
    auto root=GetAncestor(child,GA_ROOT);auto owner=GetWindow(root,GW_OWNER);
    // A hosted plugin is an owned top-level window. Without an owner we do
    // not know that this isn't the host's main frame, so leave it alone.
    if(!root||!owner||root==GetAncestor(owner,GA_ROOT))return false;
    MONITORINFO info{};info.cbSize=sizeof(info);
    if(!GetMonitorInfo(MonitorFromWindow(root,MONITOR_DEFAULTTONEAREST),&info))return false;
    return SetWindowPos(root,nullptr,info.rcWork.left+12,info.rcWork.top+12,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE)!=0;
}
#else
bool positionCurveWindowLeft(void*){return false;}
#endif
