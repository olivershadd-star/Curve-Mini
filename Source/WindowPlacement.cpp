#include "WindowPlacement.h"
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cwchar>
bool positionCurveWindowLeft(void* childHandle){
    auto child=static_cast<HWND>(childHandle);if(!child||!IsWindow(child))return false;
    auto root=GetAncestor(child,GA_ROOT);auto owner=GetWindow(root,GW_OWNER);
    // A hosted plugin is an owned top-level window. Without an owner we do
    // not know that this isn't the host's main frame, so leave it alone.
    if(!root||!owner||root==GetAncestor(owner,GA_ROOT))return false;
    wchar_t title[512]{};const int length=GetWindowTextW(root,title,512);
    if(length<=0)return false;CharLowerBuffW(title,(DWORD)length);
    // Ownership alone is insufficient: some DAWs own their main frame via
    // a hidden helper window. Only a positively identified Mini popup moves.
    if(std::wcsstr(title,L"curve mini")==nullptr||std::wcsstr(title,L"ableton")!=nullptr||std::wcsstr(title,L" live")!=nullptr)return false;
    if((GetWindowLongPtrW(root,GWL_EXSTYLE)&WS_EX_APPWINDOW)!=0)return false;
    MONITORINFO info{};info.cbSize=sizeof(info);
    if(!GetMonitorInfo(MonitorFromWindow(root,MONITOR_DEFAULTTONEAREST),&info))return false;
    return SetWindowPos(root,nullptr,info.rcWork.left+12,info.rcWork.top+12,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE)!=0;
}
#else
bool positionCurveWindowLeft(void*){return false;}
#endif
