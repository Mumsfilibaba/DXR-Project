#include "ScreenSpaceOcclusionRenderer.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/ShaderCompiler.h"

#include "Debug/Profiler.h"
#include "Debug/Console.h"

ConsoleVariable GlobalSSAORadius(EConsoleVariableType::Float);
ConsoleVariable GlobalSSAOBias(EConsoleVariableType::Float);
ConsoleVariable GlobalSSAOKernelSize(EConsoleVariableType::Int);

Bool ScreenSpaceOcclusionRenderer::Init(FrameResources& FrameResources)
{
    INIT_CONSOLE_VARIABLE("r.SSAOKernelSize", GlobalSSAOKernelSize);
    GlobalSSAOKernelSize.SetInt(16);

    INIT_CONSOLE_VARIABLE("r.SSAOBias", GlobalSSAOBias);
    GlobalSSAOBias.SetFloat(0.03f);

    INIT_CONSOLE_VARIABLE("r.SSAORadius", GlobalSSAORadius);
    GlobalSSAORadius.SetFloat(0.3f);

    if (!CreateRenderTarget(FrameResources))
    {
        return false;
    }

    TArray<UInt8> ShaderCode;
    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/SSAO.hlsl",
        "Main",
        nullptr,
        EShaderStage::Compute,
        EShaderModel::SM_6_0,
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
        CShader->SetName("SSAO Shader");
    }

    ComputePipelineStateCreateInfo PipelineStateInfo;
    PipelineStateInfo.Shader = CShader.Get();

    PipelineState = RenderLayer::CreateComputePipelineState(PipelineStateInfo);
    if (!PipelineState)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        PipelineState->SetName("SSAO PipelineState");
    }

    // Generate SSAO Kernel
    std::uniform_real_distribution<Float> RandomFloats(0.0f, 1.0f);
    std::default_random_engine Generator;

    XMVECTOR Normal = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

    TArray<XMFLOAT3> SSAOKernel;
    for (UInt32 i = 0; i < 256 && SSAOKernel.Size() < 64; ++i)
    {
        XMVECTOR XmSample = XMVectorSet(RandomFloats(Generator) * 2.0f - 1.0f, RandomFloats(Generator) * 2.0f - 1.0f, RandomFloats(Generator), 0.0f);

        Float Scale = RandomFloats(Generator);
        XmSample = XMVector3Normalize(XmSample);
        XmSample = XMVectorScale(XmSample, Scale);

        Float Dot = XMVectorGetX(XMVector3Dot(XmSample, Normal));
        if (Math::Abs(Dot) > 0.85f)
        {
            continue;
        }

        Scale = Float(i) / 64.0f;
        Scale = Math::Lerp(0.1f, 1.0f, Scale * Scale);
        XmSample = XMVectorScale(XmSample, Scale);

        XMFLOAT3 Sample;
        XMStoreFloat3(&Sample, XmSample);
        SSAOKernel.EmplaceBack(Sample);
    }

    // Generate noise
    TArray<Float16> SSAONoise;
    for (UInt32 i = 0; i < 16; i++)
    {
        const Float x = RandomFloats(Generator) * 2.0f - 1.0f;
        const Float y = RandomFloats(Generator) * 2.0f - 1.0f;
        SSAONoise.EmplaceBack(x);
        SSAONoise.EmplaceBack(y);
        SSAONoise.EmplaceBack(0.0f);
        SSAONoise.EmplaceBack(0.0f);
    }

    SSAONoiseTex = RenderLayer::CreateTexture2D(EFormat::R16G16B16A16_Float, 4, 4, 1, 1, TextureUsage_SRV, EResourceState::NonPixelShaderResource, nullptr);
    if (!SSAONoiseTex)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        SSAONoiseTex->SetName("SSAO Noise Texture");
    }

    SSAONoiseSRV = RenderLayer::CreateShaderResourceView(ShaderResourceViewCreateInfo(SSAONoiseTex.Get(), EFormat::R16G16B16A16_Float, 0, 1, 0.0f));
    if (!SSAONoiseSRV)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        FrameResources.SSAOBufferSRV->SetName("SSAO Noise Texture SRV");
    }

    CommandList CmdList;
    CmdList.Begin();

    CmdList.TransitionTexture(FrameResources.SSAOBuffer.Get(), EResourceState::Common, EResourceState::NonPixelShaderResource);
    CmdList.TransitionTexture(SSAONoiseTex.Get(), EResourceState::Common, EResourceState::CopyDest);

    CmdList.UpdateTexture2D(SSAONoiseTex.Get(), 4, 4, 0, SSAONoise.Data());

    CmdList.TransitionTexture(SSAONoiseTex.Get(), EResourceState::CopyDest, EResourceState::NonPixelShaderResource);

    CmdList.End();
    gCmdListExecutor.ExecuteCommandList(CmdList);

    const UInt32 Stride      = sizeof(XMFLOAT3);
    const UInt32 SizeInBytes = Stride * SSAOKernel.Size();
    ResourceData SSAOSampleData(SSAOKernel.Data());
    SSAOSamples = RenderLayer::CreateStructuredBuffer(
        &SSAOSampleData,
        SizeInBytes,
        Stride,
        BufferUsage_SRV | BufferUsage_Default);
    if (!SSAOSamples)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        SSAOSamples->SetName("SSAO Samples");
    }

    SSAOSamplesSRV = RenderLayer::CreateShaderResourceView(ShaderResourceViewCreateInfo(SSAOSamples.Get()));
    if (!SSAOSamplesSRV)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        SSAOSamplesSRV->SetName("SSAO Samples SRV");
    }

    TArray<ShaderDefine> Defines;
    Defines.EmplaceBack("HORIZONTAL_PASS", "1");

    // Load shader
    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/Blur.hlsl",
        "Main",
        &Defines,
        EShaderStage::Compute,
        EShaderModel::SM_6_0,
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
        CShader->SetName("SSAO Horizontal Blur Shader");
    }

    ComputePipelineStateCreateInfo PSOProperties;
    PSOProperties.Shader = CShader.Get();

    BlurHorizontalPSO = RenderLayer::CreateComputePipelineState(PSOProperties);
    if (!BlurHorizontalPSO)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        BlurHorizontalPSO->SetName("SSAO Horizontal Blur PSO");
    }

    Defines.Clear();
    Defines.EmplaceBack("VERTICAL_PASS", "1");

    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/Blur.hlsl", "Main", &Defines, EShaderStage::Compute, EShaderModel::SM_6_0, ShaderCode))
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
        CShader->SetName("SSAO Vertcial Blur Shader");
    }

    PSOProperties.Shader = CShader.Get();

    BlurVerticalPSO = RenderLayer::CreateComputePipelineState(PSOProperties);
    if (!BlurVerticalPSO)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        BlurVerticalPSO->SetName("SSAO Vertical Blur PSO");
    }

    return true;
}

void ScreenSpaceOcclusionRenderer::Release()
{
    PipelineState.Reset();
    BlurHorizontalPSO.Reset();
    BlurVerticalPSO.Reset();
    SSAOSamples.Reset();
    SSAOSamplesSRV.Reset();
    SSAONoiseTex.Reset();
    SSAONoiseSRV.Reset();
}

Bool ScreenSpaceOcclusionRenderer::ResizeResources(FrameResources& FrameResources)
{
	return CreateRenderTarget(FrameResources);
}

void ScreenSpaceOcclusionRenderer::Render(CommandList& CmdList, const FrameResources& FrameResources)
{
    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin SSAO");

    TRACE_SCOPE("SSAO");

    struct SSAOSettings
    {
        XMFLOAT2 ScreenSize;
        XMFLOAT2 NoiseSize;
        Float    Radius;
        Float    Bias;
        Int32    KernelSize;
    } SSAOSettings;

    const UInt32 Width      = FrameResources.SSAOBuffer->GetWidth();
    const UInt32 Height     = FrameResources.SSAOBuffer->GetHeight();
    SSAOSettings.ScreenSize = XMFLOAT2(Float(Width), Float(Height));
    SSAOSettings.NoiseSize  = XMFLOAT2(4.0f, 4.0f);
    SSAOSettings.Radius     = GlobalSSAORadius.GetFloat();
    SSAOSettings.KernelSize = GlobalSSAOKernelSize.GetInt32();
    SSAOSettings.Bias       = GlobalSSAOBias.GetFloat();

    ShaderResourceView* ShaderResourceViews[] =
    {
        FrameResources.GBufferSRVs[GBUFFER_VIEW_NORMAL_INDEX].Get(),
        FrameResources.GBufferSRVs[GBUFFER_DEPTH_INDEX].Get(),
        SSAONoiseSRV.Get(),
        SSAOSamplesSRV.Get()
    };

    CmdList.BindComputePipelineState(PipelineState.Get());

    CmdList.BindShaderResourceViews(EShaderStage::Compute, ShaderResourceViews, 4, 0);
    CmdList.BindSamplerStates(EShaderStage::Compute, &FrameResources.GBufferSampler, 1, 0);
    CmdList.BindConstantBuffers(EShaderStage::Compute, &FrameResources.CameraBuffer, 1, 0);
    CmdList.BindUnorderedAccessViews(EShaderStage::Compute, &FrameResources.SSAOBufferUAV, 1, 0);
    CmdList.Bind32BitShaderConstants(EShaderStage::Compute, &SSAOSettings, 7);

    constexpr UInt32 ThreadCount = 16;
    const UInt32 DispatchWidth   = Math::DivideByMultiple<UInt32>(Width, ThreadCount);
    const UInt32 DispatchHeight  = Math::DivideByMultiple<UInt32>(Height, ThreadCount);
    CmdList.Dispatch(DispatchWidth, DispatchHeight, 1);

    CmdList.UnorderedAccessTextureBarrier(FrameResources.SSAOBuffer.Get());

    CmdList.BindComputePipelineState(BlurHorizontalPSO.Get());

    CmdList.Bind32BitShaderConstants(EShaderStage::Compute, &SSAOSettings.ScreenSize, 2);

    CmdList.Dispatch(DispatchWidth, DispatchHeight, 1);

    CmdList.UnorderedAccessTextureBarrier(FrameResources.SSAOBuffer.Get());

    CmdList.BindComputePipelineState(BlurVerticalPSO.Get());

    CmdList.Bind32BitShaderConstants(EShaderStage::Compute, &SSAOSettings.ScreenSize, 2);

    CmdList.Dispatch(DispatchWidth, DispatchHeight, 1);

    CmdList.UnorderedAccessTextureBarrier(FrameResources.SSAOBuffer.Get());

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End SSAO");
}

Bool ScreenSpaceOcclusionRenderer::CreateRenderTarget(FrameResources& FrameResources)
{
    const UInt32 Width  = FrameResources.MainWindowViewport->GetWidth();
    const UInt32 Height = FrameResources.MainWindowViewport->GetHeight();
    const UInt32 Usage  = TextureUsage_Default | TextureUsage_RWTexture;

    FrameResources.SSAOBuffer = RenderLayer::CreateTexture2D(
        nullptr,
        FrameResources.SSAOBufferFormat,
        Usage,
        Width,
        Height,
        1, 1);
    if (!FrameResources.SSAOBuffer)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        FrameResources.SSAOBuffer->SetName("SSAO Buffer");
    }

    FrameResources.SSAOBufferUAV = RenderLayer::CreateUnorderedAccessView(UnorderedAccessViewCreateInfo(FrameResources.SSAOBuffer.Get()));
    if (!FrameResources.SSAOBufferUAV)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        FrameResources.SSAOBufferUAV->SetName("SSAO Buffer UAV");
    }

    FrameResources.SSAOBufferSRV = RenderLayer::CreateShaderResourceView(ShaderResourceViewCreateInfo(FrameResources.SSAOBuffer.Get()));
    if (!FrameResources.SSAOBufferSRV)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        FrameResources.SSAOBufferSRV->SetName("SSAO Buffer SRV");
    }

    return true;
}
