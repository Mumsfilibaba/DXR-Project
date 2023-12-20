#include "LightProbeRenderer.h"
#include "RHI/RHI.h"
#include "RHI/RHIShaderCompiler.h"

bool FLightProbeRenderer::Initialize(FLightSetup& LightSetup, FFrameResources& FrameResources)
{
    if (!Compressor.Initialize())
    {
        return false;
    }

    if (!CreateSkyLightResources(LightSetup))
    {
        return false;
    }

    TArray<uint8> Code;
    
    {
        FRHIShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/IrradianceGen.hlsl", CompileInfo, Code))
        {
            LOG_ERROR("Failed to compile IrradianceGen Shader");
        }
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
        IrradianceGenPSO->SetName("IrradianceGen PSO");
    }

    {
        FRHIShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
        if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/SpecularIrradianceGen.hlsl", CompileInfo, Code))
        {
            LOG_ERROR("Failed to compile SpecularIrradianceGen Shader");
        }
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
        SpecularIrradianceGenPSO->SetName("Specular IrradianceGen PSO");
    }

    FRHISamplerStateDesc SamplerInitializer;
    SamplerInitializer.AddressU = ESamplerMode::Wrap;
    SamplerInitializer.AddressV = ESamplerMode::Wrap;
    SamplerInitializer.AddressW = ESamplerMode::Wrap;
    SamplerInitializer.Filter   = ESamplerFilter::MinMagMipLinear;

    FrameResources.IrradianceSampler = RHICreateSamplerState(SamplerInitializer);
    if (!FrameResources.IrradianceSampler)
    {
        return false;
    }

    return true;
}

void FLightProbeRenderer::Release()
{
    IrradianceGenPSO.Reset();
    SpecularIrradianceGenPSO.Reset();
    IrradianceGenShader.Reset();
    SpecularIrradianceGenShader.Reset();
}

void FLightProbeRenderer::RenderSkyLightProbe(FRHICommandList& CmdList, FLightSetup& LightSetup, const FFrameResources& FrameResources)
{
    FProxyLightProbe& Skylight = LightSetup.Skylight;
    CmdList.TransitionTexture(FrameResources.Skybox.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource);
    CmdList.TransitionTexture(Skylight.IrradianceMap.Get(), EResourceAccess::Common, EResourceAccess::UnorderedAccess);

    CmdList.SetComputePipelineState(IrradianceGenPSO.Get());

    FRHIShaderResourceView* SkyboxSRV = FrameResources.Skybox->GetShaderResourceView();
    CmdList.SetShaderResourceView(IrradianceGenShader.Get(), SkyboxSRV, 0);
    CmdList.SetUnorderedAccessView(IrradianceGenShader.Get(), Skylight.IrradianceMapUAV.Get(), 0);

    {
        const uint32 IrradianceMapSize = static_cast<uint32>(Skylight.IrradianceMap->GetWidth());

        constexpr uint32 NumThreads = 16;
        const uint32 ThreadWidth  = FMath::DivideByMultiple(IrradianceMapSize, NumThreads);
        const uint32 ThreadHeight = FMath::DivideByMultiple(IrradianceMapSize, NumThreads);
        CmdList.Dispatch(ThreadWidth, ThreadHeight, 6);
    }

    CmdList.UnorderedAccessTextureBarrier(Skylight.IrradianceMap.Get());

    CmdList.DestroyResource(Skylight.IrradianceMapUAV.Get());

    CmdList.TransitionTexture(Skylight.IrradianceMap.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::PixelShaderResource);
    CmdList.TransitionTexture(Skylight.SpecularIrradianceMap.Get(), EResourceAccess::Common, EResourceAccess::UnorderedAccess);

    CmdList.SetShaderResourceView(IrradianceGenShader.Get(), SkyboxSRV, 0);

    CmdList.SetComputePipelineState(SpecularIrradianceGenPSO.Get());

    uint32 Width = Skylight.SpecularIrradianceMap->GetWidth();
    float  Roughness = 0.0f;

    const uint32 NumMiplevels   = Skylight.SpecularIrradianceMap->GetNumMipLevels();
    const float  RoughnessDelta = 1.0f / (NumMiplevels - 1);
    for (uint32 Mip = 0; Mip < NumMiplevels; Mip++)
    {
        CmdList.Set32BitShaderConstants(SpecularIrradianceGenShader.Get(), &Roughness, 1);

        FRHIUnorderedAccessView* UnorderedAccessView = Skylight.SpecularIrradianceMapUAVs[Mip].Get();
        CmdList.SetUnorderedAccessView(SpecularIrradianceGenShader.Get(), UnorderedAccessView, 0);

        constexpr uint32 NumThreads = 16;
        const uint32 ThreadWidth  = FMath::DivideByMultiple(Width, NumThreads);
        const uint32 ThreadHeight = FMath::DivideByMultiple(Width, NumThreads);
        CmdList.Dispatch(ThreadWidth, ThreadHeight, 6);

        CmdList.UnorderedAccessTextureBarrier(Skylight.SpecularIrradianceMap.Get());

        Width      = FMath::Max<uint32>(Width / 2, 1U);
        Roughness += RoughnessDelta;

        CmdList.DestroyResource(UnorderedAccessView);
    }

    CmdList.TransitionTexture(FrameResources.Skybox.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource);
    CmdList.TransitionTexture(Skylight.SpecularIrradianceMap.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::PixelShaderResource);

    Skylight.IrradianceMapUAV.Reset();
    Skylight.WeakSpecularIrradianceMapUAVs.Clear(true);
    Skylight.SpecularIrradianceMapUAVs.Clear(true);
}

bool FLightProbeRenderer::CreateSkyLightResources(FLightSetup& LightSetup)
{
    FProxyLightProbe& Skylight = LightSetup.Skylight;

    // Generate global irradiance (From Skybox)
    FRHITextureDesc LightProbeDesc = FRHITextureDesc::CreateTextureCube(LightSetup.LightProbeFormat, LightSetup.IrradianceProbeSize, 1, 1, ETextureUsageFlags::UnorderedAccess | ETextureUsageFlags::ShaderResource);
    Skylight.IrradianceMap = RHICreateTexture(LightProbeDesc);
    if (!Skylight.IrradianceMap)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        Skylight.IrradianceMap->SetName("Temp Irradiance Map");
    }

    FRHITextureUAVDesc UAVInitializer(Skylight.IrradianceMap.Get(), LightSetup.LightProbeFormat, 0, 0, 1);
    Skylight.IrradianceMapUAV = RHICreateUnorderedAccessView(UAVInitializer);
    if (!Skylight.IrradianceMapUAV)
    {
        DEBUG_BREAK();
        return false;
    }

    const int32 SpecularIrradianceMiplevels = FMath::Max(FMath::Log2(LightSetup.SpecularIrradianceProbeSize), 1);
    LightProbeDesc.Extent       = FIntVector3(LightSetup.SpecularIrradianceProbeSize, LightSetup.SpecularIrradianceProbeSize, 0);
    LightProbeDesc.NumMipLevels = uint8(SpecularIrradianceMiplevels);

    Skylight.SpecularIrradianceMap = RHICreateTexture(LightProbeDesc);
    if (!Skylight.SpecularIrradianceMap)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        Skylight.SpecularIrradianceMap->SetName("Temp Specular Irradiance Map");
    }

    for (int32 MipLevel = 0; MipLevel < SpecularIrradianceMiplevels; MipLevel++)
    {
        UAVInitializer = FRHITextureUAVDesc(Skylight.SpecularIrradianceMap.Get(), LightSetup.LightProbeFormat, MipLevel, 0, 1);
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
