#include "LightProbeSceneRenderPass.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/ShaderCompiler.h"

Bool LightProbeSceneRenderPass::Init(SharedRenderPassResources& FrameResources)
{
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

    // Generate global irradiance (From Skybox)
    const UInt16 IrradianceSize = 32;
    FrameResources.IrradianceMap = RenderLayer::CreateTextureCube(
        nullptr,
        EFormat::Format_R16G16B16A16_Float,
        TextureUsage_Default | TextureUsage_RWTexture,
        IrradianceSize,
        1,
        1);
    if (FrameResources.IrradianceMap)
    {
        FrameResources.IrradianceMap->SetName("Irradiance Map");
    }
    else
    {
        return false;
    }

    FrameResources.IrradianceMapUAV = RenderLayer::CreateUnorderedAccessView(
        FrameResources.IrradianceMap.Get(),
        EFormat::Format_R16G16B16A16_Float, 0);
    if (!FrameResources.IrradianceMapUAV)
    {
        return false;
    }

    FrameResources.IrradianceMapSRV = RenderLayer::CreateShaderResourceView(
        FrameResources.IrradianceMap.Get(),
        EFormat::Format_R16G16B16A16_Float,
        0, 1);
    if (!FrameResources.IrradianceMapSRV)
    {
        return false;
    }

    // Generate global specular irradiance (From Skybox)
    const UInt16 SpecularIrradianceSize      = 128;
    const UInt16 SpecularIrradianceMiplevels = UInt16(std::max(std::log2(SpecularIrradianceSize), 1.0));
    FrameResources.SpecularIrradianceMap = RenderLayer::CreateTextureCube(
        nullptr,
        EFormat::Format_R16G16B16A16_Float,
        TextureUsage_Default | TextureUsage_RWTexture,
        SpecularIrradianceSize,
        SpecularIrradianceMiplevels,
        1);
    if (FrameResources.SpecularIrradianceMap)
    {
        FrameResources.SpecularIrradianceMap->SetName("Specular Irradiance Map");
    }
    else
    {
        Debug::DebugBreak();
        return false;
    }

    for (UInt32 MipLevel = 0; MipLevel < SpecularIrradianceMiplevels; MipLevel++)
    {
        TSharedRef<UnorderedAccessView> Uav = RenderLayer::CreateUnorderedAccessView(
            FrameResources.SpecularIrradianceMap.Get(),
            EFormat::Format_R16G16B16A16_Float,
            MipLevel);
        if (Uav)
        {
            FrameResources.SpecularIrradianceMapUAVs.EmplaceBack(Uav);
            FrameResources.WeakSpecularIrradianceMapUAVs.EmplaceBack(Uav.Get());
        }
    }

    FrameResources.SpecularIrradianceMapSRV = RenderLayer::CreateShaderResourceView(
        FrameResources.SpecularIrradianceMap.Get(),
        EFormat::Format_R16G16B16A16_Float,
        0,
        SpecularIrradianceMiplevels);
    if (!FrameResources.SpecularIrradianceMapSRV)
    {
        return false;
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

void LightProbeSceneRenderPass::Render(
    CommandList& CmdList, 
    SharedRenderPassResources& FrameResources,
    const Scene& Scene)
{
    const UInt32 IrradianceMapSize = static_cast<UInt32>(FrameResources.IrradianceMap->GetWidth());

    CmdList.TransitionTexture(
        FrameResources.Skybox.Get(),
        EResourceState::ResourceState_PixelShaderResource,
        EResourceState::ResourceState_NonPixelShaderResource);

    CmdList.TransitionTexture(
        FrameResources.IrradianceMap.Get(),
        EResourceState::ResourceState_Common,
        EResourceState::ResourceState_UnorderedAccess);

    CmdList.BindComputePipelineState(IrradianceGenPSO.Get());

    CmdList.BindShaderResourceViews(
        EShaderStage::ShaderStage_Compute,
        &FrameResources.SkyboxSRV, 1, 0);

    CmdList.BindUnorderedAccessViews(
        EShaderStage::ShaderStage_Compute,
        &FrameResources.IrradianceMapUAV, 1, 0);

    constexpr UInt32 ThreadCount = 16;
    const UInt32 ThreadWidth  = Math::DivideByMultiple(IrradianceMapSize, ThreadCount);
    const UInt32 ThreadHeight = Math::DivideByMultiple(IrradianceMapSize, ThreadCount);
    CmdList.Dispatch(ThreadWidth, ThreadHeight, 6);

    CmdList.UnorderedAccessTextureBarrier(FrameResources.IrradianceMap.Get());

    CmdList.TransitionTexture(
        FrameResources.IrradianceMap.Get(),
        EResourceState::ResourceState_UnorderedAccess,
        EResourceState::ResourceState_PixelShaderResource);

    CmdList.TransitionTexture(
        FrameResources.SpecularIrradianceMap.Get(),
        EResourceState::ResourceState_Common,
        EResourceState::ResourceState_UnorderedAccess);

    CmdList.BindShaderResourceViews(
        EShaderStage::ShaderStage_Compute,
        &FrameResources.SkyboxSRV, 1, 0);

    CmdList.BindComputePipelineState(SpecularIrradianceGenPSO.Get());

    UInt32 Width     = FrameResources.SpecularIrradianceMap->GetWidth();
    Float  Roughness = 0.0f;

    const UInt32 NumMiplevels   = FrameResources.SpecularIrradianceMap->GetMipLevels();
    const Float  RoughnessDelta = 1.0f / (NumMiplevels - 1);
    for (UInt32 Mip = 0; Mip < NumMiplevels; Mip++)
    {
        CmdList.Bind32BitShaderConstants(
            EShaderStage::ShaderStage_Compute,
            &Roughness, 1);

        CmdList.BindUnorderedAccessViews(
            EShaderStage::ShaderStage_Compute,
            &FrameResources.SpecularIrradianceMapUAVs[Mip], 1, 0);

        constexpr UInt32 ThreadCount = 16;
        const UInt32 ThreadWidth  = Math::DivideByMultiple(Width, ThreadCount);
        const UInt32 ThreadHeight = Math::DivideByMultiple(Width, ThreadCount);
        CmdList.Dispatch(ThreadWidth, ThreadHeight, 6);

        CmdList.UnorderedAccessTextureBarrier(FrameResources.SpecularIrradianceMap.Get());

        Width = std::max<UInt32>(Width / 2, 1U);
        Roughness += RoughnessDelta;
    }

    CmdList.TransitionTexture(
        FrameResources.Skybox.Get(),
        EResourceState::ResourceState_NonPixelShaderResource,
        EResourceState::ResourceState_PixelShaderResource);

    CmdList.TransitionTexture(
        FrameResources.SpecularIrradianceMap.Get(),
        EResourceState::ResourceState_UnorderedAccess,
        EResourceState::ResourceState_PixelShaderResource);
}
