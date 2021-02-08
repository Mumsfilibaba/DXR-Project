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
    CommandList    CmdList;
    TSharedRef<ComputePipelineState> PanoramaPSO;
} static GlobalFactoryData;

Bool TextureFactory::Init()
{
    // Compile and create shader
    TArray<UInt8> Code;
    if (!ShaderCompiler::CompileFromFile("../DXR-Engine/Shaders/CubeMapGen.hlsl", "Main", nullptr, EShaderStage::Compute, EShaderModel::SM_6_0, Code))
    {
        return false;
    }

    TSharedRef<ComputeShader> Shader = RenderLayer::CreateComputeShader(Code);
    if (!Shader)
    {
        return false;
    }

    // Create pipeline
    GlobalFactoryData.PanoramaPSO = RenderLayer::CreateComputePipelineState(ComputePipelineStateCreateInfo(Shader.Get()));
    if (GlobalFactoryData.PanoramaPSO)
    {
        GlobalFactoryData.PanoramaPSO->SetName("Generate CubeMap RootSignature");
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
}

Texture2D* TextureFactory::LoadFromFile(const std::string& Filepath, UInt32 CreateFlags, EFormat Format)
{
    Int32 Width        = 0;
    Int32 Height       = 0;
    Int32 ChannelCount = 0;

    // Load based on format
    TUniquePtr<Byte> Pixels;
    if (Format == EFormat::R8G8B8A8_Unorm)
    {
        Pixels = TUniquePtr<Byte>(stbi_load(Filepath.c_str(), &Width, &Height, &ChannelCount, 4));
    }
    else if (Format == EFormat::R32G32B32A32_Float)
    {
        Pixels = TUniquePtr<Byte>(reinterpret_cast<Byte*>(stbi_loadf(Filepath.c_str(), &Width, &Height, &ChannelCount, 4)));
    }
    else
    {
        LOG_ERROR("[TextureFactory]: Format not supported");
        return nullptr;
    }

    // Check if succeeded
    if (!Pixels)
    {
        LOG_ERROR("[TextureFactory]: Failed to load image '" + Filepath + "'");
        return nullptr;
    }
    else
    {
        LOG_INFO("[TextureFactory]: Loaded image '" + Filepath + "'");
    }

    return LoadFromMemory(Pixels.Get(), Width, Height, CreateFlags, Format);
}

Texture2D* TextureFactory::LoadFromMemory(const Byte* Pixels, UInt32 Width, UInt32 Height, UInt32 CreateFlags, EFormat Format)
{
    if (Format != EFormat::R8G8B8A8_Unorm && Format != EFormat::R32G32B32A32_Float)
    {
        LOG_ERROR("[TextureFactory]: Format not supported");
        return nullptr;
    }

    const Bool GenerateMips = CreateFlags & ETextureFactoryFlags::TextureFactoryFlag_GenerateMips;
    const UInt32 NumMips = GenerateMips ? UInt32(std::min(std::log2(Width), std::log2(Height))) : 1;

    VALIDATE(NumMips != 0);

    const UInt32 Stride   = (Format == EFormat::R8G8B8A8_Unorm) ? 4 : 16;
    const UInt32 RowPitch = Width * Stride;
    
    VALIDATE(RowPitch > 0);
    
    ResourceData InitalData = ResourceData(Pixels, Format, Width);
    TSharedRef<Texture2D> Texture = RenderLayer::CreateTexture2D(Format, Width, Height, NumMips, 1, TextureFlag_SRV, EResourceState::PixelShaderResource, &InitalData);
    if (!Texture)
    {
        Debug::DebugBreak();
        return nullptr;
    }

    if (GenerateMips)
    {
        CommandList& CmdList = GlobalFactoryData.CmdList;
        CmdList.Begin();
        CmdList.TransitionTexture(Texture.Get(), EResourceState::PixelShaderResource, EResourceState::CopyDest);
        CmdList.GenerateMips(Texture.Get());
        CmdList.TransitionTexture(Texture.Get(), EResourceState::CopyDest, EResourceState::PixelShaderResource);
        CmdList.End();
        gCmdListExecutor.ExecuteCommandList(CmdList);
    }

    return Texture.ReleaseOwnership();
}

TextureCube* TextureFactory::CreateTextureCubeFromPanorma(Texture2D* PanoramaSource, UInt32 CubeMapSize, UInt32 CreateFlags, EFormat Format)
{
    VALIDATE(PanoramaSource->IsSRV());

    const Bool GenerateNumMips = CreateFlags & ETextureFactoryFlags::TextureFactoryFlag_GenerateMips;
    const UInt16 NumMips = (GenerateNumMips) ? static_cast<UInt16>(std::log2(CubeMapSize)) : 1U;

    TSharedRef<TextureCube> StagingTexture = RenderLayer::CreateTextureCube(Format, CubeMapSize, NumMips, TextureFlag_UAV, EResourceState::Common, nullptr);
    if (!StagingTexture)
    {
        return nullptr;
    }
    else
    {
        StagingTexture->SetName("TextureCube From Panorama StagingTexture");
    }

    TSharedRef<UnorderedAccessView> StagingTextureUAV = RenderLayer::CreateUnorderedAccessView(StagingTexture.Get(), Format, 0);
    if (!StagingTextureUAV)
    {
        return nullptr;
    }
    else
    {
        StagingTexture->SetName("TextureCube From Panorama StagingTexture UAV");
    }

    TSharedRef<TextureCube> Texture = RenderLayer::CreateTextureCube(Format, CubeMapSize, NumMips, TextureFlag_SRV, EResourceState::Common, nullptr);
    if (!Texture)
    {
        return nullptr;
    }

    CommandList& CmdList = GlobalFactoryData.CmdList;
    CmdList.Begin();
    
    CmdList.TransitionTexture(PanoramaSource, EResourceState::PixelShaderResource, EResourceState::NonPixelShaderResource);
    CmdList.TransitionTexture(StagingTexture.Get(), EResourceState::Common, EResourceState::UnorderedAccess);

    CmdList.BindComputePipelineState(GlobalFactoryData.PanoramaPSO.Get());

    struct ConstantBuffer
    {
        UInt32 CubeMapSize;
    } CB0;
    CB0.CubeMapSize = CubeMapSize;

    CmdList.Bind32BitShaderConstants(EShaderStage::Compute, &CB0, 1);
    CmdList.BindUnorderedAccessViews(EShaderStage::Compute, &StagingTextureUAV, 1, 0);

    ShaderResourceView* PanoramaSourceView = PanoramaSource->GetShaderResourceView();
    CmdList.BindShaderResourceViews(EShaderStage::Compute, &PanoramaSourceView, 1, 0);

    constexpr UInt32 LocalWorkGroupCount = 16;
    const UInt32 ThreadsX = Math::DivideByMultiple(CubeMapSize, LocalWorkGroupCount);
    const UInt32 ThreadsY = Math::DivideByMultiple(CubeMapSize, LocalWorkGroupCount);
    CmdList.Dispatch(ThreadsX, ThreadsY, 6);

    CmdList.TransitionTexture(PanoramaSource, EResourceState::NonPixelShaderResource, EResourceState::PixelShaderResource);
    CmdList.TransitionTexture(StagingTexture.Get(), EResourceState::UnorderedAccess, EResourceState::CopySource);
    CmdList.TransitionTexture(Texture.Get(), EResourceState::Common, EResourceState::CopyDest);

    CmdList.CopyTexture(Texture.Get(), StagingTexture.Get());

    if (GenerateNumMips)
    {
        CmdList.GenerateMips(Texture.Get());
    }

    CmdList.TransitionTexture(Texture.Get(), EResourceState::CopyDest, EResourceState::PixelShaderResource);
    
    CmdList.DestroyResource(StagingTexture.Get());
    CmdList.DestroyResource(StagingTextureUAV.Get());
    CmdList.DestroyResource(Texture.Get());

    CmdList.End();
    gCmdListExecutor.ExecuteCommandList(CmdList);

    return Texture.ReleaseOwnership();
}
