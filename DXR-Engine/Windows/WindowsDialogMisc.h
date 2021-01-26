#pragma once
#include "Application/Generic/GenericDialogMisc.h"

#include "Windows.h"

class WindowsDialogMisc : public GenericDialogMisc
{
public:
    static FORCEINLINE void MessageBox(const std::string& Title, const std::string& Message)
    {
        ::MessageBoxA(0, Message.c_str(), Title.c_str(), MB_ICONERROR | MB_OK);
    }
};