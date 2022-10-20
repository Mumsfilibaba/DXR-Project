#pragma once
#include "Core/Core.h"

#include "RHI/RHIInterface.h"

#if NULLRHI_IMPL
    #define NULLRHI_API MODULE_EXPORT
#else
    #define NULLRHI_API 
#endif

struct NULLRHI_API FNullRHIModule final
    : public FRHIInterfaceModule
{
    virtual FRHIInterface* CreateInterface() override final;
};
