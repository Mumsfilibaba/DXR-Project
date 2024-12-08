#include "LightProbeRenderer.h"
#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"

FLightProbeRenderer::FLightProbeRenderer(FSceneRenderer* InRenderer)
    : FRenderPass(InRenderer)
{
}

FLightProbeRenderer::~FLightProbeRenderer()
{
    IrradianceGenPSO.Reset();
    SpecularIrradianceGenPSO.Reset();
    IrradianceGenShader.Reset();
    SpecularIrradianceGenShader.Reset();
}

bool FLightProbeRenderer::Initialize(FFrameResources& FrameResources)
{
    if (!Compressor.Initialize())
    {
        return false;
    }

    if (!CreateSkyLightResources(FrameResources))
    {
        return false;
    }

    TArray<uint8> Code;

    FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/IrradianceGen.hlsl", CompileInfo, Code))
    {
        LOG_ERROR("Failed to compile IrradianceGen Shader");
    }

    IrradianceGenShader = RHICreateComputeShader(Code);
    if (!IrradianceGenShader)
    {
        LOG_ERROR("Failed to create IrradianceGen Shader");
    }

    IrradianceGenPSO = RHICreateComputePipelineState(FRHIComputePipelineStateInitializer(IrradianceGenShader.Get()));
    if (!IrradianceGenPSO)
    {
        LOG_ERROR("Failed to create IrradianceGen PipelineState");
    }
    else
    {
        IrradianceGenPSO->SetDebugName("IrradianceGen PSO");
    }

    CompileInfo = FShaderCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/SpecularIrradianceGen.hlsl", CompileInfo, Code))
    {
        LOG_ERROR("Failed to compile SpecularIrradianceGen Shader");
    }

    SpecularIrradianceGenShader = RHICreateComputeShader(Code);
    if (!SpecularIrradianceGenShader)
    {
        LOG_ERROR("Failed to create Specular IrradianceGen Shader");
    }

    SpecularIrradianceGenPSO = RHICreateComputePipelineState(FRHIComputePipelineStateInitializer(SpecularIrradianceGenShader.Get()));
    if (!SpecularIrradianceGenPSO)
    {
        LOG_ERROR("Failed to create Specular IrradianceGen PipelineState");
    }
    else
    {
        SpecularIrradianceGenPSO->SetDebugName("Specular IrradianceGen PSO");
    }

    FRHISamplerStateInfo SamplerInfo;
    SamplerInfo.AddressU = ESamplerMode::Wrap;
    SamplerInfo.AddressV = ESamplerMode::Wrap;
    SamplerInfo.AddressW = ESamplerMode::Wrap;
    SamplerInfo.Filter   = ESamplerFilter::MinMagMipLinear;

    FrameResources.IrradianceSampler = RHICreateSamplerState(SamplerInfo);
    if (!FrameResources.IrradianceSampler)
    {
        return false;
    }

    return true;
}

void FLightProbeRenderer::RenderSkyLightProbe(FRHICommandList& CommandList, FFrameResources& FrameResources)
{
    FProxyLightProbe& Skylight = FrameResources.Skylight;
    CommandList.TransitionTexture(FrameResources.Skybox.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource);
    CommandList.TransitionTexture(Skylight.IrradianceMap.Get(), EResourceAccess::Common, EResourceAccess::UnorderedAccess);

    CommandList.SetComputePipelineState(IrradianceGenPSO.Get());

    FRHIShaderResourceView* SkyboxSRV = FrameResources.Skybox->GetShaderResourceView();
    CommandList.SetShaderResourceView(IrradianceGenShader.Get(), SkyboxSRV, 0);
    CommandList.SetUnorderedAccessView(IrradianceGenShader.Get(), Skylight.IrradianceMapUAV.Get(), 0);
    CommandList.SetSamplerState(IrradianceGenShader.Get(), FrameResources.IrradianceSampler.Get(), 0);

    {
        const uint32 IrradianceMapSize = static_cast<uint32>(Skylight.IrradianceMap->GetWidth());

        constexpr uint32 NumThreads = 16;
        const uint32 ThreadWidth  = FMath::DivideByMultiple(IrradianceMapSize, NumThreads);
        const uint32 ThreadHeight = FMath::DivideByMultiple(IrradianceMapSize, NumThreads);
        CommandList.Dispatch(ThreadWidth, ThreadHeight, 6);
    }

    CommandList.UnorderedAccessTextureBarrier(Skylight.IrradianceMap.Get());

    CommandList.TransitionTexture(Skylight.IrradianceMap.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::PixelShaderResource);
    CommandList.TransitionTexture(Skylight.SpecularIrradianceMap.Get(), EResourceAccess::Common, EResourceAccess::UnorderedAccess);

    CommandList.SetComputePipelineState(SpecularIrradianceGenPSO.Get());
    
    CommandList.SetShaderResourceView(SpecularIrradianceGenShader.Get(), SkyboxSRV, 0);
    CommandList.SetSamplerState(SpecularIrradianceGenShader.Get(), FrameResources.IrradianceSampler.Get(), 0);

    const uint32 IrradianceMapSize = Skylight.SpecularIrradianceMap->GetWidth();
    const uint32 NumMiplevels      = Skylight.SpecularIrradianceMap->GetNumMipLevels();
    const uint32 SkyboxWidth       = FrameResources.Skybox->GetWidth();
    const float  RoughnessDelta    = 1.0f / (NumMiplevels - 1);

    float  Roughness    = 0.0f;
    uint32 CurrentWidth = IrradianceMapSize;
    for (uint32 Mip = 0; Mip < NumMiplevels; Mip++)
    {
        struct FSpecularIrradianceGenConstants
        {
            float  Roughness;
            uint32 SourceFaceResolution;
            uint32 CurrentFaceResolution;
        } Constants;

        Constants.Roughness             = Roughness;
        Constants.SourceFaceResolution  = IrradianceMapSize;
        Constants.CurrentFaceResolution = CurrentWidth;

        constexpr uint32 NumConstants = sizeof(FSpecularIrradianceGenConstants) / sizeof(uint32);
        CommandList.Set32BitShaderConstants(SpecularIrradianceGenShader.Get(), &Constants, NumConstants);

        FRHIUnorderedAccessView* UnorderedAccessView = Skylight.SpecularIrradianceMapUAVs[Mip].Get();
        CommandList.SetUnorderedAccessView(SpecularIrradianceGenShader.Get(), UnorderedAccessView, 0);

        constexpr uint32 NumThreads = 16;
        const uint32 ThreadWidth  = FMath::DivideByMultiple(CurrentWidth, NumThreads);
        const uint32 ThreadHeight = FMath::DivideByMultiple(CurrentWidth, NumThreads);
        CommandList.Dispatch(ThreadWidth, ThreadHeight, 6);

        CommandList.UnorderedAccessTextureBarrier(Skylight.SpecularIrradianceMap.Get());

        CurrentWidth = FMath::Max<uint32>(CurrentWidth / 2, 1U);
        Roughness += RoughnessDelta;
    }

    CommandList.TransitionTexture(FrameResources.Skybox.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource);
    CommandList.TransitionTexture(Skylight.SpecularIrradianceMap.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::PixelShaderResource);

    Skylight.IrradianceMapUAV.Reset();
    Skylight.WeakSpecularIrradianceMapUAVs.Clear(true);
    Skylight.SpecularIrradianceMapUAVs.Clear(true);
}

bool FLightProbeRenderer::CreateSkyLightResources(FFrameResources& FrameResources)
{
    FProxyLightProbe& Skylight = FrameResources.Skylight;

    // Generate global irradiance (From Skybox)
    FRHITextureInfo LightProbeInfo = FRHITextureInfo::CreateTextureCube(FrameResources.LightProbeFormat, FrameResources.IrradianceProbeSize, 1, 1, ETextureUsageFlags::UnorderedAccess | ETextureUsageFlags::ShaderResource);
    Skylight.IrradianceMap = RHICreateTexture(LightProbeInfo);
    if (!Skylight.IrradianceMap)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        Skylight.IrradianceMap->SetDebugName("Temp Irradiance Map");
    }

    FRHITextureUAVDesc UAVInitializer(Skylight.IrradianceMap.Get(), FrameResources.LightProbeFormat, 0, 0, 1);
    Skylight.IrradianceMapUAV = RHICreateUnorderedAccessView(UAVInitializer);
    if (!Skylight.IrradianceMapUAV)
    {
        DEBUG_BREAK();
        return false;
    }

    const int32 SpecularIrradianceMiplevels = FMath::Max<int32>(static_cast<uint32>(FMath::Log2(static_cast<float>(FrameResources.SpecularIrradianceProbeSize))), 1);
    LightProbeInfo.Extent       = FIntVector3(FrameResources.SpecularIrradianceProbeSize, FrameResources.SpecularIrradianceProbeSize, 0);
    LightProbeInfo.NumMipLevels = uint8(SpecularIrradianceMiplevels);

    Skylight.SpecularIrradianceMap = RHICreateTexture(LightProbeInfo);
    if (!Skylight.SpecularIrradianceMap)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        Skylight.SpecularIrradianceMap->SetDebugName("Temp Specular Irradiance Map");
    }

    for (int32 MipLevel = 0; MipLevel < SpecularIrradianceMiplevels; MipLevel++)
    {
        UAVInitializer = FRHITextureUAVDesc(Skylight.SpecularIrradianceMap.Get(), FrameResources.LightProbeFormat, MipLevel, 0, 1);
        FRHIUnorderedAccessViewRef UAV = RHICreateUnorderedAccessView(UAVInitializer);
        if (UAV)
        {
            Skylight.SpecularIrradianceMapUAVs.Emplace(UAV);
            Skylight.WeakSpecularIrradianceMapUAVs.Emplace(UAV.Get());
        }
        else
        {
            DEBUG_BREAK();
            return false;
        }
    }

    return true;
}
