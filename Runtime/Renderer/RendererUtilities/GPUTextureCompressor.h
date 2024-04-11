#pragma once
#include "RHI/RHIShader.h"
#include "RHI/RHIResources.h"

class FGPUTextureCompressor
{
public:
    FGPUTextureCompressor();

    bool Initialize();

    bool CompressBC6(const FRHITextureRef& Source, FRHITextureRef& Output);
    bool CompressCubeMapBC6(const FRHITextureRef& Source, FRHITextureRef& Output);

private:
    FRHIComputeShaderRef        BC6HCompressionShader;
    FRHIComputePipelineStateRef BC6HCompressionPSO;
    FRHIComputeShaderRef        BC6HCompressionCubeShader;
    FRHIComputePipelineStateRef BC6HCompressionCubePSO;
    FRHISamplerStateRef         PointSampler;
};