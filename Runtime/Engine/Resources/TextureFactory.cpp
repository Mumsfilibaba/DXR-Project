#include "TextureFactory.h"

#include "Engine/Assets/SceneData.h"

#include "RHI/RHIInterface.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIPipelineState.h"
#include "RHI/RHIShaderCompiler.h"

#include <stb_image.h>

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// TextureFactoryData

struct TextureFactoryData
{
    FRHIComputePipelineStateRef PanoramaPSO;
    FRHIComputeShaderRef        ComputeShader;
};

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// FTextureFactory

static TextureFactoryData GlobalFactoryData;

bool FTextureFactory::Init()
{
    // Compile and create shader
    TArray<uint8> Code;

    FRHIShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_0, EShaderStage::Compute);
    if (!FRHIShaderCompiler::Get().CompileFromFile("Shaders/CubeMapGen.hlsl", CompileInfo, Code))
    {
        return false;
    }

    GlobalFactoryData.ComputeShader = RHICreateComputeShader(Code);
    if (!GlobalFactoryData.ComputeShader)
    {
        return false;
    }

    // Create pipeline
    GlobalFactoryData.PanoramaPSO = RHICreateComputePipelineState(FRHIComputePipelineStateInitializer(GlobalFactoryData.ComputeShader.Get()));
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

void FTextureFactory::Release()
{
    GlobalFactoryData.PanoramaPSO.Reset();
    GlobalFactoryData.ComputeShader.Reset();
}

FRHITexture2D* FTextureFactory::LoadFromImage2D(FImage2D* InImage, uint32 CreateFlags)
{
    if (!InImage || (InImage && !InImage->bIsLoaded))
    {
        return nullptr;
    }

    const uint8* Pixels = InImage->Image.Get();
    uint32  Width  = InImage->Width;
    uint32  Height = InImage->Height;
    EFormat Format = InImage->Format;

    FRHITexture2D* NewTexture = LoadFromMemory(Pixels, Width, Height, CreateFlags, Format);
    if (NewTexture)
    {
        // Set debug name
        NewTexture->SetName(InImage->Path.GetCString());
    }

    return NewTexture;
}

FRHITexture2D* FTextureFactory::LoadFromFile(const FString& Filepath, uint32 CreateFlags, EFormat Format)
{
    int32 Width        = 0;
    int32 Height       = 0;
    int32 ChannelCount = 0;

    // Load based on format
    TUniquePtr<uint8> Pixels;
    if (Format == EFormat::R8G8B8A8_Unorm)
    {
        Pixels = TUniquePtr<uint8>(stbi_load(Filepath.GetCString(), &Width, &Height, &ChannelCount, 4));
    }
    else if (Format == EFormat::R8_Unorm)
    {
        Pixels = TUniquePtr<uint8>(stbi_load(Filepath.GetCString(), &Width, &Height, &ChannelCount, 1));
    }
    else if (Format == EFormat::R32G32B32A32_Float)
    {
        Pixels = TUniquePtr<uint8>(reinterpret_cast<uint8*>(stbi_loadf(Filepath.GetCString(), &Width, &Height, &ChannelCount, 4)));
    }
    else
    {
        LOG_ERROR("[FTextureFactory]: Format not supported");
        return nullptr;
    }

    // Check if succeeded
    if (!Pixels)
    {
        LOG_ERROR("[FTextureFactory]: Failed to load image '%s'", Filepath.GetCString());
        return nullptr;
    }
    else
    {
        LOG_INFO("[FTextureFactory]: Loaded image '%s'", Filepath.GetCString());
    }

    return LoadFromMemory(Pixels.Get(), Width, Height, CreateFlags, Format);
}

FRHITexture2D* FTextureFactory::LoadFromMemory(const uint8* Pixels, uint32 Width, uint32 Height, uint32 CreateFlags, EFormat Format)
{
    if (
        Format != EFormat::R8_Unorm && 
        Format != EFormat::R8G8_Unorm &&
        Format != EFormat::R8G8B8A8_Unorm && 
        Format != EFormat::R32G32B32A32_Float)
    {
        LOG_ERROR("[FTextureFactory]: Format not supported");
        return nullptr;
    }

    Check(Pixels != nullptr);

    const bool GenerateMips = CreateFlags & ETextureFactoryFlags::TextureFactoryFlag_GenerateMips;
    
    const uint32 NumMips = GenerateMips ? NMath::Max<uint32>(NMath::Log2(NMath::Max(Width, Height)), 1u) : 1;
    Check(NumMips != 0);

    const uint32 Stride   = GetByteStrideFromFormat(Format);
    const uint32 RowPitch = Width * Stride;
    Check(RowPitch > 0);

    FRHITextureDataInitializer InitalData(Pixels, Format, Width, Height);

    FRHITexture2DInitializer Initializer(
        Format, 
        Width, 
        Height, 
        NumMips, 
        1, 
        ETextureUsageFlags::AllowSRV, 
        EResourceAccess::PixelShaderResource,
        &InitalData);

    FRHITexture2DRef Texture = RHICreateTexture2D(Initializer);
    if (!Texture)
    {
        DEBUG_BREAK();
        return nullptr;
    }

    if (GenerateMips)
    {
        FRHICommandList CommandList;
        CommandList.TransitionTexture(Texture.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::CopyDest);
        CommandList.GenerateMips(Texture.Get());
        CommandList.TransitionTexture(Texture.Get(), EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource);

        GRHICommandExecutor.ExecuteCommandList(CommandList);
    }

    return Texture.ReleaseOwnership();
}

FRHITextureCube* FTextureFactory::CreateTextureCubeFromPanorma(FRHITexture2D* PanoramaSource, uint32 CubeMapSize, uint32 CreateFlags, EFormat Format)
{
    Check(IsEnumFlagSet(PanoramaSource->GetFlags(), ETextureUsageFlags::AllowSRV));

    const bool bGenerateNumMips = CreateFlags & ETextureFactoryFlags::TextureFactoryFlag_GenerateMips;

    const uint32 NumMips = (bGenerateNumMips) ? NMath::Max<uint32>(NMath::Log2(CubeMapSize), 1u) : 1u;
    FRHITextureCubeInitializer Initializer(
        Format,
        CubeMapSize, 
        NumMips,
        1,
        ETextureUsageFlags::AllowUAV,
        EResourceAccess::Common);

    FRHITextureCubeRef StagingTexture = RHICreateTextureCube(Initializer);
    if (!StagingTexture)
    {
        return nullptr;
    }
    else
    {
        StagingTexture->SetName("TextureCube From Panorama StagingTexture");
    }

    FRHITextureUAVInitializer UAVInitializer(StagingTexture.Get(), Format, 0, 0, 6);
    FRHIUnorderedAccessViewRef StagingTextureUAV = RHICreateUnorderedAccessView(UAVInitializer);
    if (!StagingTextureUAV)
    {
        return nullptr;
    }

    Initializer.UsageFlags = ETextureUsageFlags::AllowSRV;

    FRHITextureCubeRef Texture = RHICreateTextureCube(Initializer);
    if (!Texture)
    {
        return nullptr;
    }

    {
        FRHICommandList CommandList;
        CommandList.TransitionTexture(PanoramaSource, EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource);
        CommandList.TransitionTexture(StagingTexture.Get(), EResourceAccess::Common, EResourceAccess::UnorderedAccess);

        CommandList.SetComputePipelineState(GlobalFactoryData.PanoramaPSO.Get());

        struct ConstantBuffer
        {
            uint32 CubeMapSize;
        } CB0;
        CB0.CubeMapSize = CubeMapSize;

        CommandList.Set32BitShaderConstants(GlobalFactoryData.ComputeShader.Get(), &CB0, 1);
        CommandList.SetUnorderedAccessView(GlobalFactoryData.ComputeShader.Get(), StagingTextureUAV.Get(), 0);

        FRHIShaderResourceView* PanoramaSourceView = PanoramaSource->GetShaderResourceView();
        CommandList.SetShaderResourceView(GlobalFactoryData.ComputeShader.Get(), PanoramaSourceView, 0);

        constexpr uint32 LocalWorkGroupCount = 16;
        const uint32 ThreadsX = NMath::DivideByMultiple(CubeMapSize, LocalWorkGroupCount);
        const uint32 ThreadsY = NMath::DivideByMultiple(CubeMapSize, LocalWorkGroupCount);
        CommandList.Dispatch(ThreadsX, ThreadsY, 6);

        CommandList.TransitionTexture(PanoramaSource, EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource);
        CommandList.TransitionTexture(StagingTexture.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::CopySource);
        CommandList.TransitionTexture(Texture.Get(), EResourceAccess::Common, EResourceAccess::CopyDest);

        CommandList.CopyTexture(Texture.Get(), StagingTexture.Get());

        if (bGenerateNumMips)
        {
            CommandList.GenerateMips(Texture.Get());
        }

        CommandList.TransitionTexture(Texture.Get(), EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource);

        CommandList.DestroyResource(StagingTexture.Get());
        CommandList.DestroyResource(StagingTextureUAV.Get());

        GRHICommandExecutor.ExecuteCommandList(CommandList);
    }

    return Texture.ReleaseOwnership();
}
