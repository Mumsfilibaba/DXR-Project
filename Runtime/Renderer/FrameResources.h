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

    CRHITexture2D* BackBuffer = nullptr;

    TSharedRef<CRHIConstantBuffer> CameraBuffer;
    TSharedRef<CRHIConstantBuffer> TransformBuffer;

    TSharedRef<CRHISamplerState> PointLightShadowSampler;
    TSharedRef<CRHISamplerState> DirectionalLightShadowSampler;
    TSharedRef<CRHISamplerState> IrradianceSampler;

    TSharedRef<CRHITextureCube> Skybox;

    TSharedRef<CRHITexture2D>    IntegrationLUT;
    TSharedRef<CRHISamplerState> IntegrationLUTSampler;

    TSharedRef<CRHITexture2D> SSAOBuffer;
    TSharedRef<CRHITexture2D> FinalTarget;
    TSharedRef<CRHITexture2D> GBuffer[5];

    // Two resources that can be ping-ponged between
    TSharedRef<CRHITexture2D> ReducedDepthBuffer[2];

    TSharedRef<CRHISamplerState> GBufferSampler;
    TSharedRef<CRHISamplerState> FXAASampler;

    TSharedRef<CRHIVertexInputLayout> StdInputLayout;

    TSharedRef<CRHITexture2D>       RTOutput;
    TSharedRef<CRHIRayTracingScene> RTScene;

    SRayTracingShaderResources GlobalResources;
    SRayTracingShaderResources RayGenLocalResources;
    SRayTracingShaderResources MissLocalResources;
    TArray<SRayTracingGeometryInstance> RTGeometryInstances;

    TArray<SRayTracingShaderResources>     RTHitGroupResources;
    THashTable<class CMesh*, uint32>       RTMeshToHitGroupIndex;
    TResourceCache<CRHIShaderResourceView> RTMaterialTextureCache;

    TArrayView<const SMeshDrawCommand> GlobalMeshDrawCommands;
    TArray<uint32>                     DeferredVisibleCommands;
    TArray<uint32>                     ForwardVisibleCommands;

    TSharedRef<CRHIViewport> MainWindowViewport;
};

