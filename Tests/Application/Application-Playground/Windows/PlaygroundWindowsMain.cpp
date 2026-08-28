#include <Core/Misc/Debug.h>
#include <Core/Windows/Windows.h>

#include "PlaygroundLoop.h"

// The counterpart to Runtime/Launch/Windows/WindowsMain.cpp, calling the playground loop rather than
// EngineMain. The Agility SDK exports are deliberately left out: the playground draws a UI overlay and
// never asks for a feature that needs them.

DISABLE_UNREFERENCED_VARIABLE_WARNING

int WINAPI WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int CmdShow)
{
    const CHAR* LocalCommandLine = CommandLine;
    return PlaygroundMain(&LocalCommandLine, 1);
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
