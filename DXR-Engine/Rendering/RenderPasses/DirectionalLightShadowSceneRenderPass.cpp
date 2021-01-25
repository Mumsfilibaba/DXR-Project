#include "DirectionalLightShadowSceneRenderPass.h"

#include "RenderLayer/RenderLayer.h"

static const EFormat ShadowMapFormat = EFormat::Format_D32_Float;

struct DirectionalLightPerShadowMap
{
    XMFLOAT4X4 Matrix;
    XMFLOAT3   Position;
    Float      FarPlane;
};

Bool DirectionalLightShadowSceneRenderPass::Init(SharedRenderPassResources& FrameResources)
{
    FrameResources.DirectionalLightBuffer = RenderLayer::CreateConstantBuffer<DirectionalLightProperties>(
        nullptr,
        FrameResources.MaxDirectionalLights,
        BufferUsage_Default,
        EResourceState::ResourceState_Common);
    if (!FrameResources.DirectionalLightBuffer)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        FrameResources.DirectionalLightBuffer->SetName("DirectionalLight Buffer");
    }

    PerShadowMapBuffer = RenderLayer::CreateConstantBuffer<DirectionalLightPerShadowMap>(
        nullptr, 1,
        BufferUsage_Default,
        EResourceState::ResourceState_Common);
    if (!PerShadowMapBuffer)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PerShadowMapBuffer->SetName("PerShadowMap Buffer");
    }

    FrameResources.DirLightShadowMaps = RenderLayer::CreateTexture2D(
        nullptr,
        ShadowMapFormat,
        TextureUsage_ShadowMap,
        4096, 4096,
        1, 1,
        ClearValue(DepthStencilClearValue(1.0f, 0)));
    if (FrameResources.DirLightShadowMaps)
    {
        FrameResources.DirLightShadowMaps->SetName("Directional Light ShadowMaps");

        FrameResources.DirLightShadowMapDSV = RenderLayer::CreateDepthStencilView(
            FrameResources.DirLightShadowMaps.Get(),
            ShadowMapFormat,
            0);
        if (!FrameResources.DirLightShadowMapDSV)
        {
            Debug::DebugBreak();
        }

#if !ENABLE_VSM
        FrameResources.DirLightShadowMapSRV = RenderLayer::CreateShaderResourceView(
            FrameResources.DirLightShadowMaps.Get(),
            EFormat::Format_R32_Float,
            0, 1);
        if (!FrameResources.DirLightShadowMapSRV)
        {
            Debug::DebugBreak();
            return false;
        }
#endif
    }
    else
    {
        return false;
    }

#if ENABLE_VSM
    VSMDirLightShadowMaps = RenderLayer::CreateTexture2D(
        nullptr,
        EFormat::Format_R32G32_Float,
        TextureUsage_RenderTarget,
        Renderer::GetGlobalLightSettings().ShadowMapWidth,
        Renderer::GetGlobalLightSettings().ShadowMapHeight,
        1, 1,
        ClearValue(ColorClearValue(1.0f, 1.0f, 1.0f, 1.0f)));
    if (VSMDirLightShadowMaps)
    {
        VSMDirLightShadowMaps->SetName("Directional Light VSM");

        VSMDirLightShadowMapRTV = RenderLayer::CreateRenderTargetView(
            VSMDirLightShadowMaps.Get(),
            EFormat::Format_R32G32_Float,
            0);
        if (!VSMDirLightShadowMapRTV)
        {
            Debug::DebugBreak();
        }

        VSMDirLightShadowMapSRV = RenderLayer::CreateShaderResourceView(
            VSMDirLightShadowMaps.Get(),
            EFormat::Format_R32G32_Float,
            0, 1);
        if (!VSMDirLightShadowMapSRV)
        {
            Debug::DebugBreak();
        }
    }
    else
    {
        return false;
    }
#endif

    return true;
}

void DirectionalLightShadowSceneRenderPass::Render(CommandList& CmdList, SharedRenderPassResources& FrameResources)
{
}
