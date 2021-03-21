#pragma once
#include "Core/Application/Generic/GenericMisc.h"

#include "Windows.h"

class WindowsMisc : public GenericMisc
{
public:
    FORCEINLINE static void MessageBox(const std::string& Title, const std::string& Message)
    {
        MessageBoxA(0, Message.c_str(), Title.c_str(), MB_ICONERROR | MB_OK);
    }

    FORCEINLINE static void RequestExit(int32 ExitCode)
    {
        PostQuitMessage(ExitCode);
    }
};