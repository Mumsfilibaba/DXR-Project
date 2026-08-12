#include "RHI/RHI.h"
#include "RHI/RHICommandList.h"
#include "RendererCore/Shaders/ShaderCache.h"
#include "RendererCore/TextureFactory.h"
#include "RendererCore/TextureHelpers.h"
#include "RendererCore/TextureResourceData.h"

FTextureFactory* FTextureFactory::TextureFactory = nullptr;

class FCubeMapGenCS
{
    DECLARE_SHADER_TYPE(FCubeMapGenCS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<>;
};

IMPLEMENT_SHADER_TYPE(FCubeMapGenCS, "Shaders/CubeMapGen.hlsl", "Main", EShaderModel::SM_6_2);

class FGenerateMipsTex2DCS
{
    DECLARE_SHADER_TYPE(FGenerateMipsTex2DCS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<>;
};

IMPLEMENT_SHADER_TYPE(FGenerateMipsTex2DCS, "Shaders/GenerateMipsTex2D.hlsl", "Main", EShaderModel::SM_6_2);

class FGenerateMipsTexCubeCS
{
    DECLARE_SHADER_TYPE(FGenerateMipsTexCubeCS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<>;
};

IMPLEMENT_SHADER_TYPE(FGenerateMipsTexCubeCS, "Shaders/GenerateMipsTexCube.hlsl", "Main", EShaderModel::SM_6_2);

class FIrradianceGenCS
{
    DECLARE_SHADER_TYPE(FIrradianceGenCS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<>;
};

IMPLEMENT_SHADER_TYPE(FIrradianceGenCS, "Shaders/IrradianceGen.hlsl", "Main", EShaderModel::SM_6_2);

class FSpecularIrradianceGenCS
{
    DECLARE_SHADER_TYPE(FSpecularIrradianceGenCS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<>;
};

IMPLEMENT_SHADER_TYPE(FSpecularIrradianceGenCS, "Shaders/SpecularIrradianceGen.hlsl", "Main", EShaderModel::SM_6_2);

class FPackMaterialParamsCS
{
    DECLARE_SHADER_TYPE(FPackMaterialParamsCS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<>;
};

IMPLEMENT_SHADER_TYPE(FPackMaterialParamsCS, "Shaders/PackMaterialParams.hlsl", "Main", EShaderModel::SM_6_2);

class FBakeAlphaCS
{
    DECLARE_SHADER_TYPE(FBakeAlphaCS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<>;
};

IMPLEMENT_SHADER_TYPE(FBakeAlphaCS, "Shaders/BakeAlpha.hlsl", "Main", EShaderModel::SM_6_2);

template<typename ShaderType>
static bool AcquireComputePipeline(FRHIComputeShaderRef& OutShader, FRHIComputePipelineStateRef& OutPSO,
    TArrayView<const FRHIStaticSamplerInfo> StaticSamplers = TArrayView<const FRHIStaticSamplerInfo>())
{
    const CHAR* ShaderName = ShaderType::GetStaticType().GetName();

    OutShader = FShaderCache::Get().GetShader<ShaderType>();
    if (!OutShader)
    {
        LOG_ERROR("[FTextureFactory] Failed to create compute shader: %s", ShaderName);
        return false;
    }

    FRHIComputePipelineStateDesc PSODesc;
    PSODesc.Shader         = OutShader.Get();
    PSODesc.StaticSamplers = StaticSamplers;

    OutPSO = RHI::CreateComputePipelineState(PSODesc);
    if (!OutPSO)
    {
        LOG_ERROR("[FTextureFactory] Failed to create PSO: %s", ShaderName);
        return false;
    }

    OutPSO->SetDebugName(ShaderName);
    return true;
}

FTextureFactory::FTextureFactory()
    : TextureCompressor()
    , LinearSampler(nullptr)
    , PanoramaPSO(nullptr)
    , PanoramCS(nullptr)
    , GenerateMipsTex2D_PSO(nullptr)
    , GenerateMipsTex2D_CS(nullptr)
    , GenerateMipsTexCube_PSO(nullptr)
    , GenerateMipsTexCube_CS(nullptr)
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

    // Specular IrradianceGen
    SpecularCubeMapFilter_PSO.Reset();
    SpecularCubeMapFilter_CS.Reset();

    // Diffuse IrradianceGen
    DiffuseCubeMapFilter_PSO.Reset();
    DiffuseCubeMapFilter_CS.Reset();
}

bool FTextureFactory::Initialize()
{
    TextureFactory = new FTextureFactory();
    if (!TextureFactory->CreateResources())
    {
        return false;
    }

    return true;
}

void FTextureFactory::Release()
{
    SAFE_DELETE(TextureFactory);
}

bool FTextureFactory::CreateResources()
{
    if (!TextureCompressor.Initialize())
    {
        return false;
    }

    if (!AcquireComputePipeline<FCubeMapGenCS>(PanoramCS, PanoramaPSO))
    {
        return false;
    }

    // Static sampler: Linear Wrap at s0
    FRHIStaticSamplerInfo GenMipsStaticSampler;
    GenMipsStaticSampler.AddressU = ESamplerMode::Wrap;
    GenMipsStaticSampler.AddressV = ESamplerMode::Wrap;
    GenMipsStaticSampler.AddressW = ESamplerMode::Wrap;
    GenMipsStaticSampler.Filter   = ESamplerFilter::MinMagMipLinear;
    GenMipsStaticSampler.MinLOD   = 0.0f;
    GenMipsStaticSampler.MaxLOD   = TNumericLimits<float>::Max();

    const TArrayView<const FRHIStaticSamplerInfo> GenMipsStaticSamplers(&GenMipsStaticSampler, 1);

    if (!AcquireComputePipeline<FGenerateMipsTex2DCS>(GenerateMipsTex2D_CS, GenerateMipsTex2D_PSO, GenMipsStaticSamplers))
    {
        return false;
    }

    if (!AcquireComputePipeline<FGenerateMipsTexCubeCS>(GenerateMipsTexCube_CS, GenerateMipsTexCube_PSO, GenMipsStaticSamplers))
    {
        return false;
    }

    if (!AcquireComputePipeline<FIrradianceGenCS>(DiffuseCubeMapFilter_CS, DiffuseCubeMapFilter_PSO))
    {
        return false;
    }

    if (!AcquireComputePipeline<FSpecularIrradianceGenCS>(SpecularCubeMapFilter_CS, SpecularCubeMapFilter_PSO))
    {
        return false;
    }

    if (!AcquireComputePipeline<FPackMaterialParamsCS>(PackMaterialParams_CS, PackMaterialParams_PSO))
    {
        return false;
    }

    if (!AcquireComputePipeline<FBakeAlphaCS>(BakeAlpha_CS, BakeAlpha_PSO))
    {
        return false;
    }

    // Sampler
    FRHISamplerStateDesc LinearSamplerDesc;
    LinearSamplerDesc.AddressU = ESamplerMode::Wrap;
    LinearSamplerDesc.AddressV = ESamplerMode::Wrap;
    LinearSamplerDesc.AddressW = ESamplerMode::Wrap;
    LinearSamplerDesc.Filter   = ESamplerFilter::MinMagMipLinear;
    LinearSamplerDesc.MinLOD   = 0.0f;
    LinearSamplerDesc.MaxLOD   = TNumericLimits<float>::Max();

    LinearSampler = RHI::CreateSamplerState(LinearSamplerDesc);
    if (!LinearSampler)
    {
        return false;
    }

    FRHISamplerStateDesc CubeMapFilterSamplerDesc;
    CubeMapFilterSamplerDesc.AddressU = ESamplerMode::Wrap;
    CubeMapFilterSamplerDesc.AddressV = ESamplerMode::Wrap;
    CubeMapFilterSamplerDesc.AddressW = ESamplerMode::Wrap;
    CubeMapFilterSamplerDesc.Filter   = ESamplerFilter::MinMagMipLinear;

    CubeMapFilterSampler = RHI::CreateSamplerState(CubeMapFilterSamplerDesc);
    if (!CubeMapFilterSampler)
    {
        return false;
    }

    return true;
}

FRHITexture* FTextureFactory::LoadFromMemory(const uint8* Pixels, uint32 Width, uint32 Height, ETextureFactoryFlags Flags, EFormat Format)
{
    CHECK(Pixels != nullptr);

    const bool bGenerateMips = IsEnumFlagSet(Flags, ETextureFactoryFlags::GenerateMips);

    const uint32 NumMiplevels = bGenerateMips ? TextureHelpers::TextureSizeToMiplevels(Math::Max<uint32>(Width, Height)) : 1u;
    CHECK(NumMiplevels != 0);

    const uint32 Stride   = GetByteStrideFromFormat(Format);
    const uint32 RowPitch = Width * Stride;
    CHECK(RowPitch > 0);

    FTextureResourceData InitalData;
    InitalData.InitMipData(Pixels, RowPitch, RowPitch * Height);

    ETextureUsageFlags TextureUsage = ETextureUsageFlags::ShaderResourceTexture;
    if (bGenerateMips && NumMiplevels > 1)
    {
        TextureUsage |= ETextureUsageFlags::CopyDest;
    }

    FRHITextureDesc TextureDesc = FRHITextureDesc::CreateTexture2D(Format, Width, Height, NumMiplevels, 1, TextureUsage);
    if (!bGenerateMips || NumMiplevels <= 1)
    {
        TextureDesc.TrackingMode = ERHIResourceStateTrackingMode::Static;
    }

    FRHITextureRef Texture = RHI::CreateTexture(TextureDesc, ERHIResourceState::ShaderResource, &InitalData);
    if (!Texture)
    {
        DEBUG_BREAK();
        return nullptr;
    }

    if (bGenerateMips && NumMiplevels > 1)
    {
        FRHICommandList CommandList;
        if (GenerateMiplevels(CommandList, Texture.Get(), &InitalData))
        {
            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTextureModeChange(Texture.Get(), ERHIResourceState::ShaderResource, ERHIResourceState::ShaderResource, ERHIResourceStateTrackingMode::Static));
        }

        FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    }

    return Texture.ReleaseOwnership();
}

bool FTextureFactory::TextureCubeFromPanorma(FRHITexture* Source, FRHITexture* Dest, ETextureFactoryFlags Flags)
{
    CHECK(IsTextureCube(Dest->GetDesc().Dimension));
    CHECK(IsEnumFlagSet(Source->GetDesc().UsageFlags, ETextureUsageFlags::ShaderResourceTexture));
    CHECK(IsEnumFlagSet(Dest->GetDesc().UsageFlags, ETextureUsageFlags::ShaderResourceTexture));

    const bool bGenerateMips   = IsEnumFlagSet(Flags, ETextureFactoryFlags::GenerateMips);
    const bool bDestSupportUAV = IsEnumFlagSet(Dest->GetDesc().UsageFlags, ETextureUsageFlags::UnorderedAccessTexture);

    // If the destination does not support UAVs, create a staging texture that does
    FRHITextureRef StagingTexture;
    if (!bDestSupportUAV)
    {
        FRHITextureDesc TextureDesc = Dest->GetDesc();
        TextureDesc.UsageFlags  |= ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::CopySource;
        TextureDesc.TrackingMode = ERHIResourceStateTrackingMode::Manual;

        StagingTexture = RHI::CreateTexture(TextureDesc, ERHIResourceState::Common, nullptr);
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
    const FRHIUnorderedAccessViewDesc UAVDesc = FRHIUnorderedAccessViewDesc::CreateTextureCube(StagingTexture->GetDesc().Format, 0);

    FRHIUnorderedAccessViewRef StagingTextureUAV = RHI::CreateUnorderedAccessView(StagingTexture.Get(), UAVDesc);
    if (!StagingTextureUAV)
    {
        return false;
    }

    // Schedule work on the GPU
    {
        FRHICommandList CommandList;
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(StagingTexture.Get(), ERHIResourceState::Common, ERHIResourceState::UnorderedAccess));

        CommandList.SetComputePipelineState(PanoramaPSO.Get());

        struct FCubeMapGenConstants
        {
            uint32 CubeMapSize;
        } ShaderConstantData;

        ShaderConstantData.CubeMapSize = StagingTexture->GetDesc().Extent.X;

        CommandList.SetShaderConstants(PanoramCS.Get(), &ShaderConstantData, 1);
        CommandList.SetUnorderedAccessView(PanoramCS.Get(), StagingTextureUAV.Get(), 0);

        FRHIShaderResourceView* PanoramaSourceView = Source->GetShaderResourceView();
        CommandList.SetShaderResourceView(PanoramCS.Get(), PanoramaSourceView, 0);

        CommandList.SetSamplerState(PanoramCS.Get(), LinearSampler.Get(), 0);

        constexpr uint32 LocalWorkGroupCount = 16;
        const uint32 ThreadsX = Math::DivideByMultiple(ShaderConstantData.CubeMapSize, LocalWorkGroupCount);
        const uint32 ThreadsY = Math::DivideByMultiple(ShaderConstantData.CubeMapSize, LocalWorkGroupCount);
        CommandList.Dispatch(ThreadsX, ThreadsY, 6);

        if (!bDestSupportUAV)
        {
            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(StagingTexture.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::CopySource));
            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Dest, ERHIResourceState::Common, ERHIResourceState::CopyDest));

            CommandList.CopyTexture(Dest, StagingTexture.Get());

            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Dest, ERHIResourceState::Common, ERHIResourceState::ShaderResource));
        }
        else
        {
            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Dest, ERHIResourceState::UnorderedAccess, ERHIResourceState::ShaderResource));
        }

        if (bGenerateMips)
        {
            GenerateMiplevels(CommandList, Dest);
        }

        FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    }

    return true;
}

bool FTextureFactory::GenerateMiplevels(FRHITexture* Texture, const IRHITextureData* Mip0Data)
{
    // Schedule miplevel generation without an existing CommandList
    FRHICommandList CommandList;

    const bool bResult = GenerateMiplevels(CommandList, Texture, Mip0Data);
    if (!bResult)
    {
        return false;
    }

    // Then execute immediately
    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    return true;
}

bool FTextureFactory::GenerateMiplevels(FRHICommandList& CommandList, FRHITexture* Texture, const IRHITextureData* Mip0Data)
{
    CHECK(IsEnumFlagSet(Texture->GetDesc().UsageFlags, ETextureUsageFlags::ShaderResourceTexture));

    if (Texture->GetDesc().NumMipLevels < 2)
    {
        LOG_ERROR("Texture needs to have a full mip-chain allocated");
        return false;
    }

    // Determine if we need a staging resource
    const bool bIsTextureCube  = IsTextureCube(Texture->GetDesc().Dimension);
    const bool bDestSupportUAV = IsEnumFlagSet(Texture->GetDesc().UsageFlags, ETextureUsageFlags::UnorderedAccessTexture);

    // If the destination does not support UAVs, create a staging texture that does
    FRHITextureRef StagingTexture;
    if (!bDestSupportUAV)
    {
        FRHITextureDesc TextureDesc = Texture->GetDesc();
        TextureDesc.UsageFlags  |= ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::CopySource;
        TextureDesc.TrackingMode = ERHIResourceStateTrackingMode::Manual;

        StagingTexture = RHI::CreateTexture(TextureDesc, ERHIResourceState::Common, Mip0Data);
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
    const uint32 NumMipLevels  = StagingTexture->GetDesc().NumMipLevels;
    const uint32 NumDispatches = Math::AlignUp<uint32>(NumMipLevels, MipLevelsPerDispatch) / MipLevelsPerDispatch;

    TArray<FRHIShaderResourceViewRef> ShaderResourceViews;
    ShaderResourceViews.Reserve(NumDispatches);

    for (uint32 MipLevel = 0; MipLevel < NumMipLevels; MipLevel += MipLevelsPerDispatch)
    {
        const FRHIShaderResourceViewDesc SRVDesc = bIsTextureCube
            ? FRHIShaderResourceViewDesc::CreateTextureCube(StagingTexture->GetDesc().Format, static_cast<uint8>(MipLevel), 1)
            : FRHIShaderResourceViewDesc::CreateTexture2D  (StagingTexture->GetDesc().Format, static_cast<uint8>(MipLevel), 1);

        FRHIShaderResourceView* ShaderResourceView = RHI::CreateShaderResourceView(StagingTexture.Get(), SRVDesc);
        ShaderResourceViews.Emplace(ShaderResourceView);
    }

    // Skip the first mip since that will only be used as a source
    TArray<FRHIUnorderedAccessViewRef> UnorderedAccessViews;
    UnorderedAccessViews.Reserve(NumMipLevels - 1);

    for (uint32 MipLevel = 1; MipLevel < NumMipLevels; MipLevel++)
    {
        const FRHIUnorderedAccessViewDesc UAVDesc = bIsTextureCube
            ? FRHIUnorderedAccessViewDesc::CreateTextureCube(StagingTexture->GetDesc().Format, static_cast<uint8>(MipLevel))
            : FRHIUnorderedAccessViewDesc::CreateTexture2D  (StagingTexture->GetDesc().Format, static_cast<uint8>(MipLevel));

        FRHIUnorderedAccessView* UnorderedAccessView = RHI::CreateUnorderedAccessView(StagingTexture.Get(), UAVDesc);
        UnorderedAccessViews.Emplace(UnorderedAccessView);
    }

    if (!bDestSupportUAV)
    {
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(StagingTexture.Get(), ERHIResourceState::Common, ERHIResourceState::NonPixelShaderResource));
    }
    else
    {
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(StagingTexture.Get(), ERHIResourceState::ShaderResource, ERHIResourceState::NonPixelShaderResource));
    }

    // Determine which compute-shader and pipeline-state to use
    FRHIComputeShaderRef        ComputeShader = bIsTextureCube ? GenerateMipsTexCube_CS  : GenerateMipsTex2D_CS;
    FRHIComputePipelineStateRef PipelineState = bIsTextureCube ? GenerateMipsTexCube_PSO : GenerateMipsTex2D_PSO;
    CommandList.SetComputePipelineState(PipelineState.Get());

    struct FGenMipsConstants
    {
        uint32  SrcMipLevel;  // MipLevel to read from
        uint32  NumMipLevels; // Number of MipLevels we want to create (Up to 4)
        Vector2 TexelSize;    // Size of the first destination miplevel
    } ShaderConstantData;

    // Start with size of the destination to be divided by 2 since we are starting with mip 1
    const IntVector3 TextureExtent = StagingTexture->GetDesc().Extent;
    uint32 DstWidth  = static_cast<uint32>(TextureExtent.X) / 2;
    uint32 DstHeight = static_cast<uint32>(TextureExtent.Y) / 2;
    ShaderConstantData.SrcMipLevel = 0;

    constexpr uint32 NumThreadsTexture2D   = 1;
    constexpr uint32 NumThreadsTextureCube = 6;
    const uint32 ThreadsZ = bIsTextureCube ? NumThreadsTextureCube : NumThreadsTexture2D;

    // First miplevel is already finished and will be used as source
    uint32 RemainingMiplevels = NumMipLevels - 1;
    for (uint32 DispatchIndex = 0; DispatchIndex < NumDispatches; DispatchIndex++)
    {
        ShaderConstantData.TexelSize = Vector2(1.0f / static_cast<float>(DstWidth), 1.0f / static_cast<float>(DstHeight));

        const uint32 NumMipLevelsThisBatch = Math::Min<uint32>(MipLevelsPerDispatch, RemainingMiplevels);
        ShaderConstantData.NumMipLevels = NumMipLevelsThisBatch;

        constexpr uint32 NumConstants = sizeof(FGenMipsConstants) / sizeof(uint32);
        CommandList.SetShaderConstants(ComputeShader.Get(), &ShaderConstantData, NumConstants);

        // Bind the original texture
        CommandList.SetShaderResourceView(ComputeShader.Get(), ShaderResourceViews[DispatchIndex].Get(), 0);

        // This is the first mip-level to process, the first mip-level is skipped since that is just used for reading
        const uint32 FirstMipLevelThisBatch = DispatchIndex * MipLevelsPerDispatch;

        // Bind all the UAVs that needs processing this batch
        for (uint32 MipIndex = 0; MipIndex < NumMipLevelsThisBatch; MipIndex++)
        {
            const uint32 CurrentMipLevel = FirstMipLevelThisBatch + MipIndex;
            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTextureMip(StagingTexture.Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::UnorderedAccess, CurrentMipLevel + 1));
            CommandList.SetUnorderedAccessView(ComputeShader.Get(), UnorderedAccessViews[CurrentMipLevel].Get(), MipIndex);
        }

        // Bind null UAVs for all other bindings
        for (uint32 MipIndex = NumMipLevelsThisBatch; MipIndex < MipLevelsPerDispatch; MipIndex++)
        {
            CommandList.SetUnorderedAccessView(ComputeShader.Get(), nullptr, MipIndex);
        }

        // Dispatch work
        constexpr uint32 ThreadCount = 8;
        const uint32 ThreadsX = Math::DivideByMultiple(DstWidth, ThreadCount);
        const uint32 ThreadsY = Math::DivideByMultiple(DstHeight, ThreadCount);
        CommandList.Dispatch(ThreadsX, ThreadsY, ThreadsZ);

        // Transition all resources back
        for (uint32 MipIndex = 0; MipIndex < NumMipLevelsThisBatch; MipIndex++)
        {
            const uint32 CurrentMipLevel = FirstMipLevelThisBatch + MipIndex;
            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTextureMip(StagingTexture.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::NonPixelShaderResource, CurrentMipLevel + 1));
        }

        // Bind null UAVs for all other bindings
        for (uint32 MipIndex = NumMipLevelsThisBatch; MipIndex < MipLevelsPerDispatch; MipIndex++)
        {
            CommandList.SetUnorderedAccessView(ComputeShader.Get(), nullptr, MipIndex);
        }

        DstWidth  = DstWidth / 16;
        DstHeight = DstHeight / 16;

        RemainingMiplevels -= MipLevelsPerDispatch;
    }

    // Copy the staging-resource to the texture
    if (!bDestSupportUAV)
    {
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(StagingTexture.Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::CopySource));
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Texture, ERHIResourceState::ShaderResource, ERHIResourceState::CopyDest));

        CommandList.CopyTexture(Texture, StagingTexture.Get());

        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Texture, ERHIResourceState::CopyDest, ERHIResourceState::ShaderResource));
    }
    else
    {
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Texture, ERHIResourceState::NonPixelShaderResource, ERHIResourceState::ShaderResource));
    }

    return true;
}

bool FTextureFactory::FilterSpecularCubeMap(FRHITexture* SrcCubeMap, FRHITexture* DstCubeMap, uint32 NumMipLevels)
{
    // Schedule miplevel generation without an existing CommandList
    FRHICommandList CommandList;

    const bool bResult = FilterSpecularCubeMap(CommandList, SrcCubeMap, DstCubeMap, NumMipLevels);
    if (!bResult)
    {
        return false;
    }

    // Then execute immediately
    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    return true;
}

bool FTextureFactory::FilterSpecularCubeMap(FRHICommandList& CommandList, FRHITexture* SrcCubeMap, FRHITexture* DstCubeMap, uint32 NumMipLevels)
{
    if (!SrcCubeMap)
    {
        DEBUG_BREAK();
        return false;
    }

    if (!DstCubeMap)
    {
        DEBUG_BREAK();
        return false;
    }

    TArray<FRHIUnorderedAccessViewRef> SpecularIrradianceMapUAVs;

    const int32 SpecularIrradianceMiplevels = DstCubeMap->GetDesc().NumMipLevels;
    for (int32 MipLevel = 0; MipLevel < SpecularIrradianceMiplevels; MipLevel++)
    {
        const FRHIUnorderedAccessViewDesc UAVDesc = FRHIUnorderedAccessViewDesc::CreateTextureCube(DstCubeMap->GetDesc().Format, uint8(MipLevel));

        FRHIUnorderedAccessViewRef UAV = RHI::CreateUnorderedAccessView(DstCubeMap, UAVDesc);
        if (UAV)
        {
            SpecularIrradianceMapUAVs.Emplace(UAV);
        }
        else
        {
            DEBUG_BREAK();
            return false;
        }
    }

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(DstCubeMap, ERHIResourceState::ShaderResource, ERHIResourceState::UnorderedAccess));

    CommandList.SetComputePipelineState(SpecularCubeMapFilter_PSO.Get());
    
    CommandList.SetShaderResourceView(SpecularCubeMapFilter_CS.Get(), SrcCubeMap->GetShaderResourceView(), 0);
    CommandList.SetSamplerState(SpecularCubeMapFilter_CS.Get(), CubeMapFilterSampler.Get(), 0);

    const uint32 SpecularCubeMapSize = DstCubeMap->GetDesc().Extent.X;
    const uint32 NumMiplevels        = Math::Clamp<uint32>(DstCubeMap->GetDesc().NumMipLevels, 1, NumMipLevels);
    const uint32 SkyboxWidth         = SrcCubeMap->GetDesc().Extent.X;
    const float  RoughnessDelta      = 1.0f / (NumMiplevels - 1);

    float  Roughness    = 0.0f;
    uint32 CurrentWidth = SpecularCubeMapSize;
    for (uint32 Mip = 0; Mip < NumMiplevels; Mip++)
    {
        struct FSpecularIrradianceGenConstants
        {
            float  Roughness;
            uint32 SourceFaceResolution;
            uint32 CurrentFaceResolution;
        } Constants;

        Constants.Roughness             = Roughness;
        Constants.SourceFaceResolution  = SkyboxWidth;
        Constants.CurrentFaceResolution = CurrentWidth;

        constexpr uint32 NumConstants = sizeof(FSpecularIrradianceGenConstants) / sizeof(uint32);
        CommandList.SetShaderConstants(SpecularCubeMapFilter_CS.Get(), &Constants, NumConstants);

        FRHIUnorderedAccessView* UnorderedAccessView = SpecularIrradianceMapUAVs[Mip].Get();
        CommandList.SetUnorderedAccessView(SpecularCubeMapFilter_CS.Get(), UnorderedAccessView, 0);

        constexpr uint32 NumThreads = 16;
        constexpr uint32 ThreadsZ   = 6;

        const uint32 ThreadWidth  = Math::DivideByMultiple(CurrentWidth, NumThreads);
        const uint32 ThreadHeight = Math::DivideByMultiple(CurrentWidth, NumThreads);
        CommandList.Dispatch(ThreadWidth, ThreadHeight, ThreadsZ);

        CommandList.UnorderedAccessBarrier(DstCubeMap);

        CurrentWidth = Math::Max<uint32>(CurrentWidth / 2, 1U);
        Roughness += RoughnessDelta;
    }

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(DstCubeMap, ERHIResourceState::UnorderedAccess, ERHIResourceState::ShaderResource));
    return true;
}

bool FTextureFactory::FilterDiffuseCubeMap(FRHITexture* SrcCubeMap, FRHITexture* DstCubeMap)
{
    // Schedule miplevel generation without an existing CommandList
    FRHICommandList CommandList;

    const bool bResult = FilterDiffuseCubeMap(CommandList, SrcCubeMap, DstCubeMap);
    if (!bResult)
    {
        return false;
    }

    // Then execute immediately
    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    return true;
}

bool FTextureFactory::FilterDiffuseCubeMap(FRHICommandList& CommandList, FRHITexture* SrcCubeMap, FRHITexture* DstCubeMap)
{
    if (!SrcCubeMap)
    {
        DEBUG_BREAK();
        return false;
    }

    if (!DstCubeMap)
    {
        DEBUG_BREAK();
        return false;
    }

    const FRHIUnorderedAccessViewDesc UAVDesc = FRHIUnorderedAccessViewDesc::CreateTextureCube(DstCubeMap->GetDesc().Format, 0);

    FRHIUnorderedAccessViewRef DstCubeMapUAV = RHI::CreateUnorderedAccessView(DstCubeMap, UAVDesc);
    if (!DstCubeMapUAV)
    {
        DEBUG_BREAK();
        return false;
    }

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(DstCubeMap, ERHIResourceState::ShaderResource, ERHIResourceState::UnorderedAccess));

    CommandList.SetComputePipelineState(DiffuseCubeMapFilter_PSO.Get());

    CommandList.SetShaderResourceView(DiffuseCubeMapFilter_CS.Get(), SrcCubeMap->GetShaderResourceView(), 0);
    CommandList.SetUnorderedAccessView(DiffuseCubeMapFilter_CS.Get(), DstCubeMapUAV.Get(), 0);
    CommandList.SetSamplerState(DiffuseCubeMapFilter_CS.Get(), CubeMapFilterSampler.Get(), 0);

    constexpr uint32 NumThreads = 16;
    constexpr uint32 ThreadsZ   = 6;

    const uint32 DiffuseCubeMapSize = static_cast<uint32>(DstCubeMap->GetDesc().Extent.X);

    const uint32 ThreadWidth  = Math::DivideByMultiple(DiffuseCubeMapSize, NumThreads);
    const uint32 ThreadHeight = Math::DivideByMultiple(DiffuseCubeMapSize, NumThreads);
    CommandList.Dispatch(ThreadWidth, ThreadHeight, ThreadsZ);

    CommandList.UnorderedAccessBarrier(DstCubeMap);

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(DstCubeMap, ERHIResourceState::UnorderedAccess, ERHIResourceState::ShaderResource));
    return true;
}

bool FTextureFactory::PackMaterialParamsTexture(const FRHITextureRef& AOTexture, const FRHITextureRef& RoughnessTexture, const FRHITextureRef& MetallicTexture, FRHITextureRef& OutTexture)
{
    const FRHITextureRef& SizeRef = AOTexture ? AOTexture : (RoughnessTexture ? RoughnessTexture : MetallicTexture);
    if (!SizeRef)
    {
        LOG_ERROR("[FTextureFactory] PackMaterialParamsTexture: No valid input textures");
        return false;
    }

    const uint32 Width  = SizeRef->GetDesc().Extent.X;
    const uint32 Height = SizeRef->GetDesc().Extent.Y;

    FRHITextureDesc TempDesc = FRHITextureDesc::CreateTexture2D(EFormat::R8G8B8A8_Unorm,
        Width, Height, 1, 1, ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::CopySource);

    FRHITextureRef TempTex = RHI::CreateTexture(TempDesc, ERHIResourceState::UnorderedAccess, nullptr);
    if (!TempTex)
    {
        LOG_ERROR("[FTextureFactory] PackMaterialParamsTexture: Failed to create temporary UAV texture");
        return false;
    }

    FRHITextureDesc OutputDesc = FRHITextureDesc::CreateTexture2D(EFormat::R8G8B8A8_Unorm,
        Width, Height, 1, 1, ETextureUsageFlags::ShaderResourceTexture | ETextureUsageFlags::CopyDest);

    OutTexture = RHI::CreateTexture(OutputDesc, ERHIResourceState::CopyDest, nullptr);
    if (!OutTexture)
    {
        LOG_ERROR("[FTextureFactory] PackMaterialParamsTexture: Failed to create output texture");
        return false;
    }

    // The inputs are finalized textures, which are Static and already rest in ShaderResource
    FRHICommandList CommandList;
    CommandList.SetComputePipelineState(PackMaterialParams_PSO.Get());

    struct FPackMaterialParamsConstants
    {
        uint32 TextureSize[2];
    };

    FPackMaterialParamsConstants PackConstants;
    PackConstants.TextureSize[0] = Width;
    PackConstants.TextureSize[1] = Height;

    constexpr uint32 NumPackConstants = sizeof(FPackMaterialParamsConstants) / sizeof(uint32);
    CommandList.SetShaderConstants(PackMaterialParams_CS.Get(), &PackConstants, NumPackConstants);

    auto SafeGetSRV = [](const FRHITextureRef& Tex) -> FRHIShaderResourceView*
    {
        return Tex ? Tex->GetShaderResourceView() : nullptr;
    };

    CommandList.SetShaderResourceView(PackMaterialParams_CS.Get(), SafeGetSRV(AOTexture), 0);
    CommandList.SetShaderResourceView(PackMaterialParams_CS.Get(), SafeGetSRV(RoughnessTexture), 1);
    CommandList.SetShaderResourceView(PackMaterialParams_CS.Get(), SafeGetSRV(MetallicTexture), 2);
    CommandList.SetUnorderedAccessView(PackMaterialParams_CS.Get(), TempTex->GetUnorderedAccessView(), 0);

    const uint32 ThreadGroupsX = Math::DivideByMultiple(Width, 8u);
    const uint32 ThreadGroupsY = Math::DivideByMultiple(Height, 8u);
    CommandList.Dispatch(ThreadGroupsX, ThreadGroupsY, 1);

    CommandList.UnorderedAccessBarrier(TempTex.Get());
    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(TempTex.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::CopySource));

    FRHITextureCopyDesc CopyDesc;
    CopyDesc.DstArraySlice  = 0;
    CopyDesc.DstMipSlice    = 0;
    CopyDesc.DstPosition    = IntVector3();
    CopyDesc.SrcArraySlice  = 0;
    CopyDesc.SrcMipSlice    = 0;
    CopyDesc.SrcPosition    = IntVector3();
    CopyDesc.Size.X         = Width;
    CopyDesc.Size.Y         = Height;
    CopyDesc.Size.Z         = 1;
    CopyDesc.NumArraySlices = 1;
    CopyDesc.NumMipLevels   = 1;

    CommandList.CopyTextureRegion(OutTexture.Get(), TempTex.Get(), CopyDesc);

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTextureModeChange(OutTexture.Get(), ERHIResourceState::CopyDest, ERHIResourceState::ShaderResource, ERHIResourceStateTrackingMode::Static));

    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    return true;
}

bool FTextureFactory::BakeAlphaIntoAlbedo(const FRHITextureRef& AlbedoTexture, const FRHITextureRef& AlphaTexture, FRHITextureRef& OutTexture)
{
    if (!AlbedoTexture || !AlphaTexture)
    {
        LOG_ERROR("[FTextureFactory] BakeAlphaIntoAlbedo: Missing input texture");
        return false;
    }

    const uint32 Width        = AlbedoTexture->GetDesc().Extent.X;
    const uint32 Height       = AlbedoTexture->GetDesc().Extent.Y;
    const uint32 NumMipLevels = AlbedoTexture->GetDesc().NumMipLevels;

    FRHITextureDesc TempDesc = FRHITextureDesc::CreateTexture2D(EFormat::R8G8B8A8_Unorm,
        Width, Height, 1, 1, ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::CopySource);

    FRHITextureRef TempTex = RHI::CreateTexture(TempDesc, ERHIResourceState::UnorderedAccess, nullptr);
    if (!TempTex)
    {
        LOG_ERROR("[FTextureFactory] BakeAlphaIntoAlbedo: Failed to create temporary UAV texture");
        return false;
    }

    FRHITextureDesc OutputDesc = FRHITextureDesc::CreateTexture2D(EFormat::R8G8B8A8_Unorm, Width, Height, NumMipLevels, 1,
        ETextureUsageFlags::ShaderResourceTexture | ETextureUsageFlags::CopyDest | ETextureUsageFlags::UnorderedAccessTexture);

    OutTexture = RHI::CreateTexture(OutputDesc, ERHIResourceState::CopyDest, nullptr);
    if (!OutTexture)
    {
        LOG_ERROR("[FTextureFactory] BakeAlphaIntoAlbedo: Failed to create output texture");
        return false;
    }

    // The inputs are finalized textures, which are Static and already rest in ShaderResource
    FRHICommandList CommandList;
    CommandList.SetComputePipelineState(BakeAlpha_PSO.Get());

    struct FBakeAlphaConstants
    {
        uint32 TextureSize[2];
    };

    FBakeAlphaConstants BakeConstants;
    BakeConstants.TextureSize[0] = Width;
    BakeConstants.TextureSize[1] = Height;

    constexpr uint32 NumBakeConstants = sizeof(FBakeAlphaConstants) / sizeof(uint32);
    CommandList.SetShaderConstants(BakeAlpha_CS.Get(), &BakeConstants, NumBakeConstants);

    CommandList.SetShaderResourceView(BakeAlpha_CS.Get(), AlbedoTexture->GetShaderResourceView(), 0);
    CommandList.SetShaderResourceView(BakeAlpha_CS.Get(), AlphaTexture->GetShaderResourceView(), 1);
    CommandList.SetSamplerState(BakeAlpha_CS.Get(), LinearSampler.Get(), 0);
    CommandList.SetUnorderedAccessView(BakeAlpha_CS.Get(), TempTex->GetUnorderedAccessView(), 0);

    const uint32 ThreadGroupsX = Math::DivideByMultiple(Width, 8u);
    const uint32 ThreadGroupsY = Math::DivideByMultiple(Height, 8u);
    CommandList.Dispatch(ThreadGroupsX, ThreadGroupsY, 1);

    CommandList.UnorderedAccessBarrier(TempTex.Get());
    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(TempTex.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::CopySource));

    FRHITextureCopyDesc CopyDesc;
    CopyDesc.DstArraySlice  = 0;
    CopyDesc.DstMipSlice    = 0;
    CopyDesc.DstPosition    = IntVector3();
    CopyDesc.SrcArraySlice  = 0;
    CopyDesc.SrcMipSlice    = 0;
    CopyDesc.SrcPosition    = IntVector3();
    CopyDesc.Size.X         = Width;
    CopyDesc.Size.Y         = Height;
    CopyDesc.Size.Z         = 1;
    CopyDesc.NumArraySlices = 1;
    CopyDesc.NumMipLevels   = 1;

    CommandList.CopyTextureRegion(OutTexture.Get(), TempTex.Get(), CopyDesc);

    if (NumMipLevels > 1)
    {
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(OutTexture.Get(), ERHIResourceState::CopyDest, ERHIResourceState::ShaderResource));

        if (GenerateMiplevels(CommandList, OutTexture.Get()))
        {
            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTextureModeChange(OutTexture.Get(), ERHIResourceState::ShaderResource, ERHIResourceState::ShaderResource, ERHIResourceStateTrackingMode::Static));
        }
    }
    else
    {
        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTextureModeChange(OutTexture.Get(), ERHIResourceState::CopyDest, ERHIResourceState::ShaderResource, ERHIResourceStateTrackingMode::Static));
    }

    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    return true;
}
