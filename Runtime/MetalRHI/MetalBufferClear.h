#pragma once
#include "RHI/RHIResources.h"
#include "RHI/RHIPipelineState.h"
#include "RHI/RHIShader.h"

class FMetalUnorderedAccessViewRHI;
class FMetalCommandContext;

struct EMetalBufferClearType
{
    enum Type : uint8
    {
        Float = 0,
        Uint  = 1,
        Sint  = 2,
        Count = 3,
    };
};

class FMetalBufferClearPipelines
{
public:
    void Release();
    bool GetOrCreate(EMetalBufferClearType::Type ClearType, FRHIComputeShader*& OutShader, FRHIComputePipelineState*& OutPipeline);

private:
    FRHIComputeShaderRef        Shaders[EMetalBufferClearType::Count];
    FRHIComputePipelineStateRef Pipelines[EMetalBufferClearType::Count];
};

struct MetalClearBufferUAV
{
    static void Clear(FMetalCommandContext& Context, FMetalUnorderedAccessViewRHI* View, const uint32 Values[4], bool bIsFloat);
};
