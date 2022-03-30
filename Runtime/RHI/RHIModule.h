#pragma once
#include "RHICore.h"

#include "Core/Modules/ModuleManager.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIType

enum class ERHIType : uint32
{
    Unknown = 0,
    Null    = 1,
    D3D12   = 2,
    Vulkan  = 3,
};

inline const char* ToString(ERHIType RenderLayerApi)
{
    switch (RenderLayerApi)
    {
    case ERHIType::D3D12:  return "D3D12";
    case ERHIType::Vulkan: return "Vulkan";
    case ERHIType::Null:   return "Null";
    default:               return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// RHI Functions

RHI_API bool RHIInitialize(ERHIType InRenderApi);
RHI_API void RHIRelease();

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIModuleInterface

class RHI_API CRHIModuleInterface : public CDefaultEngineModule
{
public:

    /**
     * @brief: Creates the RHI Instance
     * 
     * @return: Returns the newly created RHIInstance
     */
    virtual class CRHIInstance* CreateInterface() = 0;

    /**
     * @brief: Creates the RHI shader compiler 
     * 
     * @return: Returns the shader compiler for this RHI Module
     */
    virtual class IRHIShaderCompiler* CreateCompiler() = 0;

protected:

    CRHIModuleInterface()  = default;
    ~CRHIModuleInterface() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Global variables

extern RHI_API class CRHIInstance*       GRHIInstance;
extern RHI_API class IRHIShaderCompiler* GShaderCompiler;
