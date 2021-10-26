#include "ScreenSpaceOcclusionRenderer.h"

#include "RHI/RHICore.h"
#include "RHI/RHIShaderCompiler.h"

#include "Core/Debug/FrameProfiler.h"
#include "Core/Debug/Console/ConsoleManager.h"
#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"

#include <random>

TConsoleVariable<float> GSSAORadius( 0.4f );
TConsoleVariable<float> GSSAOBias( 0.025f );
TConsoleVariable<int32> GSSAOKernelSize( 32 );

bool CScreenSpaceOcclusionRenderer::Init( SFrameResources& FrameResources )
{
    INIT_CONSOLE_VARIABLE( "r.SSAOKernelSize", &GSSAOKernelSize );
    INIT_CONSOLE_VARIABLE( "r.SSAOBias", &GSSAOBias );
    INIT_CONSOLE_VARIABLE( "r.SSAORadius", &GSSAORadius );

    if ( !CreateRenderTarget( FrameResources ) )
    {
        return false;
    }

    TArray<uint8> ShaderCode;
    if ( !CRHIShaderCompiler::CompileFromFile( "../DXR-Engine/Shaders/SSAO.hlsl", "Main", nullptr, EShaderStage::Compute, EShaderModel::SM_6_0, ShaderCode ) )
    {
        CDebug::DebugBreak();
        return false;
    }

    SSAOShader = RHICreateComputeShader( ShaderCode );
    if ( !SSAOShader )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        SSAOShader->SetName( "SSAO Shader" );
    }

    SComputePipelineStateCreateInfo PipelineStateInfo;
    PipelineStateInfo.Shader = SSAOShader.Get();

    PipelineState = RHICreateComputePipelineState( PipelineStateInfo );
    if ( !PipelineState )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        PipelineState->SetName( "SSAO PipelineState" );
    }

    // Generate SSAO Kernel
    std::uniform_real_distribution<float> RandomFloats( 0.0f, 1.0f );
    std::default_random_engine Generator;

    CVector3 Normal = CVector3( 0.0f, 0.0f, 1.0f );

    TArray<CVector3> SSAOKernel;
    for ( uint32 i = 0; i < 512 && SSAOKernel.Size() < 64; i++ )
    {
        CVector3 Sample = CVector3( RandomFloats( Generator ) * 2.0f - 1.0f, RandomFloats( Generator ) * 2.0f - 1.0f, RandomFloats( Generator ) );
        Sample.Normalize();

        float Dot = Sample.DotProduct( Normal );
        if ( NMath::Abs( Dot ) > 0.95f )
        {
            continue;
        }

        Sample *= RandomFloats( Generator );

        float Scale = float( i ) / 64.0f;
        Scale = NMath::Lerp( 0.1f, 1.0f, Scale * Scale );
        Sample *= Scale;

        SSAOKernel.Emplace( Sample );
    }

    // Generate noise
    TArray<SFloat16> SSAONoise;
    for ( uint32 i = 0; i < 16; i++ )
    {
        const float x = RandomFloats( Generator ) * 2.0f - 1.0f;
        const float y = RandomFloats( Generator ) * 2.0f - 1.0f;
        SSAONoise.Emplace( x );
        SSAONoise.Emplace( y );
        SSAONoise.Emplace( 0.0f );
        SSAONoise.Emplace( 0.0f );
    }

    SSAONoiseTex = RHICreateTexture2D( EFormat::R16G16B16A16_Float, 4, 4, 1, 1, TextureFlag_SRV, EResourceState::NonPixelShaderResource, nullptr );
    if ( !SSAONoiseTex )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        SSAONoiseTex->SetName( "SSAO Noise Texture" );
    }

    CRHICommandList CmdList;

    CmdList.TransitionTexture( FrameResources.SSAOBuffer.Get(), EResourceState::Common, EResourceState::NonPixelShaderResource );
    CmdList.TransitionTexture( SSAONoiseTex.Get(), EResourceState::NonPixelShaderResource, EResourceState::CopyDest );

    CmdList.UpdateTexture2D( SSAONoiseTex.Get(), 4, 4, 0, SSAONoise.Data() );

    CmdList.TransitionTexture( SSAONoiseTex.Get(), EResourceState::CopyDest, EResourceState::NonPixelShaderResource );

    CRHICommandQueue::Get().ExecuteCommandList( CmdList );

    const uint32 Stride = sizeof( CVector3 );
    SResourceData SSAOSampleData( SSAOKernel.Data(), SSAOKernel.SizeInBytes() );
    SSAOSamples = RHICreateStructuredBuffer( Stride, SSAOKernel.Size(), BufferFlag_SRV | BufferFlag_Default, EResourceState::Common, &SSAOSampleData );
    if ( !SSAOSamples )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        SSAOSamples->SetName( "SSAO Samples" );
    }

    SSAOSamplesSRV = RHICreateShaderResourceView( SSAOSamples.Get(), 0, SSAOKernel.Size() );
    if ( !SSAOSamplesSRV )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        SSAOSamplesSRV->SetName( "SSAO Samples SRV" );
    }

    TArray<SShaderDefine> Defines;
    Defines.Emplace( "HORIZONTAL_PASS", "1" );

    // Load shader
    if ( !CRHIShaderCompiler::CompileFromFile( "../DXR-Engine/Shaders/Blur.hlsl", "Main", &Defines, EShaderStage::Compute, EShaderModel::SM_6_0, ShaderCode ) )
    {
        CDebug::DebugBreak();
        return false;
    }

    BlurHorizontalShader = RHICreateComputeShader( ShaderCode );
    if ( !BlurHorizontalShader )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        BlurHorizontalShader->SetName( "SSAO Horizontal Blur Shader" );
    }

    SComputePipelineStateCreateInfo PSOProperties;
    PSOProperties.Shader = BlurHorizontalShader.Get();

    BlurHorizontalPSO = RHICreateComputePipelineState( PSOProperties );
    if ( !BlurHorizontalPSO )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        BlurHorizontalPSO->SetName( "SSAO Horizontal Blur PSO" );
    }

    Defines.Clear();
    Defines.Emplace( "VERTICAL_PASS", "1" );

    if ( !CRHIShaderCompiler::CompileFromFile( "../DXR-Engine/Shaders/Blur.hlsl", "Main", &Defines, EShaderStage::Compute, EShaderModel::SM_6_0, ShaderCode ) )
    {
        CDebug::DebugBreak();
        return false;
    }

    BlurVerticalShader = RHICreateComputeShader( ShaderCode );
    if ( !BlurVerticalShader )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        BlurVerticalShader->SetName( "SSAO Vertcial Blur Shader" );
    }

    PSOProperties.Shader = BlurVerticalShader.Get();

    BlurVerticalPSO = RHICreateComputePipelineState( PSOProperties );
    if ( !BlurVerticalPSO )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        BlurVerticalPSO->SetName( "SSAO Vertical Blur PSO" );
    }

    return true;
}

void CScreenSpaceOcclusionRenderer::Release()
{
    PipelineState.Reset();
    BlurHorizontalPSO.Reset();
    BlurVerticalPSO.Reset();
    SSAOSamples.Reset();
    SSAOSamplesSRV.Reset();
    SSAONoiseTex.Reset();
    SSAOShader.Reset();
    BlurHorizontalShader.Reset();
    BlurVerticalShader.Reset();
}

bool CScreenSpaceOcclusionRenderer::ResizeResources( SFrameResources& FrameResources )
{
    return CreateRenderTarget( FrameResources );
}

void CScreenSpaceOcclusionRenderer::Render( CRHICommandList& CmdList, SFrameResources& FrameResources )
{
    INSERT_DEBUG_CMDLIST_MARKER( CmdList, "Begin SSAO" );

    TRACE_SCOPE( "SSAO" );

    CmdList.SetComputePipelineState( PipelineState.Get() );

    struct SSAOSettings
    {
        CVector2 ScreenSize;
        CVector2 NoiseSize;
        float    Radius;
        float    Bias;
        int32    KernelSize;
    } SSAOSettings;

    const uint32 Width = FrameResources.SSAOBuffer->GetWidth();
    const uint32 Height = FrameResources.SSAOBuffer->GetHeight();
    SSAOSettings.ScreenSize = CVector2( float( Width ), float( Height ) );
    SSAOSettings.NoiseSize = CVector2( 4.0f, 4.0f );
    SSAOSettings.Radius = GSSAORadius.GetFloat();
    SSAOSettings.KernelSize = GSSAOKernelSize.GetInt();
    SSAOSettings.Bias = GSSAOBias.GetFloat();

    CmdList.SetConstantBuffer( SSAOShader.Get(), FrameResources.CameraBuffer.Get(), 0 );

    FrameResources.DebugTextures.Emplace(
        MakeSharedRef<CRHIShaderResourceView>( SSAONoiseTex->GetShaderResourceView() ),
        SSAONoiseTex,
        EResourceState::NonPixelShaderResource,
        EResourceState::NonPixelShaderResource );

    CmdList.SetShaderResourceView( SSAOShader.Get(), FrameResources.GBuffer[GBUFFER_VIEW_NORMAL_INDEX]->GetShaderResourceView(), 0 );
    CmdList.SetShaderResourceView( SSAOShader.Get(), FrameResources.GBuffer[GBUFFER_DEPTH_INDEX]->GetShaderResourceView(), 1 );
    CmdList.SetShaderResourceView( SSAOShader.Get(), SSAONoiseTex->GetShaderResourceView(), 2 );
    CmdList.SetShaderResourceView( SSAOShader.Get(), SSAOSamplesSRV.Get(), 3 );

    CmdList.SetSamplerState( SSAOShader.Get(), FrameResources.GBufferSampler.Get(), 0 );

    CRHIUnorderedAccessView* SSAOBufferUAV = FrameResources.SSAOBuffer->GetUnorderedAccessView();
    CmdList.SetUnorderedAccessView( SSAOShader.Get(), SSAOBufferUAV, 0 );
    CmdList.Set32BitShaderConstants( SSAOShader.Get(), &SSAOSettings, 7 );

    constexpr uint32 ThreadCount = 16;
    const uint32 DispatchWidth = NMath::DivideByMultiple<uint32>( Width, ThreadCount );
    const uint32 DispatchHeight = NMath::DivideByMultiple<uint32>( Height, ThreadCount );
    CmdList.Dispatch( DispatchWidth, DispatchHeight, 1 );

    CmdList.UnorderedAccessTextureBarrier( FrameResources.SSAOBuffer.Get() );

    CmdList.SetComputePipelineState( BlurHorizontalPSO.Get() );

    CmdList.Set32BitShaderConstants( BlurHorizontalShader.Get(), &SSAOSettings.ScreenSize, 2 );

    CmdList.Dispatch( DispatchWidth, DispatchHeight, 1 );

    CmdList.UnorderedAccessTextureBarrier( FrameResources.SSAOBuffer.Get() );

    CmdList.SetComputePipelineState( BlurVerticalPSO.Get() );

    CmdList.Set32BitShaderConstants( BlurVerticalShader.Get(), &SSAOSettings.ScreenSize, 2 );

    CmdList.Dispatch( DispatchWidth, DispatchHeight, 1 );

    CmdList.UnorderedAccessTextureBarrier( FrameResources.SSAOBuffer.Get() );

    INSERT_DEBUG_CMDLIST_MARKER( CmdList, "End SSAO" );
}

bool CScreenSpaceOcclusionRenderer::CreateRenderTarget( SFrameResources& FrameResources )
{
    const uint32 Width = FrameResources.MainWindowViewport->GetWidth();
    const uint32 Height = FrameResources.MainWindowViewport->GetHeight();
    const uint32 Flags = TextureFlags_RWTexture;

    FrameResources.SSAOBuffer = RHICreateTexture2D( FrameResources.SSAOBufferFormat, Width, Height, 1, 1, Flags, EResourceState::Common, nullptr );
    if ( !FrameResources.SSAOBuffer )
    {
        CDebug::DebugBreak();
        return false;
    }
    else
    {
        FrameResources.SSAOBuffer->SetName( "SSAO Buffer" );
    }

    return true;
}
