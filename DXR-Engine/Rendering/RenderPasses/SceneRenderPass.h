#pragma once
#include "RenderLayer/Buffer.h"
#include "RenderLayer/CommandList.h"

#include "Rendering/DebugUI.h"
#include "Rendering/MeshDrawCommand.h"

#include "Scene/Scene.h"

#define GBUFFER_ALBEDO_INDEX      0
#define GBUFFER_NORMAL_INDEX      1
#define GBUFFER_MATERIAL_INDEX    2
#define GBUFFER_DEPTH_INDEX       3
#define GBUFFER_VIEW_NORMAL_INDEX 4

struct LightSettings
{
    UInt16 ShadowMapWidth       = 4096;
    UInt16 ShadowMapHeight      = 4096;
    UInt16 PointLightShadowSize = 1024;
};

struct SharedRenderPassResources
{
    Texture2D*        BackBuffer    = nullptr;
    RenderTargetView* BackBufferRTV = nullptr;

    TSharedRef<ConstantBuffer> CameraBuffer;
    TSharedRef<ConstantBuffer> TransformBuffer;
    TSharedRef<ConstantBuffer> PointLightBuffer;
    TSharedRef<ConstantBuffer> DirectionalLightBuffer;

    TSharedRef<TextureCube>         IrradianceMap;
    TSharedRef<UnorderedAccessView> IrradianceMapUAV;
    TSharedRef<ShaderResourceView>  IrradianceMapSRV;
    TSharedRef<SamplerState>        IrradianceSampler;

    TSharedRef<TextureCube>                 SpecularIrradianceMap;
    TSharedRef<ShaderResourceView>          SpecularIrradianceMapSRV;
    TArray<TSharedRef<UnorderedAccessView>> SpecularIrradianceMapUAVs;
    TArray<UnorderedAccessView*>            WeakSpecularIrradianceMapUAVs;

    TSharedRef<TextureCube>        Skybox;
    TSharedRef<ShaderResourceView> SkyboxSRV;

    const UInt32 MaxPointLights       = 256;
    const UInt32 MaxDirectionalLights = 256;
    const UInt32 MaxPointLightShadows = 8;
    TSharedRef<SamplerState>        ShadowMapSampler;
    TSharedRef<SamplerState>        ShadowMapCompSampler;
    TSharedRef<TextureCubeArray>    PointLightShadowMaps;
    TSharedRef<ShaderResourceView>  PointLightShadowMapSRV;
    TArray<TStaticArray<TSharedRef<DepthStencilView>, 6>> PointLightShadowMapDSVs;

    TSharedRef<ShaderResourceView> DirLightShadowMapSRV;
    TSharedRef<DepthStencilView>   DirLightShadowMapDSV;
    TSharedRef<Texture2D>          DirLightShadowMaps;

    TSharedRef<ShaderResourceView> VSMDirLightShadowMapSRV;
    TSharedRef<RenderTargetView>   VSMDirLightShadowMapRTV;
    TSharedRef<Texture2D>          VSMDirLightShadowMaps;

    TSharedRef<Texture2D>           ReflectionTexture;
    TSharedRef<ShaderResourceView>  ReflectionTextureSRV;
    TSharedRef<UnorderedAccessView> ReflectionTextureUAV;

    TSharedRef<Texture2D>          IntegrationLUT;
    TSharedRef<ShaderResourceView> IntegrationLUTSRV;
    TSharedRef<SamplerState>       IntegrationLUTSampler;

    TSharedRef<Texture2D>           FinalTarget;
    TSharedRef<ShaderResourceView>  FinalTargetSRV;
    TSharedRef<RenderTargetView>    FinalTargetRTV;
    TSharedRef<UnorderedAccessView> FinalTargetUAV;

   const EFormat DepthBufferFormat  = EFormat::Format_D32_Float;
   const EFormat SSAOBufferFormat   = EFormat::Format_R16_Float;
   const EFormat FinalTargetFormat  = EFormat::Format_R16G16B16A16_Float;
   const EFormat RenderTargetFormat = EFormat::Format_R8G8B8A8_Unorm;
   const EFormat AlbedoFormat       = EFormat::Format_R8G8B8A8_Unorm;
   const EFormat MaterialFormat     = EFormat::Format_R8G8B8A8_Unorm;
   const EFormat NormalFormat       = EFormat::Format_R10G10B10A2_Unorm;
   const EFormat ViewNormalFormat   = EFormat::Format_R10G10B10A2_Unorm;
   const EFormat LightProbeFormat   = EFormat::Format_R16G16B16A16_Float;

    TSharedRef<Texture2D>           GBuffer[5];
    TSharedRef<ShaderResourceView>  GBufferSRVs[5];
    TSharedRef<RenderTargetView>    GBufferRTVs[5];
    TSharedRef<DepthStencilView>    GBufferDSV;
    TSharedRef<SamplerState>        GBufferSampler;

    TSharedRef<InputLayoutState> StdInputLayout;

    TArray<MeshDrawCommand> DeferredVisibleCommands;
    TArray<MeshDrawCommand> ForwardVisibleCommands;

    TArray<ImGuiImage> DebugTextures;

    TSharedRef<Viewport> MainWindowViewport;

    LightSettings CurrentLightSettings;
};

class SceneRenderPass
{
public:
    SceneRenderPass()	= default;
    ~SceneRenderPass()	= default;

    virtual Bool Init(SharedRenderPassResources& FrameResources)
    {
    }

    virtual Bool ResizeResources(SharedRenderPassResources& FrameResources)
    {
    }

    virtual void Render(
        CommandList& CmdList, 
        SharedRenderPassResources& FrameResource,
        const Scene& Scene)
    {
    }
};