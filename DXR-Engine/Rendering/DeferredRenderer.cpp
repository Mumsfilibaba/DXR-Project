#include "DeferredRenderer.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/ShaderCompiler.h"

#include "Rendering/MeshDrawCommand.h"
#include "Rendering/Mesh.h"
#include "Rendering/Material.h"

#include "Debug/Profiler.h"

Bool DeferredRenderer::Init(FrameResources& FrameResources)
{
    if (!CreateGBuffer(FrameResources))
    {
        return false;
    }

    {
        SamplerStateCreateInfo CreateInfo;
        CreateInfo.AddressU = ESamplerMode::SamplerMode_Clamp;
        CreateInfo.AddressV = ESamplerMode::SamplerMode_Clamp;
        CreateInfo.AddressW = ESamplerMode::SamplerMode_Clamp;
        CreateInfo.Filter   = ESamplerFilter::SamplerFilter_MinMagMipPoint;

        FrameResources.GBufferSampler = RenderLayer::CreateSamplerState(CreateInfo);
        if (!FrameResources.GBufferSampler)
        {
            return false;
        }
    }

    TArray<UInt8> ShaderCode;
    {
        TArray<ShaderDefine> Defines =
        {
            { "ENABLE_PARALLAX_MAPPING", "1" },
            { "ENABLE_NORMAL_MAPPING",   "1" },
        };

        if (!ShaderCompiler::CompileFromFile(
            "../DXR-Engine/Shaders/GeometryPass.hlsl",
            "VSMain",
            &Defines,
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
            VShader->SetName("GeometryPass VertexShader");
        }

        if (!ShaderCompiler::CompileFromFile(
            "../DXR-Engine/Shaders/GeometryPass.hlsl",
            "PSMain",
            &Defines,
            EShaderStage::ShaderStage_Pixel,
            EShaderModel::ShaderModel_6_0,
            ShaderCode))
        {
            Debug::DebugBreak();
            return false;
        }

        TSharedRef<PixelShader> PShader = RenderLayer::CreatePixelShader(ShaderCode);
        if (!PShader)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            PShader->SetName("GeometryPass PixelShader");
        }

        DepthStencilStateCreateInfo DepthStencilStateInfo;
        DepthStencilStateInfo.DepthFunc      = EComparisonFunc::ComparisonFunc_LessEqual;
        DepthStencilStateInfo.DepthEnable    = true;
        DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::DepthWriteMask_All;

        TSharedRef<DepthStencilState> GeometryDepthStencilState = RenderLayer::CreateDepthStencilState(DepthStencilStateInfo);
        if (!GeometryDepthStencilState)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            GeometryDepthStencilState->SetName("GeometryPass DepthStencilState");
        }

        RasterizerStateCreateInfo RasterizerStateInfo;
        RasterizerStateInfo.CullMode = ECullMode::CullMode_Back;

        TSharedRef<RasterizerState> GeometryRasterizerState = RenderLayer::CreateRasterizerState(RasterizerStateInfo);
        if (!GeometryRasterizerState)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            GeometryRasterizerState->SetName("GeometryPass RasterizerState");
        }

        BlendStateCreateInfo BlendStateInfo;
        BlendStateInfo.IndependentBlendEnable      = false;
        BlendStateInfo.RenderTarget[0].BlendEnable = false;

        TSharedRef<BlendState> BlendState = RenderLayer::CreateBlendState(BlendStateInfo);
        if (!BlendState)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            BlendState->SetName("GeometryPass BlendState");
        }

        GraphicsPipelineStateCreateInfo PipelineStateInfo;
        PipelineStateInfo.InputLayoutState                       = FrameResources.StdInputLayout.Get();
        PipelineStateInfo.BlendState                             = BlendState.Get();
        PipelineStateInfo.DepthStencilState                      = GeometryDepthStencilState.Get();
        PipelineStateInfo.RasterizerState                        = GeometryRasterizerState.Get();
        PipelineStateInfo.ShaderState.VertexShader               = VShader.Get();
        PipelineStateInfo.ShaderState.PixelShader                = PShader.Get();
        PipelineStateInfo.PipelineFormats.DepthStencilFormat     = FrameResources.DepthBufferFormat;
        PipelineStateInfo.PipelineFormats.RenderTargetFormats[0] = EFormat::Format_R8G8B8A8_Unorm;
        PipelineStateInfo.PipelineFormats.RenderTargetFormats[1] = FrameResources.NormalFormat;
        PipelineStateInfo.PipelineFormats.RenderTargetFormats[2] = EFormat::Format_R8G8B8A8_Unorm;
        PipelineStateInfo.PipelineFormats.RenderTargetFormats[3] = FrameResources.ViewNormalFormat;
        PipelineStateInfo.PipelineFormats.NumRenderTargets       = 4;

        PipelineState = RenderLayer::CreateGraphicsPipelineState(PipelineStateInfo);
        if (!PipelineState)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            PipelineState->SetName("GeometryPass PipelineState");
        }
    }

    // PrePass
    {
        if (!ShaderCompiler::CompileFromFile(
            "../DXR-Engine/Shaders/PrePass.hlsl",
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
            VShader->SetName("PrePass VertexShader");
        }

        DepthStencilStateCreateInfo DepthStencilStateInfo;
        DepthStencilStateInfo.DepthFunc      = EComparisonFunc::ComparisonFunc_Less;
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
            DepthStencilState->SetName("Prepass DepthStencilState");
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
            RasterizerState->SetName("Prepass RasterizerState");
        }

        BlendStateCreateInfo BlendStateInfo;
        BlendStateInfo.IndependentBlendEnable      = false;
        BlendStateInfo.RenderTarget[0].BlendEnable = false;

        TSharedRef<BlendState> BlendState = RenderLayer::CreateBlendState(BlendStateInfo);
        if (!BlendState)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            BlendState->SetName("Prepass BlendState");
        }

        GraphicsPipelineStateCreateInfo PipelineStateInfo;
        PipelineStateInfo.InputLayoutState                   = FrameResources.StdInputLayout.Get();
        PipelineStateInfo.BlendState                         = BlendState.Get();
        PipelineStateInfo.DepthStencilState                  = DepthStencilState.Get();
        PipelineStateInfo.RasterizerState                    = RasterizerState.Get();
        PipelineStateInfo.ShaderState.VertexShader           = VShader.Get();
        PipelineStateInfo.PipelineFormats.DepthStencilFormat = FrameResources.DepthBufferFormat;

        PrePassPipelineState = RenderLayer::CreateGraphicsPipelineState(PipelineStateInfo);
        if (!PrePassPipelineState)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            PrePassPipelineState->SetName("PrePass PipelineState");
        }
    }

    constexpr UInt32  LUTSize   = 512;
    constexpr EFormat LUTFormat = EFormat::Format_R16G16_Float;
    if (!RenderLayer::UAVSupportsFormat(EFormat::Format_R16G16_Float))
    {
        LOG_ERROR("[Renderer]: Format_R16G16_Float is not supported for UAVs");

        Debug::DebugBreak();
        return false;
    }

    TSharedRef<Texture2D> StagingTexture = RenderLayer::CreateTexture2D(
        nullptr,
        LUTFormat,
        TextureUsage_Default | TextureUsage_UAV,
        LUTSize,
        LUTSize,
        1, 1);
    if (!StagingTexture)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        StagingTexture->SetName("Staging IntegrationLUT");
    }

    TSharedRef<UnorderedAccessView> StagingTextureUAV = RenderLayer::CreateUnorderedAccessView(
        StagingTexture.Get(), 
        LUTFormat, 0);
    if (!StagingTextureUAV)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        StagingTextureUAV->SetName("IntegrationLUT UAV");
    }

    FrameResources.IntegrationLUT = RenderLayer::CreateTexture2D(
        nullptr,
        LUTFormat,
        TextureUsage_Default | TextureUsage_SRV,
        LUTSize,
        LUTSize,
        1, 1);
    if (!FrameResources.IntegrationLUT)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        FrameResources.IntegrationLUT->SetName("IntegrationLUT");
    }

    FrameResources.IntegrationLUTSRV = RenderLayer::CreateShaderResourceView(
        FrameResources.IntegrationLUT.Get(),
        LUTFormat,
        0, 1);
    if (!FrameResources.IntegrationLUTSRV)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        FrameResources.IntegrationLUTSRV->SetName("IntegrationLUT SRV");
    }

    SamplerStateCreateInfo CreateInfo;
    CreateInfo.AddressU = ESamplerMode::SamplerMode_Clamp;
    CreateInfo.AddressV = ESamplerMode::SamplerMode_Clamp;
    CreateInfo.AddressW = ESamplerMode::SamplerMode_Clamp;
    CreateInfo.Filter   = ESamplerFilter::SamplerFilter_MinMagMipPoint;

    FrameResources.IntegrationLUTSampler = RenderLayer::CreateSamplerState(CreateInfo);
    if (!FrameResources.IntegrationLUTSampler)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        FrameResources.IntegrationLUTSampler->SetName("IntegrationLUT Sampler");
    }

    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/BRDFIntegationGen.hlsl",
        "Main",
        nullptr,
        EShaderStage::ShaderStage_Compute,
        EShaderModel::ShaderModel_6_0,
        ShaderCode))
    {
        Debug::DebugBreak();
        return false;
    }

    TSharedRef<ComputeShader> CShader = RenderLayer::CreateComputeShader(ShaderCode);
    if (!CShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        CShader->SetName("BRDFIntegationGen ComputeShader");
    }

    ComputePipelineStateCreateInfo PipelineStateInfo;
    PipelineStateInfo.Shader = CShader.Get();

    TSharedRef<ComputePipelineState> BRDF_PipelineState = RenderLayer::CreateComputePipelineState(PipelineStateInfo);
    if (!BRDF_PipelineState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        BRDF_PipelineState->SetName("BRDFIntegationGen PipelineState");
    }

    CommandList CmdList;
    CmdList.Begin();

    CmdList.TransitionTexture(
        StagingTexture.Get(),
        EResourceState::ResourceState_Common,
        EResourceState::ResourceState_UnorderedAccess);

    CmdList.BindComputePipelineState(BRDF_PipelineState.Get());

    CmdList.BindUnorderedAccessViews(
        EShaderStage::ShaderStage_Compute,
        &StagingTextureUAV, 1, 0);

    constexpr UInt32 ThreadCount = 16;
    const UInt32 DispatchWidth  = Math::DivideByMultiple(LUTSize, ThreadCount);
    const UInt32 DispatchHeight = Math::DivideByMultiple(LUTSize, ThreadCount);
    CmdList.Dispatch(DispatchWidth, DispatchHeight, 1);

    CmdList.UnorderedAccessTextureBarrier(StagingTexture.Get());

    CmdList.TransitionTexture(
        StagingTexture.Get(),
        EResourceState::ResourceState_UnorderedAccess,
        EResourceState::ResourceState_CopySource);

    CmdList.TransitionTexture(
        FrameResources.IntegrationLUT.Get(),
        EResourceState::ResourceState_Common,
        EResourceState::ResourceState_CopyDest);

    CmdList.CopyTexture(FrameResources.IntegrationLUT.Get(), StagingTexture.Get());

    CmdList.TransitionTexture(
        FrameResources.IntegrationLUT.Get(),
        EResourceState::ResourceState_CopyDest,
        EResourceState::ResourceState_PixelShaderResource);

    CmdList.DestroyResource(StagingTexture.Get());
    CmdList.DestroyResource(StagingTextureUAV.Get());
    CmdList.DestroyResource(BRDF_PipelineState.Get());

    CmdList.End();
    GlobalCmdListExecutor.ExecuteCommandList(CmdList);

    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/DeferredLightPass.hlsl",
        "Main",
        nullptr,
        EShaderStage::ShaderStage_Compute,
        EShaderModel::ShaderModel_6_0,
        ShaderCode))
    {
        Debug::DebugBreak();
        return false;
    }

    CShader = RenderLayer::CreateComputeShader(ShaderCode);
    if (!CShader)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        CShader->SetName("DeferredLightPass Shader");
    }

    ComputePipelineStateCreateInfo DeferredLightPassCreateInfo;
    DeferredLightPassCreateInfo.Shader = CShader.Get();

    TiledLightPassPSO = RenderLayer::CreateComputePipelineState(DeferredLightPassCreateInfo);
    if (!TiledLightPassPSO)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        TiledLightPassPSO->SetName("DeferredLightPass PipelineState");
    }

    return true;
}

void DeferredRenderer::Release()
{
    PipelineState.Reset();
    PrePassPipelineState.Reset();
    TiledLightPassPSO.Reset();
}

void DeferredRenderer::RenderPrePass(
    CommandList& CmdList,
    const FrameResources& FrameResources)
{
    // Clear FrameResources.GBuffer
    ColorClearValue BlackClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    CmdList.ClearRenderTargetView(FrameResources.GBufferRTVs[GBUFFER_ALBEDO_INDEX].Get(), BlackClearColor);
    CmdList.ClearRenderTargetView(FrameResources.GBufferRTVs[GBUFFER_NORMAL_INDEX].Get(), BlackClearColor);
    CmdList.ClearRenderTargetView(FrameResources.GBufferRTVs[GBUFFER_MATERIAL_INDEX].Get(), BlackClearColor);
    CmdList.ClearRenderTargetView(FrameResources.GBufferRTVs[GBUFFER_VIEW_NORMAL_INDEX].Get(), BlackClearColor);
    CmdList.ClearDepthStencilView(FrameResources.GBufferDSV.Get(), DepthStencilClearValue(1.0f, 0));

    const Float RenderWidth  = Float(FrameResources.MainWindowViewport->GetWidth());
    const Float RenderHeight = Float(FrameResources.MainWindowViewport->GetHeight());

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin PrePass");

    TRACE_SCOPE("PrePass");

    CmdList.BindViewport(
        RenderWidth,
        RenderHeight,
        0.0f,
        1.0f,
        0.0f,
        0.0f);

    CmdList.BindScissorRect(
        RenderWidth,
        RenderHeight,
        0, 0);

    struct PerObject
    {
        XMFLOAT4X4 Matrix;
    } PerObjectBuffer;

    CmdList.BindRenderTargets(nullptr, 0, FrameResources.GBufferDSV.Get());

    CmdList.BindGraphicsPipelineState(PrePassPipelineState.Get());

    CmdList.BindConstantBuffers(
        EShaderStage::ShaderStage_Vertex,
        FrameResources.CameraBuffer.GetAddressOf(),
        1, 0);

    for (const MeshDrawCommand& Command : FrameResources.DeferredVisibleCommands)
    {
        if (!Command.Material->HasHeightMap())
        {
            CmdList.BindVertexBuffers(&Command.VertexBuffer, 1, 0);
            CmdList.BindIndexBuffer(Command.IndexBuffer);

            PerObjectBuffer.Matrix = Command.CurrentActor->GetTransform().GetMatrix();

            CmdList.Bind32BitShaderConstants(
                EShaderStage::ShaderStage_Vertex,
                &PerObjectBuffer, 16);

            CmdList.DrawIndexedInstanced(Command.IndexCount, 1, 0, 0, 0);
        }
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End PrePass");
}

void DeferredRenderer::RenderBasePass(
    CommandList& CmdList,
    const FrameResources& FrameResources)
{
    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin GeometryPass");

    TRACE_SCOPE("GeometryPass");

    const Float RenderWidth  = Float(FrameResources.MainWindowViewport->GetWidth());
    const Float RenderHeight = Float(FrameResources.MainWindowViewport->GetHeight());

    CmdList.BindViewport(
        RenderWidth,
        RenderHeight,
        0.0f,
        1.0f,
        0.0f,
        0.0f);

    CmdList.BindScissorRect(
        RenderWidth,
        RenderHeight,
        0, 0);

    RenderTargetView* RenderTargets[] =
    {
        FrameResources.GBufferRTVs[GBUFFER_ALBEDO_INDEX].Get(),
        FrameResources.GBufferRTVs[GBUFFER_NORMAL_INDEX].Get(),
        FrameResources.GBufferRTVs[GBUFFER_MATERIAL_INDEX].Get(),
        FrameResources.GBufferRTVs[GBUFFER_VIEW_NORMAL_INDEX].Get(),
    };
    CmdList.BindRenderTargets(RenderTargets, 4, FrameResources.GBufferDSV.Get());

    // Setup Pipeline
    CmdList.BindGraphicsPipelineState(PipelineState.Get());

    struct TransformBuffer
    {
        XMFLOAT4X4 Transform;
        XMFLOAT4X4 TransformInv;
    } TransformPerObject;

    for (const MeshDrawCommand& Command : FrameResources.DeferredVisibleCommands)
    {
        CmdList.BindVertexBuffers(&Command.VertexBuffer, 1, 0);
        CmdList.BindIndexBuffer(Command.IndexBuffer);

        if (Command.Material->IsBufferDirty())
        {
            Command.Material->BuildBuffer(CmdList);
        }

        CmdList.BindConstantBuffers(
            EShaderStage::ShaderStage_Vertex,
            &FrameResources.CameraBuffer,
            1, 0);

        ConstantBuffer* MaterialBuffer = Command.Material->GetMaterialBuffer();
        CmdList.BindConstantBuffers(
            EShaderStage::ShaderStage_Pixel,
            &MaterialBuffer,
            1, 1);

        TransformPerObject.Transform = Command.CurrentActor->GetTransform().GetMatrix();
        TransformPerObject.TransformInv = Command.CurrentActor->GetTransform().GetMatrixInverse();

        ShaderResourceView* const* ShaderResourceViews = Command.Material->GetShaderResourceViews();
        CmdList.BindShaderResourceViews(
            EShaderStage::ShaderStage_Pixel,
            ShaderResourceViews,
            6, 0);

        SamplerState* Sampler = Command.Material->GetMaterialSampler();
        CmdList.BindSamplerStates(
            EShaderStage::ShaderStage_Pixel,
            &Sampler,
            1, 0);

        CmdList.Bind32BitShaderConstants(
            EShaderStage::ShaderStage_Vertex,
            &TransformPerObject, 32);

        CmdList.DrawIndexedInstanced(Command.IndexCount, 1, 0, 0, 0);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End GeometryPass");
}

void DeferredRenderer::RenderDeferredTiledLightPass(
    CommandList& CmdList,
    const FrameResources& FrameResources,
    const SceneLightSetup& LightSetup)
{
    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin LightPass");

    TRACE_SCOPE("LightPass");

    CmdList.BindComputePipelineState(TiledLightPassPSO.Get());

    ShaderResourceView* ShaderResourceViews[] =
    {
        FrameResources.GBufferSRVs[GBUFFER_ALBEDO_INDEX].Get(),
        FrameResources.GBufferSRVs[GBUFFER_NORMAL_INDEX].Get(),
        FrameResources.GBufferSRVs[GBUFFER_MATERIAL_INDEX].Get(),
        FrameResources.GBufferSRVs[GBUFFER_DEPTH_INDEX].Get(),
        FrameResources.ReflectionTextureSRV.Get(),
        LightSetup.IrradianceMapSRV.Get(),
        LightSetup.SpecularIrradianceMapSRV.Get(),
        FrameResources.IntegrationLUTSRV.Get(),
        LightSetup.DirLightShadowMapSRV.Get(),
        LightSetup.PointLightShadowMapSRV.Get(),
        FrameResources.SSAOBufferSRV.Get()
    };

    CmdList.BindShaderResourceViews(
        EShaderStage::ShaderStage_Compute,
        ShaderResourceViews,
        11, 0);

    ConstantBuffer* ConstantBuffers[] =
    {
        FrameResources.CameraBuffer.Get(),
        LightSetup.PointLightBuffer.Get(),
        LightSetup.DirectionalLightBuffer.Get()
    };

    CmdList.BindConstantBuffers(
        EShaderStage::ShaderStage_Compute,
        ConstantBuffers,
        3, 0);

    SamplerState* SamplerStates[] =
    {
        FrameResources.GBufferSampler.Get(),
        FrameResources.IntegrationLUTSampler.Get(),
        FrameResources.IrradianceSampler.Get(),
        FrameResources.ShadowMapCompSampler.Get(),
        FrameResources.ShadowMapSampler.Get()
    };

    CmdList.BindSamplerStates(
        EShaderStage::ShaderStage_Compute,
        SamplerStates,
        5,
        0);

    CmdList.BindUnorderedAccessViews(
        EShaderStage::ShaderStage_Compute,
        &FrameResources.FinalTargetUAV,
        1,
        0);

    struct LightPassSettings
    {
        Int32 NumPointLights;
        Int32 NumSkyLightMips;
        Int32 ScreenWidth;
        Int32 ScreenHeight;
    } Settings;

    Settings.NumPointLights  = 4;
    Settings.NumSkyLightMips = LightSetup.SpecularIrradianceMap->GetMipLevels();
    Settings.ScreenWidth     = FrameResources.FinalTarget->GetWidth();
    Settings.ScreenHeight    = FrameResources.FinalTarget->GetHeight();

    CmdList.Bind32BitShaderConstants(
        EShaderStage::ShaderStage_Compute,
        &Settings, 4);

    constexpr UInt32 ThreadCount = 16;
    const UInt32 WorkGroupWidth  = Math::DivideByMultiple<UInt32>(Settings.ScreenWidth, ThreadCount);
    const UInt32 WorkGroupHeight = Math::DivideByMultiple<UInt32>(Settings.ScreenHeight, ThreadCount);
    CmdList.Dispatch(WorkGroupWidth, WorkGroupHeight, 1);

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End LightPass");
}

Bool DeferredRenderer::CreateGBuffer(FrameResources& FrameResources)
{
    const UInt32 Width  = FrameResources.MainWindowViewport->GetWidth();
    const UInt32 Height = FrameResources.MainWindowViewport->GetHeight();
    const UInt32 Usage  = TextureUsage_Default | TextureUsage_RenderTarget;

    FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX] = RenderLayer::CreateTexture2D(
        nullptr,
        FrameResources.AlbedoFormat,
        Usage,
        Width,
        Height,
        1, 1,
        ClearValue(ColorClearValue(0.0f, 0.0f, 0.0f, 1.0f)));
    if (FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX])
    {
        FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX]->SetName("FrameResources.GBuffer Albedo");

        FrameResources.GBufferSRVs[GBUFFER_ALBEDO_INDEX] = RenderLayer::CreateShaderResourceView(
            FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX].Get(),
            FrameResources.AlbedoFormat, 0, 1);
        if (!FrameResources.GBufferSRVs[GBUFFER_ALBEDO_INDEX])
        {
            return false;
        }

        FrameResources.GBufferRTVs[GBUFFER_ALBEDO_INDEX] = RenderLayer::CreateRenderTargetView(
            FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX].Get(),
            FrameResources.AlbedoFormat, 0);
        if (!FrameResources.GBufferSRVs[GBUFFER_ALBEDO_INDEX])
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    // Normal
    FrameResources.GBuffer[GBUFFER_NORMAL_INDEX] = RenderLayer::CreateTexture2D(
        nullptr,
        FrameResources.NormalFormat,
        Usage,
        Width,
        Height,
        1, 1,
        ClearValue(ColorClearValue(0.0f, 0.0f, 0.0f, 1.0f)));
    if (FrameResources.GBuffer[GBUFFER_NORMAL_INDEX])
    {
        FrameResources.GBuffer[GBUFFER_NORMAL_INDEX]->SetName("FrameResources.GBuffer Normal");

        FrameResources.GBufferSRVs[GBUFFER_NORMAL_INDEX] = RenderLayer::CreateShaderResourceView(
            FrameResources.GBuffer[GBUFFER_NORMAL_INDEX].Get(),
            FrameResources.NormalFormat,
            0, 1);
        if (!FrameResources.GBufferSRVs[GBUFFER_NORMAL_INDEX])
        {
            return false;
        }

        FrameResources.GBufferRTVs[GBUFFER_NORMAL_INDEX] = RenderLayer::CreateRenderTargetView(
            FrameResources.GBuffer[GBUFFER_NORMAL_INDEX].Get(),
            FrameResources.NormalFormat,
            0);
        if (!FrameResources.GBufferSRVs[GBUFFER_NORMAL_INDEX])
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    // Material Properties
    FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX] = RenderLayer::CreateTexture2D(
        nullptr,
        FrameResources.MaterialFormat,
        Usage,
        Width,
        Height,
        1, 1,
        ClearValue(ColorClearValue(0.0f, 0.0f, 0.0f, 1.0f)));
    if (FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX])
    {
        FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX]->SetName("FrameResources.GBuffer Material");

        FrameResources.GBufferSRVs[GBUFFER_MATERIAL_INDEX] = RenderLayer::CreateShaderResourceView(
            FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX].Get(),
            FrameResources.MaterialFormat,
            0, 1);
        if (!FrameResources.GBufferSRVs[GBUFFER_MATERIAL_INDEX])
        {
            return false;
        }

        FrameResources.GBufferRTVs[GBUFFER_MATERIAL_INDEX] = RenderLayer::CreateRenderTargetView(
            FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX].Get(),
            FrameResources.MaterialFormat,
            0);
        if (!FrameResources.GBufferSRVs[GBUFFER_MATERIAL_INDEX])
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    // DepthStencil
    const UInt32 UsageDS = TextureUsage_Default | TextureUsage_DSV | TextureUsage_SRV;
    FrameResources.GBuffer[GBUFFER_DEPTH_INDEX] = RenderLayer::CreateTexture2D(
        nullptr,
        EFormat::Format_R32_Typeless,
        UsageDS,
        Width,
        Height,
        1, 1,
        ClearValue(DepthStencilClearValue(1.0f, 0)));
    if (FrameResources.GBuffer[GBUFFER_DEPTH_INDEX])
    {
        FrameResources.GBuffer[GBUFFER_DEPTH_INDEX]->SetName("FrameResources.GBuffer DepthStencil");

        FrameResources.GBufferSRVs[GBUFFER_DEPTH_INDEX] = RenderLayer::CreateShaderResourceView(
            FrameResources.GBuffer[GBUFFER_DEPTH_INDEX].Get(),
            EFormat::Format_R32_Float,
            0,
            1);
        if (!FrameResources.GBufferSRVs[GBUFFER_DEPTH_INDEX])
        {
            return false;
        }

        FrameResources.GBufferDSV = RenderLayer::CreateDepthStencilView(
            FrameResources.GBuffer[GBUFFER_DEPTH_INDEX].Get(),
            FrameResources.DepthBufferFormat,
            0);
        if (!FrameResources.GBufferDSV)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    // View Normal
    FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX] = RenderLayer::CreateTexture2D(
        nullptr,
        FrameResources.ViewNormalFormat,
        Usage,
        Width,
        Height,
        1, 1,
        ClearValue(ColorClearValue(0.0f, 0.0f, 0.0f, 1.0f)));
    if (FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX])
    {
        FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX]->SetName("FrameResources.GBuffer View Normal");

        FrameResources.GBufferSRVs[GBUFFER_VIEW_NORMAL_INDEX] = RenderLayer::CreateShaderResourceView(
            FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX].Get(),
            FrameResources.ViewNormalFormat,
            0, 1);
        if (!FrameResources.GBufferSRVs[GBUFFER_VIEW_NORMAL_INDEX])
        {
            return false;
        }

        FrameResources.GBufferRTVs[GBUFFER_VIEW_NORMAL_INDEX] = RenderLayer::CreateRenderTargetView(
            FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX].Get(),
            FrameResources.ViewNormalFormat,
            0);
        if (!FrameResources.GBufferSRVs[GBUFFER_VIEW_NORMAL_INDEX])
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    // Final Image
    FrameResources.FinalTarget = RenderLayer::CreateTexture2D(
        nullptr,
        FrameResources.FinalTargetFormat,
        TextureUsage_Default | TextureUsage_RenderTarget | TextureUsage_UAV,
        Width,
        Height,
        1, 1,
        ClearValue(ColorClearValue(0.0f, 0.0f, 0.0f, 1.0f)));
    if (FrameResources.FinalTarget)
    {
        FrameResources.FinalTarget->SetName("Final Target");

        FrameResources.FinalTargetSRV = RenderLayer::CreateShaderResourceView(
            FrameResources.FinalTarget.Get(),
            FrameResources.FinalTargetFormat,
            0, 1);
        if (!FrameResources.FinalTargetSRV)
        {
            return false;
        }

        FrameResources.FinalTargetRTV = RenderLayer::CreateRenderTargetView(
            FrameResources.FinalTarget.Get(),
            FrameResources.FinalTargetFormat,
            0);
        if (!FrameResources.FinalTargetRTV)
        {
            return false;
        }

        FrameResources.FinalTargetUAV = RenderLayer::CreateUnorderedAccessView(
            FrameResources.FinalTarget.Get(),
            FrameResources.FinalTargetFormat,
            0);
        if (!FrameResources.FinalTargetUAV)
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    return true;
}
