#pragma once
#include "RendererModule.h"
#include "Core/Containers/Map.h"
#include "Core/Containers/ArrayView.h"
#include "RHI/RHIResources.h"
#include "RHI/RHIRayTracing.h"
#include "Renderer/MeshDrawCommand.h"

enum EGBufferIndex
{
    GBufferIndex_Albedo     = 0,
    GBufferIndex_Normal     = 1,
    GBufferIndex_Material   = 2,
    GBufferIndex_Depth      = 3,
    GBufferIndex_ViewNormal = 4,
    GBufferIndex_Velocity   = 5,

    GBuffer_NumBuffers
};

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
            int32 NewIndex = Resources.Size();
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

    uint32 Size() const
    {
        return Resources.Size();
    }

private:
    TArray<TResource*>      Resources;
    TMap<TResource*, int32> ResourceIndices;
};

struct RENDERER_API FFrameResources
{
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
    const EFormat VelocityFormat     = EFormat::R16G16_Float;

    FRHITexture*           BackBuffer = nullptr;

    // GlobalBuffers
    FRHIBufferRef          CameraBuffer;
    FRHIBufferRef          TransformBuffer;

    // Samplers
    FRHISamplerStateRef    PointLightShadowSampler;
    FRHISamplerStateRef    DirectionalLightShadowSampler;
    FRHISamplerStateRef    IrradianceSampler;
    FRHISamplerStateRef    GBufferSampler;
    FRHISamplerStateRef    FXAASampler;

    FRHITextureRef         Skybox;

    FRHITextureRef         IntegrationLUT;
    FRHISamplerStateRef    IntegrationLUTSampler;

    // GBuffer
    FRHITextureRef         SSAOBuffer;
    FRHITextureRef         FinalTarget;
    FRHITextureRef         GBuffer[GBuffer_NumBuffers];

    // Two resources that can be ping-ponged between
    FRHITextureRef         ReducedDepthBuffer[2];

    FRHIVertexInputLayoutRef MeshInputLayout;

    // RayTracing
    FRHITextureRef         RTOutput;
    FRHIRayTracingSceneRef RTScene;

    FRayTracingShaderResources             GlobalResources;
    FRayTracingShaderResources             RayGenLocalResources;
    FRayTracingShaderResources             MissLocalResources;
    TArray<FRHIRayTracingGeometryInstance> RTGeometryInstances;

    TArray<FRayTracingShaderResources>     RTHitGroupResources;
    TMap<class FMesh*, uint32>             RTMeshToHitGroupIndex;
    TResourceCache<FRHIShaderResourceView> RTMaterialTextureCache;

    TArrayView<const FMeshDrawCommand>     GlobalMeshDrawCommands;
    TArray<uint32>                         DeferredVisibleCommands;
    TArray<uint32>                         ForwardVisibleCommands;

    FRHIViewportRef                        MainViewport;
    EFormat                                BackBufferFormat;
};

