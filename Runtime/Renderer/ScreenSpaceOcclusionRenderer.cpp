#include "ScreenSpaceOcclusionRenderer.h"
#include "Renderer.h"
#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/IntVector2.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "RHI/RHIInterface.h"
#include "RHI/RHIShaderCompiler.h"

// TODO: Remove and replace. There are better and easier implementations to do yourself
#include <random>

TAutoConsoleVariable<float> GSSAORadius(
    "Renderer.SSAO.Radius",
    "Specifies the radius of the Screen-Space Ray-Trace in SSAO",
    0.2f);
TAutoConsoleVariable<float> GSSAOBias(
    "Renderer.SSAO.Bias", 
    "Specifies the bias when testing the Screen-Space Rays against the depth-buffer",
    0.075f);
TAutoConsoleVariable<int32> GSSAOKernelSize(
    "Renderer.SSAO.KernelSize",
    "Specifies the number of samples for each pixel",
    16);

bool FScreenSpaceOcclusionRenderer::Init(FFrameResources& FrameResources)
{
    if (!CreateRenderTarget(FrameResources))
    {
        return false;
    }

    TArray<uint8> ShaderCode;
    {
        FRHIShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Compute);
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
        FRHIComputePipelineStateDesc PSOInitializer(SSAOShader.Get());
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

    TArray<FShaderDefine> Defines;
    Defines.Emplace("HORIZONTAL_PASS", "1");

    {
        FRHIShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Compute, Defines.CreateView());
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
        FRHIComputePipelineStateDesc PSOInitializer;
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
        FRHIShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Compute, Defines.CreateView());
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
        FRHIComputePipelineStateDesc PSOInitializer;
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
    SSAOShader.Reset();
    BlurHorizontalShader.Reset();
    BlurVerticalShader.Reset();
}

bool FScreenSpaceOcclusionRenderer::ResizeResources(FFrameResources& FrameResources)
{
    return CreateRenderTarget(FrameResources);
}

void FScreenSpaceOcclusionRenderer::Render(FRHICommandList& CommandList, FFrameResources& FrameResources)
{
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin SSAO");

    TRACE_SCOPE("SSAO");

    struct FSSAOSettings
    {
        FVector2    ScreenSize;
        FVector2    NoiseSize;

        FIntVector2 GBufferSize;

        float    Radius;
        float    Bias;
        int32    KernelSize;
    } SSAOSettings;

    const uint32 Width         = FrameResources.SSAOBuffer->GetWidth();
    const uint32 Height        = FrameResources.SSAOBuffer->GetHeight();
    const uint32 GBufferWidth  = FrameResources.GBuffer[GBufferIndex_Depth]->GetWidth();
    const uint32 GBufferHeight = FrameResources.GBuffer[GBufferIndex_Depth]->GetHeight();
    
    SSAOSettings.ScreenSize  = FVector2(float(Width), float(Height));
    SSAOSettings.NoiseSize   = FVector2(4.0f, 4.0f);
    SSAOSettings.GBufferSize = FIntVector2(GBufferWidth, GBufferHeight);
    SSAOSettings.Radius      = GSSAORadius.GetValue();
    SSAOSettings.KernelSize  = GSSAOKernelSize.GetValue();
    SSAOSettings.Bias        = GSSAOBias.GetValue();

    CommandList.SetComputePipelineState(PipelineState.Get());
    CommandList.SetConstantBuffer(SSAOShader.Get(), FrameResources.CameraBuffer.Get(), 0);

    CommandList.SetShaderResourceView(SSAOShader.Get(), FrameResources.GBuffer[GBufferIndex_ViewNormal]->GetShaderResourceView(), 0);
    CommandList.SetShaderResourceView(SSAOShader.Get(), FrameResources.GBuffer[GBufferIndex_Depth]->GetShaderResourceView(), 1);

    CommandList.SetSamplerState(SSAOShader.Get(), FrameResources.GBufferSampler.Get(), 0);

    FRHIUnorderedAccessView* SSAOBufferUAV = FrameResources.SSAOBuffer->GetUnorderedAccessView();
    CommandList.SetUnorderedAccessView(SSAOShader.Get(), SSAOBufferUAV, 0);
    CommandList.Set32BitShaderConstants(SSAOShader.Get(), &SSAOSettings, 9);

    constexpr uint32 ThreadCount = 16;
    const uint32 DispatchWidth   = NMath::DivideByMultiple<uint32>(Width, ThreadCount);
    const uint32 DispatchHeight  = NMath::DivideByMultiple<uint32>(Height, ThreadCount);

    // Actual SSAO tracing
    {
        GPU_TRACE_SCOPE(CommandList, "SSAO Tracing");

        CommandList.Dispatch(DispatchWidth, DispatchHeight, 1);
        CommandList.UnorderedAccessTextureBarrier(FrameResources.SSAOBuffer.Get());
    }

    // Horizontal blur
    {
        GPU_TRACE_SCOPE(CommandList, "SSAO Horizontal blur");
        
        CommandList.SetComputePipelineState(BlurHorizontalPSO.Get());
        CommandList.Set32BitShaderConstants(BlurHorizontalShader.Get(), &SSAOSettings.ScreenSize, 2);
        CommandList.Dispatch(DispatchWidth, DispatchHeight, 1);

        CommandList.UnorderedAccessTextureBarrier(FrameResources.SSAOBuffer.Get());
    }

    // Vertical blur
    {
        GPU_TRACE_SCOPE(CommandList, "SSAO Vertical blur");

        CommandList.SetComputePipelineState(BlurVerticalPSO.Get());
        CommandList.Set32BitShaderConstants(BlurVerticalShader.Get(), &SSAOSettings.ScreenSize, 2);
        CommandList.Dispatch(DispatchWidth, DispatchHeight, 1);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End SSAO");
}

bool FScreenSpaceOcclusionRenderer::CreateRenderTarget(FFrameResources& FrameResources)
{
    const ETextureUsageFlags Flags = ETextureUsageFlags::UnorderedAccess | ETextureUsageFlags::ShaderResource;
    
    const uint32 Width  = FrameResources.MainWindowViewport->GetWidth() / 2;
    const uint32 Height = FrameResources.MainWindowViewport->GetHeight() / 2;

    FRHITextureDesc SSAOBufferDesc = FRHITextureDesc::CreateTexture2D(
        FrameResources.SSAOBufferFormat,
        Width, 
        Height, 
        1,
        1,
        Flags);

    FrameResources.SSAOBuffer = RHICreateTexture(SSAOBufferDesc);
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
