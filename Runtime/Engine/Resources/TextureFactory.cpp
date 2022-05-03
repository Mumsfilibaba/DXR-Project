#include "TextureFactory.h"

#include "Engine/Assets/SceneData.h"

#include "RHI/RHICoreInterface.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIPipelineState.h"
#include "RHI/RHIShaderCompiler.h"

#include <stb_image.h>

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// TextureFactoryData

struct TextureFactoryData
{
    TSharedRef<CRHIComputePipelineState> PanoramaPSO;
    TSharedRef<CRHIComputeShader>        ComputeShader;
    CRHICommandList CmdList;
};

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// CTextureFactory

static TextureFactoryData GlobalFactoryData;

bool CTextureFactory::Init()
{
    // Compile and create shader
    TArray<uint8> Code;
    if (!CRHIShaderCompiler::CompileFromFile("../Runtime/Shaders/CubeMapGen.hlsl", "Main", nullptr, EShaderStage::Compute, EShaderModel::SM_6_0, Code))
    {
        return false;
    }

    GlobalFactoryData.ComputeShader = RHICreateComputeShader(Code);
    if (!GlobalFactoryData.ComputeShader)
    {
        return false;
    }

    // Create pipeline
    GlobalFactoryData.PanoramaPSO = RHICreateComputePipelineState(CRHIComputePipelineStateInitializer(GlobalFactoryData.ComputeShader.Get()));
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

void CTextureFactory::Release()
{
    GlobalFactoryData.PanoramaPSO.Reset();
    GlobalFactoryData.ComputeShader.Reset();
}

CRHITexture2D* CTextureFactory::LoadFromImage2D(SImage2D* InImage, uint32 CreateFlags)
{
    if (!InImage || (InImage && !InImage->bIsLoaded))
    {
        return nullptr;
    }

    const uint8* Pixels = InImage->Image.Get();
    uint32  Width = InImage->Width;
    uint32  Height = InImage->Height;
    EFormat Format = InImage->Format;

    CRHITexture2D* NewTexture = LoadFromMemory(Pixels, Width, Height, CreateFlags, Format);
    if (NewTexture)
    {
        // Set debug name
        NewTexture->SetName(InImage->Path.CStr());
    }

    return NewTexture;
}

CRHITexture2D* CTextureFactory::LoadFromFile(const String& Filepath, uint32 CreateFlags, EFormat Format)
{
    int32 Width = 0;
    int32 Height = 0;
    int32 ChannelCount = 0;

    // Load based on format
    TUniquePtr<uint8> Pixels;
    if (Format == EFormat::R8G8B8A8_Unorm)
    {
        Pixels = TUniquePtr<uint8>(stbi_load(Filepath.CStr(), &Width, &Height, &ChannelCount, 4));
    }
    else if (Format == EFormat::R8_Unorm)
    {
        Pixels = TUniquePtr<uint8>(stbi_load(Filepath.CStr(), &Width, &Height, &ChannelCount, 1));
    }
    else if (Format == EFormat::R32G32B32A32_Float)
    {
        Pixels = TUniquePtr<uint8>(reinterpret_cast<uint8*>(stbi_loadf(Filepath.CStr(), &Width, &Height, &ChannelCount, 4)));
    }
    else
    {
        LOG_ERROR("[CTextureFactory]: Format not supported");
        return nullptr;
    }

    // Check if succeeded
    if (!Pixels)
    {
        LOG_ERROR("[CTextureFactory]: Failed to load image '" + Filepath + "'");
        return nullptr;
    }
    else
    {
        LOG_INFO("[CTextureFactory]: Loaded image '" + Filepath + "'");
    }

    return LoadFromMemory(Pixels.Get(), Width, Height, CreateFlags, Format);
}

CRHITexture2D* CTextureFactory::LoadFromMemory(const uint8* Pixels, uint32 Width, uint32 Height, uint32 CreateFlags, EFormat Format)
{
    if (Format != EFormat::R8_Unorm && Format != EFormat::R8G8B8A8_Unorm && Format != EFormat::R32G32B32A32_Float)
    {
        LOG_ERROR("[CTextureFactory]: Format not supported");
        return nullptr;
    }

    Assert(Pixels != nullptr);

    const bool GenerateMips = CreateFlags & ETextureFactoryFlags::TextureFactoryFlag_GenerateMips;
    const uint32 NumMips = GenerateMips ? NMath::Max<uint32>(NMath::Log2(NMath::Max(Width, Height)), 1u) : 1;

    Assert(NumMips != 0);

    const uint32 Stride   = GetByteStrideFromFormat(Format);
    const uint32 RowPitch = Width * Stride;

    Assert(RowPitch > 0);

    CRHITextureDataInitializer InitalData(Pixels, Format, Width, Height);

    CRHITexture2DInitializer Initializer(Format, Width, Height, NumMips, 1, ETextureUsageFlags::AllowSRV, EResourceAccess::PixelShaderResource, &InitalData);
    TSharedRef<CRHITexture2D> Texture = RHICreateTexture2D(Initializer);
    if (!Texture)
    {
        CDebug::DebugBreak();
        return nullptr;
    }

    if (GenerateMips)
    {
        CRHICommandList& CmdList = GlobalFactoryData.CmdList;
        CmdList.TransitionTexture(Texture.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::CopyDest);
        CmdList.GenerateMips(Texture.Get());
        CmdList.TransitionTexture(Texture.Get(), EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource);

        CRHICommandQueue::Get().ExecuteCommandList(CmdList);
    }

    return Texture.ReleaseOwnership();
}

CRHITextureCube* CTextureFactory::CreateTextureCubeFromPanorma(CRHITexture2D* PanoramaSource, uint32 CubeMapSize, uint32 CreateFlags, EFormat Format)
{
    Assert((PanoramaSource->GetFlags() & ETextureUsageFlags::AllowSRV) != ETextureUsageFlags::None);

    const bool GenerateNumMips = CreateFlags & ETextureFactoryFlags::TextureFactoryFlag_GenerateMips;
    const uint32 NumMips = (GenerateNumMips) ? NMath::Max<uint32>(NMath::Log2(CubeMapSize), 1u) : 1u;

    CRHITextureCubeInitializer Initializer(Format, CubeMapSize, NumMips, 1, ETextureUsageFlags::AllowUAV, EResourceAccess::Common);

    TSharedRef<CRHITextureCube> StagingTexture = RHICreateTextureCube(Initializer);
    if (!StagingTexture)
    {
        return nullptr;
    }
    else
    {
        StagingTexture->SetName("TextureCube From Panorama StagingTexture");
    }

    TSharedRef<CRHIUnorderedAccessView> StagingTextureUAV = RHICreateUnorderedAccessView(StagingTexture.Get(), Format, 0);
    if (!StagingTextureUAV)
    {
        return nullptr;
    }
    else
    {
        StagingTexture->SetName("TextureCube From Panorama StagingTexture UAV");
    }

    Initializer.UsageFlags = ETextureUsageFlags::AllowSRV;

    TSharedRef<CRHITextureCube> Texture = RHICreateTextureCube(Initializer);
    if (!Texture)
    {
        return nullptr;
    }

    CRHICommandList& CmdList = GlobalFactoryData.CmdList;

    CmdList.TransitionTexture(PanoramaSource, EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource);
    CmdList.TransitionTexture(StagingTexture.Get(), EResourceAccess::Common, EResourceAccess::UnorderedAccess);

    CmdList.SetComputePipelineState(GlobalFactoryData.PanoramaPSO.Get());

    struct ConstantBuffer
    {
        uint32 CubeMapSize;
    } CB0;
    CB0.CubeMapSize = CubeMapSize;

    CmdList.Set32BitShaderConstants(GlobalFactoryData.ComputeShader.Get(), &CB0, 1);
    CmdList.SetUnorderedAccessView(GlobalFactoryData.ComputeShader.Get(), StagingTextureUAV.Get(), 0);

    CRHIShaderResourceView* PanoramaSourceView = PanoramaSource->GetDefaultShaderResourceView();
    CmdList.SetShaderResourceView(GlobalFactoryData.ComputeShader.Get(), PanoramaSourceView, 0);

    constexpr uint32 LocalWorkGroupCount = 16;
    const uint32 ThreadsX = NMath::DivideByMultiple(CubeMapSize, LocalWorkGroupCount);
    const uint32 ThreadsY = NMath::DivideByMultiple(CubeMapSize, LocalWorkGroupCount);
    CmdList.Dispatch(ThreadsX, ThreadsY, 6);

    CmdList.TransitionTexture(PanoramaSource, EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource);
    CmdList.TransitionTexture(StagingTexture.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::CopySource);
    CmdList.TransitionTexture(Texture.Get(), EResourceAccess::Common, EResourceAccess::CopyDest);

    CmdList.CopyTexture(Texture.Get(), StagingTexture.Get());

    if (GenerateNumMips)
    {
        CmdList.GenerateMips(Texture.Get());
    }

    CmdList.TransitionTexture(Texture.Get(), EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource);

    CRHICommandQueue::Get().ExecuteCommandList(CmdList);

    return Texture.ReleaseOwnership();
}
