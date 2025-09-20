#include "RHI/RHITexture.h"
#include "Engine/World/Lights/SkyLight.h"
#include "RendererCore/TextureFactory.h"
#include "Renderer/Scene/SceneSkyLight.h"
#include "Renderer/FrameResources.h"

FSceneSkyLight::FSceneSkyLight(FScene* InScene, FSkyLight* InSkyLight)
    : FSceneObject(InScene)
    , SkyLight(InSkyLight)
    , SpecularCubeMap(nullptr)
    , DiffuseCubeMap(nullptr)
{
    if (SkyLight)
    {
        SourceCubeMap = SkyLight->GetCubeMap();
    }
}

FSceneSkyLight::~FSceneSkyLight()
{
    SkyLight = nullptr;
}

void FSceneSkyLight::FilterStaticCubeMaps()
{
    if (!SourceCubeMap)
    {
        LOG_WARNING("[FSceneSkyLight::FilterStaticCubeMaps] Trying to filter cube-map without a source");
        return;
    }

    // Format for filtering before compressing the static cube-map
    constexpr EFormat TempCubeMapFormat = EFormat::R16G16B16A16_Float;

    // Create specular cube-map
    constexpr uint32 SpecularCubeMapSize = 256;
    const uint32 SpecularIrradianceMiplevels = Math::Max<uint32>(static_cast<uint32>(Math::Log2(static_cast<float>(SpecularCubeMapSize))), 1);

    const ETextureUsageFlags TextureFlags = ETextureUsageFlags::UnorderedAccess | ETextureUsageFlags::ShaderResource;
    FRHITextureInfo SpecularCubeMapInfo = FRHITextureInfo::CreateTextureCube(TempCubeMapFormat, SpecularCubeMapSize, SpecularIrradianceMiplevels, 1,TextureFlags);

    FRHITextureRef TempSpecularCubeMap = FRHI::Get()->CreateTexture(SpecularCubeMapInfo, EResourceAccess::PixelShaderResource);
    if (!TempSpecularCubeMap)
    {
        DEBUG_BREAK();
        return;
    }
    else
    {
        TempSpecularCubeMap->SetDebugName("Temp Specular CubeMap");
    }

    // Create diffuse cube-map
    const uint32 DiffuseCubeMapSize = 32;
    FRHITextureInfo DiffuseCubeMapInfo = FRHITextureInfo::CreateTextureCube(TempCubeMapFormat, DiffuseCubeMapSize, 1, 1, TextureFlags);

    FRHITextureRef TempDiffuseCubeMap = FRHI::Get()->CreateTexture(DiffuseCubeMapInfo, EResourceAccess::PixelShaderResource);
    if (!TempDiffuseCubeMap)
    {
        DEBUG_BREAK();
        return;
    }
    else
    {
        TempDiffuseCubeMap->SetDebugName("Temp Diffuse CubeMap");
    }

    FRHICommandList CommandList;

    // Filter specular cube-map

    // When calculating NumMips we skip 3 miplevels since those are too small for the 
    // compressed texture since they are smaller than the compressed block-size.
    constexpr uint32 NumMipsSkipped = 3;

    // Calculate the amount of compressed miplevels
    const int32 NumSpecularMipLevels =  Math::Max<int32>(static_cast<int32>(SpecularIrradianceMiplevels) - NumMipsSkipped, 1);

    bool bResult = FTextureFactory::Get().FilterSpecularCubeMap(CommandList, SourceCubeMap.Get(), TempSpecularCubeMap.Get(), NumSpecularMipLevels);
    if (!bResult)
    {
        DEBUG_BREAK();
        return;
    }

    // Filter diffuse cube-map
    bResult = FTextureFactory::Get().FilterDiffuseCubeMap(CommandList, SourceCubeMap.Get(), TempDiffuseCubeMap.Get());
    if (!bResult)
    {
        DEBUG_BREAK();
        return;
    }

    // Compress the cube-maps
    FTextureCompressor& TextureCompressor = FTextureFactory::Get().GetTextureCompressor();
    TextureCompressor.CompressCubeMapBC6(CommandList, TempSpecularCubeMap, SpecularCubeMap);

    if (SpecularCubeMap)
    {
        SpecularCubeMap->SetDebugName("Specular CubeMap");
    }

    TextureCompressor.CompressCubeMapBC6(CommandList, TempDiffuseCubeMap, DiffuseCubeMap);
    if (DiffuseCubeMap)
    {
        DiffuseCubeMap->SetDebugName("Diffuse CubeMap");
    }

    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
}
