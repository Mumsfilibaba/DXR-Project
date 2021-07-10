#include "TextureFactory.h"

#include "RenderLayer/CommandList.h"
#include "RenderLayer/PipelineState.h"
#include "RenderLayer/RenderLayer.h"
#include "RenderLayer/ShaderCompiler.h"

#ifdef min
#undef min
#endif

#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

struct TextureFactoryData
{
    TRef<ComputePipelineState> PanoramaPSO;
    TRef<ComputeShader>        ComputeShader;
    CommandList CmdList;
};

static TextureFactoryData GlobalFactoryData;

bool TextureFactory::Init()
{
    // Compile and create shader
    TArray<uint8> Code;
    if ( !ShaderCompiler::CompileFromFile( "../DXR-Engine/Shaders/CubeMapGen.hlsl", "Main", nullptr, EShaderStage::Compute, EShaderModel::SM_6_0, Code ) )
    {
        return false;
    }

    GlobalFactoryData.ComputeShader = CreateComputeShader( Code );
    if ( !GlobalFactoryData.ComputeShader )
    {
        return false;
    }

    // Create pipeline
    GlobalFactoryData.PanoramaPSO = CreateComputePipelineState( ComputePipelineStateCreateInfo( GlobalFactoryData.ComputeShader.Get() ) );
    if ( GlobalFactoryData.PanoramaPSO )
    {
        GlobalFactoryData.PanoramaPSO->SetName( "Generate CubeMap RootSignature" );
        return true;
    }
    else
    {
        return false;
    }
}

void TextureFactory::Release()
{
    GlobalFactoryData.PanoramaPSO.Reset();
    GlobalFactoryData.ComputeShader.Reset();
}

Texture2D* TextureFactory::LoadFromFile( const std::string& Filepath, uint32 CreateFlags, EFormat Format )
{
    int32 Width = 0;
    int32 Height = 0;
    int32 ChannelCount = 0;

    // Load based on format
    TUniquePtr<uint8> Pixels;
    if ( Format == EFormat::R8G8B8A8_Unorm )
    {
        Pixels = TUniquePtr<uint8>( stbi_load( Filepath.c_str(), &Width, &Height, &ChannelCount, 4 ) );
    }
    else if ( Format == EFormat::R8_Unorm )
    {
        Pixels = TUniquePtr<uint8>( stbi_load( Filepath.c_str(), &Width, &Height, &ChannelCount, 1 ) );
    }
    else if ( Format == EFormat::R32G32B32A32_Float )
    {
        Pixels = TUniquePtr<uint8>( reinterpret_cast<uint8*>(stbi_loadf( Filepath.c_str(), &Width, &Height, &ChannelCount, 4 )) );
    }
    else
    {
        LOG_ERROR( "[TextureFactory]: Format not supported" );
        return nullptr;
    }

    // Check if succeeded
    if ( !Pixels )
    {
        LOG_ERROR( "[TextureFactory]: Failed to load image '" + Filepath + "'" );
        return nullptr;
    }
    else
    {
        LOG_INFO( "[TextureFactory]: Loaded image '" + Filepath + "'" );
    }

    return LoadFromMemory( Pixels.Get(), Width, Height, CreateFlags, Format );
}

Texture2D* TextureFactory::LoadFromMemory( const uint8* Pixels, uint32 Width, uint32 Height, uint32 CreateFlags, EFormat Format )
{
    if ( Format != EFormat::R8_Unorm && Format != EFormat::R8G8B8A8_Unorm && Format != EFormat::R32G32B32A32_Float )
    {
        LOG_ERROR( "[TextureFactory]: Format not supported" );
        return nullptr;
    }

    const bool GenerateMips = CreateFlags & ETextureFactoryFlags::TextureFactoryFlag_GenerateMips;
    const uint32 NumMips = GenerateMips ? Math::Max<uint32>( Math::Log2( Math::Max( Width, Height ) ), 1u ) : 1;

    Assert( NumMips != 0 );

    const uint32 Stride = GetByteStrideFromFormat( Format );
    const uint32 RowPitch = Width * Stride;

    Assert( RowPitch > 0 );

    ResourceData InitalData = ResourceData( Pixels, Format, Width );
    TRef<Texture2D> Texture = CreateTexture2D( Format, Width, Height, NumMips, 1, TextureFlag_SRV, EResourceState::PixelShaderResource, &InitalData );
    if ( !Texture )
    {
        Debug::DebugBreak();
        return nullptr;
    }

    if ( GenerateMips )
    {
        CommandList& CmdList = GlobalFactoryData.CmdList;
        CmdList.TransitionTexture( Texture.Get(), EResourceState::PixelShaderResource, EResourceState::CopyDest );
        CmdList.GenerateMips( Texture.Get() );
        CmdList.TransitionTexture( Texture.Get(), EResourceState::CopyDest, EResourceState::PixelShaderResource );
        GCmdListExecutor.ExecuteCommandList( CmdList );
    }

    return Texture.ReleaseOwnership();
}

TextureCube* TextureFactory::CreateTextureCubeFromPanorma( Texture2D* PanoramaSource, uint32 CubeMapSize, uint32 CreateFlags, EFormat Format )
{
    Assert( PanoramaSource->IsSRV() );

    const bool GenerateNumMips = CreateFlags & ETextureFactoryFlags::TextureFactoryFlag_GenerateMips;
    const uint32 NumMips = (GenerateNumMips) ? Math::Max<uint32>( Math::Log2( CubeMapSize ), 1u ) : 1u;

    TRef<TextureCube> StagingTexture = CreateTextureCube( Format, CubeMapSize, NumMips, TextureFlag_UAV, EResourceState::Common, nullptr );
    if ( !StagingTexture )
    {
        return nullptr;
    }
    else
    {
        StagingTexture->SetName( "TextureCube From Panorama StagingTexture" );
    }

    TRef<UnorderedAccessView> StagingTextureUAV = CreateUnorderedAccessView( StagingTexture.Get(), Format, 0 );
    if ( !StagingTextureUAV )
    {
        return nullptr;
    }
    else
    {
        StagingTexture->SetName( "TextureCube From Panorama StagingTexture UAV" );
    }

    TRef<TextureCube> Texture = CreateTextureCube( Format, CubeMapSize, NumMips, TextureFlag_SRV, EResourceState::Common, nullptr );
    if ( !Texture )
    {
        return nullptr;
    }

    CommandList& CmdList = GlobalFactoryData.CmdList;

    CmdList.TransitionTexture( PanoramaSource, EResourceState::PixelShaderResource, EResourceState::NonPixelShaderResource );
    CmdList.TransitionTexture( StagingTexture.Get(), EResourceState::Common, EResourceState::UnorderedAccess );

    CmdList.SetComputePipelineState( GlobalFactoryData.PanoramaPSO.Get() );

    struct ConstantBuffer
    {
        uint32 CubeMapSize;
    } CB0;
    CB0.CubeMapSize = CubeMapSize;

    CmdList.Set32BitShaderConstants( GlobalFactoryData.ComputeShader.Get(), &CB0, 1 );
    CmdList.SetUnorderedAccessView( GlobalFactoryData.ComputeShader.Get(), StagingTextureUAV.Get(), 0 );

    ShaderResourceView* PanoramaSourceView = PanoramaSource->GetShaderResourceView();
    CmdList.SetShaderResourceView( GlobalFactoryData.ComputeShader.Get(), PanoramaSourceView, 0 );

    constexpr uint32 LocalWorkGroupCount = 16;
    const uint32 ThreadsX = Math::DivideByMultiple( CubeMapSize, LocalWorkGroupCount );
    const uint32 ThreadsY = Math::DivideByMultiple( CubeMapSize, LocalWorkGroupCount );
    CmdList.Dispatch( ThreadsX, ThreadsY, 6 );

    CmdList.TransitionTexture( PanoramaSource, EResourceState::NonPixelShaderResource, EResourceState::PixelShaderResource );
    CmdList.TransitionTexture( StagingTexture.Get(), EResourceState::UnorderedAccess, EResourceState::CopySource );
    CmdList.TransitionTexture( Texture.Get(), EResourceState::Common, EResourceState::CopyDest );

    CmdList.CopyTexture( Texture.Get(), StagingTexture.Get() );

    if ( GenerateNumMips )
    {
        CmdList.GenerateMips( Texture.Get() );
    }

    CmdList.TransitionTexture( Texture.Get(), EResourceState::CopyDest, EResourceState::PixelShaderResource );

    GCmdListExecutor.ExecuteCommandList( CmdList );

    return Texture.ReleaseOwnership();
}
