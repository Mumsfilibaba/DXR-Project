#pragma once
#include "Core/Templates/Utility/EnumOperators.h"
#include "RHI/RHITypes.h"
#include "RHI/RHIShader.h"
#include "RHI/RHISamplerState.h"
#include "RHI/RHIPipelineState.h"
#include "RendererCore/TextureCompressor.h"

class FRHITexture;

enum class ETextureFactoryFlags : uint32
{
    None         = 0,
    GenerateMips = FLAG(1),
};

ENUM_CLASS_OPERATORS(ETextureFactoryFlags);

struct FTextureFactoryHelpers
{
    static FORCEINLINE uint32 TextureSizeToMiplevels(uint32 TextureSize)
    {
        return Math::Max<uint32>(static_cast<uint32>(Math::Log2(static_cast<float>(TextureSize))), 1u);
    }
};

struct RENDERERCORE_API FTextureFactory
{
public:
    static bool Initialize();
    static void Release();

    static FORCEINLINE FTextureFactory& Get()
    {
        return *GTextureFactory;
    }

public:

    // Helper function to create a texture from memory
    FRHITexture* LoadFromMemory(const uint8* Pixels, uint32 Width, uint32 Height, ETextureFactoryFlags Flags, EFormat Format);

    // Source is a panorama image and Dest is a cube texture
    // This function assumes that the 'Dest' is in 'EResourceState::Common'
    bool TextureCubeFromPanorma(FRHITexture* Source, FRHITexture* Dest, ETextureFactoryFlags Flags);

    // Generates a chain of miplevels. This function assumes that the 'Texture' is in 'EResourceState::PixelShaderResource'
    bool GenerateMiplevels(FRHITexture* Texture);
    bool GenerateMiplevels(FRHICommandList& CommandList, FRHITexture* Texture);

    // Filters a cube-map for use in specular light-calculations
    bool FilterSpecularCubeMap(FRHITexture* SrcCubeMap, FRHITexture* DstCubeMap, uint32 NumMipLevels = uint32(~0));
    bool FilterSpecularCubeMap(FRHICommandList& CommandList, FRHITexture* SrcCubeMap, FRHITexture* DstCubeMap, uint32 NumMipLevels = uint32(~0));

    // Filters a cube-map for use in diffuse light-calculations
    bool FilterDiffuseCubeMap(FRHITexture* SrcCubeMap, FRHITexture* DstCubeMap);
    bool FilterDiffuseCubeMap(FRHICommandList& CommandList, FRHITexture* SrcCubeMap, FRHITexture* DstCubeMap);

    FORCEINLINE FTextureCompressor& GetTextureCompressor() 
    {
        return TextureCompressor;
    }

private:
    FTextureFactory();
    ~FTextureFactory();

    bool CreateResources();

    FTextureCompressor          TextureCompressor;

    FRHISamplerStateRef         LinearSampler;
    FRHISamplerStateRef         CubeMapFilterSampler;

    FRHIComputePipelineStateRef PanoramaPSO;
    FRHIComputeShaderRef        PanoramCS;

    FRHIComputePipelineStateRef GenerateMipsTex2D_PSO;
    FRHIComputeShaderRef        GenerateMipsTex2D_CS;

    FRHIComputePipelineStateRef GenerateMipsTexCube_PSO;
    FRHIComputeShaderRef        GenerateMipsTexCube_CS;

    FRHIComputePipelineStateRef DiffuseCubeMapFilter_PSO;
    FRHIComputeShaderRef        DiffuseCubeMapFilter_CS;
    
    FRHIComputePipelineStateRef SpecularCubeMapFilter_PSO;
    FRHIComputeShaderRef        SpecularCubeMapFilter_CS;

    static FTextureFactory* GTextureFactory;
};