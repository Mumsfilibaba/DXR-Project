#pragma once
#include "Core/Core.h"

#include "RHI/RHIInterface.h"

#if NULLRHI_IMPL
    #define NULLRHI_API MODULE_EXPORT
#else
    #define NULLRHI_API 
#endif

struct FNullRHIModule final 
    : public FRHIInterfaceModule
{
    FNullRHIModule()  = default;
    ~FNullRHIModule() = default;

    virtual class FRHIInterface* CreateInterface() override final;
};
