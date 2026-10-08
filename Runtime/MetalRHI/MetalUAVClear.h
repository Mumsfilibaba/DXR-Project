#pragma once
#include "RHI/RHIPipelineState.h"
#include "RHI/RHIShader.h"
#include "ShaderCore/MSLShaderBindings.h"

class FMetalUnorderedAccessViewRHI;
class FMetalCommandContext;
class FMetalComputePipelineStateRHI;

enum class EMetalUAVClearTarget : uint8
{
    Buffer         = 0,
    Texture1D      = 1,
    Texture1DArray = 2,
    Texture2D      = 3,
    Texture2DArray = 4,
    Texture3D      = 5,
    Count          = 6,
};

class FMetalUAVClearPipelines
{
public:
    void Release();
    FMetalComputePipelineStateRHI* GetOrCreate(EMetalUAVClearTarget Target, EMSLTextureComponent Component);
    FMetalComputePipelineStateRHI* GetOrCreateRaw();

private:
    static constexpr int32 NumComponents = 3;
    static constexpr int32 NumPipelines  = static_cast<int32>(EMetalUAVClearTarget::Count) * NumComponents;

    FRHIComputeShaderRef        Shaders[NumPipelines];
    FRHIComputePipelineStateRef Pipelines[NumPipelines];
    FRHIComputeShaderRef        RawShader;
    FRHIComputePipelineStateRef RawPipeline;
};

struct MetalUAVClear
{
    static void Clear(FMetalCommandContext& Context, FMetalUnorderedAccessViewRHI* View, const uint32 Values[4]);
};
