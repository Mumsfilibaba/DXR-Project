#include "TextureFactory.h"
#include "TextureResourceData.h"
#include "RHI/RHI.h"
#include "RHI/RHICommandList.h"
#include "RHI/ShaderCompiler.h"

struct TextureFactoryData
{
    FRHISamplerStateRef         PanoramaGenSampler;
    FRHIComputePipelineStateRef PanoramaPSO;
    FRHIComputeShaderRef        ComputeShader;
};

static TextureFactoryData GlobalFactoryData;

bool FTextureFactory::Init()
{
    // Compile and create shader
    TArray<uint8> Code;

    FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/CubeMapGen.hlsl", CompileInfo, Code))
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
        GlobalFactoryData.PanoramaPSO->SetDebugName("Generate CubeMap RootSignature");
    }
    else
    {
        return false;
    }

    FRHISamplerStateInfo SamplerInfo;
    SamplerInfo.AddressU = ESamplerMode::Wrap;
    SamplerInfo.AddressV = ESamplerMode::Wrap;
    SamplerInfo.AddressW = ESamplerMode::Wrap;
    SamplerInfo.Filter   = ESamplerFilter::MinMagMipLinear;
    SamplerInfo.MinLOD   = 0.0f;
    SamplerInfo.MaxLOD   = TNumericLimits<float>::Max();

    GlobalFactoryData.PanoramaGenSampler = RHICreateSamplerState(SamplerInfo);
    if (!GlobalFactoryData.PanoramaGenSampler)
    {
        return false;
    }

    return true;
}

void FTextureFactory::Release()
{
    GlobalFactoryData.PanoramaGenSampler.Reset();
    GlobalFactoryData.PanoramaPSO.Reset();
    GlobalFactoryData.ComputeShader.Reset();
}

FRHITexture* FTextureFactory::LoadFromMemory(const uint8* Pixels, uint32 Width, uint32 Height, uint32 CreateFlags, EFormat Format)
{
    CHECK(Pixels != nullptr);

    const bool GenerateMips = CreateFlags & ETextureFactoryFlags::TextureFactoryFlag_GenerateMips;
    
    const uint32 NumMips = GenerateMips ? FMath::Max<uint32>(FMath::Log2(FMath::Max(Width, Height)), 1u) : 1;
    CHECK(NumMips != 0);

    const uint32 Stride   = GetByteStrideFromFormat(Format);
    const uint32 RowPitch = Width * Stride;
    CHECK(RowPitch > 0);

    FTextureResourceData InitalData;
    InitalData.InitMipData(Pixels, RowPitch, RowPitch*Height);

    FRHITextureInfo TextureInfo = FRHITextureInfo::CreateTexture2D(Format, Width, Height, NumMips, 1, ETextureUsageFlags::ShaderResource);
    FRHITextureRef Texture = RHICreateTexture(TextureInfo, EResourceAccess::PixelShaderResource, &InitalData);
    if (!Texture)
    {
        DEBUG_BREAK();
        return nullptr;
    }

    if (GenerateMips && NumMips > 1)
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

    const uint32 NumMips = bGenerateNumMips ? FMath::Max<uint32>(FMath::Log2(CubeMapSize), 1u) : 1u;

    FRHITextureInfo TextureInfo = FRHITextureInfo::CreateTextureCube(Format, CubeMapSize, NumMips, 1, ETextureUsageFlags::UnorderedAccess);

    FRHITextureRef StagingTexture = RHICreateTexture(TextureInfo, EResourceAccess::Common, nullptr);
    if (!StagingTexture)
    {
        return nullptr;
    }
    else
    {
        StagingTexture->SetDebugName("TextureCube From Panorama StagingTexture");
    }

    FRHITextureUAVDesc UAVInitializer(StagingTexture.Get(), Format, 0, 0, 1);
    FRHIUnorderedAccessViewRef StagingTextureUAV = RHICreateUnorderedAccessView(UAVInitializer);
    if (!StagingTextureUAV)
    {
        return nullptr;
    }

    TextureInfo.UsageFlags = ETextureUsageFlags::ShaderResource;

    FRHITextureRef Texture = RHICreateTexture(TextureInfo, EResourceAccess::Common, nullptr);
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

        CommandList.SetSamplerState(GlobalFactoryData.ComputeShader.Get(), GlobalFactoryData.PanoramaGenSampler.Get(), 0);

        constexpr uint32 LocalWorkGroupCount = 16;
        const uint32 ThreadsX = FMath::DivideByMultiple(CubeMapSize, LocalWorkGroupCount);
        const uint32 ThreadsY = FMath::DivideByMultiple(CubeMapSize, LocalWorkGroupCount);
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

        GRHICommandExecutor.ExecuteCommandList(CommandList);
    }

    return Texture.ReleaseOwnership();
}
