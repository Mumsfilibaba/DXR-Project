#include "Windows/Windows.h"

#include "Main/EngineLoop.h"

#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

int WINAPI WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, int CmdShow)
{
    TArrayView<const Char*> Args;
    return EngineMain(Args);
}

#pragma warning(pop)