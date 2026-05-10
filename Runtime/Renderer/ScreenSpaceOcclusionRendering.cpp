#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/IntVector2.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/ConsoleManager.h"
#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"
#include "Renderer/SceneRenderer.h"
#include "Renderer/ScreenSpaceOcclusionRendering.h"

static float GSSAORadius = 0.2f;
static FAutoConsoleVariableRef CVarSSAORadius(
    "Renderer.SSAO.Radius",
    "Specifies the radius of the Screen-Space Ray-Trace in SSAO",
    GSSAORadius);

static float GSSAOBias = 0.04f;
static FAutoConsoleVariableRef CVarSSAOBias(
    "Renderer.SSAO.Bias",
    "Specifies the bias when testing the Screen-Space Rays against the depth-buffer",
    GSSAOBias);

static int32 GSSAOKernelSize = 8;
static FAutoConsoleVariableRef CVarSSAOKernelSize(
    "Renderer.SSAO.KernelSize",
    "Specifies the number of samples for each pixel",
    GSSAOKernelSize);

FScreenSpaceOcclusionPass::FScreenSpaceOcclusionPass(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
{
}

FScreenSpaceOcclusionPass::~FScreenSpaceOcclusionPass()
{
    PipelineState.Reset();
    BlurHorizontalPSO.Reset();
    BlurVerticalPSO.Reset();
    SSAOShader.Reset();
    BlurHorizontalShader.Reset();
    BlurVerticalShader.Reset();
}

bool FScreenSpaceOcclusionPass::Initialize(FFrameResources& FrameResources)
{
	const uint32 Width  = FrameResources.CurrentRenderWidth;
	const uint32 Height = FrameResources.CurrentRenderHeight;

    if (!CreateResources(FrameResources, Width, Height))
    {
        return false;
    }

    TArray<uint8> ShaderCode;

    FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/SSAO.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    SSAOShader = FRHI::Get()->CreateComputeShader(ShaderCode);
    if (!SSAOShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputePipelineStateDesc PSODesc;
    PSODesc.Shader = SSAOShader.Get();

    PipelineState = FRHI::Get()->CreateComputePipelineState(PSODesc);
    if (!PipelineState)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        PipelineState->SetDebugName("SSAO PipelineState");
    }

    TArray<FShaderDefine> Defines = 
    {
        { "HORIZONTAL_PASS", "(1)" }
    };

    CompileInfo = FShaderCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute, Defines);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/Blur.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    BlurHorizontalShader = FRHI::Get()->CreateComputeShader(ShaderCode);
    if (!BlurHorizontalShader)
    {
        DEBUG_BREAK();
        return false;
    }

    PSODesc.Shader = BlurHorizontalShader.Get();
    BlurHorizontalPSO = FRHI::Get()->CreateComputePipelineState(PSODesc);

    if (!BlurHorizontalPSO)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        BlurHorizontalPSO->SetDebugName("SSAO Horizontal Blur PSO");
    }

    Defines.Clear();
    Defines.Emplace("VERTICAL_PASS", "1");

    CompileInfo = FShaderCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute, MakeArrayView(Defines));
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/Blur.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    BlurVerticalShader = FRHI::Get()->CreateComputeShader(ShaderCode);
    if (!BlurVerticalShader)
    {
        DEBUG_BREAK();
        return false;
    }

    PSODesc.Shader = BlurVerticalShader.Get();

    BlurVerticalPSO = FRHI::Get()->CreateComputePipelineState(PSODesc);
    if (!BlurVerticalPSO)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        BlurVerticalPSO->SetDebugName("SSAO Vertical Blur PSO");
    }

    return true;
}

void FScreenSpaceOcclusionPass::Execute(FRHICommandList& CommandList, FFrameResources& FrameResources)
{
    if (FrameResources.SSAOBuffer->GetWidth() == 0 || FrameResources.SSAOBuffer->GetHeight() == 0)
    {
        return;
    }

    RHI_EVENT_SCOPE(CommandList, "SSAO");

    TRACE_SCOPE("SSAO");

    GPU_TRACE_SCOPE(CommandList, "SSAO");

    struct FSSAOSettingsHLSL
    {
        // 0-16
        FVector2 ScreenSize;
        FVector2 NoiseSize;

        // 16-32
        FIntVector2 GBufferSize;
        float       Radius;
        float       Bias;

        // 32-40
        uint32 KernelSize;
        uint32 FrameIndex;
    } SSAOSettings;

    const uint32 Width         = FrameResources.SSAOBuffer->GetWidth();
    const uint32 Height        = FrameResources.SSAOBuffer->GetHeight();
    const uint32 GBufferWidth  = FrameResources.GBuffer[GBufferIndex_Depth]->GetWidth();
    const uint32 GBufferHeight = FrameResources.GBuffer[GBufferIndex_Depth]->GetHeight();

    SSAOSettings.ScreenSize  = FVector2(float(Width), float(Height));
    SSAOSettings.NoiseSize   = FVector2(4.0f, 4.0f);
    SSAOSettings.GBufferSize = FIntVector2(GBufferWidth, GBufferHeight);
    SSAOSettings.Radius      = GSSAORadius;
    SSAOSettings.KernelSize  = GSSAOKernelSize;
    SSAOSettings.Bias        = GSSAOBias;
    SSAOSettings.FrameIndex  = GetRenderer()->GetFrameCounter().GetFrameIndex();

    CommandList.SetComputePipelineState(PipelineState.Get());
    CommandList.SetConstantBuffer(SSAOShader.Get(), FrameResources.CameraBuffer.Get(), 0);

    CommandList.SetShaderResourceView(SSAOShader.Get(), FrameResources.GBuffer[GBufferIndex_Normal]->GetShaderResourceView(), 0);
    CommandList.SetShaderResourceView(SSAOShader.Get(), FrameResources.GBuffer[GBufferIndex_Depth]->GetShaderResourceView(), 1);

    CommandList.SetSamplerState(SSAOShader.Get(), FrameResources.GBufferSampler.Get(), 0);

    FRHIUnorderedAccessView* SSAOBufferUAV = FrameResources.SSAOBuffer->GetUnorderedAccessView();
    CommandList.SetUnorderedAccessView(SSAOShader.Get(), SSAOBufferUAV, 0);

    constexpr uint32 NumConstants = sizeof(FSSAOSettingsHLSL) / sizeof(uint32);
    CommandList.SetShaderConstants(SSAOShader.Get(), &SSAOSettings, NumConstants);

    constexpr uint32 ThreadCount = 16;
    const uint32 DispatchWidth   = Math::DivideByMultiple<uint32>(Width, ThreadCount);
    const uint32 DispatchHeight  = Math::DivideByMultiple<uint32>(Height, ThreadCount);

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
        CommandList.SetShaderConstants(BlurHorizontalShader.Get(), &SSAOSettings.ScreenSize, 2);
        
        CommandList.Dispatch(DispatchWidth, DispatchHeight, 1);

        CommandList.UnorderedAccessTextureBarrier(FrameResources.SSAOBuffer.Get());
    }

    // Vertical blur
    {
        GPU_TRACE_SCOPE(CommandList, "SSAO Vertical blur");

        CommandList.SetComputePipelineState(BlurVerticalPSO.Get());
        
        CommandList.SetUnorderedAccessView(SSAOShader.Get(), SSAOBufferUAV, 0);
        CommandList.SetShaderConstants(BlurVerticalShader.Get(), &SSAOSettings.ScreenSize, 2);
        
        CommandList.Dispatch(DispatchWidth, DispatchHeight, 1);
    }
}

bool FScreenSpaceOcclusionPass::CreateResources(FFrameResources& FrameResources, uint32 Width, uint32 Height)
{
    const ETextureUsageFlags Flags = ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::ShaderResourceTexture;

    FRHITextureDesc SSAOBufferDesc = FRHITextureDesc::CreateTexture2D(FGlobalTextureFormats::SSAOBufferFormat, Width, Height, 1, 1, Flags);
    FrameResources.SSAOBuffer = FRHI::Get()->CreateTexture(SSAOBufferDesc, EResourceAccess::NonPixelShaderResource);
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
