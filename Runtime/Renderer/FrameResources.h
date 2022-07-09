#pragma once
#include "RendererModule.h"

#include "RHI/RHIResources.h"
#include "RHI/RHIViewport.h"

#include "Renderer/MeshDrawCommand.h"

#include "InterfaceRenderer/InterfaceRenderer.h"

#include "Core/Containers/HashTable.h"
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
    TArray<TResource*>            Resources;
    THashTable<TResource*, int32> ResourceIndices;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SFrameResources

struct RENDERER_API SFrameResources
{
    SFrameResources()  = default;
    ~SFrameResources() = default;

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

    TSharedRef<FRHIConstantBuffer> CameraBuffer;
    TSharedRef<FRHIConstantBuffer> TransformBuffer;

    TSharedRef<FRHISamplerState> PointLightShadowSampler;
    TSharedRef<FRHISamplerState> DirectionalLightShadowSampler;
    TSharedRef<FRHISamplerState> IrradianceSampler;

    FRHITextureCubeRef Skybox;

    FRHITexture2DRef    IntegrationLUT;
    TSharedRef<FRHISamplerState> IntegrationLUTSampler;

    FRHITexture2DRef SSAOBuffer;
    FRHITexture2DRef FinalTarget;
    FRHITexture2DRef GBuffer[5];

    // Two resources that can be ping-ponged between
    FRHITexture2DRef ReducedDepthBuffer[2];

    TSharedRef<FRHISamplerState> GBufferSampler;
    TSharedRef<FRHISamplerState> FXAASampler;

    TSharedRef<FRHIVertexInputLayout> StdInputLayout;

    FRHITexture2DRef       RTOutput;
    TSharedRef<FRHIRayTracingScene> RTScene;

    FRayTracingShaderResources GlobalResources;
    FRayTracingShaderResources RayGenLocalResources;
    FRayTracingShaderResources MissLocalResources;
    TArray<FRHIRayTracingGeometryInstance> RTGeometryInstances;

    TArray<FRayTracingShaderResources>     RTHitGroupResources;
    THashTable<class FMesh*, uint32>       RTMeshToHitGroupIndex;
    TResourceCache<FRHIShaderResourceView> RTMaterialTextureCache;

    TArrayView<const SMeshDrawCommand> GlobalMeshDrawCommands;
    TArray<uint32>                     DeferredVisibleCommands;
    TArray<uint32>                     ForwardVisibleCommands;

    TSharedRef<FRHIViewport> MainWindowViewport;
};

