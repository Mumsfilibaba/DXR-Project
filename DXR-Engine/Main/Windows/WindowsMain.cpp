#include "Windows/Windows.h"

#include "Main/EngineLoop.h"

#include "Core/Delegates/Delegate.h"
#include "Core/Delegates/MulticastDelegate.h"

#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

int Function(int i)
{
    return i + 1;
}

struct A
{
    int Func(int i)
    {
        return i + 3;
    }
};

int WINAPI WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, int CmdShow)
{
    TDelegate<void(int)> Delegate;
    Delegate.Unbind();

    TMulticastDelegate<void(int)> Delegates;
    return EngineMain();
}

#pragma warning(pop)