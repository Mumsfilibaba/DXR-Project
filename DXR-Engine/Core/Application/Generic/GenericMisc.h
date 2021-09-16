#pragma once
#include "Core.h"

class CGenericMisc
{
public:

    /* Creates an output console if the platform supports it */
    static FORCEINLINE class CGenericOutputConsole* CreateOutputConsole()
    {
        return nullptr;
    }
};
