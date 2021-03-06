#include "DeferredRenderer.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/ShaderCompiler.h"

#include "Rendering/MeshDrawCommand.h"
#include "Rendering/Resources/Mesh.h"
#include "Rendering/Resources/Material.h"

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

        FrameResources.GBufferSampler = CreateSamplerState(CreateInfo);
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

        BaseVertexShader = CreateVertexShader(ShaderCode);
        if (!BaseVertexShader)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            BaseVertexShader->SetName("GeometryPass VertexShader");
        }

        if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/GeometryPass.hlsl", "PSMain", &Defines, EShaderStage::Pixel, EShaderModel::SM_6_0, ShaderCode))
        {
            Debug::DebugBreak();
            return false;
        }

        BasePixelShader = CreatePixelShader(ShaderCode);
        if (!BasePixelShader)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            BasePixelShader->SetName("GeometryPass PixelShader");
        }

        DepthStencilStateCreateInfo DepthStencilStateInfo;
        DepthStencilStateInfo.DepthFunc      = EComparisonFunc::LessEqual;
        DepthStencilStateInfo.DepthEnable    = true;
        DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::All;

        TRef<DepthStencilState> GeometryDepthStencilState = CreateDepthStencilState(DepthStencilStateInfo);
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

        TRef<RasterizerState> GeometryRasterizerState = CreateRasterizerState(RasterizerStateInfo);
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

        TRef<BlendState> BlendState = CreateBlendState(BlendStateInfo);
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
        PipelineStateInfo.ShaderState.VertexShader               = BaseVertexShader.Get();
        PipelineStateInfo.ShaderState.PixelShader                = BasePixelShader.Get();
        PipelineStateInfo.PipelineFormats.DepthStencilFormat     = FrameResources.DepthBufferFormat;
        PipelineStateInfo.PipelineFormats.RenderTargetFormats[0] = EFormat::R8G8B8A8_Unorm;
        PipelineStateInfo.PipelineFormats.RenderTargetFormats[1] = FrameResources.NormalFormat;
        PipelineStateInfo.PipelineFormats.RenderTargetFormats[2] = EFormat::R8G8B8A8_Unorm;
        PipelineStateInfo.PipelineFormats.RenderTargetFormats[3] = FrameResources.ViewNormalFormat;
        PipelineStateInfo.PipelineFormats.NumRenderTargets       = 4;

        PipelineState = CreateGraphicsPipelineState(PipelineStateInfo);
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

        PrePassVertexShader = CreateVertexShader(ShaderCode);
        if (!PrePassVertexShader)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            PrePassVertexShader->SetName("PrePass VertexShader");
        }

        DepthStencilStateCreateInfo DepthStencilStateInfo;
        DepthStencilStateInfo.DepthFunc      = EComparisonFunc::Less;
        DepthStencilStateInfo.DepthEnable    = true;
        DepthStencilStateInfo.DepthWriteMask = EDepthWriteMask::All;

        TRef<DepthStencilState> DepthStencilState = CreateDepthStencilState(DepthStencilStateInfo);
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

        TRef<RasterizerState> RasterizerState = CreateRasterizerState(RasterizerStateInfo);
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

        TRef<BlendState> BlendState = CreateBlendState(BlendStateInfo);
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
        PipelineStateInfo.ShaderState.VertexShader           = PrePassVertexShader.Get();
        PipelineStateInfo.PipelineFormats.DepthStencilFormat = FrameResources.DepthBufferFormat;

        PrePassPipelineState = CreateGraphicsPipelineState(PipelineStateInfo);
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
    if (!UAVSupportsFormat(LUTFormat))
    {
        LOG_ERROR("[Renderer]: R16G16_Float is not supported for UAVs");

        Debug::DebugBreak();
        return false;
    }

    TRef<Texture2D> StagingTexture = CreateTexture2D(LUTFormat, LUTSize, LUTSize, 1, 1, TextureFlag_UAV, EResourceState::Common, nullptr);
    if (!StagingTexture)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        StagingTexture->SetName("Staging IntegrationLUT");
    }

    FrameResources.IntegrationLUT = CreateTexture2D(LUTFormat, LUTSize, LUTSize, 1, 1, TextureFlag_SRV, EResourceState::Common, nullptr);
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

    FrameResources.IntegrationLUTSampler = CreateSamplerState(CreateInfo);
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

    TRef<ComputeShader> CShader = CreateComputeShader(ShaderCode);
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

    TRef<ComputePipelineState> BRDF_PipelineState = CreateComputePipelineState(PipelineStateInfo);
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

    CmdList.SetComputePipelineState(BRDF_PipelineState.Get());

    UnorderedAccessView* StagingUAV = StagingTexture->GetUnorderedAccessView();
    CmdList.SetUnorderedAccessView(CShader.Get(), StagingUAV, 0);

    constexpr UInt32 ThreadCount = 16;
    const UInt32 DispatchWidth  = Math::DivideByMultiple(LUTSize, ThreadCount);
    const UInt32 DispatchHeight = Math::DivideByMultiple(LUTSize, ThreadCount);
    CmdList.Dispatch(DispatchWidth, DispatchHeight, 1);

    CmdList.UnorderedAccessTextureBarrier(StagingTexture.Get());

    CmdList.TransitionTexture(StagingTexture.Get(), EResourceState::UnorderedAccess, EResourceState::CopySource);
    CmdList.TransitionTexture(FrameResources.IntegrationLUT.Get(), EResourceState::Common, EResourceState::CopyDest);

    CmdList.CopyTexture(FrameResources.IntegrationLUT.Get(), StagingTexture.Get());

    CmdList.TransitionTexture(FrameResources.IntegrationLUT.Get(), EResourceState::CopyDest, EResourceState::PixelShaderResource);

    CmdList.End();
    gCmdListExecutor.ExecuteCommandList(CmdList);

    {
        if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/DeferredLightPass.hlsl", "Main", nullptr, EShaderStage::Compute, EShaderModel::SM_6_0, ShaderCode))
        {
            Debug::DebugBreak();
            return false;
        }

        TiledLightShader = CreateComputeShader(ShaderCode);
        if (!TiledLightShader)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            TiledLightShader->SetName("DeferredLightPass Shader");
        }

        ComputePipelineStateCreateInfo DeferredLightPassCreateInfo;
        DeferredLightPassCreateInfo.Shader = TiledLightShader.Get();

        TiledLightPassPSO = CreateComputePipelineState(DeferredLightPassCreateInfo);
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

        TiledLightDebugShader = CreateComputeShader(ShaderCode);
        if (!TiledLightDebugShader)
        {
            Debug::DebugBreak();
            return false;
        }
        else
        {
            TiledLightDebugShader->SetName("DeferredLightPass Debug Shader");
        }

        ComputePipelineStateCreateInfo DeferredLightPassCreateInfo;
        DeferredLightPassCreateInfo.Shader = TiledLightDebugShader.Get();

        TiledLightPassPSODebug = CreateComputePipelineState(DeferredLightPassCreateInfo);
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
    BaseVertexShader.Reset();
    BasePixelShader.Reset();
    PrePassVertexShader.Reset();
    TiledLightPassPSODebug.Reset();
    TiledLightShader.Reset();
    TiledLightDebugShader.Reset();
}

void DeferredRenderer::RenderPrePass(CommandList& CmdList, const FrameResources& FrameResources)
{
    const Float RenderWidth  = Float(FrameResources.MainWindowViewport->GetWidth());
    const Float RenderHeight = Float(FrameResources.MainWindowViewport->GetHeight());

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin PrePass");

    TRACE_SCOPE("PrePass");

    CmdList.SetViewport(RenderWidth, RenderHeight, 0.0f, 1.0f, 0.0f, 0.0f);
    CmdList.SetScissorRect(RenderWidth, RenderHeight, 0, 0);

    struct PerObject
    {
        XMFLOAT4X4 Matrix;
    } PerObjectBuffer;

    CmdList.SetRenderTargets(nullptr, 0, FrameResources.GBuffer[GBUFFER_DEPTH_INDEX]->GetDepthStencilView());

    CmdList.SetGraphicsPipelineState(PrePassPipelineState.Get());

    CmdList.SetConstantBuffer(PrePassVertexShader.Get(), FrameResources.CameraBuffer.Get(), 0);

    for (const MeshDrawCommand& Command : FrameResources.DeferredVisibleCommands)
    {
        if (!Command.Material->HasHeightMap())
        {
            CmdList.SetVertexBuffers(&Command.VertexBuffer, 1, 0);
            CmdList.SetIndexBuffer(Command.IndexBuffer);

            PerObjectBuffer.Matrix = Command.CurrentActor->GetTransform().GetMatrix();

            CmdList.Set32BitShaderConstants(PrePassVertexShader.Get(), &PerObjectBuffer, 16);

            CmdList.DrawIndexedInstanced(Command.IndexBuffer->GetNumIndicies(), 1, 0, 0, 0);
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

    CmdList.SetViewport(RenderWidth, RenderHeight, 0.0f, 1.0f, 0.0f, 0.0f);
    CmdList.SetScissorRect(RenderWidth, RenderHeight, 0, 0);

    RenderTargetView* RenderTargets[] =
    {
        FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX]->GetRenderTargetView(),
        FrameResources.GBuffer[GBUFFER_NORMAL_INDEX]->GetRenderTargetView(),
        FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX]->GetRenderTargetView(),
        FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX]->GetRenderTargetView(),
    };
    CmdList.SetRenderTargets(RenderTargets, 4, FrameResources.GBuffer[GBUFFER_DEPTH_INDEX]->GetDepthStencilView());

    // Setup Pipeline
    CmdList.SetGraphicsPipelineState(PipelineState.Get());

    struct TransformBuffer
    {
        XMFLOAT4X4 Transform;
        XMFLOAT4X4 TransformInv;
    } TransformPerObject;

    for (const MeshDrawCommand& Command : FrameResources.DeferredVisibleCommands)
    {
        CmdList.SetVertexBuffers(&Command.VertexBuffer, 1, 0);
        CmdList.SetIndexBuffer(Command.IndexBuffer);

        if (Command.Material->IsBufferDirty())
        {
            Command.Material->BuildBuffer(CmdList);
        }

        CmdList.SetConstantBuffer(BaseVertexShader.Get(), FrameResources.CameraBuffer.Get(), 0);

        ConstantBuffer* MaterialBuffer = Command.Material->GetMaterialBuffer();
        CmdList.SetConstantBuffer(BasePixelShader.Get(), MaterialBuffer, 0);

        TransformPerObject.Transform    = Command.CurrentActor->GetTransform().GetMatrix();
        TransformPerObject.TransformInv = Command.CurrentActor->GetTransform().GetMatrixInverse();

        ShaderResourceView* const* ShaderResourceViews = Command.Material->GetShaderResourceViews();
        CmdList.SetShaderResourceView(BasePixelShader.Get(), ShaderResourceViews[0], 0);
        CmdList.SetShaderResourceView(BasePixelShader.Get(), ShaderResourceViews[1], 1);
        CmdList.SetShaderResourceView(BasePixelShader.Get(), ShaderResourceViews[2], 2);
        CmdList.SetShaderResourceView(BasePixelShader.Get(), ShaderResourceViews[3], 3);
        CmdList.SetShaderResourceView(BasePixelShader.Get(), ShaderResourceViews[4], 4);
        CmdList.SetShaderResourceView(BasePixelShader.Get(), ShaderResourceViews[5], 5);

        SamplerState* Sampler = Command.Material->GetMaterialSampler();
        CmdList.SetSamplerState(BasePixelShader.Get(), Sampler, 0);

        CmdList.Set32BitShaderConstants(BaseVertexShader.Get(), &TransformPerObject, 32);

        CmdList.DrawIndexedInstanced(Command.IndexBuffer->GetNumIndicies(), 1, 0, 0, 0);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End GeometryPass");
}

void DeferredRenderer::RenderDeferredTiledLightPass(CommandList& CmdList, const FrameResources& FrameResources, const LightSetup& LightSetup)
{
    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin LightPass");

    TRACE_SCOPE("LightPass");

    ComputeShader* LightPassShader = nullptr;
    if (GlobalDrawTileDebug.GetBool())
    {
        LightPassShader = TiledLightDebugShader.Get();
        CmdList.SetComputePipelineState(TiledLightPassPSODebug.Get());
    }
    else
    {
        LightPassShader = TiledLightShader.Get();
        CmdList.SetComputePipelineState(TiledLightPassPSO.Get());
    }

    CmdList.SetShaderResourceView(LightPassShader, FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX]->GetShaderResourceView(), 0);
    CmdList.SetShaderResourceView(LightPassShader, FrameResources.GBuffer[GBUFFER_NORMAL_INDEX]->GetShaderResourceView(), 1);
    CmdList.SetShaderResourceView(LightPassShader, FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX]->GetShaderResourceView(), 2);
    CmdList.SetShaderResourceView(LightPassShader, FrameResources.GBuffer[GBUFFER_DEPTH_INDEX]->GetShaderResourceView(), 3);

    if (IsRayTracingSupported())
    {
        CmdList.SetSamplerState(LightPassShader, FrameResources.PointShadowSampler.Get(), 0);
        CmdList.SetSamplerState(LightPassShader, FrameResources.DirectionalShadowSampler.Get(), 1);

        CmdList.SetShaderResourceView(LightPassShader, FrameResources.RTReflections->GetShaderResourceView(), 4);
        CmdList.SetShaderResourceView(LightPassShader, LightSetup.DirLightShadowMaps->GetShaderResourceView(), 5);
        CmdList.SetShaderResourceView(LightPassShader, LightSetup.PointLightShadowMaps->GetShaderResourceView(), 6);
        CmdList.SetShaderResourceView(LightPassShader, FrameResources.SSAOBuffer->GetShaderResourceView(), 7);
    }
    else
    {
        CmdList.SetSamplerState(LightPassShader, FrameResources.IntegrationLUTSampler.Get(), 0);
        CmdList.SetSamplerState(LightPassShader, FrameResources.IrradianceSampler.Get(), 1);
        CmdList.SetSamplerState(LightPassShader, FrameResources.PointShadowSampler.Get(), 2);
        CmdList.SetSamplerState(LightPassShader, FrameResources.DirectionalShadowSampler.Get(), 3);

        CmdList.SetShaderResourceView(LightPassShader, LightSetup.IrradianceMap->GetShaderResourceView(), 4);
        CmdList.SetShaderResourceView(LightPassShader, LightSetup.SpecularIrradianceMap->GetShaderResourceView(), 5);
        CmdList.SetShaderResourceView(LightPassShader, FrameResources.IntegrationLUT->GetShaderResourceView(), 6);
        CmdList.SetShaderResourceView(LightPassShader, LightSetup.DirLightShadowMaps->GetShaderResourceView(), 7);
        CmdList.SetShaderResourceView(LightPassShader, LightSetup.PointLightShadowMaps->GetShaderResourceView(), 8);
        CmdList.SetShaderResourceView(LightPassShader, FrameResources.SSAOBuffer->GetShaderResourceView(), 9);
    }

    CmdList.SetConstantBuffer(LightPassShader, FrameResources.CameraBuffer.Get(), 0);
    CmdList.SetConstantBuffer(LightPassShader, LightSetup.PointLightsBuffer.Get(), 1);
    CmdList.SetConstantBuffer(LightPassShader, LightSetup.PointLightsPosRadBuffer.Get(), 2);
    CmdList.SetConstantBuffer(LightPassShader, LightSetup.ShadowCastingPointLightsBuffer.Get(), 3);
    CmdList.SetConstantBuffer(LightPassShader, LightSetup.ShadowCastingPointLightsPosRadBuffer.Get(), 4);
    CmdList.SetConstantBuffer(LightPassShader, LightSetup.DirectionalLightsBuffer.Get(), 5);


    UnorderedAccessView* FinalTargetUAV = FrameResources.FinalTarget->GetUnorderedAccessView();
    CmdList.SetUnorderedAccessView(LightPassShader, FinalTargetUAV, 0);

    struct LightPassSettings
    {
        Int32 NumPointLights;
        Int32 NumShadowCastingPointLights;
        Int32 NumSkyLightMips;
        Int32 ScreenWidth;
        Int32 ScreenHeight;
    } Settings;

    Settings.NumShadowCastingPointLights = LightSetup.ShadowCastingPointLightsData.Size();
    Settings.NumPointLights  = LightSetup.PointLightsData.Size();
    Settings.NumSkyLightMips = LightSetup.SpecularIrradianceMap->GetNumMips();
    Settings.ScreenWidth     = FrameResources.FinalTarget->GetWidth();
    Settings.ScreenHeight    = FrameResources.FinalTarget->GetHeight();

    CmdList.Set32BitShaderConstants(LightPassShader, &Settings, 5);

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
    FrameResources.GBuffer[GBUFFER_ALBEDO_INDEX] = CreateTexture2D(
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
    FrameResources.GBuffer[GBUFFER_NORMAL_INDEX] = CreateTexture2D(
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
    FrameResources.GBuffer[GBUFFER_MATERIAL_INDEX] = CreateTexture2D(
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
    FrameResources.GBuffer[GBUFFER_DEPTH_INDEX] = CreateTexture2D(
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
    FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX] = CreateTexture2D(
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
    FrameResources.FinalTarget = CreateTexture2D(
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
