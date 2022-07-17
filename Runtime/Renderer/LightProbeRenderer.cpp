#include "LightProbeRenderer.h"

#include "RHI/RHICoreInterface.h"
#include "RHI/RHIShaderCompiler.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FLightProbeRenderer

bool FLightProbeRenderer::Init(FLightSetup& LightSetup, FFrameResources& FrameResources)
{
    if (!CreateSkyLightResources(LightSetup))
    {
        return false;
    }

    TArray<uint8> Code;
    
    {
        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Compute);
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
        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Compute);
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

    FRHISamplerStateInitializer SamplerInitializer;
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
    const uint32 IrradianceMapSize = static_cast<uint32>(LightSetup.IrradianceMap->GetWidth());

    CmdList.TransitionTexture(FrameResources.Skybox.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource);
    CmdList.TransitionTexture(LightSetup.IrradianceMap.Get(), EResourceAccess::Common, EResourceAccess::UnorderedAccess);

    CmdList.SetComputePipelineState(IrradianceGenPSO.Get());

    FRHIShaderResourceView* SkyboxSRV = FrameResources.Skybox->GetShaderResourceView();
    CmdList.SetShaderResourceView(IrradianceGenShader.Get(), SkyboxSRV, 0);
    CmdList.SetUnorderedAccessView(IrradianceGenShader.Get(), LightSetup.IrradianceMapUAV.Get(), 0);

    {
        const FIntVector3 ThreadCount = IrradianceGenShader->GetThreadGroupXYZ();
        const uint32 ThreadWidth  = NMath::DivideByMultiple(IrradianceMapSize, ThreadCount.x);
        const uint32 ThreadHeight = NMath::DivideByMultiple(IrradianceMapSize, ThreadCount.y);
        CmdList.Dispatch(ThreadWidth, ThreadHeight, 6);
    }

    CmdList.UnorderedAccessTextureBarrier(LightSetup.IrradianceMap.Get());

    CmdList.TransitionTexture(LightSetup.IrradianceMap.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::PixelShaderResource);
    CmdList.TransitionTexture(LightSetup.SpecularIrradianceMap.Get(), EResourceAccess::Common, EResourceAccess::UnorderedAccess);

    CmdList.SetShaderResourceView(IrradianceGenShader.Get(), SkyboxSRV, 0);

    CmdList.SetComputePipelineState(SpecularIrradianceGenPSO.Get());

    uint32 Width = LightSetup.SpecularIrradianceMap->GetWidth();
    float  Roughness = 0.0f;

    const uint32 NumMiplevels   = LightSetup.SpecularIrradianceMap->GetNumMips();
    const float  RoughnessDelta = 1.0f / (NumMiplevels - 1);
    for (uint32 Mip = 0; Mip < NumMiplevels; Mip++)
    {
        CmdList.Set32BitShaderConstants(SpecularIrradianceGenShader.Get(), &Roughness, 1);
        CmdList.SetUnorderedAccessView(SpecularIrradianceGenShader.Get(), LightSetup.SpecularIrradianceMapUAVs[Mip].Get(), 0);

        {
            const FIntVector3 ThreadCount = SpecularIrradianceGenShader->GetThreadGroupXYZ();
            const uint32 ThreadWidth  = NMath::DivideByMultiple(Width, ThreadCount.x);
            const uint32 ThreadHeight = NMath::DivideByMultiple(Width, ThreadCount.y);
            CmdList.Dispatch(ThreadWidth, ThreadHeight, 6);
        }

        CmdList.UnorderedAccessTextureBarrier(LightSetup.SpecularIrradianceMap.Get());

        Width = NMath::Max<uint32>(Width / 2, 1U);
        Roughness += RoughnessDelta;
    }

    CmdList.TransitionTexture(FrameResources.Skybox.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource);
    CmdList.TransitionTexture(LightSetup.SpecularIrradianceMap.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::PixelShaderResource);
}

bool FLightProbeRenderer::CreateSkyLightResources(FLightSetup& LightSetup)
{
    // Generate global irradiance (From Skybox)
    FRHITextureCubeInitializer LightProbeInitializer(LightSetup.LightProbeFormat, LightSetup.IrradianceSize, 1, 1, ETextureUsageFlags::RWTexture, EResourceAccess::Common);
    LightSetup.IrradianceMap = RHICreateTextureCube(LightProbeInitializer);
    if (!LightSetup.IrradianceMap)
    {
        PlatformDebugBreak();
        return false;
    }
    else
    {
        LightSetup.IrradianceMap->SetName("Irradiance Map");
    }

    FRHITextureUAVInitializer UAVInitializer(LightSetup.IrradianceMap.Get(), LightSetup.LightProbeFormat, 0, 0, 6);
    LightSetup.IrradianceMapUAV = RHICreateUnorderedAccessView(UAVInitializer);
    if (!LightSetup.IrradianceMapUAV)
    {
        PlatformDebugBreak();
        return false;
    }

    const uint16 SpecularIrradianceMiplevels = NMath::Max<uint16>(NMath::Log2(LightSetup.SpecularIrradianceSize), 1u);
    LightProbeInitializer.Extent  = LightSetup.SpecularIrradianceSize;
    LightProbeInitializer.NumMips = uint8(SpecularIrradianceMiplevels);

    LightSetup.SpecularIrradianceMap = RHICreateTextureCube(LightProbeInitializer);
    if (!LightSetup.SpecularIrradianceMap)
    {
        PlatformDebugBreak();
        return false;
    }
    else
    {
        LightSetup.SpecularIrradianceMap->SetName("Specular Irradiance Map");
    }

    for (uint32 MipLevel = 0; MipLevel < SpecularIrradianceMiplevels; MipLevel++)
    {
        UAVInitializer = FRHITextureUAVInitializer(LightSetup.SpecularIrradianceMap.Get(), LightSetup.LightProbeFormat, MipLevel, 0, 6);
        FRHIUnorderedAccessViewRef UAV = RHICreateUnorderedAccessView(UAVInitializer);
        if (UAV)
        {
            LightSetup.SpecularIrradianceMapUAVs.Emplace(UAV);
            LightSetup.WeakSpecularIrradianceMapUAVs.Emplace(UAV.Get());
        }
        else
        {
            PlatformDebugBreak();
            return false;
        }
    }

    return true;
}
