#pragma once

#if defined(PLATFORM_MACOS) 
#include "Core/Application/Generic/GenericMisc.h"
#include "Core/Application/Mac/MacOutputConsole.h"

class CMacMisc : public CGenericMisc
{
public:

    /* Creates an output console if the platform supports it */
    static FORCEINLINE class CGenericOutputConsole* CreateOutputConsole()
    {
        return new CMacOutputConsole();
    }
};

#endif
