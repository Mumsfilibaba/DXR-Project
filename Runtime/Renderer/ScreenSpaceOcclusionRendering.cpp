#include "ScreenSpaceOcclusionRendering.h"
#include "SceneRenderer.h"
#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/IntVector2.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"

// TODO: Remove and replace. There are better and easier implementations to do yourself
#include <random>

static TAutoConsoleVariable<float> CVarSSAORadius(
    "Renderer.SSAO.Radius",
    "Specifies the radius of the Screen-Space Ray-Trace in SSAO",
    0.2f);

static TAutoConsoleVariable<float> CVarSSAOBias(
    "Renderer.SSAO.Bias", 
    "Specifies the bias when testing the Screen-Space Rays against the depth-buffer",
    0.075f);

static TAutoConsoleVariable<int32> CVarSSAOKernelSize(
    "Renderer.SSAO.KernelSize",
    "Specifies the number of samples for each pixel",
    16);

FScreenSpaceOcclusionPass::FScreenSpaceOcclusionPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
{
}

FScreenSpaceOcclusionPass::~FScreenSpaceOcclusionPass()
{
    Release();
}

bool FScreenSpaceOcclusionPass::Initialize(FFrameResources& FrameResources)
{
    const uint32 Width  = FrameResources.MainViewport->GetWidth();
    const uint32 Height = FrameResources.MainViewport->GetHeight();

    if (!CreateResources(FrameResources, Width, Height))
    {
        return false;
    }

    TArray<uint8> ShaderCode;
    {
        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/SSAO.hlsl", CompileInfo, ShaderCode))
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
        FRHIComputePipelineStateInitializer PSOInitializer(SSAOShader.Get());
        PipelineState = RHICreateComputePipelineState(PSOInitializer);
        if (!PipelineState)
        {
            DEBUG_BREAK();
            return false;
        }
        else
        {
            PipelineState->SetDebugName("SSAO PipelineState");
        }
    }

    TArray<FShaderDefine> Defines = 
    {
        { "HORIZONTAL_PASS", "(1)" }
    };

    {
        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute, Defines);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/Blur.hlsl", CompileInfo, ShaderCode))
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
            BlurHorizontalPSO->SetDebugName("SSAO Horizontal Blur PSO");
        }
    }

    Defines.Clear();
    Defines.Emplace("VERTICAL_PASS", "1");

    {
        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute, MakeArrayView(Defines));
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/Blur.hlsl", CompileInfo, ShaderCode))
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
            BlurVerticalPSO->SetDebugName("SSAO Vertical Blur PSO");
        }
    }

    return true;
}

void FScreenSpaceOcclusionPass::Release()
{
    PipelineState.Reset();
    BlurHorizontalPSO.Reset();
    BlurVerticalPSO.Reset();
    SSAOShader.Reset();
    BlurHorizontalShader.Reset();
    BlurVerticalShader.Reset();
}

bool FScreenSpaceOcclusionPass::ResizeResources(FRHICommandList& CommandList, FFrameResources& FrameResources, uint32 Width, uint32 Height)
{
    // Destroy the old resource
    CommandList.DestroyResource(FrameResources.SSAOBuffer.Get());

    // Create the new resources
    return CreateResources(FrameResources, Width, Height);
}

void FScreenSpaceOcclusionPass::Execute(FRHICommandList& CommandList, FFrameResources& FrameResources)
{
    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "Begin SSAO");

    TRACE_SCOPE("SSAO");

    struct FSSAOSettings
    {
        FVector2    ScreenSize;
        FVector2    NoiseSize;
        FIntVector2 GBufferSize;
        float       Radius;
        float       Bias;
        int32       KernelSize;
    } SSAOSettings;

    const uint32 Width         = FrameResources.SSAOBuffer->GetWidth();
    const uint32 Height        = FrameResources.SSAOBuffer->GetHeight();
    const uint32 GBufferWidth  = FrameResources.GBuffer[GBufferIndex_Depth]->GetWidth();
    const uint32 GBufferHeight = FrameResources.GBuffer[GBufferIndex_Depth]->GetHeight();
    
    SSAOSettings.ScreenSize  = FVector2(float(Width), float(Height));
    SSAOSettings.NoiseSize   = FVector2(4.0f, 4.0f);
    SSAOSettings.GBufferSize = FIntVector2(GBufferWidth, GBufferHeight);
    SSAOSettings.Radius      = CVarSSAORadius.GetValue();
    SSAOSettings.KernelSize  = CVarSSAOKernelSize.GetValue();
    SSAOSettings.Bias        = CVarSSAOBias.GetValue();

    CommandList.SetComputePipelineState(PipelineState.Get());
    CommandList.SetConstantBuffer(SSAOShader.Get(), FrameResources.CameraBuffer.Get(), 0);

    CommandList.SetShaderResourceView(SSAOShader.Get(), FrameResources.GBuffer[GBufferIndex_ViewNormal]->GetShaderResourceView(), 0);
    CommandList.SetShaderResourceView(SSAOShader.Get(), FrameResources.GBuffer[GBufferIndex_Depth]->GetShaderResourceView(), 1);

    CommandList.SetSamplerState(SSAOShader.Get(), FrameResources.GBufferSampler.Get(), 0);

    FRHIUnorderedAccessView* SSAOBufferUAV = FrameResources.SSAOBuffer->GetUnorderedAccessView();
    CommandList.SetUnorderedAccessView(SSAOShader.Get(), SSAOBufferUAV, 0);
    CommandList.Set32BitShaderConstants(SSAOShader.Get(), &SSAOSettings, 9);

    constexpr uint32 ThreadCount = 16;
    const uint32 DispatchWidth   = FMath::DivideByMultiple<uint32>(Width, ThreadCount);
    const uint32 DispatchHeight  = FMath::DivideByMultiple<uint32>(Height, ThreadCount);

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
        
        CommandList.SetUnorderedAccessView(SSAOShader.Get(), SSAOBufferUAV, 0);
        CommandList.Set32BitShaderConstants(BlurHorizontalShader.Get(), &SSAOSettings.ScreenSize, 2);
        
        CommandList.Dispatch(DispatchWidth, DispatchHeight, 1);

        CommandList.UnorderedAccessTextureBarrier(FrameResources.SSAOBuffer.Get());
    }

    // Vertical blur
    {
        GPU_TRACE_SCOPE(CommandList, "SSAO Vertical blur");

        CommandList.SetComputePipelineState(BlurVerticalPSO.Get());
        
        CommandList.SetUnorderedAccessView(SSAOShader.Get(), SSAOBufferUAV, 0);
        CommandList.Set32BitShaderConstants(BlurVerticalShader.Get(), &SSAOSettings.ScreenSize, 2);
        
        CommandList.Dispatch(DispatchWidth, DispatchHeight, 1);
    }

    INSERT_DEBUG_CMDLIST_MARKER(CommandList, "End SSAO");
}

bool FScreenSpaceOcclusionPass::CreateResources(FFrameResources& FrameResources, uint32 Width, uint32 Height)
{
    const ETextureUsageFlags Flags = ETextureUsageFlags::UnorderedAccess | ETextureUsageFlags::ShaderResource;
    
    Width  = Width / 2;
    Height = Height / 2;

    FRHITextureInfo SSAOBufferInfo = FRHITextureInfo::CreateTexture2D(FrameResources.SSAOBufferFormat, Width, Height, 1, 1, Flags);
    FrameResources.SSAOBuffer = RHICreateTexture(SSAOBufferInfo, EResourceAccess::NonPixelShaderResource);
    if (!FrameResources.SSAOBuffer)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        FrameResources.SSAOBuffer->SetDebugName("SSAO Buffer");
    }

    return true;
}