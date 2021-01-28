#include "SSAOSceneRenderPass.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/ShaderCompiler.h"
#include "RenderLayer/Viewport.h"

Bool SSAOSceneRenderPass::Init(SharedRenderPassResources& FrameResources)
{
    if (!CreateRenderTarget(FrameResources))
    {
        return false;
    }

    TArray<UInt8> ShaderCode;
    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/SSAO.hlsl",
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
        XMVECTOR XmSample = XMVectorSet(
            RandomFloats(Generator) * 2.0f - 1.0f,
            RandomFloats(Generator) * 2.0f - 1.0f,
            RandomFloats(Generator),
            0.0f);

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

    SSAONoiseTex = RenderLayer::CreateTexture2D(
        nullptr,
        EFormat::Format_R16G16B16A16_Float,
        TextureUsage_SRV,
        4,
        4,
        1, 1);
    if (!SSAONoiseTex)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        SSAONoiseTex->SetName("SSAO Noise Texture");
    }

    SSAONoiseSRV = RenderLayer::CreateShaderResourceView(
        SSAONoiseTex.Get(),
        EFormat::Format_R16G16B16A16_Float,
        0, 1);
    if (!SSAONoiseSRV)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        SSAOBufferSRV->SetName("SSAO Noise Texture SRV");
    }

    CommandList CmdList;
    CmdList.Begin();
    CmdList.TransitionTexture(
        SSAOBuffer.Get(),
        EResourceState::ResourceState_Common,
        EResourceState::ResourceState_PixelShaderResource);

    CmdList.TransitionTexture(
        SSAONoiseTex.Get(),
        EResourceState::ResourceState_Common,
        EResourceState::ResourceState_CopyDest);

    CmdList.UpdateTexture2D(
        SSAONoiseTex.Get(),
        4, 4, 0,
        SSAONoise.Data());

    CmdList.TransitionTexture(
        SSAONoiseTex.Get(),
        EResourceState::ResourceState_CopyDest,
        EResourceState::ResourceState_NonPixelShaderResource);

    CmdList.End();
    GlobalCmdListExecutor.ExecuteCommandList(CmdList);

    const UInt32 Stride = sizeof(XMFLOAT3);
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

    SSAOSamplesSRV = RenderLayer::CreateShaderResourceView(
        SSAOSamples.Get(),
        0, SSAOKernel.Size(),
        sizeof(XMFLOAT3));
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

    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/Blur.hlsl",
        "Main",
        &Defines,
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

Bool SSAOSceneRenderPass::ResizeResources(SharedRenderPassResources& FrameResources)
{
    return CreateRenderTarget(FrameResources);
}

void SSAOSceneRenderPass::Render(
    CommandList& CmdList, 
    SharedRenderPassResources& FrameResources,
    const Scene& Scene)
{
}

Bool SSAOSceneRenderPass::CreateRenderTarget(SharedRenderPassResources& FrameResources)
{
    const UInt32 Width  = FrameResources.MainWindowViewport->GetWidth();
    const UInt32 Height = FrameResources.MainWindowViewport->GetHeight();
    const UInt32 Usage  = TextureUsage_Default | TextureUsage_RWTexture;

    SSAOBuffer = RenderLayer::CreateTexture2D(
        nullptr,
        FrameResources.SSAOBufferFormat,
        Usage,
        Width,
        Height,
        1, 1);
    if (!SSAOBuffer)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        SSAOBuffer->SetName("SSAO Buffer");
    }

    SSAOBufferUAV = RenderLayer::CreateUnorderedAccessView(
        SSAOBuffer.Get(),
        FrameResources.SSAOBufferFormat,
        0);
    if (!SSAOBufferUAV)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        SSAOBufferUAV->SetName("SSAO Buffer UAV");
    }

    SSAOBufferSRV = RenderLayer::CreateShaderResourceView(
        SSAOBuffer.Get(),
        FrameResources.SSAOBufferFormat,
        0, 1);
    if (!SSAOBufferSRV)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        SSAOBufferSRV->SetName("SSAO Buffer SRV");
    }

    return true;
}
