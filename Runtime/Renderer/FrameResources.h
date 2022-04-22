#pragma once
#include "RendererModule.h"

#include "RHI/RHIResources.h"

#include "Renderer/MeshDrawCommand.h"

#include "InterfaceRenderer/InterfaceRenderer.h"

#include "Core/Containers/HashTable.h"

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
    SFrameResources() = default;
    ~SFrameResources() = default;

    void Release();

    const ERHIFormat DepthBufferFormat  = ERHIFormat::D32_Float;
    const ERHIFormat SSAOBufferFormat   = ERHIFormat::R8_Unorm;
    const ERHIFormat FinalTargetFormat  = ERHIFormat::R16G16B16A16_Float;
    const ERHIFormat RTOutputFormat     = ERHIFormat::R16G16B16A16_Float;
    const ERHIFormat BackBufferFormat   = ERHIFormat::B8G8R8A8_Unorm;
    const ERHIFormat RenderTargetFormat = ERHIFormat::R8G8B8A8_Unorm;
    const ERHIFormat AlbedoFormat       = ERHIFormat::R8G8B8A8_Unorm;
    const ERHIFormat MaterialFormat     = ERHIFormat::R8G8B8A8_Unorm;
    const ERHIFormat NormalFormat       = ERHIFormat::R10G10B10A2_Unorm;
    const ERHIFormat ViewNormalFormat   = ERHIFormat::R10G10B10A2_Unorm;

    CRHITexture2D* BackBuffer = nullptr;

    TSharedRef<CRHIBuffer> CameraBuffer;
    TSharedRef<CRHIBuffer> TransformBuffer;

    CRHISamplerStateRef PointLightShadowSampler;
    CRHISamplerStateRef DirectionalLightShadowSampler;
    CRHISamplerStateRef IrradianceSampler;

    CRHITextureCubeRef Skybox;

    CRHITexture2DRef    IntegrationLUT;
    CRHISamplerStateRef IntegrationLUTSampler;

    CRHITexture2DRef SSAOBuffer;
    CRHITexture2DRef FinalTarget;
    CRHITexture2DRef GBuffer[5];

    // Two resources that can be ping-ponged between
    CRHITexture2DRef ReducedDepthBuffer[2];

    CRHISamplerStateRef GBufferSampler;
    CRHISamplerStateRef FXAASampler;

    CRHIVertexInputLayoutRef StdInputLayout;

    CRHITexture2DRef       RTOutput;
    TSharedRef<CRHIRayTracingScene> RTScene;

    SRayTracingShaderResources GlobalResources;
    SRayTracingShaderResources RayGenLocalResources;
    SRayTracingShaderResources MissLocalResources;
    TArray<SRHIRayTracingGeometryInstance> RTGeometryInstances;

    TArray<SRayTracingShaderResources>     RTHitGroupResources;
    THashTable<class CMesh*, uint32>       RTMeshToHitGroupIndex;
    TResourceCache<CRHIShaderResourceView> RTMaterialTextureCache;

    TArray<SMeshDrawCommand> DeferredVisibleCommands;
    TArray<SMeshDrawCommand> ForwardVisibleCommands;

    TSharedRef<CRHIViewport> MainWindowViewport;
};

