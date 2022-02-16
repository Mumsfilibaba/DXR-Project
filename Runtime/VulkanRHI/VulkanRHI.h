#pragma once
#include "Core/Core.h"

#include "RHI/RHIModule.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CVulkanRHI

class CVulkanRHI final : public CRHIModule
{
public:

    CVulkanRHI()  = default;
    ~CVulkanRHI() = default;

    virtual class CRHIInstance*       CreateInterface() override final;
    virtual class IRHIShaderCompiler* CreateCompiler()  override final;
};