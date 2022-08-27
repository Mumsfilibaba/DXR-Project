#pragma once
#include "MetalCore.h"

#include "RHI/RHIModule.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalModule

struct FMetalModule final : public FRHIModule
{
    FMetalModule()  = default;
    ~FMetalModule() = default;

    virtual class FRHICoreInterface* CreateInterface() override final;
};
