#include "Windows/Windows.h"

#include "Main/EngineLoop.h"

#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

int WINAPI WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, int CmdShow)
{
    return EngineMain();
}

#pragma warning(pop)