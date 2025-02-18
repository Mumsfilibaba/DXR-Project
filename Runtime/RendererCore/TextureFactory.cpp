#include "RHI/RHI.h"
#include "RHI/RHICommandList.h"
#include "RHI/ShaderCompiler.h"
#include "RendererCore/TextureFactory.h"
#include "RendererCore/TextureResourceData.h"

FTextureFactory* FTextureFactory::Instance = nullptr;

FTextureFactory::FTextureFactory()
    : LinearSampler(nullptr)
    , PanoramaPSO(nullptr)
    , PanoramCS(nullptr)
{
}

FTextureFactory::~FTextureFactory()
{
    // Samplers
    LinearSampler.Reset();

    // Panorama
    PanoramaPSO.Reset();
    PanoramCS.Reset();

    // GenerateMips Texture2D
    GenerateMipsTex2D_PSO.Reset();
    GenerateMipsTex2D_CS.Reset();

    // GenerateMips TextureCube
    GenerateMipsTexCube_PSO.Reset();
    GenerateMipsTexCube_CS.Reset();
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

    // Compile "Cube-Map from Panorama" shader
    FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/CubeMapGen.hlsl", CompileInfo, Code))
    {
        return false;
    }

    PanoramCS = RHICreateComputeShader(Code);
    if (!PanoramCS)
    {
        return false;
    }

    // Create "Cube-Map from Panorama" pipeline
    PanoramaPSO = RHICreateComputePipelineState(FRHIComputePipelineStateInitializer(PanoramCS.Get()));
    if (PanoramaPSO)
    {
        PanoramaPSO->SetDebugName("Generate CubeMap RootSignature");
    }
    else
    {
        return false;
    }

    // Compile "GenerateMips Texure2D" shader
    CompileInfo = FShaderCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/GenerateMipsTex2D.hlsl", CompileInfo, Code))
    {
        return false;
    }

    GenerateMipsTex2D_CS = RHICreateComputeShader(Code);
    if (!GenerateMipsTex2D_CS)
    {
        return false;
    }

    // Create "GenerateMips Texure2D" pipeline
    GenerateMipsTex2D_PSO = RHICreateComputePipelineState(FRHIComputePipelineStateInitializer(GenerateMipsTex2D_CS.Get()));
    if (GenerateMipsTex2D_PSO)
    {
        GenerateMipsTex2D_PSO->SetDebugName("GenerateMips Texure2D PSO");
    }
    else
    {
        return false;
    }

    // Compile "GenerateMips TexureCube" shader
    CompileInfo = FShaderCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/GenerateMipsTexCube.hlsl", CompileInfo, Code))
    {
        return false;
    }

    GenerateMipsTexCube_CS = RHICreateComputeShader(Code);
    if (!GenerateMipsTexCube_CS)
    {
        return false;
    }

    // Create "GenerateMips TexureCube" pipeline
    GenerateMipsTexCube_PSO = RHICreateComputePipelineState(FRHIComputePipelineStateInitializer(GenerateMipsTexCube_CS.Get()));
    if (GenerateMipsTexCube_PSO)
    {
        GenerateMipsTexCube_PSO->SetDebugName("GenerateMips TexureCube PSO");
    }
    else
    {
        return false;
    }

    // Sampler
    FRHISamplerStateInfo SamplerInfo;
    SamplerInfo.AddressU = ESamplerMode::Wrap;
    SamplerInfo.AddressV = ESamplerMode::Wrap;
    SamplerInfo.AddressW = ESamplerMode::Wrap;
    SamplerInfo.Filter   = ESamplerFilter::MinMagMipLinear;
    SamplerInfo.MinLOD   = 0.0f;
    SamplerInfo.MaxLOD   = TNumericLimits<float>::Max();

    LinearSampler = RHICreateSamplerState(SamplerInfo);
    if (!LinearSampler)
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
        CommandList.TransitionTexture(Texture.Get(), FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::CopyDest));
        CommandList.GenerateMips(Texture.Get());
        CommandList.TransitionTexture(Texture.Get(), FRHITextureTransition::Make(EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource));

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
    FRHITextureUAVInfo UAVInfo(StagingTexture.Get(), StagingTexture->GetFormat(), 0, 0, 1);

    FRHIUnorderedAccessViewRef StagingTextureUAV = RHICreateUnorderedAccessView(UAVInfo);
    if (!StagingTextureUAV)
    {
        return false;
    }

    // Schedule work on the GPU
    {
        FRHICommandList CommandList;
        CommandList.TransitionTexture(Source, FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource));
        CommandList.TransitionTexture(StagingTexture.Get(), FRHITextureTransition::Make(EResourceAccess::Common, EResourceAccess::UnorderedAccess));

        CommandList.SetComputePipelineState(PanoramaPSO.Get());

        struct FCubeMapGenConstants
        {
            uint32 CubeMapSize;
        } CB0;

        CB0.CubeMapSize = StagingTexture->GetExtent().X;

        CommandList.Set32BitShaderConstants(PanoramCS.Get(), &CB0, 1);
        CommandList.SetUnorderedAccessView(PanoramCS.Get(), StagingTextureUAV.Get(), 0);

        FRHIShaderResourceView* PanoramaSourceView = Source->GetShaderResourceView();
        CommandList.SetShaderResourceView(PanoramCS.Get(), PanoramaSourceView, 0);

        CommandList.SetSamplerState(PanoramCS.Get(), LinearSampler.Get(), 0);

        constexpr uint32 LocalWorkGroupCount = 16;
        const uint32 ThreadsX = FMath::DivideByMultiple(CB0.CubeMapSize, LocalWorkGroupCount);
        const uint32 ThreadsY = FMath::DivideByMultiple(CB0.CubeMapSize, LocalWorkGroupCount);
        CommandList.Dispatch(ThreadsX, ThreadsY, 6);

        CommandList.TransitionTexture(Source, FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource));

        if (!bDestSupportUAV)
        {
            CommandList.TransitionTexture(StagingTexture.Get(), FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::CopySource));
            CommandList.TransitionTexture(Dest, FRHITextureTransition::Make(EResourceAccess::Common, EResourceAccess::CopyDest));

            CommandList.CopyTexture(Dest, StagingTexture.Get());
        }
        else
        {
            CommandList.TransitionTexture(Dest, FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::CopyDest));
        }

        if (bGenerateMips)
        {
            CommandList.GenerateMips(Dest);
        }

        CommandList.TransitionTexture(Dest, FRHITextureTransition::Make(EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource));

        FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    }

    return true;
}

bool FTextureFactory::GenerateMiplevels(FRHITexture* Texture)
{
    CHECK(IsEnumFlagSet(Texture->GetFlags(), ETextureUsageFlags::ShaderResource));

    if (Texture->GetNumMipLevels() < 2)
    {
        LOG_ERROR("Texture needs to have a full mip-chain allocated");
        return false;
    }

    // Determine if we need a staging resource
    const bool bIsTextureCube  = IsTextureCube(Texture->GetDimension());
    const bool bDestSupportUAV = IsEnumFlagSet(Texture->GetFlags(), ETextureUsageFlags::UnorderedAccess);

    // If the destination does not support UAVs, create a staging texture that does
    FRHITextureRef StagingTexture;
    if (!bDestSupportUAV)
    {
        FRHITextureInfo TextureInfo = Texture->GetInfo();
        TextureInfo.UsageFlags |= ETextureUsageFlags::UnorderedAccess;

        StagingTexture = RHICreateTexture(TextureInfo, EResourceAccess::Common, nullptr);
        if (!StagingTexture)
        {
            return false;
        }
        else
        {
            StagingTexture->SetDebugName("GenerateMiplevels StagingTexture");
        }
    }
    else
    {
        StagingTexture = MakeSharedRef<FRHITexture>(Texture);
    }

    // Calculate how many compute-dispatches that we need
    constexpr uint32 MipLevelsPerDispatch = 4;
    const uint32 NumMipLevels  = StagingTexture->GetNumMipLevels();
    const uint32 NumDispatches = FMath::AlignUp<uint32>(NumMipLevels, MipLevelsPerDispatch) / MipLevelsPerDispatch;

    // Create UAV for each miplevel
    FRHITextureUAVInfo UAVInfo;
    UAVInfo.Texture         = StagingTexture.Get();
    UAVInfo.Format          = StagingTexture->GetFormat();
    UAVInfo.FirstArraySlice = 0;
    UAVInfo.NumSlices       = 1;

    // Skip the first mip since that will only be used as a source
    TArray<FRHIUnorderedAccessViewRef> UnorderedAccessViews;
    UnorderedAccessViews.Reserve(NumMipLevels - 1);

    for (uint32 MipLevel = 1; MipLevel < NumMipLevels; MipLevel++)
    {
        UAVInfo.MipLevel = MipLevel;

        FRHIUnorderedAccessView* UnorderedAccessView = RHICreateUnorderedAccessView(UAVInfo);
        UnorderedAccessViews.Emplace(UnorderedAccessView);
    }

    FRHIShaderResourceView* ShaderResourceView = StagingTexture->GetShaderResourceView();

    // Schedule work on the GPU
    {
        FRHICommandList CommandList;

        // Copy the texture over to the staging-resource
        if (!bDestSupportUAV)
        {
            CommandList.TransitionTexture(Texture, FRHITextureTransition::Make(EResourceAccess::CopyDest, EResourceAccess::CopySource));
            CommandList.TransitionTexture(StagingTexture.Get(), FRHITextureTransition::Make(EResourceAccess::Common, EResourceAccess::CopyDest));

            CommandList.CopyTexture(StagingTexture.Get(), Texture);

            CommandList.TransitionTexture(StagingTexture.Get(), FRHITextureTransition::Make(EResourceAccess::CopyDest, EResourceAccess::UnorderedAccess));
            CommandList.TransitionTexture(Texture, FRHITextureTransition::Make(EResourceAccess::CopyDest, EResourceAccess::NonPixelShaderResource));
        }
        else
        {
            CommandList.TransitionTexture(StagingTexture.Get(), FRHITextureTransition::Make(EResourceAccess::CopyDest, EResourceAccess::UnorderedAccess));
        }

        // Determine which compute-shader and pipeline-state to use
        FRHIComputeShaderRef        ComputeShader = bIsTextureCube ? GenerateMipsTexCube_CS  : GenerateMipsTex2D_CS;
        FRHIComputePipelineStateRef PipelineState = bIsTextureCube ? GenerateMipsTexCube_PSO : GenerateMipsTex2D_PSO;
        CommandList.SetComputePipelineState(PipelineState.Get());

        struct FGenMipsConstants
        {
            uint32   SrcMipLevel;  // MipLevel to read from
            uint32   NumMipLevels; // Number of MipLevels we want to create (Up to 4)
            FVector2 TexelSize;    // Size of 
        } ConstantData;

        const FIntVector3 TextureExtent = StagingTexture->GetExtent();
        uint32 DstWidth  = static_cast<uint32>(TextureExtent.X);
        uint32 DstHeight = static_cast<uint32>(TextureExtent.Y);
        ConstantData.SrcMipLevel = 0;

        constexpr uint32 NumThreadsTexture2D   = 1;
        constexpr uint32 NumThreadsTextureCube = 6;
        const uint32 ThreadsZ = bIsTextureCube ? NumThreadsTextureCube : NumThreadsTexture2D;

        // Bind the original texture
        CommandList.SetShaderResourceView(ComputeShader.Get(), ShaderResourceView, 0);

        uint32 RemainingMiplevels = NumMipLevels;
        for (uint32 Index = 0; Index < NumDispatches; Index++)
        {
            ConstantData.TexelSize    = FVector2(1.0f / static_cast<float>(DstWidth), 1.0f / static_cast<float>(DstHeight));
            ConstantData.NumMipLevels = FMath::Min<uint32>(MipLevelsPerDispatch, RemainingMiplevels);

            constexpr uint32 NumConstants = sizeof(FGenMipsConstants) / sizeof(uint32);
            CommandList.Set32BitShaderConstants(ComputeShader.Get(), &ConstantData, NumConstants);

            const uint32 CurrentMipLevel = Index * MipLevelsPerDispatch;
            CommandList.SetUnorderedAccessView(ComputeShader.Get(), UnorderedAccessViews[CurrentMipLevel + 0].Get(), 0);
            CommandList.SetUnorderedAccessView(ComputeShader.Get(), UnorderedAccessViews[CurrentMipLevel + 1].Get(), 0);
            CommandList.SetUnorderedAccessView(ComputeShader.Get(), UnorderedAccessViews[CurrentMipLevel + 2].Get(), 0);
            CommandList.SetUnorderedAccessView(ComputeShader.Get(), UnorderedAccessViews[CurrentMipLevel + 3].Get(), 0);

            constexpr uint32 ThreadCount = 8;
            const uint32 ThreadsX = FMath::DivideByMultiple(DstWidth, ThreadCount);
            const uint32 ThreadsY = FMath::DivideByMultiple(DstHeight, ThreadCount);
            CommandList.Dispatch(ThreadsX, ThreadsY, ThreadsZ);

            CommandList.TransitionTexture(StagingTexture.Get(), FRHITextureTransition::MakePartial(EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource, CurrentMipLevel + 1));
            CommandList.TransitionTexture(StagingTexture.Get(), FRHITextureTransition::MakePartial(EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource, CurrentMipLevel + 2));
            CommandList.TransitionTexture(StagingTexture.Get(), FRHITextureTransition::MakePartial(EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource, CurrentMipLevel + 3));
            CommandList.TransitionTexture(StagingTexture.Get(), FRHITextureTransition::MakePartial(EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource, CurrentMipLevel + 4));

            DstWidth  = DstWidth / 16;
            DstHeight = DstHeight / 16;

            ConstantData.SrcMipLevel += 3;
            RemainingMiplevels -= MipLevelsPerDispatch;
        }
    }

    return true;
}