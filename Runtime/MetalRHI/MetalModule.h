#pragma once
#include "MetalCore.h"

#include "RHI/RHIModule.h"

struct FMetalModule final 
    : public FRHIModule
{
    FMetalModule()  = default;
    ~FMetalModule() = default;

    virtual class FRHIInterface* CreateInterface() override final;
};
