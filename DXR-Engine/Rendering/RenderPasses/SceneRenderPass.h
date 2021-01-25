#pragma once
#include "RenderLayer/CommandList.h"
#include "RenderLayer/Buffer.h"

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

    TSharedRef<Texture2D>           GBuffer[5];
    TSharedRef<ShaderResourceView>  GBufferSRVs[5];
    TSharedRef<RenderTargetView>    GBufferRTVs[5];
    TSharedRef<DepthStencilView>    GBufferDSV;
    TSharedRef<SamplerState>        GBufferSampler;

    TSharedRef<InputLayoutState> StdInputLayout;

    TArray<MeshDrawCommand> DeferredVisibleCommands;
    TArray<MeshDrawCommand> ForwardVisibleCommands;

    TArray<ImGuiImage> DebugTextures;

    LightSettings CurrentLightSettings;
};

class SceneRenderPass
{
public:
    SceneRenderPass()	= default;
    ~SceneRenderPass()	= default;

    virtual void Render(CommandList& CmdList, SharedRenderPassResources& FrameResource)
    {
        UNREFERENCED_VARIABLE(CmdList);
        UNREFERENCED_VARIABLE(FrameResource);
    }
};