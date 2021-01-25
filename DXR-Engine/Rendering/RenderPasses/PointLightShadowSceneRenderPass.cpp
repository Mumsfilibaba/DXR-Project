#include "PointLightShadowSceneRenderPass.h"

#include "RenderLayer/RenderLayer.h"

static const EFormat ShadowMapFormat = EFormat::Format_D32_Float;
static const UInt16  ShadowMapSize   = 1024;

struct PointLightPerShadowMap
{
    XMFLOAT4X4 Matrix;
    XMFLOAT3   Position;
    Float      FarPlane;
};

Bool PointLightShadowSceneRenderPass::Init(SharedRenderPassResources& FrameResources)
{
    FrameResources.PointLightBuffer = RenderLayer::CreateConstantBuffer<PointLightProperties>(
        nullptr,
        FrameResources.MaxPointLights,
        BufferUsage_Default,
        EResourceState::ResourceState_Common);
    if (!FrameResources.PointLightBuffer)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        FrameResources.PointLightBuffer->SetName("PointLight Buffer");
    }

    PerShadowMapBuffer = RenderLayer::CreateConstantBuffer<PointLightPerShadowMap>(
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

    FrameResources.PointLightShadowMaps = RenderLayer::CreateTextureCubeArray(
        nullptr,
        ShadowMapFormat,
        TextureUsage_ShadowMap,
        ShadowMapSize,
        1, FrameResources.MaxPointLightShadows, 1,
        ClearValue(DepthStencilClearValue(1.0f, 0)));
    if (FrameResources.PointLightShadowMaps)
    {
        FrameResources.PointLightShadowMaps->SetName("PointLight ShadowMaps");

        FrameResources.PointLightShadowMapDSVs.Resize(FrameResources.MaxPointLightShadows);
        for (UInt32 i = 0; i < FrameResources.MaxPointLightShadows; i++)
        {
            for (UInt32 Face = 0; Face < 6; Face++)
            {
                TStaticArray<TSharedRef<DepthStencilView>, 6>& DepthCube = FrameResources.PointLightShadowMapDSVs[i];
                DepthCube[Face] = RenderLayer::CreateDepthStencilView(
                    FrameResources.PointLightShadowMaps.Get(),
                    ShadowMapFormat,
                    0, i, Face);
                if (!DepthCube[Face])
                {
                    Debug::DebugBreak();
                    return false;
                }
            }
        }

        FrameResources.PointLightShadowMapSRV = RenderLayer::CreateShaderResourceView(
            FrameResources.PointLightShadowMaps.Get(),
            EFormat::Format_R32_Float,
            0, 1, 0, FrameResources.MaxPointLightShadows);
        if (!FrameResources.PointLightShadowMapSRV)
        {
            Debug::DebugBreak();
            return false;
        }
    }
    else
    {
        return false;
    }

	return true;
}

void PointLightShadowSceneRenderPass::Render(CommandList& CmdList, SharedRenderPassResources& FrameResources)
{
}
