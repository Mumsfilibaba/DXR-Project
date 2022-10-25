#include "TextureFactory.h"

#include "Engine/Assets/SceneData.h"

#include "RHI/RHIInterface.h"
#include "RHI/RHICommandList.h"
#include "RHI/RHIShaderCompiler.h"

#include <stb_image.h>

struct TextureFactoryData
{
    FRHIComputePipelineStateRef PanoramaPSO;
    FRHIComputeShaderRef        ComputeShader;
};

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
    GlobalFactoryData.PanoramaPSO = RHICreateComputePipelineState(FRHIComputePipelineStateDesc(GlobalFactoryData.ComputeShader.Get()));
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

FRHITexture* FTextureFactory::LoadFromMemory(const uint8* Pixels, uint32 Width, uint32 Height, uint32 CreateFlags, EFormat Format)
{
    CHECK(Pixels != nullptr);

    const bool GenerateMips = CreateFlags & ETextureFactoryFlags::TextureFactoryFlag_GenerateMips;
    
    const uint32 NumMips = GenerateMips ? NMath::Max<uint32>(NMath::Log2(NMath::Max(Width, Height)), 1u) : 1;
    CHECK(NumMips != 0);

    const uint32 Stride   = GetByteStrideFromFormat(Format);
    const uint32 RowPitch = Width * Stride;
    CHECK(RowPitch > 0);

    FTextureResourceData InitalData;
    InitalData.InitMipData(Pixels, RowPitch, RowPitch*Height);

    FRHITextureDesc TextureDesc = FRHITextureDesc::CreateTexture2D(
        Format,
        Width,
        Height,
        NumMips,
        1, 
        ETextureUsageFlags::ShaderResource);

    FRHITextureRef Texture = RHICreateTexture(TextureDesc, EResourceAccess::PixelShaderResource, &InitalData);
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

FRHITexture* FTextureFactory::CreateTextureCubeFromPanorma(FRHITexture* PanoramaSource, uint32 CubeMapSize, uint32 CreateFlags, EFormat Format)
{
    CHECK(IsEnumFlagSet(PanoramaSource->GetFlags(), ETextureUsageFlags::ShaderResource));

    const bool bGenerateNumMips = CreateFlags & ETextureFactoryFlags::TextureFactoryFlag_GenerateMips;

    const uint32 NumMips = (bGenerateNumMips) ? NMath::Max<uint32>(NMath::Log2(CubeMapSize), 1u) : 1u;

    FRHITextureDesc TextureDesc = FRHITextureDesc::CreateTextureCube(
        Format,
        CubeMapSize, 
        NumMips,
        1,
        ETextureUsageFlags::UnorderedAccess);

    FRHITextureRef StagingTexture = RHICreateTexture(TextureDesc, EResourceAccess::Common, nullptr);
    if (!StagingTexture)
    {
        return nullptr;
    }
    else
    {
        StagingTexture->SetName("TextureCube From Panorama StagingTexture");
    }

    FRHITextureUAVDesc UAVInitializer(StagingTexture.Get(), Format, 0, 0, 6);
    FRHIUnorderedAccessViewRef StagingTextureUAV = RHICreateUnorderedAccessView(UAVInitializer);
    if (!StagingTextureUAV)
    {
        return nullptr;
    }

    TextureDesc.UsageFlags = ETextureUsageFlags::ShaderResource;

    FRHITextureRef Texture = RHICreateTexture(TextureDesc, EResourceAccess::Common, nullptr);
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
