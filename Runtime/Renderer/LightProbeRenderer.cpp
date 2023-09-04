#include "LightProbeRenderer.h"
#include "RHI/RHI.h"
#include "RHI/RHIShaderCompiler.h"

bool FLightProbeRenderer::Init(FLightSetup& LightSetup, FFrameResources& FrameResources)
{
    if (!CreateSkyLightResources(LightSetup))
    {
        return false;
    }

    TArray<uint8> Code;
    
    {
        FRHIShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Compute);
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
        FRHIShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Compute);
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

void FLightProbeRenderer::RenderSkyLightProbe(FRHICommandList& CmdList, const FLightSetup& LightSetup, const FFrameResources& FrameResources)
{
    const FProxyLightProbe& Skylight = LightSetup.Skylight;
    const uint32 IrradianceMapSize = static_cast<uint32>(Skylight.IrradianceMap->GetWidth());

    CmdList.TransitionTexture(FrameResources.Skybox.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource);
    CmdList.TransitionTexture(Skylight.IrradianceMap.Get(), EResourceAccess::Common, EResourceAccess::UnorderedAccess);

    CmdList.SetComputePipelineState(IrradianceGenPSO.Get());

    FRHIShaderResourceView* SkyboxSRV = FrameResources.Skybox->GetShaderResourceView();
    CmdList.SetShaderResourceView(IrradianceGenShader.Get(), SkyboxSRV, 0);
    CmdList.SetUnorderedAccessView(IrradianceGenShader.Get(), Skylight.IrradianceMapUAV.Get(), 0);

    {
        const FIntVector3 ThreadCount = IrradianceGenShader->GetThreadGroupXYZ();
        const uint32 ThreadWidth  = FMath::DivideByMultiple(IrradianceMapSize, ThreadCount.x);
        const uint32 ThreadHeight = FMath::DivideByMultiple(IrradianceMapSize, ThreadCount.y);
        CmdList.Dispatch(ThreadWidth, ThreadHeight, 6);
    }

    CmdList.UnorderedAccessTextureBarrier(Skylight.IrradianceMap.Get());

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
        CmdList.SetUnorderedAccessView(SpecularIrradianceGenShader.Get(), Skylight.SpecularIrradianceMapUAVs[Mip].Get(), 0);

        {
            const FIntVector3 ThreadCount = SpecularIrradianceGenShader->GetThreadGroupXYZ();
            const uint32 ThreadWidth  = FMath::DivideByMultiple(Width, ThreadCount.x);
            const uint32 ThreadHeight = FMath::DivideByMultiple(Width, ThreadCount.y);
            CmdList.Dispatch(ThreadWidth, ThreadHeight, 6);
        }

        CmdList.UnorderedAccessTextureBarrier(Skylight.SpecularIrradianceMap.Get());

        Width = FMath::Max<uint32>(Width / 2, 1U);
        Roughness += RoughnessDelta;
    }

    CmdList.TransitionTexture(FrameResources.Skybox.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource);
    CmdList.TransitionTexture(Skylight.SpecularIrradianceMap.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::PixelShaderResource);
}

bool FLightProbeRenderer::CreateSkyLightResources(FLightSetup& LightSetup)
{
    FProxyLightProbe& Skylight = LightSetup.Skylight;

    // Generate global irradiance (From Skybox)
    FRHITextureDesc LightProbeDesc = FRHITextureDesc::CreateTextureCube(LightSetup.LightProbeFormat, LightSetup.IrradianceSize, 1, 1, ETextureUsageFlags::UnorderedAccess | ETextureUsageFlags::ShaderResource);

    Skylight.IrradianceMap = RHICreateTexture(LightProbeDesc);
    if (!Skylight.IrradianceMap)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        Skylight.IrradianceMap->SetName("Irradiance Map");
    }

    FRHITextureUAVDesc UAVInitializer(Skylight.IrradianceMap.Get(), LightSetup.LightProbeFormat, 0, 0, 6);
    Skylight.IrradianceMapUAV = RHICreateUnorderedAccessView(UAVInitializer);
    if (!Skylight.IrradianceMapUAV)
    {
        DEBUG_BREAK();
        return false;
    }

    const uint16 SpecularIrradianceMiplevels = FMath::Max<uint16>(FMath::Log2(LightSetup.SpecularIrradianceSize), 1u);
    LightProbeDesc.Extent       = FIntVector3(LightSetup.SpecularIrradianceSize, LightSetup.SpecularIrradianceSize, 0);
    LightProbeDesc.NumMipLevels = uint8(SpecularIrradianceMiplevels);

    Skylight.SpecularIrradianceMap = RHICreateTexture(LightProbeDesc);
    if (!Skylight.SpecularIrradianceMap)
    {
        DEBUG_BREAK();
        return false;
    }
    else
    {
        Skylight.SpecularIrradianceMap->SetName("Specular Irradiance Map");
    }

    for (uint32 MipLevel = 0; MipLevel < SpecularIrradianceMiplevels; MipLevel++)
    {
        UAVInitializer = FRHITextureUAVDesc(Skylight.SpecularIrradianceMap.Get(), LightSetup.LightProbeFormat, MipLevel, 0, 6);
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
