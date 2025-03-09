#pragma once
#include "RHI/RHIShader.h"
#include "RHI/RHIResources.h"

class RENDERERCORE_API FTextureCompressor
{
public:
    FTextureCompressor();
    ~FTextureCompressor();

    bool Initialize();

    bool CompressBC6(const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture);
    bool CompressBC6(FRHICommandList& CommandList, const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture);

    bool CompressCubeMapBC6(const FRHITextureRef& SrcCubeMap, FRHITextureRef& OutCubeMap);
    bool CompressCubeMapBC6(FRHICommandList& CommandList, const FRHITextureRef& SrcCubeMap, FRHITextureRef& OutCubeMap);

private:
    FRHIComputeShaderRef        BC6HCompressionShader;
    FRHIComputePipelineStateRef BC6HCompressionPSO;
    FRHIComputeShaderRef        BC6HCompressionCubeShader;
    FRHIComputePipelineStateRef BC6HCompressionCubePSO;
    FRHISamplerStateRef         PointSampler;
};