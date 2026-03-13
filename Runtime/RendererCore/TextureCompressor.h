#pragma once
#include "RHI/RHIShader.h"
#include "RHI/RHIResources.h"

class RENDERERCORE_API FTextureCompressor
{
public:
    FTextureCompressor();
    ~FTextureCompressor();

    bool Initialize();

    bool CompressBC1(const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture);
    bool CompressBC1(FRHICommandList& CommandList, const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture);

    bool CompressBC2(const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture);
    bool CompressBC2(FRHICommandList& CommandList, const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture);

    bool CompressBC3(const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture);
    bool CompressBC3(FRHICommandList& CommandList, const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture);

    bool CompressBC4(const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture);
    bool CompressBC4(FRHICommandList& CommandList, const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture);

    bool CompressBC5(const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture);
    bool CompressBC5(FRHICommandList& CommandList, const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture);

    bool CompressBC6(const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture);
    bool CompressBC6(FRHICommandList& CommandList, const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture);

    bool CompressCubeMapBC6(const FRHITextureRef& SrcCubeMap, FRHITextureRef& OutCubeMap);
    bool CompressCubeMapBC6(FRHICommandList& CommandList, const FRHITextureRef& SrcCubeMap, FRHITextureRef& OutCubeMap);

private:
    bool InitializeBC1ToBC5();
    bool InitializeBC6H();

    bool CompressSinglePass64(FRHICommandList& CommandList, const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture, 
        FRHIComputeShader* Shader, FRHIComputePipelineState* PSO, EFormat OutputFormat);

    bool CompressSinglePass128(FRHICommandList& CommandList, const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture,
        FRHIComputeShader* Shader, FRHIComputePipelineState* PSO, EFormat OutputFormat);

    // BC1-BC5 (single-pass)
    FRHIComputeShaderRef        BC1CompressionShader;
    FRHIComputePipelineStateRef BC1CompressionPSO;
    FRHIComputeShaderRef        BC2CompressionShader;
    FRHIComputePipelineStateRef BC2CompressionPSO;
    FRHIComputeShaderRef        BC3CompressionShader;
    FRHIComputePipelineStateRef BC3CompressionPSO;
    FRHIComputeShaderRef        BC4CompressionShader;
    FRHIComputePipelineStateRef BC4CompressionPSO;
    FRHIComputeShaderRef        BC5CompressionShader;
    FRHIComputePipelineStateRef BC5CompressionPSO;

    // BC6H
    FRHIComputeShaderRef        BC6HCompressionShader;
    FRHIComputePipelineStateRef BC6HCompressionPSO;
    FRHIComputeShaderRef        BC6HCompressionCubeShader;
    FRHIComputePipelineStateRef BC6HCompressionCubePSO;

    FRHISamplerStateRef         PointSampler;
};
