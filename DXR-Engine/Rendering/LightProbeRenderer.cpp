#include "LightProbeRenderer.h"

#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/ShaderCompiler.h"

bool LightProbeRenderer::Init( LightSetup& LightSetup, FrameResources& FrameResources )
{
    if ( !CreateSkyLightResources( LightSetup ) )
    {
        return false;
    }

    TArray<uint8> Code;
    if ( !ShaderCompiler::CompileFromFile( "../DXR-Engine/Shaders/IrradianceGen.hlsl", "Main", nullptr, EShaderStage::Compute, EShaderModel::SM_6_0, Code ) )
    {
        LOG_ERROR( "Failed to compile IrradianceGen Shader" );
        Debug::DebugBreak();
    }

    IrradianceGenShader = CreateComputeShader( Code );
    if ( !IrradianceGenShader )
    {
        LOG_ERROR( "Failed to create IrradianceGen Shader" );
        Debug::DebugBreak();
    }
    else
    {
        IrradianceGenShader->SetName( "IrradianceGen Shader" );
    }

    IrradianceGenPSO = CreateComputePipelineState( ComputePipelineStateCreateInfo( IrradianceGenShader.Get() ) );
    if ( !IrradianceGenPSO )
    {
        LOG_ERROR( "Failed to create IrradianceGen PipelineState" );
        Debug::DebugBreak();
    }
    else
    {
        IrradianceGenPSO->SetName( "IrradianceGen PSO" );
    }

    if ( !ShaderCompiler::CompileFromFile( "../DXR-Engine/Shaders/SpecularIrradianceGen.hlsl", "Main", nullptr, EShaderStage::Compute, EShaderModel::SM_6_0, Code ) )
    {
        LOG_ERROR( "Failed to compile SpecularIrradianceGen Shader" );
        Debug::DebugBreak();
    }

    SpecularIrradianceGenShader = CreateComputeShader( Code );
    if ( !SpecularIrradianceGenShader )
    {
        LOG_ERROR( "Failed to create Specular IrradianceGen Shader" );
        Debug::DebugBreak();
    }
    else
    {
        SpecularIrradianceGenShader->SetName( "Specular IrradianceGen Shader" );
    }

    SpecularIrradianceGenPSO = CreateComputePipelineState( ComputePipelineStateCreateInfo( SpecularIrradianceGenShader.Get() ) );
    if ( !SpecularIrradianceGenPSO )
    {
        LOG_ERROR( "Failed to create Specular IrradianceGen PipelineState" );
        Debug::DebugBreak();
    }
    else
    {
        SpecularIrradianceGenPSO->SetName( "Specular IrradianceGen PSO" );
    }

    SamplerStateCreateInfo CreateInfo;
    CreateInfo.AddressU = ESamplerMode::Wrap;
    CreateInfo.AddressV = ESamplerMode::Wrap;
    CreateInfo.AddressW = ESamplerMode::Wrap;
    CreateInfo.Filter = ESamplerFilter::MinMagMipLinear;

    FrameResources.IrradianceSampler = CreateSamplerState( CreateInfo );
    if ( !FrameResources.IrradianceSampler )
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

void LightProbeRenderer::RenderSkyLightProbe( CommandList& CmdList, const LightSetup& LightSetup, const FrameResources& FrameResources )
{
    const uint32 IrradianceMapSize = static_cast<uint32>(LightSetup.IrradianceMap->GetSize());

    CmdList.TransitionTexture( FrameResources.Skybox.Get(), EResourceState::PixelShaderResource, EResourceState::NonPixelShaderResource );
    CmdList.TransitionTexture( LightSetup.IrradianceMap.Get(), EResourceState::Common, EResourceState::UnorderedAccess );

    CmdList.SetComputePipelineState( IrradianceGenPSO.Get() );

    ShaderResourceView* SkyboxSRV = FrameResources.Skybox->GetShaderResourceView();
    CmdList.SetShaderResourceView( IrradianceGenShader.Get(), SkyboxSRV, 0 );
    CmdList.SetUnorderedAccessView( IrradianceGenShader.Get(), LightSetup.IrradianceMapUAV.Get(), 0 );

    {
        const XMUINT3 ThreadCount = IrradianceGenShader->GetThreadGroupXYZ();
        const uint32 ThreadWidth = NMath::DivideByMultiple( IrradianceMapSize, ThreadCount.x );
        const uint32 ThreadHeight = NMath::DivideByMultiple( IrradianceMapSize, ThreadCount.y );
        CmdList.Dispatch( ThreadWidth, ThreadHeight, 6 );
    }

    CmdList.UnorderedAccessTextureBarrier( LightSetup.IrradianceMap.Get() );

    CmdList.TransitionTexture( LightSetup.IrradianceMap.Get(), EResourceState::UnorderedAccess, EResourceState::PixelShaderResource );
    CmdList.TransitionTexture( LightSetup.SpecularIrradianceMap.Get(), EResourceState::Common, EResourceState::UnorderedAccess );

    CmdList.SetShaderResourceView( IrradianceGenShader.Get(), SkyboxSRV, 0 );

    CmdList.SetComputePipelineState( SpecularIrradianceGenPSO.Get() );

    uint32 Width = LightSetup.SpecularIrradianceMap->GetSize();
    float  Roughness = 0.0f;

    const uint32 NumMiplevels = LightSetup.SpecularIrradianceMap->GetNumMips();
    const float  RoughnessDelta = 1.0f / (NumMiplevels - 1);
    for ( uint32 Mip = 0; Mip < NumMiplevels; Mip++ )
    {
        CmdList.Set32BitShaderConstants( SpecularIrradianceGenShader.Get(), &Roughness, 1 );
        CmdList.SetUnorderedAccessView( SpecularIrradianceGenShader.Get(), LightSetup.SpecularIrradianceMapUAVs[Mip].Get(), 0 );

        {
            const XMUINT3 ThreadCount = SpecularIrradianceGenShader->GetThreadGroupXYZ();
            const uint32 ThreadWidth = NMath::DivideByMultiple( Width, ThreadCount.x );
            const uint32 ThreadHeight = NMath::DivideByMultiple( Width, ThreadCount.y );
            CmdList.Dispatch( ThreadWidth, ThreadHeight, 6 );
        }

        CmdList.UnorderedAccessTextureBarrier( LightSetup.SpecularIrradianceMap.Get() );

        Width = std::max<uint32>( Width / 2, 1U );
        Roughness += RoughnessDelta;
    }

    CmdList.TransitionTexture( FrameResources.Skybox.Get(), EResourceState::NonPixelShaderResource, EResourceState::PixelShaderResource );
    CmdList.TransitionTexture( LightSetup.SpecularIrradianceMap.Get(), EResourceState::UnorderedAccess, EResourceState::PixelShaderResource );
}

bool LightProbeRenderer::CreateSkyLightResources( LightSetup& LightSetup )
{
    // Generate global irradiance (From Skybox)
    LightSetup.IrradianceMap = CreateTextureCube( LightSetup.LightProbeFormat, LightSetup.IrradianceSize, 1, TextureFlags_RWTexture, EResourceState::Common, nullptr );
    if ( !LightSetup.IrradianceMap )
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        LightSetup.IrradianceMap->SetName( "Irradiance Map" );
    }

    LightSetup.IrradianceMapUAV = CreateUnorderedAccessView( LightSetup.IrradianceMap.Get(), LightSetup.LightProbeFormat, 0 );
    if ( !LightSetup.IrradianceMapUAV )
    {
        Debug::DebugBreak();
        return false;
    }

    const uint16 SpecularIrradianceMiplevels = NMath::Max<uint16>( NMath::Log2( LightSetup.SpecularIrradianceSize ), 1u );
    LightSetup.SpecularIrradianceMap = CreateTextureCube(
        LightSetup.LightProbeFormat,
        LightSetup.SpecularIrradianceSize,
        SpecularIrradianceMiplevels,
        TextureFlags_RWTexture,
        EResourceState::Common,
        nullptr );
    if ( !LightSetup.SpecularIrradianceMap )
    {
        Debug::DebugBreak();
        return false;
    }
    else
    {
        LightSetup.SpecularIrradianceMap->SetName( "Specular Irradiance Map" );
    }

    for ( uint32 MipLevel = 0; MipLevel < SpecularIrradianceMiplevels; MipLevel++ )
    {
        TRef<UnorderedAccessView> Uav = CreateUnorderedAccessView( LightSetup.SpecularIrradianceMap.Get(), LightSetup.LightProbeFormat, MipLevel );
        if ( Uav )
        {
            LightSetup.SpecularIrradianceMapUAVs.EmplaceBack( Uav );
            LightSetup.WeakSpecularIrradianceMapUAVs.EmplaceBack( Uav.Get() );
        }
        else
        {
            Debug::DebugBreak();
            return false;
        }
    }

    return true;
}
