#pragma once
#include "Core.h"

// TODO: Should be in a config file
#define ENABLE_API_DEBUGGING       (0)
#define ENABLE_API_GPU_DEBUGGING   (0)
#define ENABLE_API_GPU_BREADCRUMBS (0)

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

class CRHIModule
{
public:
    static bool Init( ERHIModule InRenderApi );
    static void Release();
};

extern CORE_API class CRHICore* GRHICore;
extern CORE_API class IRHIShaderCompiler* GShaderCompiler;