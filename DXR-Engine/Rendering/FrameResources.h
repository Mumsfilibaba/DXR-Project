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
#define GBUFFER_GEOM_NORMAL_INDEX 4
#define GBUFFER_VELOCITY_INDEX    5

template<typename TResource>
class PtrResourceCache
{
public:
    Int32 Add(TResource* Resource)
    {
        if (Resource == nullptr)
        {
            return -1;
        }

        auto TextureIndexPair = ResourceIndices.find(Resource);
        if (TextureIndexPair == ResourceIndices.end())
        {
            Int32 NewIndex = Resources.Size();
            ResourceIndices[Resource] = NewIndex;
            Resources.EmplaceBack(Resource);

            return NewIndex;
        }
        else
        {
            return TextureIndexPair->second;
        }
    }

    TResource* Get(UInt32 Index) const
    {
        return Resources[Index];
    }

    UInt32 Size() const
    {
        return Resources.Size();
    }

private:
    TArray<TResource*>                    Resources;
    std::unordered_map<TResource*, Int32> ResourceIndices;
};

struct FrameResources
{
    FrameResources()  = default;
    ~FrameResources() = default;

    void Release();

    const EFormat DepthBufferFormat  = EFormat::D32_Float;
    const EFormat SSAOBufferFormat   = EFormat::R16_Float;
    const EFormat FinalTargetFormat  = EFormat::R16G16B16A16_Float;
    const EFormat RTOutputFormat     = EFormat::R16G16B16A16_Float;
    const EFormat RenderTargetFormat = EFormat::R8G8B8A8_Unorm;
    const EFormat AlbedoFormat       = EFormat::R8G8B8A8_Unorm;
    const EFormat MaterialFormat     = EFormat::R8G8B8A8_Unorm;
    const EFormat NormalFormat       = EFormat::R10G10B10A2_Unorm;
    const EFormat GeomNormalFormat   = EFormat::R10G10B10A2_Unorm;
    const EFormat VelocityFormat     = EFormat::R16G16B16A16_Float;

    Texture2D* BackBuffer = nullptr;

    TRef<ConstantBuffer> CameraBuffer;
    TRef<ConstantBuffer> TransformBuffer;

    TRef<SamplerState> DirectionalShadowSampler;
    TRef<SamplerState> PointShadowSampler;
    TRef<SamplerState> IrradianceSampler;

    TRef<TextureCube> Skybox;

    TRef<Texture2D>    IntegrationLUT;
    TRef<SamplerState> IntegrationLUTSampler;

    TRef<Texture2D> SSAOBuffer;
    TRef<Texture2D> FinalTarget;
    TRef<Texture2D> GBuffer[6];
    TRef<Texture2D> PrevDepth;
    TRef<Texture2D> PrevGeomNormals;

    TRef<SamplerState>   GBufferSampler;
    TRef<SamplerState>   FXAASampler;
    TRef<Texture2DArray> BlueNoise;

    TRef<InputLayoutState> StdInputLayout;

    TRef<Texture2D> RTReflections;
    TRef<Texture2D> RTRayPDF;

    TRef<RayTracingScene> RTScene;

    RayTracingShaderResources   GlobalResources;
    RayTracingShaderResources   RayGenLocalResources;
    RayTracingShaderResources   MissLocalResources;
    TArray<RayTracingGeometryInstance> RTGeometryInstances;

    TArray<RayTracingShaderResources>       RTHitGroupResources;
    std::unordered_map<class Mesh*, UInt32> RTMeshToHitGroupIndex;
    PtrResourceCache<ShaderResourceView>    RTMaterialTextureCache;

    TArray<MeshDrawCommand> DeferredVisibleCommands;
    TArray<MeshDrawCommand> ForwardVisibleCommands;

    TArray<ImGuiImage> DebugTextures;

    TRef<Viewport> MainWindowViewport;
};

