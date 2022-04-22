#include "TextureFactory.h"

#include "Engine/Assets/SceneData.h"

#include "RHI/RHIInstance.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIPipeline.h"
#include "RHI/RHIShaderCompiler.h"

#include <stb_image.h>

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// TextureFactoryData

struct STextureFactoryData
{
    TSharedRef<CRHIComputePipelineState> PanoramaPSO;
    TSharedRef<CRHIComputeShader>        ComputeShader;
    CRHICommandList                      CmdList;
};

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// CTextureFactory

static STextureFactoryData GlobalFactoryData;

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
    CRHIComputePipelineStateInitializer ComputePipelineStateInitializer(GlobalFactoryData.ComputeShader.Get());
    GlobalFactoryData.PanoramaPSO = RHICreateComputePipelineState(ComputePipelineStateInitializer);
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

CRHITexture2D* CTextureFactory::LoadFromImage2D(SImage2D* InImage, ETextureFactoryFlags CreateFlags)
{
    if (!InImage || (InImage && !InImage->bIsLoaded))
    {
        return nullptr;
    }

    const uint8* Pixels = InImage->Image.Get();
    uint32  Width = InImage->Width;
    uint32  Height = InImage->Height;
    ERHIFormat Format = InImage->Format;

    CRHITexture2D* NewTexture = LoadFromMemory(Pixels, Width, Height, CreateFlags, Format);
    if (NewTexture)
    {
        // Set debug name
        NewTexture->SetName(InImage->Path.CStr());
    }

    return NewTexture;
}

CRHITexture2D* CTextureFactory::LoadFromFile(const String& Filepath, ETextureFactoryFlags CreateFlags, ERHIFormat Format)
{
    int32 Width        = 0;
    int32 Height       = 0;
    int32 ChannelCount = 0;

    // Load based on format
    TUniquePtr<uint8> Pixels;
    if (Format == ERHIFormat::R8G8B8A8_Unorm)
    {
        Pixels = TUniquePtr<uint8>(stbi_load(Filepath.CStr(), &Width, &Height, &ChannelCount, 4));
    }
    else if (Format == ERHIFormat::R8_Unorm)
    {
        Pixels = TUniquePtr<uint8>(stbi_load(Filepath.CStr(), &Width, &Height, &ChannelCount, 1));
    }
    else if (Format == ERHIFormat::R32G32B32A32_Float)
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

CRHITexture2D* CTextureFactory::LoadFromMemory(const uint8* Pixels, uint32 Width, uint32 Height, ETextureFactoryFlags CreateFlags, ERHIFormat Format)
{
    if (Format != ERHIFormat::R8_Unorm && Format != ERHIFormat::R8G8B8A8_Unorm && Format != ERHIFormat::R32G32B32A32_Float)
    {
        LOG_ERROR("[CTextureFactory]: Format not supported");
        return nullptr;
    }

    Check(Pixels != nullptr);

    const bool bGenerateMips = bool(CreateFlags & ETextureFactoryFlags::GenerateMips);

    const uint32 NumMips = bGenerateMips ? NMath::Max<uint32>(NMath::Log2(NMath::Max(Width, Height)), 1u) : 1;
    Check(NumMips != 0);

    const uint32 Stride   = GetByteStrideFromFormat(Format);
    const uint32 RowPitch = Width * Stride;
    Check(RowPitch > 0);

    const uint32 Size = RowPitch * Height;

    CRHITexture2DInitializer Initializer(Format, Width, Height, NumMips, 1, ETextureUsageFlags::AllowShaderResource, EResourceAccess::PixelShaderResource);
    Initializer.InitialSubresourceData.Emplace(Pixels, Size);

    CRHITexture2DRef Texture = RHICreateTexture2D(Initializer);
    if (!Texture)
    {
        CDebug::DebugBreak();
        return nullptr;
    }

    if (bGenerateMips)
    {
        CRHICommandList& CmdList = GlobalFactoryData.CmdList;
        CmdList.TransitionTexture(Texture.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::CopyDest);
        CmdList.GenerateMips(Texture.Get());
        CmdList.TransitionTexture(Texture.Get(), EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource);

        CRHICommandExecutionManager::Get().ExecuteCommandList(CmdList);
    }

    return Texture.ReleaseOwnership();
}

CRHITextureCube* CTextureFactory::CreateTextureCubeFromPanorma(CRHITexture2D* PanoramaSource, uint32 CubeMapSize, ETextureFactoryFlags CreateFlags, ERHIFormat Format)
{
    Check((PanoramaSource != nullptr) && bool(PanoramaSource->GetFlags()W & ETextureUsageFlags::AllowShaderResource));

    const bool bGenerateMips = bool(CreateFlags & ETextureFactoryFlags::GenerateMips);

    const uint32 NumMips = (bGenerateMips) ? NMath::Max<uint32>(NMath::Log2(CubeMapSize), 1u) : 1u;

    CRHITextureCubeInitializer CubeInitializer(Format, CubeMapSize, 1u, NumMips, 1u, ETextureUsageFlags::AllowUnorderedAccess, EResourceAccess::Common);
    CRHITextureCubeRef StagingTexture = RHICreateTextureCube(CubeInitializer);
    if (!StagingTexture)
    {
        return nullptr;
    }
    else
    {
        StagingTexture->SetName("TextureCube From Panorama StagingTexture");
    }

    CRHITextureUAVInitializer UAVInitializer(StagingTexture.Get(), Format, 0, 0, kRHIAllRemainingArraySlices);

    CRHIUnorderedAccessViewRef StagingTextureUAV = RHICreateUnorderedAccessView(UAVInitializer);
    if (!StagingTextureUAV)
    {
        return nullptr;
    }
    else
    {
        StagingTexture->SetName("TextureCube From Panorama StagingTexture UAV");
    }

    CubeInitializer = CRHITextureCubeInitializer(Format, CubeMapSize, 1u, NumMips, 1u, ETextureUsageFlags::AllowShaderResource, EResourceAccess::Common);
    CRHITextureCubeRef Texture = RHICreateTextureCube(CubeInitializer);
    if (!Texture)
    {
        return nullptr;
    }

    CRHICommandList& CmdList = GlobalFactoryData.CmdList;
    CmdList.TransitionTexture(PanoramaSource, EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource);
    CmdList.TransitionTexture(StagingTexture.Get(), EResourceAccess::Common, EResourceAccess::UnorderedAccess);

    CmdList.SetComputePipelineState(GlobalFactoryData.PanoramaPSO.Get());

    // Use structure in case we need more in the future
    struct SConstantBuffer
    {
        uint32 CubeMapSize;
    } ConstantBuffer = { CubeMapSize };

    SSetShaderConstantsInfo ShaderConstants(&ConstantBuffer, sizeof(SConstantBuffer));
    CmdList.Set32BitShaderConstants(GlobalFactoryData.ComputeShader.Get(), ShaderConstants);

    CmdList.SetUnorderedAccessView(GlobalFactoryData.ComputeShader.Get(), StagingTextureUAV.Get(), 0);
    CmdList.SetShaderResourceTexture(GlobalFactoryData.ComputeShader.Get(), PanoramaSource, 0);

    constexpr uint32 LocalWorkGroupCount = 16;
    const uint32 ThreadsX = NMath::DivideByMultiple(CubeMapSize, LocalWorkGroupCount);
    const uint32 ThreadsY = NMath::DivideByMultiple(CubeMapSize, LocalWorkGroupCount);
    CmdList.Dispatch(ThreadsX, ThreadsY, 6);

    CmdList.TransitionTexture(PanoramaSource, EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource);
    CmdList.TransitionTexture(StagingTexture.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::CopySource);
    CmdList.TransitionTexture(Texture.Get(), EResourceAccess::Common, EResourceAccess::CopyDest);

    CmdList.CopyTexture(Texture.Get(), StagingTexture.Get());

    if (bGenerateMips)
    {
        CmdList.GenerateMips(Texture.Get());
    }

    CmdList.TransitionTexture(Texture.Get(), EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource);

    CRHICommandExecutionManager::Get().ExecuteCommandList(CmdList);

    return Texture.ReleaseOwnership();
}
