#pragma once

#if defined(PLATFORM_WINDOWS)
#include "Core/Application/Generic/GenericMisc.h"
#include "Core/Application/Windows/WindowsOutputConsole.h"

#include "Windows.h"

class CWindowsMisc : public CGenericMisc
{
public:

    /* Creates a output console if the platform supports it */
    static FORCEINLINE CGenericOutputConsole* CreateOutputConsole()
    {
        return DBG_NEW WindowsOutputConsole();
    }
};

#endif