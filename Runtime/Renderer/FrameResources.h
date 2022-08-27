#pragma once
#include "RendererModule.h"

#include "RHI/RHIResources.h"
#include "RHI/RHIViewport.h"

#include "Renderer/MeshDrawCommand.h"

#include "InterfaceRenderer/InterfaceRenderer.h"

#include "Core/Containers/Map.h"
#include "Core/Containers/ArrayView.h"

#define GBUFFER_ALBEDO_INDEX      (0)
#define GBUFFER_NORMAL_INDEX      (1)
#define GBUFFER_MATERIAL_INDEX    (2)
#define GBUFFER_DEPTH_INDEX       (3)
#define GBUFFER_VIEW_NORMAL_INDEX (4)

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TResourceCache

template<typename TResource>
class TResourceCache
{
public:
    int32 Add(TResource* Resource)
    {
        if (Resource == nullptr)
        {
            return -1;
        }

        auto TextureIndexPair = ResourceIndices.find(Resource);
        if (TextureIndexPair == ResourceIndices.end())
        {
            int32 NewIndex = Resources.GetSize();
            ResourceIndices[Resource] = NewIndex;
            Resources.Emplace(Resource);

            return NewIndex;
        }
        else
        {
            return TextureIndexPair->second;
        }
    }

    TResource* Get(uint32 Index) const
    {
        return Resources[Index];
    }

    uint32 GetSize() const
    {
        return Resources.GetSize();
    }

private:
    TArray<TResource*>      Resources;
    TMap<TResource*, int32> ResourceIndices;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FFrameResources

struct RENDERER_API FFrameResources
{
    FFrameResources()  = default;
    ~FFrameResources() = default;

    void Release();

    const EFormat DepthBufferFormat  = EFormat::D32_Float;
    const EFormat SSAOBufferFormat   = EFormat::R8_Unorm;
    const EFormat FinalTargetFormat  = EFormat::R16G16B16A16_Float;
    const EFormat RTOutputFormat     = EFormat::R16G16B16A16_Float;
    const EFormat RenderTargetFormat = EFormat::R8G8B8A8_Unorm;
    const EFormat AlbedoFormat       = EFormat::R8G8B8A8_Unorm;
    const EFormat MaterialFormat     = EFormat::R8G8B8A8_Unorm;
    const EFormat NormalFormat       = EFormat::R10G10B10A2_Unorm;
    const EFormat ViewNormalFormat   = EFormat::R10G10B10A2_Unorm;

    FRHITexture2D* BackBuffer = nullptr;

    // GlobalBuffers
    FRHIConstantBufferRef    CameraBuffer;
    FRHIConstantBufferRef    TransformBuffer;

    // Samplers
    FRHISamplerStateRef      PointLightShadowSampler;
    FRHISamplerStateRef      DirectionalLightShadowSampler;
    FRHISamplerStateRef      IrradianceSampler;
    FRHISamplerStateRef      GBufferSampler;
    FRHISamplerStateRef      FXAASampler;

    FRHITextureCubeRef       Skybox;

    FRHITexture2DRef         IntegrationLUT;
    FRHISamplerStateRef      IntegrationLUTSampler;

    // GBuffer
    FRHITexture2DRef         SSAOBuffer;
    FRHITexture2DRef         FinalTarget;
    FRHITexture2DRef         GBuffer[5];

    // Two resources that can be ping-ponged between
    FRHITexture2DRef         ReducedDepthBuffer[2];

    FRHIVertexInputLayoutRef MeshInputLayout;

    // RayTracing
    FRHITexture2DRef         RTOutput;
    FRHIRayTracingSceneRef   RTScene;

    FRayTracingShaderResources GlobalResources;
    FRayTracingShaderResources RayGenLocalResources;
    FRayTracingShaderResources MissLocalResources;
    TArray<FRHIRayTracingGeometryInstance> RTGeometryInstances;

    TArray<FRayTracingShaderResources>     RTHitGroupResources;
    TMap<class FMesh*, uint32>             RTMeshToHitGroupIndex;
    TResourceCache<FRHIShaderResourceView> RTMaterialTextureCache;

    TArrayView<const FMeshDrawCommand> GlobalMeshDrawCommands;
    TArray<uint32>                     DeferredVisibleCommands;
    TArray<uint32>                     ForwardVisibleCommands;

    FRHIViewportRef                    MainWindowViewport;
};

