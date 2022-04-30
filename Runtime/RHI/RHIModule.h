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
#define ENABLE_API_DEBUGGING       (0)
#define ENABLE_API_GPU_DEBUGGING   (0)
#define ENABLE_API_GPU_BREADCRUMBS (0)

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERHIInstanceType

enum class ERHIInstanceType : uint32
{
    Unknown = 0,
    Null    = 1,
    D3D12   = 2,
};

inline const char* ToString(ERHIInstanceType RenderLayerApi)
{
    switch (RenderLayerApi)
    {
    case ERHIInstanceType::D3D12: return "D3D12";
    case ERHIInstanceType::Null:  return "Null";
    default:                      return "Unknown";
    }
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// RHI Functions

RHI_API bool RHIInitialize(ERHIInstanceType InRenderApi);
RHI_API void RHIRelease();

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIModule

class RHI_API CRHIModule : public CDefaultEngineModule
{
public:

     /** @brief: Creates the core RHI object */
    virtual class CRHIInstance* CreateInterface() = 0;

     /** @brief: Creates the RHI shader compiler */
    virtual class IRHIShaderCompiler* CreateCompiler() = 0;

protected:

    CRHIModule() = default;
    ~CRHIModule() = default;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Global variables

extern RHI_API class CRHIInstance*       GRHIInstance;
extern RHI_API class IRHIShaderCompiler* GShaderCompiler;
