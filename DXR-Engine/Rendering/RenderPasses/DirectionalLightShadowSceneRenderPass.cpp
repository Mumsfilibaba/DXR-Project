#include "DirectionalLightShadowSceneRenderPass.h"

#include "Debug/Profiler.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/ShaderCompiler.h"

#include "Scene/Scene.h"
#include "Scene/Lights/DirectionalLight.h"

#include "Rendering/Mesh.h"

static const EFormat ShadowMapFormat = EFormat::Format_D32_Float;

struct DirectionalLightPerShadowMap
{
    XMFLOAT4X4 Matrix;
    XMFLOAT3   Position;
    Float      FarPlane;
};

Bool DirectionalLightShadowSceneRenderPass::Init(SharedRenderPassResources& FrameResources)
{
    if (!CreateShadowMaps(FrameResources))
    {
        return false;
    }

    FrameResources.DirectionalLightBuffer = RenderLayer::CreateConstantBuffer<DirectionalLightProperties>(
        nullptr,
        FrameResources.MaxDirectionalLights,
        BufferUsage_Default,
        EResourceState::ResourceState_VertexAndConstantBuffer);
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
        EResourceState::ResourceState_VertexAndConstantBuffer);
    if (!PerShadowMapBuffer)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PerShadowMapBuffer->SetName("PerShadowMap Buffer");
    }

    TArray<UInt8> ShaderCode;
#if ENABLE_VSM
    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/ShadowMap.hlsl",
        "VSM_VSMain",
        nullptr,
        EShaderStage::ShaderStage_Vertex,
        EShaderModel::ShaderModel_6_0,
        ShaderCode))
    {
        Debug::DebugBreak();
        return false;
    }

    TSharedRef<VertexShader> VSShader = RenderLayer::CreateVertexShader(ShaderCode);
    if (!VSShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        VSShader->SetName("ShadowMap VertexShader");
    }

    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/ShadowMap.hlsl",
        "VSM_PSMain",
        nullptr,
        EShaderStage::ShaderStage_Pixel,
        EShaderModel::ShaderModel_6_0,
        ShaderCode))
    {
        Debug::DebugBreak();
        return false;
    }

    TSharedRef<PixelShader> PSShader = RenderLayer::CreatePixelShader(ShaderCode);
    if (!PSShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PSShader->SetName("ShadowMap PixelShader");
    }
#else
    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/ShadowMap.hlsl",
        "Main",
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
        VShader->SetName("ShadowMap VertexShader");
    }
#endif

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
    PipelineStateInfo.BlendState               = BlendState.Get();
    PipelineStateInfo.DepthStencilState        = DepthStencilState.Get();
    PipelineStateInfo.IBStripCutValue          = EIndexBufferStripCutValue::IndexBufferStripCutValue_Disabled;
    PipelineStateInfo.InputLayoutState         = FrameResources.StdInputLayout.Get();
    PipelineStateInfo.PrimitiveTopologyType    = EPrimitiveTopologyType::PrimitiveTopologyType_Triangle;
    PipelineStateInfo.RasterizerState          = RasterizerState.Get();
    PipelineStateInfo.SampleCount              = 1;
    PipelineStateInfo.SampleQuality            = 0;
    PipelineStateInfo.SampleMask               = 0xffffffff;
    PipelineStateInfo.ShaderState.VertexShader = VShader.Get();
    PipelineStateInfo.ShaderState.PixelShader  = nullptr;
    PipelineStateInfo.PipelineFormats.NumRenderTargets   = 0;
    PipelineStateInfo.PipelineFormats.DepthStencilFormat = ShadowMapFormat;

#if ENABLE_VSM
    VSMShadowMapPSO = MakeShared<D3D12GraphicsPipelineState>(Device.Get());
    if (!VSMShadowMapPSO->Initialize(PSOProperties))
    {
        return false;
    }
#else
    PipelineState = RenderLayer::CreateGraphicsPipelineState(PipelineStateInfo);
    if (!PipelineState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PipelineState->SetName("ShadowMap PipelineState");
    }
#endif

    return true;
}

Bool DirectionalLightShadowSceneRenderPass::ResizeResources(SharedRenderPassResources& FrameResources)
{
    return CreateShadowMaps(FrameResources);
}

void DirectionalLightShadowSceneRenderPass::Render(
    CommandList& CmdList, 
    SharedRenderPassResources& FrameResources,
    const Scene& Scene)
{
    DirLightFrame++;
    if (DirLightFrame > 6)
    {
        UpdateDirLight = true;
        DirLightFrame  = 0;
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin Update DirectionalLightBuffer");

    if (UpdateDirLight)
    {
        TRACE_SCOPE("Update Directional LightBuffers");

        CmdList.TransitionBuffer(
            FrameResources.DirectionalLightBuffer.Get(),
            EResourceState::ResourceState_VertexAndConstantBuffer,
            EResourceState::ResourceState_CopyDest);

        UInt32 NumPointLights = 0;
        UInt32 NumDirLights   = 0;

        // TODO: Not efficent to loop through all lights twice 
        for (Light* Light : Scene.GetLights())
        {
            XMFLOAT3 Color = Light->GetColor();
            Float    Intensity = Light->GetIntensity();
            if (IsSubClassOf<DirectionalLight>(Light) && UpdateDirLight)
            {
                DirectionalLight* CurrentLight = Cast<DirectionalLight>(Light);
                VALIDATE(CurrentLight != nullptr);

                DirectionalLightProperties Properties;
                Properties.Color         = XMFLOAT3(Color.x * Intensity, Color.y * Intensity, Color.z * Intensity);
                Properties.ShadowBias    = CurrentLight->GetShadowBias();
                Properties.Direction     = CurrentLight->GetDirection();
                Properties.LightMatrix   = CurrentLight->GetMatrix();
                Properties.MaxShadowBias = CurrentLight->GetMaxShadowBias();

                constexpr UInt32 SizeInBytes = sizeof(DirectionalLightProperties);
                CmdList.UpdateBuffer(
                    FrameResources.DirectionalLightBuffer.Get(),
                    NumDirLights * SizeInBytes,
                    SizeInBytes,
                    &Properties);

                NumDirLights++;
            }
        }

        CmdList.TransitionBuffer(
            FrameResources.DirectionalLightBuffer.Get(),
            EResourceState::ResourceState_CopyDest,
            EResourceState::ResourceState_VertexAndConstantBuffer);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End Update DirectionalLightBuffer");

    CmdList.TransitionTexture(
        FrameResources.DirLightShadowMaps.Get(),
        EResourceState::ResourceState_PixelShaderResource,
        EResourceState::ResourceState_DepthWrite);

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin Render DirectionalLight ShadowMaps");

    if (UpdateDirLight)
    {
        TRACE_SCOPE("Render DirectionalLight ShadowMaps");

        CmdList.ClearDepthStencilView(
            FrameResources.DirLightShadowMapDSV.Get(),
            DepthStencilClearValue(1.0f, 0));

#if ENABLE_VSM
        CmdList.TransitionTexture(VSMDirLightShadowMaps.Get(), EResourceState::ResourceState_PixelShaderResource, EResourceState::ResourceState_RenderTarget);

        //Float32 DepthClearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
        //CmdList.ClearRenderTargetView(VSMDirLightShadowMaps->GetRenderTargetView(0).Get(), DepthClearColor);
        //
        //D3D12RenderTargetView* DirLightRTVS[] =
        //{
        //    VSMDirLightShadowMaps->GetRenderTargetView(0).Get(),
        //};
        //CmdList.BindRenderTargets(DirLightRTVS, 1, DirLightShadowMaps->GetDepthStencilView(0).Get());
        //CmdList.BindGraphicsPipelineState(VSMShadowMapPSO.Get());
#else
        CmdList.BindRenderTargets(nullptr, 0, FrameResources.DirLightShadowMapDSV.Get());
        CmdList.BindGraphicsPipelineState(PipelineState.Get());
#endif

        // Setup view
        CmdList.BindViewport(
            static_cast<Float>(FrameResources.CurrentLightSettings.ShadowMapWidth),
            static_cast<Float>(FrameResources.CurrentLightSettings.ShadowMapHeight),
            0.0f,
            1.0f,
            0.0f,
            0.0f);

        CmdList.BindScissorRect(
            FrameResources.CurrentLightSettings.ShadowMapWidth,
            FrameResources.CurrentLightSettings.ShadowMapHeight,
            0, 0);

        CmdList.BindPrimitiveTopology(EPrimitiveTopology::PrimitiveTopology_TriangleList);

        // PerObject Structs
        struct ShadowPerObject
        {
            XMFLOAT4X4    Matrix;
            Float        ShadowOffset;
        } ShadowPerObjectBuffer;

        DirectionalLightPerShadowMap PerShadowMapData;
        for (Light* Light : Scene.GetLights())
        {
            if (IsSubClassOf<DirectionalLight>(Light))
            {
                DirectionalLight* DirLight = Cast<DirectionalLight>(Light);
                PerShadowMapData.Matrix = DirLight->GetMatrix();
                PerShadowMapData.Position = DirLight->GetShadowMapPosition();
                PerShadowMapData.FarPlane = DirLight->GetShadowFarPlane();

                CmdList.TransitionBuffer(
                    PerShadowMapBuffer.Get(),
                    EResourceState::ResourceState_VertexAndConstantBuffer,
                    EResourceState::ResourceState_CopyDest);

                CmdList.UpdateBuffer(
                    PerShadowMapBuffer.Get(),
                    0, sizeof(DirectionalLightPerShadowMap),
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

                break;
            }
        }

        UpdateDirLight = false;
    }
    else
    {
        CmdList.BindPrimitiveTopology(EPrimitiveTopology::PrimitiveTopology_TriangleList);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End Render DirectionalLight ShadowMaps");

#if ENABLE_VSM
    CmdList.TransitionTexture(
        VSMDirLightShadowMaps.Get(),
        EResourceState::ResourceState_RenderTarget,
        EResourceState::ResourceState_PixelShaderResource);
#endif
    CmdList.TransitionTexture(
        FrameResources.DirLightShadowMaps.Get(),
        EResourceState::ResourceState_DepthWrite,
        EResourceState::ResourceState_NonPixelShaderResource);
}

Bool DirectionalLightShadowSceneRenderPass::CreateShadowMaps(SharedRenderPassResources& FrameResources)
{
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
            return false;
        }
        else
        {
            FrameResources.DirLightShadowMapDSV->SetName("DirectionalLight DepthStencilView");
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
        else
        {
            FrameResources.DirLightShadowMapDSV->SetName("DirectionalLight ShaderResourceView");
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
            return false;
    }
        else
        {
            FrameResources.DirLightShadowMapDSV->SetName("VSM DirectionalLight DepthStencilView");
        }

        VSMDirLightShadowMapSRV = RenderLayer::CreateShaderResourceView(
            VSMDirLightShadowMaps.Get(),
            EFormat::Format_R32G32_Float,
            0, 1);
        if (!VSMDirLightShadowMapSRV)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            FrameResources.DirLightShadowMapDSV->SetName("VSM DirectionalLight ShaderResourceView");
        }
}
    else
    {
        return false;
    }
#endif

    return true;
}
