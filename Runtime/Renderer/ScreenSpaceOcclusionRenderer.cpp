#include "ScreenSpaceOcclusionRenderer.h"
#include "Renderer.h"

#include "RHI/RHICoreInterface.h"
#include "RHI/RHIShaderCompiler.h"

#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"
#include "Core/Debug/Profiler/FrameProfiler.h"
#include "Core/Debug/Console/ConsoleManager.h"

// TODO: Remove and replace. There are better and easier implementations to do yourself
#include <random>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Console-variable

TAutoConsoleVariable<float> GSSAORadius("Renderer.SSAORadius", 0.4f);
TAutoConsoleVariable<float> GSSAOBias("Renderer.SSAOBias", 0.025f);
TAutoConsoleVariable<int32> GSSAOKernelSize("Renderer.SSAOKernelSize", 32);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FScreenSpaceOcclusionRenderer

bool FScreenSpaceOcclusionRenderer::Init(FFrameResources& FrameResources)
{
    if (!CreateRenderTarget(FrameResources))
    {
        return false;
    }

    TArray<uint8> ShaderCode;
    {
        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Compute);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/SSAO.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }
    }

    SSAOShader = RHICreateComputeShader(ShaderCode);
    if (!SSAOShader)
    {
        DEBUG_BREAK();
        return false;
    }

    {
        FRHIComputePipelineStateInitializer PSOInitializer;
        PSOInitializer.Shader = SSAOShader.Get();

        PipelineState = RHICreateComputePipelineState(PSOInitializer);
        if (!PipelineState)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            PipelineState->SetName("SSAO PipelineState");
        }
    }

    // Generate SSAO Kernel
    std::default_random_engine            Generator;
    std::uniform_real_distribution<float> RandomFloats(0.0f, 1.0f);

    FVector3 Normal = FVector3(0.0f, 0.0f, 1.0f);

    TArray<FVector3> SSAOKernel;
    for (uint32 i = 0; i < 512 && SSAOKernel.Size() < 64; i++)
    {
        FVector3 Sample = FVector3(RandomFloats(Generator) * 2.0f - 1.0f, RandomFloats(Generator) * 2.0f - 1.0f, RandomFloats(Generator));
        Sample.Normalize();

        float Dot = Sample.DotProduct(Normal);
        if (NMath::Abs(Dot) > 0.95f)
        {
            continue;
        }

        Sample *= RandomFloats(Generator);

        float Scale = float(i) / 64.0f;
        Scale = NMath::Lerp(0.1f, 1.0f, Scale * Scale);
        Sample *= Scale;

        SSAOKernel.Emplace(Sample);
    }

    // Generate noise
    TArray<FFloat16> SSAONoise;
    for (uint32 i = 0; i < 16; i++)
    {
        const float x = RandomFloats(Generator) * 2.0f - 1.0f;
        const float y = RandomFloats(Generator) * 2.0f - 1.0f;
        SSAONoise.Emplace(x);
        SSAONoise.Emplace(y);
        SSAONoise.Emplace(0.0f);
        SSAONoise.Emplace(0.0f);
    }

    FRHITexture2DInitializer SSAONoiseInitializer(
        EFormat::R16G16B16A16_Float, 
        4,
        4,
        1,
        1,
        ETextureUsageFlags::AllowSRV,
        EResourceAccess::NonPixelShaderResource);
    SSAONoiseTex = RHICreateTexture2D(SSAONoiseInitializer);
    if (!SSAONoiseTex)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        SSAONoiseTex->SetName("SSAO Noise Texture");
    }

    FRHICommandList CmdList;

    CmdList.TransitionTexture(
        FrameResources.SSAOBuffer.Get(),
        EResourceAccess::Common, 
        EResourceAccess::NonPixelShaderResource);
    CmdList.TransitionTexture(
        SSAONoiseTex.Get(),
        EResourceAccess::NonPixelShaderResource,
        EResourceAccess::CopyDest);

    CmdList.UpdateTexture2D(SSAONoiseTex.Get(), 4, 4, 0, SSAONoise.Data());

    CmdList.TransitionTexture(
        SSAONoiseTex.Get(),
        EResourceAccess::CopyDest,
        EResourceAccess::NonPixelShaderResource);

    GRHICommandExecutor.ExecuteCommandList(CmdList);

    FRHIBufferDataInitializer SSAOSampleData(SSAOKernel.Data(), SSAOKernel.SizeInBytes());
    
    FRHIGenericBufferInitializer SSAOSamplesInitializer(
        EBufferUsageFlags::AllowSRV | EBufferUsageFlags::Default,
        SSAOKernel.SizeInBytes(),
        sizeof(FVector3),
        EResourceAccess::Common,
        &SSAOSampleData);

    SSAOSamples = RHICreateGenericBuffer(SSAOSamplesInitializer);
    if (!SSAOSamples)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        SSAOSamples->SetName("SSAO Samples");
    }

    FRHIBufferSRVInitializer SRVInitializer(SSAOSamples.Get(), 0, SSAOKernel.Size());
    SSAOSamplesSRV = RHICreateShaderResourceView(SRVInitializer);
    if (!SSAOSamplesSRV)
    {
        DEBUG_BREAK();
        return false;
    }

    TArray<FShaderDefine> Defines;
    Defines.Emplace("HORIZONTAL_PASS", "1");

    {
        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Compute, Defines.CreateView());
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/Blur.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }
    }

    BlurHorizontalShader = RHICreateComputeShader(ShaderCode);
    if (!BlurHorizontalShader)
    {
        DEBUG_BREAK();
        return false;
    }

    {
        FRHIComputePipelineStateInitializer PSOInitializer;
        PSOInitializer.Shader = BlurHorizontalShader.Get();

        BlurHorizontalPSO = RHICreateComputePipelineState(PSOInitializer);
        if (!BlurHorizontalPSO)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            BlurHorizontalPSO->SetName("SSAO Horizontal Blur PSO");
        }
    }

    Defines.Clear();
    Defines.Emplace("VERTICAL_PASS", "1");

    {
        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Compute, Defines.CreateView());
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/Blur.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }
    }

    BlurVerticalShader = RHICreateComputeShader(ShaderCode);
    if (!BlurVerticalShader)
    {
        DEBUG_BREAK();
        return false;
    }

    {
        FRHIComputePipelineStateInitializer PSOInitializer;
        PSOInitializer.Shader = BlurVerticalShader.Get();

        BlurVerticalPSO = RHICreateComputePipelineState(PSOInitializer);
        if (!BlurVerticalPSO)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            BlurVerticalPSO->SetName("SSAO Vertical Blur PSO");
        }
    }

    return true;
}

void FScreenSpaceOcclusionRenderer::Release()
{
    PipelineState.Reset();
    BlurHorizontalPSO.Reset();
    BlurVerticalPSO.Reset();
    SSAOSamples.Reset();
    SSAOSamplesSRV.Reset();
    SSAONoiseTex.Reset();
    SSAOShader.Reset();
    BlurHorizontalShader.Reset();
    BlurVerticalShader.Reset();
}

bool FScreenSpaceOcclusionRenderer::ResizeResources(FFrameResources& FrameResources)
{
    return CreateRenderTarget(FrameResources);
}

void FScreenSpaceOcclusionRenderer::Render(FRHICommandList& CmdList, FFrameResources& FrameResources)
{
    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin SSAO");

    TRACE_SCOPE("SSAO");

    CmdList.SetComputePipelineState(PipelineState.Get());

    struct SSAOSettings
    {
        FVector2 ScreenSize;
        FVector2 NoiseSize;
        float    Radius;
        float    Bias;
        int32    KernelSize;
    } SSAOSettings;

    const uint32 Width  = FrameResources.SSAOBuffer->GetWidth();
    const uint32 Height = FrameResources.SSAOBuffer->GetHeight();
    SSAOSettings.ScreenSize = FVector2(float(Width), float(Height));
    SSAOSettings.NoiseSize  = FVector2(4.0f, 4.0f);
    SSAOSettings.Radius     = GSSAORadius.GetFloat();
    SSAOSettings.KernelSize = GSSAOKernelSize.GetInt();
    SSAOSettings.Bias       = GSSAOBias.GetFloat();

    CmdList.SetConstantBuffer(SSAOShader.Get(), FrameResources.CameraBuffer.Get(), 0);

    AddDebugTexture(
        MakeSharedRef<FRHIShaderResourceView>(SSAONoiseTex->GetShaderResourceView()),
        SSAONoiseTex,
        EResourceAccess::NonPixelShaderResource,
        EResourceAccess::NonPixelShaderResource);

    CmdList.SetShaderResourceView(SSAOShader.Get(), FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX]->GetShaderResourceView(), 0);
    CmdList.SetShaderResourceView(SSAOShader.Get(), FrameResources.GBuffer[GBUFFER_DEPTH_INDEX]->GetShaderResourceView(), 1);
    CmdList.SetShaderResourceView(SSAOShader.Get(), SSAONoiseTex->GetShaderResourceView(), 2);
    CmdList.SetShaderResourceView(SSAOShader.Get(), SSAOSamplesSRV.Get(), 3);

    CmdList.SetSamplerState(SSAOShader.Get(), FrameResources.GBufferSampler.Get(), 0);

    FRHIUnorderedAccessView* SSAOBufferUAV = FrameResources.SSAOBuffer->GetUnorderedAccessView();
    CmdList.SetUnorderedAccessView(SSAOShader.Get(), SSAOBufferUAV, 0);
    CmdList.Set32BitShaderConstants(SSAOShader.Get(), &SSAOSettings, 7);

    constexpr uint32 ThreadCount = 16;

    const uint32 DispatchWidth  = NMath::DivideByMultiple<uint32>(Width, ThreadCount);
    const uint32 DispatchHeight = NMath::DivideByMultiple<uint32>(Height, ThreadCount);
    CmdList.Dispatch(DispatchWidth, DispatchHeight, 1);

    CmdList.UnorderedAccessTextureBarrier(FrameResources.SSAOBuffer.Get());

    CmdList.SetComputePipelineState(BlurHorizontalPSO.Get());

    CmdList.Set32BitShaderConstants(BlurHorizontalShader.Get(), &SSAOSettings.ScreenSize, 2);

    CmdList.Dispatch(DispatchWidth, DispatchHeight, 1);

    CmdList.UnorderedAccessTextureBarrier(FrameResources.SSAOBuffer.Get());

    CmdList.SetComputePipelineState(BlurVerticalPSO.Get());

    CmdList.Set32BitShaderConstants(BlurVerticalShader.Get(), &SSAOSettings.ScreenSize, 2);

    CmdList.Dispatch(DispatchWidth, DispatchHeight, 1);

    CmdList.UnorderedAccessTextureBarrier(FrameResources.SSAOBuffer.Get());

    INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End SSAO");
}

bool FScreenSpaceOcclusionRenderer::CreateRenderTarget(FFrameResources& FrameResources)
{
    const ETextureUsageFlags Flags = ETextureUsageFlags::RWTexture;
    
    const uint32 Width  = FrameResources.MainWindowViewport->GetWidth();
    const uint32 Height = FrameResources.MainWindowViewport->GetHeight();

    FRHITexture2DInitializer SSAOBufferInitializer(FrameResources.SSAOBufferFormat, Width, Height, 1, 1, Flags, EResourceAccess::Common);
    FrameResources.SSAOBuffer = RHICreateTexture2D(SSAOBufferInitializer);
    if (!FrameResources.SSAOBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        FrameResources.SSAOBuffer->SetName("SSAO Buffer");
    }

    return true;
}
