#pragma once
#include "Core/Modules/ModuleManager.h"

#if MONOLITHIC_BUILD
#define RHI_API
#else
#if RHI_IMPL
#define RHI_API MODULE_EXPORT
#else
#define RHI_API MODULE_IMPORT
#endif
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Config

// TODO: Should be in a config file
#define ENABLE_API_DEBUGGING       (1)
#define ENABLE_API_GPU_DEBUGGING   (0) // D3D12
#define ENABLE_API_GPU_BREADCRUMBS (0) // D3D12

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
    default:                      return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// RHI Functions

RHI_API bool RHIInitialize(ERHIType InRenderApi);
RHI_API void RHIRelease();

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIModule

class RHI_API CRHIModule : public CDefaultEngineModule
{
public:

    /**
     * Creates the RHI Instance
     * 
     * @return: Returns the newly created RHIInstance
     */
    virtual class CRHIInstance* CreateInterface() = 0;

    /**
     * Creates the RHI shader compiler 
     * 
     * @return: Returns the shader compiler for this RHI Module
     */
    virtual class IRHIShaderCompiler* CreateCompiler() = 0;

protected:

    CRHIModule()  = default;
    ~CRHIModule() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Global variables

extern RHI_API class CRHIInstance*       GRHIInstance;
extern RHI_API class IRHIShaderCompiler* GShaderCompiler;
