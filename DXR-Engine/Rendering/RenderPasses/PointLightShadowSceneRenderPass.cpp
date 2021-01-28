#include "PointLightShadowSceneRenderPass.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/ShaderCompiler.h"

#include "Debug/Profiler.h"

#include "Rendering/Mesh.h"

#include "Scene/Frustum.h"
#include "Scene/Lights/PointLight.h"

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
    if (!CreateShadowMaps(FrameResources))
    {
        return false;
    }

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

    // Linear Shadow Maps
    TArray<UInt8> ShaderCode;
    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/ShadowMap.hlsl",
        "VSMain",
        nullptr,
        EShaderStage::ShaderStage_Vertex,
        EShaderModel::ShaderModel_6_0,
        ShaderCode))
    {
        Debug::DebugBreak();
        return false;
    }

    TSharedRef<VertexShader> VShader = RenderLayer::CreateVertexShader(ShaderCode);
    if (!VShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        VShader->SetName("Linear ShadowMap VertexShader");
    }

#if !ENABLE_VSM
    TSharedRef<PixelShader> PShader;
#endif
    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/ShadowMap.hlsl",
        "PSMain",
        nullptr,
        EShaderStage::ShaderStage_Pixel,
        EShaderModel::ShaderModel_6_0,
        ShaderCode))
    {
        Debug::DebugBreak();
        return false;
    }

    PShader = RenderLayer::CreatePixelShader(ShaderCode);
    if (!PShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PShader->SetName("Linear ShadowMap PixelShader");
    }

    DepthStencilStateCreateInfo DepthStencilStateInfo;
    DepthStencilStateInfo.DepthFunc      = EComparisonFunc::ComparisonFunc_LessEqual;
    DepthStencilStateInfo.DepthEnable    = true;
    DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::DepthWriteMask_All;

    TSharedRef<DepthStencilState> DepthStencilState = RenderLayer::CreateDepthStencilState(DepthStencilStateInfo);
    if (!DepthStencilState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        DepthStencilState->SetName("Shadow DepthStencilState");
    }

    RasterizerStateCreateInfo RasterizerStateInfo;
    RasterizerStateInfo.CullMode = ECullMode::CullMode_Back;

    TSharedRef<RasterizerState> RasterizerState = RenderLayer::CreateRasterizerState(RasterizerStateInfo);
    if (!RasterizerState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        RasterizerState->SetName("Shadow RasterizerState");
    }

    BlendStateCreateInfo BlendStateInfo;

    TSharedRef<BlendState> BlendState = RenderLayer::CreateBlendState(BlendStateInfo);
    if (!BlendState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        BlendState->SetName("Shadow BlendState");
    }

    GraphicsPipelineStateCreateInfo PipelineStateInfo;
    PipelineStateInfo.BlendState            = BlendState.Get();
    PipelineStateInfo.DepthStencilState     = DepthStencilState.Get();
    PipelineStateInfo.IBStripCutValue       = EIndexBufferStripCutValue::IndexBufferStripCutValue_Disabled;
    PipelineStateInfo.InputLayoutState      = FrameResources.StdInputLayout.Get();
    PipelineStateInfo.PrimitiveTopologyType = EPrimitiveTopologyType::PrimitiveTopologyType_Triangle;
    PipelineStateInfo.RasterizerState       = RasterizerState.Get();
    PipelineStateInfo.SampleCount           = 1;
    PipelineStateInfo.SampleQuality         = 0;
    PipelineStateInfo.SampleMask            = 0xffffffff;
    PipelineStateInfo.ShaderState.VertexShader = VShader.Get();
    PipelineStateInfo.ShaderState.PixelShader  = PShader.Get();
    PipelineStateInfo.PipelineFormats.NumRenderTargets   = 0;
    PipelineStateInfo.PipelineFormats.DepthStencilFormat = ShadowMapFormat;

    PipelineState = RenderLayer::CreateGraphicsPipelineState(PipelineStateInfo);
    if (!PipelineState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PipelineState->SetName("Linear ShadowMap PipelineState");
    }

    return true;
}

Bool PointLightShadowSceneRenderPass::ResizeResources(SharedRenderPassResources& FrameResources)
{
    return CreateShadowMaps(FrameResources);
}

void PointLightShadowSceneRenderPass::Render(
    CommandList& CmdList, 
    SharedRenderPassResources& FrameResources,
    const Scene& Scene)
{
    PointLightFrame++;
    if (PointLightFrame > 6)
    {
        UpdatePointLight = true;
        PointLightFrame  = 0;
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin Update PointLightBuffer");

    if (UpdatePointLight)
    {
        TRACE_SCOPE("Update LightBuffers");

        CmdList.TransitionBuffer(
            FrameResources.PointLightBuffer.Get(),
            EResourceState::ResourceState_VertexAndConstantBuffer,
            EResourceState::ResourceState_CopyDest);

        UInt32 NumPointLights = 0;
        UInt32 NumDirLights   = 0;
        for (Light* Light : Scene.GetLights())
        {
            Float    Intensity = Light->GetIntensity();
            XMFLOAT3 Color     = Light->GetColor();
            if (IsSubClassOf<PointLight>(Light))
            {
                PointLight* CurrentLight = Cast<PointLight>(Light);
                VALIDATE(CurrentLight != nullptr);

                PointLightProperties Properties;
                Properties.Color         = XMFLOAT3(Color.x * Intensity, Color.y * Intensity, Color.z * Intensity);
                Properties.Position      = CurrentLight->GetPosition();
                Properties.ShadowBias    = CurrentLight->GetShadowBias();
                Properties.MaxShadowBias = CurrentLight->GetMaxShadowBias();
                Properties.FarPlane      = CurrentLight->GetShadowFarPlane();

                constexpr Float MinLuma = 0.05f;
                const Float Dot =
                    Properties.Color.x * 0.2126f +
                    Properties.Color.y * 0.7152f +
                    Properties.Color.z * 0.0722f;

                Float Radius = sqrt(Dot / MinLuma);
                Properties.Radius = Radius;

                constexpr UInt32 SizeInBytes = sizeof(PointLightProperties);
                CmdList.UpdateBuffer(
                    FrameResources.PointLightBuffer.Get(),
                    NumPointLights * SizeInBytes,
                    SizeInBytes,
                    &Properties);

                NumPointLights++;
            }
        }

        CmdList.TransitionBuffer(
            FrameResources.PointLightBuffer.Get(),
            EResourceState::ResourceState_CopyDest,
            EResourceState::ResourceState_VertexAndConstantBuffer);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End Update PointLightBuffer");

    CmdList.TransitionTexture(
        FrameResources.PointLightShadowMaps.Get(),
        EResourceState::ResourceState_PixelShaderResource,
        EResourceState::ResourceState_DepthWrite);

    // Render PointLight ShadowMaps
    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin Render PointLight ShadowMaps");

    if (UpdatePointLight)
    {
        TRACE_SCOPE("Render PointLight ShadowMaps");

        const UInt32 PointLightShadowSize = FrameResources.CurrentLightSettings.PointLightShadowSize;
        CmdList.BindViewport(
            static_cast<Float>(PointLightShadowSize),
            static_cast<Float>(PointLightShadowSize),
            0.0f,
            1.0f,
            0.0f,
            0.0f);

        CmdList.BindScissorRect(
            static_cast<Float>(PointLightShadowSize),
            static_cast<Float>(PointLightShadowSize),
            0, 0);

        CmdList.BindGraphicsPipelineState(PipelineState.Get());

        // PerObject Structs
        struct ShadowPerObject
        {
            XMFLOAT4X4 Matrix;
            Float      ShadowOffset;
        } ShadowPerObjectBuffer;

        UInt32 PointLightShadowIndex = 0;
        PointLightPerShadowMap PerShadowMapData;
        for (Light* Light : Scene.GetLights())
        {
            if (IsSubClassOf<PointLight>(Light))
            {
                PointLight* CurrentLight = Cast<PointLight>(Light);
                for (UInt32 Face = 0; Face < 6; Face++)
                {
                    auto& Cube = FrameResources.PointLightShadowMapDSVs[PointLightShadowIndex];
                    CmdList.ClearDepthStencilView(Cube[Face].Get(), DepthStencilClearValue(1.0f, 0));
                    CmdList.BindRenderTargets(nullptr, 0, Cube[Face].Get());

                    PerShadowMapData.Matrix = CurrentLight->GetMatrix(Face);
                    PerShadowMapData.Position = CurrentLight->GetPosition();
                    PerShadowMapData.FarPlane = CurrentLight->GetShadowFarPlane();

                    CmdList.TransitionBuffer(
                        PerShadowMapBuffer.Get(),
                        EResourceState::ResourceState_VertexAndConstantBuffer,
                        EResourceState::ResourceState_CopyDest);

                    CmdList.UpdateBuffer(
                        PerShadowMapBuffer.Get(),
                        0, sizeof(PointLightPerShadowMap),
                        &PerShadowMapData);

                    CmdList.TransitionBuffer(
                        PerShadowMapBuffer.Get(),
                        EResourceState::ResourceState_CopyDest,
                        EResourceState::ResourceState_VertexAndConstantBuffer);

                    CmdList.BindConstantBuffers(
                        EShaderStage::ShaderStage_Vertex,
                        PerShadowMapBuffer.GetAddressOf(),
                        1, 0);

                    // Draw all objects to depthbuffer
                    if (GlobalFrustumCullEnabled)
                    {
                        Frustum CameraFrustum = Frustum(CurrentLight->GetShadowFarPlane(), CurrentLight->GetViewMatrix(Face), CurrentLight->GetProjectionMatrix(Face));
                        for (const MeshDrawCommand& Command : Scene.GetMeshDrawCommands())
                        {
                            const XMFLOAT4X4& Transform = Command.CurrentActor->GetTransform().GetMatrix();
                            XMMATRIX XmTransform = XMMatrixTranspose(XMLoadFloat4x4(&Transform));
                            XMVECTOR XmTop       = XMVectorSetW(XMLoadFloat3(&Command.Mesh->BoundingBox.Top), 1.0f);
                            XMVECTOR XmBottom    = XMVectorSetW(XMLoadFloat3(&Command.Mesh->BoundingBox.Bottom), 1.0f);
                            XmTop    = XMVector4Transform(XmTop, XmTransform);
                            XmBottom = XMVector4Transform(XmBottom, XmTransform);

                            AABB Box;
                            XMStoreFloat3(&Box.Top, XmTop);
                            XMStoreFloat3(&Box.Bottom, XmBottom);
                            if (CameraFrustum.CheckAABB(Box))
                            {
                                CmdList.BindVertexBuffers(&Command.VertexBuffer, 1, 0);
                                CmdList.BindIndexBuffer(Command.IndexBuffer);

                                ShadowPerObjectBuffer.Matrix       = Command.CurrentActor->GetTransform().GetMatrix();
                                ShadowPerObjectBuffer.ShadowOffset = Command.Mesh->ShadowOffset;

                                CmdList.Bind32BitShaderConstants(
                                    EShaderStage::ShaderStage_Vertex,
                                    &ShadowPerObjectBuffer, 17);

                                CmdList.DrawIndexedInstanced(Command.IndexCount, 1, 0, 0, 0);
                            }
                        }
                    }
                    else
                    {
                        for (const MeshDrawCommand& Command : Scene.GetMeshDrawCommands())
                        {
                            CmdList.BindVertexBuffers(&Command.VertexBuffer, 1, 0);
                            CmdList.BindIndexBuffer(Command.IndexBuffer);

                            ShadowPerObjectBuffer.Matrix       = Command.CurrentActor->GetTransform().GetMatrix();
                            ShadowPerObjectBuffer.ShadowOffset = Command.Mesh->ShadowOffset;

                            CmdList.Bind32BitShaderConstants(
                                EShaderStage::ShaderStage_Vertex,
                                &ShadowPerObjectBuffer, 17);

                            CmdList.DrawIndexedInstanced(Command.IndexCount, 1, 0, 0, 0);
                        }
                    }
                }

                PointLightShadowIndex++;
            }
        }

        UpdatePointLight = false;
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End Render PointLight ShadowMaps");

    CmdList.TransitionTexture(
        FrameResources.PointLightShadowMaps.Get(),
        EResourceState::ResourceState_DepthWrite,
        EResourceState::ResourceState_NonPixelShaderResource);
}

Bool PointLightShadowSceneRenderPass::CreateShadowMaps(SharedRenderPassResources& FrameResources)
{
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
