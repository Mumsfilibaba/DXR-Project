#pragma once
#include "MetalCore.h"

#include "RHI/RHIModule.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalModule

class CMetalModule final : public CRHIModule
{
public:

    CMetalModule()  = default;
    ~CMetalModule() = default;

    virtual class CRHICoreInterface*  CreateInterface() override final;
    virtual class IRHIShaderCompiler* CreateCompiler()  override final;
};
