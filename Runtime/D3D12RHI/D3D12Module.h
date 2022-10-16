#pragma once
#include "Core/Core.h"

#include "RHI/RHIInterface.h"

struct FD3D12InterfaceModule final
    : public FRHIInterfaceModule
{
    FD3D12InterfaceModule()  = default;
    ~FD3D12InterfaceModule() = default;

    virtual class FRHIInterface* CreateInterface() override final;
};