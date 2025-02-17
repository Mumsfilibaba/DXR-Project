#include "RHI/RHI.h"
#include "RHI/RHICommandList.h"
#include "RHI/ShaderCompiler.h"
#include "RendererCore/TextureFactory.h"
#include "RendererCore/TextureResourceData.h"

FTextureFactory* FTextureFactory::Instance = nullptr;

FTextureFactory::FTextureFactory()
    : PanoramaGenSampler(nullptr)
    , PanoramaPSO(nullptr)
    , ComputeShader(nullptr)
{
}

FTextureFactory::~FTextureFactory()
{
    PanoramaGenSampler.Reset();
    PanoramaPSO.Reset();
    ComputeShader.Reset();
}

bool FTextureFactory::Initialize()
{
    Instance = new FTextureFactory();
    if (!Instance->CreateResources())
    {
        return false;
    }

    return true;
}

void FTextureFactory::Release()
{
    SAFE_DELETE(Instance);
}

bool FTextureFactory::CreateResources()
{
    // Compile and create shader
    TArray<uint8> Code;

    FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/CubeMapGen.hlsl", CompileInfo, Code))
    {
        return false;
    }

    ComputeShader = RHICreateComputeShader(Code);
    if (!ComputeShader)
    {
        return false;
    }

    // Create pipeline
    PanoramaPSO = RHICreateComputePipelineState(FRHIComputePipelineStateInitializer(ComputeShader.Get()));
    if (PanoramaPSO)
    {
        PanoramaPSO->SetDebugName("Generate CubeMap RootSignature");
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

    PanoramaGenSampler = RHICreateSamplerState(SamplerInfo);
    if (!PanoramaGenSampler)
    {
        return false;
    }

    return true;
}

FRHITexture* FTextureFactory::LoadFromMemory(const uint8* Pixels, uint32 Width, uint32 Height, ETextureFactoryFlags Flags, EFormat Format)
{
    CHECK(Pixels != nullptr);

    const bool bGenerateMips = IsEnumFlagSet(Flags, ETextureFactoryFlags::GenerateMips);

    const uint32 NumMiplevels = bGenerateMips ? FTextureFactoryHelpers::TextureSizeToMiplevels(FMath::Max<uint32>(Width, Height)) : 1u;
    CHECK(NumMiplevels != 0);

    const uint32 Stride   = GetByteStrideFromFormat(Format);
    const uint32 RowPitch = Width * Stride;
    CHECK(RowPitch > 0);

    FTextureResourceData InitalData;
    InitalData.InitMipData(Pixels, RowPitch, RowPitch * Height);

    FRHITextureInfo TextureInfo = FRHITextureInfo::CreateTexture2D(Format, Width, Height, NumMiplevels, 1, ETextureUsageFlags::ShaderResource);
    FRHITextureRef Texture = RHICreateTexture(TextureInfo, EResourceAccess::PixelShaderResource, &InitalData);
    if (!Texture)
    {
        DEBUG_BREAK();
        return nullptr;
    }

    if (bGenerateMips && NumMiplevels > 1)
    {
        FRHICommandList CommandList;
        CommandList.TransitionTexture(Texture.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::CopyDest);
        CommandList.GenerateMips(Texture.Get());
        CommandList.TransitionTexture(Texture.Get(), EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource);

        FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    }

    return Texture.ReleaseOwnership();
}

bool FTextureFactory::TextureCubeFromPanorma(FRHITexture* Source, FRHITexture* Dest, ETextureFactoryFlags Flags)
{
    CHECK(IsTextureCube(Dest->GetDimension()));
    CHECK(IsEnumFlagSet(Source->GetFlags(), ETextureUsageFlags::ShaderResource));
    CHECK(IsEnumFlagSet(Dest->GetFlags(), ETextureUsageFlags::ShaderResource));

    const bool bGenerateMips   = IsEnumFlagSet(Flags, ETextureFactoryFlags::GenerateMips);
    const bool bDestSupportUAV = IsEnumFlagSet(Dest->GetFlags(), ETextureUsageFlags::UnorderedAccess);

    // If the destination does not support UAVs, create a staging texture that does
    FRHITextureRef StagingTexture;
    if (!bDestSupportUAV)
    {
        FRHITextureInfo TextureInfo = Dest->GetInfo();
        TextureInfo.UsageFlags |= ETextureUsageFlags::UnorderedAccess;

        StagingTexture = RHICreateTexture(TextureInfo, EResourceAccess::Common, nullptr);
        if (!StagingTexture)
        {
            return false;
        }
        else
        {
            StagingTexture->SetDebugName("TextureCube From Panorama StagingTexture");
        }
    }
    else
    {
        StagingTexture = MakeSharedRef<FRHITexture>(Dest);
    }

    // Create UAV for the staging-texture
    FRHITextureUAVDesc UAVInitializer(StagingTexture.Get(), StagingTexture->GetFormat(), 0, 0, 1);

    FRHIUnorderedAccessViewRef StagingTextureUAV = RHICreateUnorderedAccessView(UAVInitializer);
    if (!StagingTextureUAV)
    {
        return false;
    }

    {
        FRHICommandList CommandList;
        CommandList.TransitionTexture(Source, EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource);
        CommandList.TransitionTexture(StagingTexture.Get(), EResourceAccess::Common, EResourceAccess::UnorderedAccess);

        CommandList.SetComputePipelineState(PanoramaPSO.Get());

        struct FCubeMapGenConstants
        {
            uint32 CubeMapSize;
        } CB0;

        CB0.CubeMapSize = StagingTexture->GetExtent().X;

        CommandList.Set32BitShaderConstants(ComputeShader.Get(), &CB0, 1);
        CommandList.SetUnorderedAccessView(ComputeShader.Get(), StagingTextureUAV.Get(), 0);

        FRHIShaderResourceView* PanoramaSourceView = Source->GetShaderResourceView();
        CommandList.SetShaderResourceView(ComputeShader.Get(), PanoramaSourceView, 0);

        CommandList.SetSamplerState(ComputeShader.Get(), PanoramaGenSampler.Get(), 0);

        constexpr uint32 LocalWorkGroupCount = 16;
        const uint32 ThreadsX = FMath::DivideByMultiple(CB0.CubeMapSize, LocalWorkGroupCount);
        const uint32 ThreadsY = FMath::DivideByMultiple(CB0.CubeMapSize, LocalWorkGroupCount);
        CommandList.Dispatch(ThreadsX, ThreadsY, 6);

        CommandList.TransitionTexture(Source, EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource);

        if (!bDestSupportUAV)
        {
            CommandList.TransitionTexture(StagingTexture.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::CopySource);
            CommandList.TransitionTexture(Dest, EResourceAccess::Common, EResourceAccess::CopyDest);

            CommandList.CopyTexture(Dest, StagingTexture.Get());
        }
        else
        {
            CommandList.TransitionTexture(Dest, EResourceAccess::UnorderedAccess, EResourceAccess::CopyDest);
        }

        if (bGenerateMips)
        {
            CommandList.GenerateMips(Dest);
        }

        CommandList.TransitionTexture(Dest, EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource);

        FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    }

    return true;
}
