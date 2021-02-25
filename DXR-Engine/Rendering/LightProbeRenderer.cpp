#include "LightProbeRenderer.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/ShaderCompiler.h"

Bool LightProbeRenderer::Init(LightSetup& LightSetup, FrameResources& FrameResources)
{
    if (!CreateSkyLightResources(LightSetup))
    {
        return false;
    }

    TArray<UInt8> Code;
    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/IrradianceGen.hlsl", "Main", nullptr, EShaderStage::Compute, EShaderModel::SM_6_0, Code))
    {
        LOG_ERROR("Failed to compile IrradianceGen Shader");
        Debug::DebugBreak();
    }

    IrradianceGenShader = CreateComputeShader(Code);
    if (!IrradianceGenShader)
    {
        LOG_ERROR("Failed to create IrradianceGen Shader");
        Debug::DebugBreak();
    }
    else
    {
        IrradianceGenShader->SetName("IrradianceGen Shader");
    }

    IrradianceGenPSO = CreateComputePipelineState(ComputePipelineStateCreateInfo(IrradianceGenShader.Get()));
    if (!IrradianceGenPSO)
    {
        LOG_ERROR("Failed to create IrradianceGen PipelineState");
        Debug::DebugBreak();
    }
    else
    {
        IrradianceGenPSO->SetName("IrradianceGen PSO");
    }

    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/SpecularIrradianceGen.hlsl", "Main", nullptr, EShaderStage::Compute, EShaderModel::SM_6_0, Code))
    {
        LOG_ERROR("Failed to compile SpecularIrradianceGen Shader");
        Debug::DebugBreak();
    }

    SpecularIrradianceGenShader = CreateComputeShader(Code);
    if (!SpecularIrradianceGenShader)
    {
        LOG_ERROR("Failed to create Specular IrradianceGen Shader");
        Debug::DebugBreak();
    }
    else
    {
        SpecularIrradianceGenShader->SetName("Specular IrradianceGen Shader");
    }

    SpecularIrradianceGenPSO = CreateComputePipelineState(ComputePipelineStateCreateInfo(SpecularIrradianceGenShader.Get()));
    if (!SpecularIrradianceGenPSO)
    {
        LOG_ERROR("Failed to create Specular IrradianceGen PipelineState");
        Debug::DebugBreak();
    }
    else
    {
        SpecularIrradianceGenPSO->SetName("Specular IrradianceGen PSO");
    }

    SamplerStateCreateInfo CreateInfo;
    CreateInfo.AddressU = ESamplerMode::Wrap;
    CreateInfo.AddressV = ESamplerMode::Wrap;
    CreateInfo.AddressW = ESamplerMode::Wrap;
    CreateInfo.Filter   = ESamplerFilter::MinMagMipLinear;

    FrameResources.IrradianceSampler = CreateSamplerState(CreateInfo);
    if (!FrameResources.IrradianceSampler)
    {
        return false;
    }

    return true;
}

void LightProbeRenderer::Release()
{
    IrradianceGenPSO.Reset();
    SpecularIrradianceGenPSO.Reset();
    IrradianceGenShader.Reset();
    SpecularIrradianceGenShader.Reset();
}

void LightProbeRenderer::RenderSkyLightProbe(CommandList& CmdList, const LightSetup& LightSetup, const FrameResources& FrameResources)
{
    const UInt32 IrradianceMapSize = static_cast<UInt32>(LightSetup.IrradianceMap->GetSize());

    CmdList.TransitionTexture(FrameResources.Skybox.Get(), EResourceState::PixelShaderResource, EResourceState::NonPixelShaderResource);
    CmdList.TransitionTexture(LightSetup.IrradianceMap.Get(), EResourceState::Common, EResourceState::UnorderedAccess);

    CmdList.SetComputePipelineState(IrradianceGenPSO.Get());
    
    ShaderResourceView* SkyboxSRV = FrameResources.Skybox->GetShaderResourceView();
    CmdList.SetShaderResourceView(IrradianceGenShader.Get(), SkyboxSRV, 0);
    CmdList.SetUnorderedAccessView(IrradianceGenShader.Get(), LightSetup.IrradianceMapUAV.Get(), 0);

    {
        const XMUINT3 ThreadCount = IrradianceGenShader->GetThreadGroupXYZ();
        const UInt32 ThreadWidth  = Math::DivideByMultiple(IrradianceMapSize, ThreadCount.x);
        const UInt32 ThreadHeight = Math::DivideByMultiple(IrradianceMapSize, ThreadCount.y);
        CmdList.Dispatch(ThreadWidth, ThreadHeight, 6);
    }

    CmdList.UnorderedAccessTextureBarrier(LightSetup.IrradianceMap.Get());

    CmdList.TransitionTexture(LightSetup.IrradianceMap.Get(), EResourceState::UnorderedAccess, EResourceState::PixelShaderResource);
    CmdList.TransitionTexture(LightSetup.SpecularIrradianceMap.Get(), EResourceState::Common, EResourceState::UnorderedAccess);

    CmdList.SetShaderResourceView(IrradianceGenShader.Get(), SkyboxSRV, 0);

    CmdList.SetComputePipelineState(SpecularIrradianceGenPSO.Get());

    UInt32 Width = LightSetup.SpecularIrradianceMap->GetSize();
    Float  Roughness = 0.0f;

    const UInt32 NumMiplevels   = LightSetup.SpecularIrradianceMap->GetNumMips();
    const Float  RoughnessDelta = 1.0f / (NumMiplevels - 1);
    for (UInt32 Mip = 0; Mip < NumMiplevels; Mip++)
    {
        CmdList.Set32BitShaderConstants(SpecularIrradianceGenShader.Get(), &Roughness, 1);
        CmdList.SetUnorderedAccessView(SpecularIrradianceGenShader.Get(), LightSetup.SpecularIrradianceMapUAVs[Mip].Get(), 0);

        {
            const XMUINT3 ThreadCount = SpecularIrradianceGenShader->GetThreadGroupXYZ();
            const UInt32 ThreadWidth  = Math::DivideByMultiple(Width, ThreadCount.x);
            const UInt32 ThreadHeight = Math::DivideByMultiple(Width, ThreadCount.y);
            CmdList.Dispatch(ThreadWidth, ThreadHeight, 6);
        }

        CmdList.UnorderedAccessTextureBarrier(LightSetup.SpecularIrradianceMap.Get());

        Width = std::max<UInt32>(Width / 2, 1U);
        Roughness += RoughnessDelta;
    }

    CmdList.TransitionTexture(FrameResources.Skybox.Get(), EResourceState::NonPixelShaderResource, EResourceState::PixelShaderResource);
    CmdList.TransitionTexture(LightSetup.SpecularIrradianceMap.Get(), EResourceState::UnorderedAccess, EResourceState::PixelShaderResource);
}

Bool LightProbeRenderer::CreateSkyLightResources(LightSetup& LightSetup)
{
    // Generate global irradiance (From Skybox)
    const UInt16 IrradianceSize = 32;
    LightSetup.IrradianceMap = CreateTextureCube(EFormat::R16G16B16A16_Float, IrradianceSize, 1, TextureFlags_RWTexture, EResourceState::Common, nullptr);
    if (!LightSetup.IrradianceMap)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        LightSetup.IrradianceMap->SetName("Irradiance Map");
    }

    LightSetup.IrradianceMapUAV = CreateUnorderedAccessView(LightSetup.IrradianceMap.Get(), EFormat::R16G16B16A16_Float, 0);
    if (!LightSetup.IrradianceMapUAV)
    {
        Debug::DebugBreak();
        return false;
    }

    const UInt16 SpecularIrradianceSize      = 128;
    const UInt16 SpecularIrradianceMiplevels = UInt16(std::max(std::log2(SpecularIrradianceSize), 1.0));
    LightSetup.SpecularIrradianceMap = CreateTextureCube(
        EFormat::R16G16B16A16_Float, 
        SpecularIrradianceSize, 
        SpecularIrradianceMiplevels, 
        TextureFlags_RWTexture,
        EResourceState::Common, 
        nullptr);
    if (!LightSetup.SpecularIrradianceMap)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        LightSetup.SpecularIrradianceMap->SetName("Specular Irradiance Map");
    }

    for (UInt32 MipLevel = 0; MipLevel < SpecularIrradianceMiplevels; MipLevel++)
    {
        TRef<UnorderedAccessView> Uav = CreateUnorderedAccessView(LightSetup.SpecularIrradianceMap.Get(), EFormat::R16G16B16A16_Float, MipLevel);
        if (Uav)
        {
            LightSetup.SpecularIrradianceMapUAVs.EmplaceBack(Uav);
            LightSetup.WeakSpecularIrradianceMapUAVs.EmplaceBack(Uav.Get());
        }
        else
        {
            Debug::DebugBreak();
            return false;
        }
    }

    return true;
}
