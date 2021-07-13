#pragma once
#include "RenderLayer/Resources.h"
#include "RenderLayer/Viewport.h"

#include "Rendering/MeshDrawCommand.h"
#include "Rendering/DebugUI.h"

#include <unordered_map>

#define GBUFFER_ALBEDO_INDEX      0
#define GBUFFER_NORMAL_INDEX      1
#define GBUFFER_MATERIAL_INDEX    2
#define GBUFFER_DEPTH_INDEX       3
#define GBUFFER_VIEW_NORMAL_INDEX 4

template<typename TResource>
class ResourceCache
{
public:
    int32 Add( TResource* Resource )
    {
        if ( Resource == nullptr )
        {
            return -1;
        }

        auto TextureIndexPair = ResourceIndices.find( Resource );
        if ( TextureIndexPair == ResourceIndices.end() )
        {
            int32 NewIndex = Resources.Size();
            ResourceIndices[Resource] = NewIndex;
            Resources.EmplaceBack( Resource );

            return NewIndex;
        }
        else
        {
            return TextureIndexPair->second;
        }
    }

    TResource* Get( uint32 Index ) const
    {
        return Resources[Index];
    }

    uint32 Size() const
    {
        return Resources.Size();
    }

private:
    TArray<TResource*>                    Resources;
    std::unordered_map<TResource*, int32> ResourceIndices;
};

struct FrameResources
{
    FrameResources() = default;
    ~FrameResources() = default;

    void Release();

    const EFormat DepthBufferFormat = EFormat::D32_Float;
    const EFormat SSAOBufferFormat = EFormat::R16_Unorm;
    const EFormat FinalTargetFormat = EFormat::R16G16B16A16_Float;
    const EFormat RTOutputFormat = EFormat::R16G16B16A16_Float;
    const EFormat RenderTargetFormat = EFormat::R8G8B8A8_Unorm;
    const EFormat AlbedoFormat = EFormat::R8G8B8A8_Unorm;
    const EFormat MaterialFormat = EFormat::R8G8B8A8_Unorm;
    const EFormat NormalFormat = EFormat::R10G10B10A2_Unorm;
    const EFormat ViewNormalFormat = EFormat::R10G10B10A2_Unorm;

    Texture2D* BackBuffer = nullptr;

    TSharedRef<ConstantBuffer> CameraBuffer;
    TSharedRef<ConstantBuffer> TransformBuffer;

    TSharedRef<SamplerState> PointLightShadowSampler;
    TSharedRef<SamplerState> DirectionalLightShadowSampler;
    TSharedRef<SamplerState> IrradianceSampler;

    TSharedRef<TextureCube> Skybox;

    TSharedRef<Texture2D>    IntegrationLUT;
    TSharedRef<SamplerState> IntegrationLUTSampler;

    TSharedRef<Texture2D> SSAOBuffer;
    TSharedRef<Texture2D> FinalTarget;
    TSharedRef<Texture2D> GBuffer[5];

    // Two resources that can be ping ponged inbetween
    TSharedRef<Texture2D> ReducedDepthBuffer[2];

    TSharedRef<SamplerState> GBufferSampler;
    TSharedRef<SamplerState> FXAASampler;

    TSharedRef<InputLayoutState> StdInputLayout;

    TSharedRef<Texture2D>       RTOutput;
    TSharedRef<RayTracingScene> RTScene;

    RayTracingShaderResources GlobalResources;
    RayTracingShaderResources RayGenLocalResources;
    RayTracingShaderResources MissLocalResources;
    TArray<RayTracingGeometryInstance> RTGeometryInstances;

    TArray<RayTracingShaderResources>       RTHitGroupResources;
    std::unordered_map<class Mesh*, uint32> RTMeshToHitGroupIndex;
    ResourceCache<ShaderResourceView>       RTMaterialTextureCache;

    TArray<MeshDrawCommand> DeferredVisibleCommands;
    TArray<MeshDrawCommand> ForwardVisibleCommands;

    TArray<ImGuiImage> DebugTextures;

    TSharedRef<Viewport> MainWindowViewport;
};

