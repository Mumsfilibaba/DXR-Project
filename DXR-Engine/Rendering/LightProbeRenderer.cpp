#include "LightProbeRenderer.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/ShaderCompiler.h"

Bool LightProbeRenderer::Init(SceneLightSetup& LightSetup, FrameResources& FrameResources)
{
	if (!CreateSkyLightResources(LightSetup))
	{
		return false;
	}

    TArray<UInt8> Code;
    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/IrradianceGen.hlsl",
        "Main",
        nullptr,
        EShaderStage::ShaderStage_Compute,
        EShaderModel::ShaderModel_6_0,
        Code))
    {
        LOG_ERROR("Failed to compile IrradianceGen Shader");
        Debug::DebugBreak();
    }

    TSharedRef<ComputeShader> IrradianceGenShader = RenderLayer::CreateComputeShader(Code);
    if (!IrradianceGenShader)
    {
        LOG_ERROR("Failed to create IrradianceGen Shader");
        Debug::DebugBreak();
    }
    else
    {
        IrradianceGenShader->SetName("IrradianceGen Shader");
    }

    IrradianceGenPSO = RenderLayer::CreateComputePipelineState(ComputePipelineStateCreateInfo(IrradianceGenShader.Get()));
    if (!IrradianceGenPSO)
    {
        LOG_ERROR("Failed to create IrradianceGen PipelineState");
        Debug::DebugBreak();
    }
    else
    {
        IrradianceGenPSO->SetName("IrradianceGen PSO");
    }

    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/SpecularIrradianceGen.hlsl",
        "Main",
        nullptr,
        EShaderStage::ShaderStage_Compute,
        EShaderModel::ShaderModel_6_0,
        Code))
    {
        LOG_ERROR("Failed to compile SpecularIrradianceGen Shader");
        Debug::DebugBreak();
    }

    TSharedRef<ComputeShader> SpecularIrradianceGenShader = RenderLayer::CreateComputeShader(Code);
    if (!SpecularIrradianceGenShader)
    {
        LOG_ERROR("Failed to create Specular IrradianceGen Shader");
        Debug::DebugBreak();
    }
    else
    {
        SpecularIrradianceGenShader->SetName("Specular IrradianceGen Shader");
    }

    SpecularIrradianceGenPSO = RenderLayer::CreateComputePipelineState(ComputePipelineStateCreateInfo(SpecularIrradianceGenShader.Get()));
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
    CreateInfo.AddressU = ESamplerMode::SamplerMode_Wrap;
    CreateInfo.AddressV = ESamplerMode::SamplerMode_Wrap;
    CreateInfo.AddressW = ESamplerMode::SamplerMode_Wrap;
    CreateInfo.Filter   = ESamplerFilter::SamplerFilter_MinMagMipLinear;

    FrameResources.IrradianceSampler = RenderLayer::CreateSamplerState(CreateInfo);
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
}

void LightProbeRenderer::RenderSkyLightProbe(CommandList& CmdList, const SceneLightSetup& LightSetup, const FrameResources& FrameResources)
{
    const UInt32 IrradianceMapSize = static_cast<UInt32>(LightSetup.IrradianceMap->GetWidth());

    CmdList.TransitionTexture(FrameResources.Skybox.Get(), EResourceState::ResourceState_PixelShaderResource, EResourceState::ResourceState_NonPixelShaderResource);
    CmdList.TransitionTexture(LightSetup.IrradianceMap.Get(), EResourceState::ResourceState_Common, EResourceState::ResourceState_UnorderedAccess);

    CmdList.BindComputePipelineState(IrradianceGenPSO.Get());

    CmdList.BindShaderResourceViews(EShaderStage::ShaderStage_Compute, &FrameResources.SkyboxSRV, 1, 0);
    CmdList.BindUnorderedAccessViews(EShaderStage::ShaderStage_Compute, &LightSetup.IrradianceMapUAV, 1, 0);

    {
        constexpr UInt32 ThreadCount = 16;
        const UInt32 ThreadWidth  = Math::DivideByMultiple(IrradianceMapSize, ThreadCount);
        const UInt32 ThreadHeight = Math::DivideByMultiple(IrradianceMapSize, ThreadCount);
        CmdList.Dispatch(ThreadWidth, ThreadHeight, 6);
    }

    CmdList.UnorderedAccessTextureBarrier(LightSetup.IrradianceMap.Get());

    CmdList.TransitionTexture(LightSetup.IrradianceMap.Get(), EResourceState::ResourceState_UnorderedAccess, EResourceState::ResourceState_PixelShaderResource);
    CmdList.TransitionTexture(LightSetup.SpecularIrradianceMap.Get(), EResourceState::ResourceState_Common, EResourceState::ResourceState_UnorderedAccess);

    CmdList.BindShaderResourceViews(EShaderStage::ShaderStage_Compute, &FrameResources.SkyboxSRV, 1, 0);

    CmdList.BindComputePipelineState(SpecularIrradianceGenPSO.Get());

    UInt32 Width = LightSetup.SpecularIrradianceMap->GetWidth();
    Float  Roughness = 0.0f;

    const UInt32 NumMiplevels   = LightSetup.SpecularIrradianceMap->GetMipLevels();
    const Float  RoughnessDelta = 1.0f / (NumMiplevels - 1);
    for (UInt32 Mip = 0; Mip < NumMiplevels; Mip++)
    {
        CmdList.Bind32BitShaderConstants(EShaderStage::ShaderStage_Compute, &Roughness, 1);
        CmdList.BindUnorderedAccessViews(EShaderStage::ShaderStage_Compute, &LightSetup.SpecularIrradianceMapUAVs[Mip], 1, 0);

        {
            constexpr UInt32 ThreadCount = 16;
            const UInt32 ThreadWidth  = Math::DivideByMultiple(Width, ThreadCount);
            const UInt32 ThreadHeight = Math::DivideByMultiple(Width, ThreadCount);
            CmdList.Dispatch(ThreadWidth, ThreadHeight, 6);
        }

        CmdList.UnorderedAccessTextureBarrier(LightSetup.SpecularIrradianceMap.Get());

        Width = std::max<UInt32>(Width / 2, 1U);
        Roughness += RoughnessDelta;
    }

    CmdList.TransitionTexture(FrameResources.Skybox.Get(), EResourceState::ResourceState_NonPixelShaderResource, EResourceState::ResourceState_PixelShaderResource);
    CmdList.TransitionTexture(LightSetup.SpecularIrradianceMap.Get(), EResourceState::ResourceState_UnorderedAccess, EResourceState::ResourceState_PixelShaderResource);
}

Bool LightProbeRenderer::CreateSkyLightResources(SceneLightSetup& LightSetup)
{
    // Generate global irradiance (From Skybox)
    const UInt16 IrradianceSize = 32;
    LightSetup.IrradianceMap = RenderLayer::CreateTextureCube(
        nullptr,
        EFormat::Format_R16G16B16A16_Float,
        TextureUsage_Default | TextureUsage_RWTexture,
        IrradianceSize,
        1,
        1);
    if (!LightSetup.IrradianceMap)
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        LightSetup.IrradianceMap->SetName("Irradiance Map");
    }

    UnorderedAccessViewCreateInfo IrradianceUavInfo(LightSetup.IrradianceMap.Get());
    LightSetup.IrradianceMapUAV = RenderLayer::CreateUnorderedAccessView(IrradianceUavInfo);
    if (!LightSetup.IrradianceMapUAV)
    {
        Debug::DebugBreak();
        return false;
    }

    ShaderResourceViewCreateInfo IrradianceSrvInfo(LightSetup.IrradianceMap.Get());
    LightSetup.IrradianceMapSRV = RenderLayer::CreateShaderResourceView(IrradianceSrvInfo);
    if (!LightSetup.IrradianceMapSRV)
    {
        Debug::DebugBreak();
        return false;
    }

    const UInt16 SpecularIrradianceSize      = 128;
    const UInt16 SpecularIrradianceMiplevels = UInt16(std::max(std::log2(SpecularIrradianceSize), 1.0));
    LightSetup.SpecularIrradianceMap = RenderLayer::CreateTextureCube(
        nullptr,
        EFormat::Format_R16G16B16A16_Float,
        TextureUsage_Default | TextureUsage_RWTexture,
        SpecularIrradianceSize,
        SpecularIrradianceMiplevels,
        1);
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
        UnorderedAccessViewCreateInfo SpecularIrradianceUavInfo(LightSetup.SpecularIrradianceMap.Get(), EFormat::Format_R16G16B16A16_Float, MipLevel);
        TSharedRef<UnorderedAccessView> Uav = RenderLayer::CreateUnorderedAccessView(SpecularIrradianceUavInfo);
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

    ShaderResourceViewCreateInfo SpecularIrradianceSrvInfo(LightSetup.SpecularIrradianceMap.Get());
    LightSetup.SpecularIrradianceMapSRV = RenderLayer::CreateShaderResourceView(SpecularIrradianceSrvInfo);
    if (!LightSetup.SpecularIrradianceMapSRV)
    {
        Debug::DebugBreak();
        return false;
    }

    return true;
}
