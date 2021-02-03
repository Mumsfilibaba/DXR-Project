#include "DeferredRenderer.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/ShaderCompiler.h"

#include "Rendering/MeshDrawCommand.h"
#include "Rendering/Mesh.h"
#include "Rendering/Material.h"

#include "Debug/Profiler.h"
#include "Debug/Console.h"

ConsoleVariable GlobalDrawTileDebug(EConsoleVariableType::Bool);

Bool DeferredRenderer::Init(FrameResources& FrameResources)
{
    INIT_CONSOLE_VARIABLE("r.DrawTileDebug", GlobalDrawTileDebug);
    GlobalDrawTileDebug.SetBool(false);

    if (!CreateGBuffer(FrameResources))
    {
        return false;
    }

    {
        SamplerStateCreateInfo CreateInfo;
        CreateInfo.AddressU = ESamplerMode::Clamp;
        CreateInfo.AddressV = ESamplerMode::Clamp;
        CreateInfo.AddressW = ESamplerMode::Clamp;
        CreateInfo.Filter   = ESamplerFilter::MinMagMipPoint;

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

        if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/GeometryPass.hlsl", "VSMain", &Defines, EShaderStage::Vertex, EShaderModel::SM_6_0, ShaderCode))
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

        if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/GeometryPass.hlsl", "PSMain", &Defines, EShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode))
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
        DepthStencilStateInfo.DepthFunc      = EComparisonFunc::LessEqual;
        DepthStencilStateInfo.DepthEnable    = true;
        DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::All;

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
        RasterizerStateInfo.CullMode = ECullMode::Back;

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
        PipelineStateInfo.PipelineFormats.RenderTargetFormats[0] = EFormat::R8G8B8A8_Unorm;
        PipelineStateInfo.PipelineFormats.RenderTargetFormats[1] = FrameResources.NormalFormat;
        PipelineStateInfo.PipelineFormats.RenderTargetFormats[2] = EFormat::R8G8B8A8_Unorm;
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
        if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/PrePass.hlsl", "Main", nullptr, EShaderStage::Vertex, EShaderModel::SM_6_0, ShaderCode))
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
        DepthStencilStateInfo.DepthFunc      = EComparisonFunc::Less;
        DepthStencilStateInfo.DepthEnable    = true;
        DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::All;

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
        RasterizerStateInfo.CullMode = ECullMode::Back;

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
    constexpr EFormat LUTFormat = EFormat::R16G16_Float;
    if (!RenderLayer::UAVSupportsFormat(LUTFormat))
    {
        LOG_ERROR("[Renderer]: R16G16_Float is not supported for UAVs");

        Debug::DebugBreak();
        return false;
    }

    TSharedRef<Texture2D> StagingTexture = RenderLayer::CreateTexture2D(LUTFormat, LUTSize, LUTSize, 1, 1, TextureFlag_UAV, EResourceState::Common, nullptr);
    if (!StagingTexture)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        StagingTexture->SetName("Staging IntegrationLUT");
    }

    FrameResources.IntegrationLUT = RenderLayer::CreateTexture2D(LUTFormat, LUTSize, LUTSize, 1, 1, TextureFlag_SRV, EResourceState::Common, nullptr);
    if (!FrameResources.IntegrationLUT)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        FrameResources.IntegrationLUT->SetName("IntegrationLUT");
    }

    SamplerStateCreateInfo CreateInfo;
    CreateInfo.AddressU = ESamplerMode::Clamp;
    CreateInfo.AddressV = ESamplerMode::Clamp;
    CreateInfo.AddressW = ESamplerMode::Clamp;
    CreateInfo.Filter   = ESamplerFilter::MinMagMipPoint;

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

    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/BRDFIntegationGen.hlsl", "Main", nullptr, EShaderStage::Compute, EShaderModel::SM_6_0, ShaderCode))
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

    CmdList.TransitionTexture(StagingTexture.Get(), EResourceState::Common, EResourceState::UnorderedAccess);

    CmdList.BindComputePipelineState(BRDF_PipelineState.Get());

    UnorderedAccessView* StagingUAV = StagingTexture->GetUnorderedAccessView();
    CmdList.BindUnorderedAccessViews(EShaderStage::Compute, &StagingUAV, 1, 0);

    constexpr UInt32 ThreadCount = 16;
    const UInt32 DispatchWidth  = Math::DivideByMultiple(LUTSize, ThreadCount);
    const UInt32 DispatchHeight = Math::DivideByMultiple(LUTSize, ThreadCount);
    CmdList.Dispatch(DispatchWidth, DispatchHeight, 1);

    CmdList.UnorderedAccessTextureBarrier(StagingTexture.Get());

    CmdList.TransitionTexture(StagingTexture.Get(), EResourceState::UnorderedAccess, EResourceState::CopySource);
    CmdList.TransitionTexture(FrameResources.IntegrationLUT.Get(), EResourceState::Common, EResourceState::CopyDest);

    CmdList.CopyTexture(FrameResources.IntegrationLUT.Get(), StagingTexture.Get());

    CmdList.TransitionTexture(FrameResources.IntegrationLUT.Get(), EResourceState::CopyDest, EResourceState::PixelShaderResource);

    CmdList.DestroyResource(StagingTexture.Get());
    CmdList.DestroyResource(BRDF_PipelineState.Get());

    CmdList.End();
    gCmdListExecutor.ExecuteCommandList(CmdList);

    {
        if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/DeferredLightPass.hlsl", "Main", nullptr, EShaderStage::Compute, EShaderModel::SM_6_0, ShaderCode))
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
    }

    {
        TArray<ShaderDefine> Defines =
        {
            ShaderDefine("DRAW_TILE_DEBUG", "1")
        };

        if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/DeferredLightPass.hlsl", "Main", &Defines, EShaderStage::Compute, EShaderModel::SM_6_0, ShaderCode))
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

        TiledLightPassPSODebug = RenderLayer::CreateComputePipelineState(DeferredLightPassCreateInfo);
        if (!TiledLightPassPSODebug)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            TiledLightPassPSODebug->SetName("DeferredLightPass PipelineState Debug");
        }
    }

    return true;
}

void DeferredRenderer::Release()
{
    PipelineState.Reset();
    PrePassPipelineState.Reset();
    TiledLightPassPSO.Reset();
}

void DeferredRenderer::RenderPrePass(CommandList& CmdList, const FrameResources& FrameResources)
{
    const Float RenderWidth  = Float(FrameResources.MainWindowViewport->GetWidth());
    const Float RenderHeight = Float(FrameResources.MainWindowViewport->GetHeight());

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin PrePass");

    TRACE_SCOPE("PrePass");

    CmdList.BindViewport(RenderWidth, RenderHeight, 0.0f, 1.0f, 0.0f, 0.0f);
    CmdList.BindScissorRect(RenderWidth, RenderHeight, 0, 0);

    struct PerObject
    {
        XMFLOAT4X4 Matrix;
    } PerObjectBuffer;

    CmdList.BindRenderTargets(nullptr, 0, FrameResources.GBuffer[GBUFFER_DEPTH_INDEX]->GetDepthStencilView());

    CmdList.BindGraphicsPipelineState(PrePassPipelineState.Get());

    CmdList.BindConstantBuffers(EShaderStage::Vertex, FrameResources.CameraBuffer.GetAddressOf(), 1, 0);

    for (const MeshDrawCommand& Command : FrameResources.DeferredVisibleCommands)
    {
        if (!Command.Material->HasHeightMap())
        {
            CmdList.BindVertexBuffers(&Command.VertexBuffer, 1, 0);
            CmdList.BindIndexBuffer(Command.IndexBuffer);

            PerObjectBuffer.Matrix = Command.CurrentActor->GetTransform().GetMatrix();

            CmdList.Bind32BitShaderConstants(EShaderStage::Vertex, &PerObjectBuffer, 16);

            CmdList.DrawIndexedInstanced(Command.IndexCount, 1, 0, 0, 0);
        }
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End PrePass");
}

void DeferredRenderer::RenderBasePass(CommandList& CmdList, const FrameResources& FrameResources)
{
    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin GeometryPass");

    TRACE_SCOPE("GeometryPass");

    const Float RenderWidth  = Float(FrameResources.MainWindowViewport->GetWidth());
    const Float RenderHeight = Float(FrameResources.MainWindowViewport->GetHeight());

    CmdList.BindViewport(RenderWidth, RenderHeight, 0.0f, 1.0f, 0.0f, 0.0f);
    CmdList.BindScissorRect(RenderWidth, RenderHeight, 0, 0);

    RenderTargetView* RenderTargets[] =
    {
        FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX]->GetRenderTargetView(),
        FrameResources.GBuffer[GBUFFER_NORMAL_INDEX]->GetRenderTargetView(),
        FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX]->GetRenderTargetView(),
        FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX]->GetRenderTargetView(),
    };
    CmdList.BindRenderTargets(RenderTargets, 4, FrameResources.GBuffer[GBUFFER_DEPTH_INDEX]->GetDepthStencilView());

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

        CmdList.BindConstantBuffers(EShaderStage::Vertex, &FrameResources.CameraBuffer, 1, 0);

        ConstantBuffer* MaterialBuffer = Command.Material->GetMaterialBuffer();
        CmdList.BindConstantBuffers(EShaderStage::Pixel, &MaterialBuffer, 1, 1);

        TransformPerObject.Transform    = Command.CurrentActor->GetTransform().GetMatrix();
        TransformPerObject.TransformInv = Command.CurrentActor->GetTransform().GetMatrixInverse();

        ShaderResourceView* const* ShaderResourceViews = Command.Material->GetShaderResourceViews();
        CmdList.BindShaderResourceViews(EShaderStage::Pixel, ShaderResourceViews, 6, 0);

        SamplerState* Sampler = Command.Material->GetMaterialSampler();
        CmdList.BindSamplerStates(EShaderStage::Pixel, &Sampler, 1, 0);

        CmdList.Bind32BitShaderConstants(EShaderStage::Vertex, &TransformPerObject, 32);

        CmdList.DrawIndexedInstanced(Command.IndexCount, 1, 0, 0, 0);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End GeometryPass");
}

void DeferredRenderer::RenderDeferredTiledLightPass(CommandList& CmdList, const FrameResources& FrameResources, const SceneLightSetup& LightSetup)
{
    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin LightPass");

    TRACE_SCOPE("LightPass");

    if (GlobalDrawTileDebug.GetBool())
    {
        CmdList.BindComputePipelineState(TiledLightPassPSODebug.Get());
    }
    else
    {
        CmdList.BindComputePipelineState(TiledLightPassPSO.Get());
    }

    ShaderResourceView* ShaderResourceViews[] =
    {
        FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX]->GetShaderResourceView(),
        FrameResources.GBuffer[GBUFFER_NORMAL_INDEX]->GetShaderResourceView(),
        FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX]->GetShaderResourceView(),
        FrameResources.GBuffer[GBUFFER_DEPTH_INDEX]->GetShaderResourceView(),
        nullptr,//FrameResources.ReflectionTexture->GetShaderResourceView(),
        LightSetup.IrradianceMap->GetShaderResourceView(),
        LightSetup.SpecularIrradianceMap->GetShaderResourceView(),
        FrameResources.IntegrationLUT->GetShaderResourceView(),
        LightSetup.DirLightShadowMaps->GetShaderResourceView(),
        LightSetup.PointLightShadowMaps->GetShaderResourceView(),
        FrameResources.SSAOBuffer->GetShaderResourceView(),
    };

    CmdList.BindShaderResourceViews(EShaderStage::Compute, ShaderResourceViews, 11, 0);

    ConstantBuffer* ConstantBuffers[] =
    {
        FrameResources.CameraBuffer.Get(),
        LightSetup.PointLightBuffer.Get(),
        LightSetup.DirectionalLightBuffer.Get()
    };

    CmdList.BindConstantBuffers(EShaderStage::Compute, ConstantBuffers, 3, 0);

    SamplerState* SamplerStates[] =
    {
        FrameResources.GBufferSampler.Get(),
        FrameResources.IntegrationLUTSampler.Get(),
        FrameResources.IrradianceSampler.Get(),
        FrameResources.PointShadowSampler.Get(),
        FrameResources.DirectionalShadowSampler.Get()
    };

    CmdList.BindSamplerStates(EShaderStage::Compute, SamplerStates, 5, 0);

    UnorderedAccessView* FinalTargetUAV = FrameResources.FinalTarget->GetUnorderedAccessView();
    CmdList.BindUnorderedAccessViews(EShaderStage::Compute, &FinalTargetUAV, 1, 0);

    struct LightPassSettings
    {
        Int32 NumPointLights;
        Int32 NumSkyLightMips;
        Int32 ScreenWidth;
        Int32 ScreenHeight;
    } Settings;

    Settings.NumPointLights  = 4;
    Settings.NumSkyLightMips = LightSetup.SpecularIrradianceMap->GetNumMips();
    Settings.ScreenWidth     = FrameResources.FinalTarget->GetWidth();
    Settings.ScreenHeight    = FrameResources.FinalTarget->GetHeight();

    CmdList.Bind32BitShaderConstants(EShaderStage::Compute, &Settings, 4);

    constexpr UInt32 ThreadCount = 16;
    const UInt32 WorkGroupWidth  = Math::DivideByMultiple<UInt32>(Settings.ScreenWidth, ThreadCount);
    const UInt32 WorkGroupHeight = Math::DivideByMultiple<UInt32>(Settings.ScreenHeight, ThreadCount);
    CmdList.Dispatch(WorkGroupWidth, WorkGroupHeight, 1);

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End LightPass");
}

Bool DeferredRenderer::ResizeResources(FrameResources& FrameResources)
{
    return CreateGBuffer(FrameResources);
}

Bool DeferredRenderer::CreateGBuffer(FrameResources& FrameResources)
{
    const UInt32 Width  = FrameResources.MainWindowViewport->GetWidth();
    const UInt32 Height = FrameResources.MainWindowViewport->GetHeight();
    const UInt32 Usage  = TextureFlags_RenderTarget;

    // Albedo
    FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX] = RenderLayer::CreateTexture2D(
        FrameResources.AlbedoFormat, 
        Width, Height, 1, 1, Usage, 
        EResourceState::Common, 
        nullptr);
    if (FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX])
    {
        FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX]->SetName("GBuffer Albedo");
    }
    else
    {
        return false;
    }

    // Normal
    FrameResources.GBuffer[GBUFFER_NORMAL_INDEX] = RenderLayer::CreateTexture2D(
        FrameResources.NormalFormat, 
        Width, Height, 1, 1, Usage, 
        EResourceState::Common, 
        nullptr);
    if (FrameResources.GBuffer[GBUFFER_NORMAL_INDEX])
    {
        FrameResources.GBuffer[GBUFFER_NORMAL_INDEX]->SetName("GBuffer Normal");
    }
    else
    {
        return false;
    }

    // Material Properties
    FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX] = RenderLayer::CreateTexture2D(
        FrameResources.MaterialFormat, 
        Width, Height, 1, 1, Usage, 
        EResourceState::Common,
        nullptr);
    if (FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX])
    {
        FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX]->SetName("GBuffer Material");
    }
    else
    {
        return false;
    }

    // DepthStencil
    const UInt32 UsageDS = TextureFlag_DSV | TextureFlag_SRV;
    FrameResources.GBuffer[GBUFFER_DEPTH_INDEX] = RenderLayer::CreateTexture2D(
        FrameResources.DepthBufferFormat, 
        Width, Height, 1, 1, UsageDS, 
        EResourceState::Common, 
        nullptr);
    if (FrameResources.GBuffer[GBUFFER_DEPTH_INDEX])
    {
        FrameResources.GBuffer[GBUFFER_DEPTH_INDEX]->SetName("GBuffer DepthStencil");
    }
    else
    {
        return false;
    }

    // View Normal
    FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX] = RenderLayer::CreateTexture2D(
        FrameResources.ViewNormalFormat, 
        Width, Height, 1, 1, Usage, 
        EResourceState::Common, 
        nullptr);
    if (FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX])
    {
        FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX]->SetName("GBuffer ViewNormal");
    }
    else
    {
        return false;
    }

    // Final Image
    FrameResources.FinalTarget = RenderLayer::CreateTexture2D(
        FrameResources.FinalTargetFormat, 
        Width, Height, 1, 1, 
        Usage | TextureFlag_UAV, 
        EResourceState::Common, 
        nullptr);
    if (FrameResources.FinalTarget)
    {
        FrameResources.FinalTarget->SetName("Final Target");
    }
    else
    {
        return false;
    }

    return true;
}
