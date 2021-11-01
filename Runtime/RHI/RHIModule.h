#pragma once
#include "RHIAPI.h"

#include "Core/Modules/IEngineModule.h"

// TODO: Should be in a config file
#define ENABLE_API_DEBUGGING       (0)
#define ENABLE_API_GPU_DEBUGGING   (0)
#define ENABLE_API_GPU_BREADCRUMBS (0)

///////////////////////////////////////////////////////////////////////////////////////////////////

enum class ERHIModule : uint32
{
    Unknown = 0,
    Null = 1,
    D3D12 = 2,
};

inline const char* ToString( ERHIModule RenderLayerApi )
{
    switch ( RenderLayerApi )
    {
        case ERHIModule::D3D12: return "D3D12";
        case ERHIModule::Null:  return "Null";
        default:                return "Unknown";
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

RHI_API bool InitRHI( ERHIModule InRenderApi );
RHI_API void ReleaseRHI();

///////////////////////////////////////////////////////////////////////////////////////////////////

class RHI_API CRHIModule : public CDefaultEngineModule
{
public:

    /* Creates the core RHI object */
    virtual class CRHICore* CreateCore() = 0;

    /* Creates the RHI shader compiler */
    virtual class IRHIShaderCompiler* CreateCompiler() = 0;

protected:

    CRHIModule() = default;
    ~CRHIModule() = default;
};

///////////////////////////////////////////////////////////////////////////////////////////////////

extern RHI_API class CRHICore* GRHICore;
extern RHI_API class IRHIShaderCompiler* GShaderCompiler;