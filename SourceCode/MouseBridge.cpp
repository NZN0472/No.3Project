#include "MouseBridge.h"
#include <Windows.h>

bool MouseBridge::prevL = false;
bool MouseBridge::curL = false;
bool MouseBridge::trgL = false;

void MouseBridge::Update()
{
    prevL = curL;
    curL = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    trgL = (curL && !prevL);
}

bool MouseBridge::TrgL()
{
    return trgL;
}

bool MouseBridge::DownL()
{
    return curL;
}
