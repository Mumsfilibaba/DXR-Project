#include "TiledDeferredLightSceneRenderPass.h"

#include "RenderLayer/ShaderCompiler.h"
#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/Viewport.h"

Bool TiledDeferredLightSceneRenderPass::Init(SharedRenderPassResources& FrameResources)
{
    if (!CreateRenderTarget(FrameResources))
    {
        return false;
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

    TSharedRef<UnorderedAccessView> StagingTextureUAV = RenderLayer::CreateUnorderedAccessView(StagingTexture.Get(), LUTFormat, 0);
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

    TArray<UInt8> ShaderCode;
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
    const UInt32 DispatchWidth   = Math::DivideByMultiple(LUTSize, ThreadCount);
    const UInt32 DispatchHeight  = Math::DivideByMultiple(LUTSize, ThreadCount);
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

    TArray<UInt8> ShaderCode;
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

    TSharedRef<ComputeShader> CShader = RenderLayer::CreateComputeShader(ShaderCode);
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

    PipelineState = RenderLayer::CreateComputePipelineState(DeferredLightPassCreateInfo);
    if (!PipelineState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PipelineState->SetName("DeferredLightPass PipelineState");
    }

    return true;
}

Bool TiledDeferredLightSceneRenderPass::ResizeResources(SharedRenderPassResources& FrameResources)
{
    return CreateRenderTarget(FrameResources);
}

void TiledDeferredLightSceneRenderPass::Render(
    CommandList& CmdList, 
    SharedRenderPassResources& FrameResources,
    const Scene& Scene)
{
    CmdList.TransitionTexture(
        FrameResources.IrradianceMap.Get(),
        EResourceState::ResourceState_PixelShaderResource,
        EResourceState::ResourceState_NonPixelShaderResource);

    CmdList.TransitionTexture(
        FrameResources.SpecularIrradianceMap.Get(),
        EResourceState::ResourceState_PixelShaderResource,
        EResourceState::ResourceState_NonPixelShaderResource);

    CmdList.TransitionTexture(
        FrameResources.IntegrationLUT.Get(),
        EResourceState::ResourceState_PixelShaderResource,
        EResourceState::ResourceState_NonPixelShaderResource);
}

Bool TiledDeferredLightSceneRenderPass::CreateRenderTarget(SharedRenderPassResources& FrameResources)
{
    const UInt32 Width  = FrameResources.MainWindowViewport->GetWidth();
    const UInt32 Height = FrameResources.MainWindowViewport->GetHeight();
    const UInt32 Usage  = TextureUsage_Default | TextureUsage_RenderTarget;

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
