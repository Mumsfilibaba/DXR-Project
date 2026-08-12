#include "Core/Templates/NumericLimits.h"
#include "RHI/RHI.h"
#include "RendererCore/Shaders/ShaderCache.h"
#include "RendererCore/TextureCompressor.h"

#define BC_BLOCK_SIZE int32(4)
#define CS_NUM_THREADS (8)
#define BC7_UNORM_FORMAT (98)
#define BC7_THREAD_GROUP (64)
#define BC7_BLOCKS_PER_GROUP_456 (4)
#define BC7_BLOCKS_PER_GROUP_137 (1)
#define BC7_BLOCKS_PER_GROUP_02 (1)
#define BC7_BLOCKS_PER_GROUP_ENC (4)

struct FCompressionBufferHLSL
{
    uint32  TextureSizeInBlocks[2];
    Vector2 TextureSizeRcp;
};

struct FBC7CompressionBufferHLSL
{
    uint32 TexWidth;
    uint32 NumBlockX;
    uint32 Format;
    uint32 ModeId;
    uint32 StartBlockId;
    uint32 NumTotalBlocks;
    float  AlphaWeight;
};

class FBlockCompressionBC1CS
{
    DECLARE_SHADER_TYPE(FBlockCompressionBC1CS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<>;
};

IMPLEMENT_SHADER_TYPE(FBlockCompressionBC1CS, "Shaders/BlockCompression/BlockCompressionBC1.hlsl", "Main", EShaderModel::SM_6_2);

class FBlockCompressionBC2CS
{
    DECLARE_SHADER_TYPE(FBlockCompressionBC2CS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<>;
};

IMPLEMENT_SHADER_TYPE(FBlockCompressionBC2CS, "Shaders/BlockCompression/BlockCompressionBC2.hlsl", "Main", EShaderModel::SM_6_2);

class FBlockCompressionBC3CS
{
    DECLARE_SHADER_TYPE(FBlockCompressionBC3CS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<>;
};

IMPLEMENT_SHADER_TYPE(FBlockCompressionBC3CS, "Shaders/BlockCompression/BlockCompressionBC3.hlsl", "Main", EShaderModel::SM_6_2);

class FBlockCompressionBC4CS
{
    DECLARE_SHADER_TYPE(FBlockCompressionBC4CS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<>;
};

IMPLEMENT_SHADER_TYPE(FBlockCompressionBC4CS, "Shaders/BlockCompression/BlockCompressionBC4.hlsl", "Main", EShaderModel::SM_6_2);

class FBlockCompressionBC5CS
{
    DECLARE_SHADER_TYPE(FBlockCompressionBC5CS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<>;
};

IMPLEMENT_SHADER_TYPE(FBlockCompressionBC5CS, "Shaders/BlockCompression/BlockCompressionBC5.hlsl", "Main", EShaderModel::SM_6_2);

class FBC6HCubeMap : SHADER_PERMUTATION_BOOL("ENABLE_CUBE_MAP");

class FBlockCompressionBC6HCS
{
    DECLARE_SHADER_TYPE(FBlockCompressionBC6HCS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<FBC6HCubeMap>;
};

IMPLEMENT_SHADER_TYPE(FBlockCompressionBC6HCS, "Shaders/BlockCompression/BlockCompressionBC6H.hlsl", "Main", EShaderModel::SM_6_2);

class FBC7TryMode456CS
{
    DECLARE_SHADER_TYPE(FBC7TryMode456CS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<>;
};

IMPLEMENT_SHADER_TYPE(FBC7TryMode456CS, "Shaders/BlockCompression/BlockCompressionBC7.hlsl", "TryMode456CS", EShaderModel::SM_6_2);

class FBC7TryMode137CS
{
    DECLARE_SHADER_TYPE(FBC7TryMode137CS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<>;
};

IMPLEMENT_SHADER_TYPE(FBC7TryMode137CS, "Shaders/BlockCompression/BlockCompressionBC7.hlsl", "TryMode137CS", EShaderModel::SM_6_2);

class FBC7TryMode02CS
{
    DECLARE_SHADER_TYPE(FBC7TryMode02CS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<>;
};

IMPLEMENT_SHADER_TYPE(FBC7TryMode02CS, "Shaders/BlockCompression/BlockCompressionBC7.hlsl", "TryMode02CS", EShaderModel::SM_6_2);

class FBC7EncodeBlockCS
{
    DECLARE_SHADER_TYPE(FBC7EncodeBlockCS, EShaderStage::Compute);

    using FPermutation = TShaderPermutation<>;

    static void ModifyCompilationEnvironment(const FShaderPermutationDesc&, FShaderCompilationEnvironment& Environment)
    {
        Environment.SetDefine("BC7_ENCODE_ONLY", "(1)");
    }
};

IMPLEMENT_SHADER_TYPE(FBC7EncodeBlockCS, "Shaders/BlockCompression/BlockCompressionBC7.hlsl", "EncodeBlockCS", EShaderModel::SM_6_2);

template<typename ShaderType>
static bool AcquireComputePipeline(FRHIComputeShaderRef& OutShader, FRHIComputePipelineStateRef& OutPSO,
    TArrayView<const FRHIStaticSamplerInfo> StaticSamplers = TArrayView<const FRHIStaticSamplerInfo>(),
    const typename ShaderType::FPermutation& Permutation = typename ShaderType::FPermutation())
{
    const CHAR* ShaderName = ShaderType::GetStaticType().GetName();

    OutShader = FShaderCache::Get().GetShader<ShaderType>(Permutation);
    if (!OutShader)
    {
        LOG_ERROR("[FTextureCompressor] Failed to create compute shader: %s", ShaderName);
        return false;
    }

    FRHIComputePipelineStateDesc PSODesc;
    PSODesc.Shader         = OutShader.Get();
    PSODesc.StaticSamplers = StaticSamplers;

    OutPSO = RHI::CreateComputePipelineState(PSODesc);
    if (!OutPSO)
    {
        LOG_ERROR("[FTextureCompressor] Failed to create PSO: %s", ShaderName);
        return false;
    }

    OutPSO->SetDebugName(ShaderName);
    return true;
}

FTextureCompressor::FTextureCompressor()
    : BC1CompressionShader(nullptr)
    , BC1CompressionPSO(nullptr)
    , BC2CompressionShader(nullptr)
    , BC2CompressionPSO(nullptr)
    , BC3CompressionShader(nullptr)
    , BC3CompressionPSO(nullptr)
    , BC4CompressionShader(nullptr)
    , BC4CompressionPSO(nullptr)
    , BC5CompressionShader(nullptr)
    , BC5CompressionPSO(nullptr)
    , BC6HCompressionShader(nullptr)
    , BC6HCompressionPSO(nullptr)
    , BC6HCompressionCubeShader(nullptr)
    , BC6HCompressionCubePSO(nullptr)
    , BC7TryMode456Shader(nullptr)
    , BC7TryMode456PSO(nullptr)
    , BC7TryMode137Shader(nullptr)
    , BC7TryMode137PSO(nullptr)
    , BC7TryMode02Shader(nullptr)
    , BC7TryMode02PSO(nullptr)
    , BC7EncodeBlockShader(nullptr)
    , BC7EncodeBlockPSO(nullptr)
{
}

FTextureCompressor::~FTextureCompressor()
{
}

bool FTextureCompressor::Initialize()
{
    FRHISamplerStateDesc SamplerDesc;
    SamplerDesc.AddressU = ESamplerMode::Wrap;
    SamplerDesc.AddressV = ESamplerMode::Wrap;
    SamplerDesc.AddressW = ESamplerMode::Wrap;
    SamplerDesc.Filter   = ESamplerFilter::MinMagMipLinear;
    SamplerDesc.MinLOD   = 0.0f;
    SamplerDesc.MaxLOD   = TNumericLimits<float>::Max();

    PointSampler = RHI::CreateSamplerState(SamplerDesc);
    if (!PointSampler)
    {
        return false;
    }

    if (!InitializeBC1ToBC5())
    {
        return false;
    }

    if (!InitializeBC6H())
    {
        return false;
    }

    if (!InitializeBC7())
    {
        return false;
    }

    return true;
}

bool FTextureCompressor::InitializeBC1ToBC5()
{
    FRHIStaticSamplerInfo StaticSampler;
    StaticSampler.AddressU = ESamplerMode::Clamp;
    StaticSampler.AddressV = ESamplerMode::Clamp;
    StaticSampler.AddressW = ESamplerMode::Clamp;
    StaticSampler.Filter   = ESamplerFilter::MinMagMipPoint;
    StaticSampler.MinLOD   = 0.0f;
    StaticSampler.MaxLOD   = 0.0f;

    const TArrayView<const FRHIStaticSamplerInfo> StaticSamplers(&StaticSampler, 1);

    if (!AcquireComputePipeline<FBlockCompressionBC1CS>(BC1CompressionShader, BC1CompressionPSO, StaticSamplers))
    {
        return false;
    }

    if (!AcquireComputePipeline<FBlockCompressionBC2CS>(BC2CompressionShader, BC2CompressionPSO, StaticSamplers))
    {
        return false;
    }

    if (!AcquireComputePipeline<FBlockCompressionBC3CS>(BC3CompressionShader, BC3CompressionPSO, StaticSamplers))
    {
        return false;
    }

    if (!AcquireComputePipeline<FBlockCompressionBC4CS>(BC4CompressionShader, BC4CompressionPSO, StaticSamplers))
    {
        return false;
    }

    if (!AcquireComputePipeline<FBlockCompressionBC5CS>(BC5CompressionShader, BC5CompressionPSO, StaticSamplers))
    {
        return false;
    }

    return true;
}

bool FTextureCompressor::InitializeBC6H()
{
    FRHIStaticSamplerInfo BC6HStaticSampler;
    BC6HStaticSampler.AddressU = ESamplerMode::Wrap;
    BC6HStaticSampler.AddressV = ESamplerMode::Wrap;
    BC6HStaticSampler.AddressW = ESamplerMode::Wrap;
    BC6HStaticSampler.Filter   = ESamplerFilter::MinMagMipLinear;
    BC6HStaticSampler.MinLOD   = 0.0f;
    BC6HStaticSampler.MaxLOD   = TNumericLimits<float>::Max();

    const TArrayView<const FRHIStaticSamplerInfo> StaticSamplers(&BC6HStaticSampler, 1);

    FBlockCompressionBC6HCS::FPermutation Permutation;
    Permutation.Set<FBC6HCubeMap>(false);

    if (!AcquireComputePipeline<FBlockCompressionBC6HCS>(BC6HCompressionShader, BC6HCompressionPSO, StaticSamplers, Permutation))
    {
        return false;
    }

    Permutation.Set<FBC6HCubeMap>(true);

    if (!AcquireComputePipeline<FBlockCompressionBC6HCS>(BC6HCompressionCubeShader, BC6HCompressionCubePSO, StaticSamplers, Permutation))
    {
        return false;
    }

    return true;
}

bool FTextureCompressor::CompressSinglePass64(FRHICommandList& CommandList, const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture, FRHIComputeShader* Shader, FRHIComputePipelineState* PSO, EFormat OutputFormat)
{
    const FRHITextureDesc SourceDesc = SrcTexture->GetDesc();
    if (!IsBlockCompressedAligned(SourceDesc.Extent.X) || !IsBlockCompressedAligned(SourceDesc.Extent.Y))
    {
        LOG_ERROR("[FTextureCompressor] Cannot compress a texture with dimensions that are not a multiple of 4");
        return false;
    }

    const int32 BlocksX  = Math::DivideByMultiple(SourceDesc.Extent.X, BC_BLOCK_SIZE);
    const int32 BlocksY  = Math::DivideByMultiple(SourceDesc.Extent.Y, BC_BLOCK_SIZE);
    const uint32 NumMips = Math::Min(Math::MipCountAboveMinSize(SourceDesc.Extent.X, SourceDesc.Extent.Y, BC_BLOCK_SIZE), SourceDesc.NumMipLevels);

    FRHITextureDesc CompressedTexDesc = FRHITextureDesc::CreateTexture2D(EFormat::R32G32_Uint, BlocksX, BlocksY, NumMips, 1, ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::CopySource);
    FRHITextureRef  CompressedTex     = RHI::CreateTexture(CompressedTexDesc, ERHIResourceState::UnorderedAccess);

    if (!CompressedTex)
    {
        LOG_ERROR("[FTextureCompressor] Failed to create temporary compressed texture");
        return false;
    }

    FRHITextureDesc OutputDesc = FRHITextureDesc::CreateTexture2D(OutputFormat, SourceDesc.Extent.X, SourceDesc.Extent.Y, NumMips, 1, ETextureUsageFlags::ShaderResourceTexture | ETextureUsageFlags::CopyDest);
    OutTexture = RHI::CreateTexture(OutputDesc, ERHIResourceState::CopyDest);

    if (!OutTexture)
    {
        LOG_ERROR("[FTextureCompressor] Failed to create compressed texture");
        return false;
    }

    TArray<FRHIShaderResourceViewRef> SourceSRVs;
    SourceSRVs.Reserve(NumMips);

    TArray<FRHIUnorderedAccessViewRef> CompressedUAVs;
    CompressedUAVs.Reserve(NumMips);

    for (uint32 Mip = 0; Mip < NumMips; Mip++)
    {
        const FRHIShaderResourceViewDesc SRVDesc = FRHIShaderResourceViewDesc::CreateTexture2D(SrcTexture->GetDesc().Format, uint8(Mip), 1);

        FRHIShaderResourceViewRef SourceSRV = RHI::CreateShaderResourceView(SrcTexture.Get(), SRVDesc);
        if (!SourceSRV)
        {
            LOG_ERROR("[FTextureCompressor] Failed to create source SRV for mip %u", Mip);
            return false;
        }

        SourceSRVs.Emplace(SourceSRV);

        const FRHIUnorderedAccessViewDesc UAVDesc = FRHIUnorderedAccessViewDesc::CreateTexture2D(EFormat::R32G32_Uint, uint8(Mip));

        FRHIUnorderedAccessViewRef CompressedUAV = RHI::CreateUnorderedAccessView(CompressedTex.Get(), UAVDesc);
        if (!CompressedUAV)
        {
            LOG_ERROR("[FTextureCompressor] Failed to create compressed UAV for mip %u", Mip);
            return false;
        }

        CompressedUAVs.Emplace(CompressedUAV);
    }

    CommandList.SetComputePipelineState(PSO);

    int32 MipWidth  = SourceDesc.Extent.X;
    int32 MipHeight = SourceDesc.Extent.Y;

    for (uint32 Mip = 0; Mip < NumMips; Mip++)
    {
        const int32 MipBlocksX = Math::DivideByMultiple(MipWidth, BC_BLOCK_SIZE);
        const int32 MipBlocksY = Math::DivideByMultiple(MipHeight, BC_BLOCK_SIZE);

        CommandList.SetShaderResourceView(Shader, SourceSRVs[Mip].Get(), 0);
        CommandList.SetUnorderedAccessView(Shader, CompressedUAVs[Mip].Get(), 0);

        FCompressionBufferHLSL Buffer;
        Buffer.TextureSizeInBlocks[0] = Math::AlignUp(MipBlocksX, BC_BLOCK_SIZE);
        Buffer.TextureSizeInBlocks[1] = Math::AlignUp(MipBlocksY, BC_BLOCK_SIZE);
        Buffer.TextureSizeRcp         = Vector2(1.0f / static_cast<float>(MipWidth), 1.0f / static_cast<float>(MipHeight));

        constexpr uint32 NumConstants = sizeof(FCompressionBufferHLSL) / sizeof(uint32);
        CommandList.SetShaderConstants(Shader, &Buffer, NumConstants);

        const int32 ThreadGroupsX = Math::DivideByMultiple(MipBlocksX, int32(CS_NUM_THREADS));
        const int32 ThreadGroupsY = Math::DivideByMultiple(MipBlocksY, int32(CS_NUM_THREADS));
        CommandList.Dispatch(ThreadGroupsX, ThreadGroupsY, 1);
        CommandList.UnorderedAccessBarrier(CompressedTex.Get());

        MipWidth  = Math::Max(MipWidth / 2, 1);
        MipHeight = Math::Max(MipHeight / 2, 1);
    }

    FRHITextureCopyDesc CopyDesc;
    CopyDesc.DstArraySlice  = 0;
    CopyDesc.DstMipSlice    = 0;
    CopyDesc.DstPosition    = IntVector3();
    CopyDesc.SrcArraySlice  = 0;
    CopyDesc.SrcMipSlice    = 0;
    CopyDesc.SrcPosition    = IntVector3();
    CopyDesc.Size.X         = BlocksX;
    CopyDesc.Size.Y         = BlocksY;
    CopyDesc.Size.Z         = 1;
    CopyDesc.NumArraySlices = 1;
    CopyDesc.NumMipLevels   = NumMips;

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(CompressedTex.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::CopySource));
    CommandList.CopyTextureRegion(OutTexture.Get(), CompressedTex.Get(), CopyDesc);
    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTextureModeChange(
        OutTexture.Get(), ERHIResourceState::CopyDest, ERHIResourceState::ShaderResource, ERHIResourceStateTrackingMode::Static));
    return true;
}

bool FTextureCompressor::CompressSinglePass128(FRHICommandList& CommandList, const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture, FRHIComputeShader* Shader, FRHIComputePipelineState* PSO, EFormat OutputFormat)
{
    const FRHITextureDesc SourceDesc = SrcTexture->GetDesc();
    if (!IsBlockCompressedAligned(SourceDesc.Extent.X) || !IsBlockCompressedAligned(SourceDesc.Extent.Y))
    {
        LOG_ERROR("[FTextureCompressor] Cannot compress a texture with dimensions that are not a multiple of 4");
        return false;
    }

    const int32 BlocksX  = Math::DivideByMultiple(SourceDesc.Extent.X, BC_BLOCK_SIZE);
    const int32 BlocksY  = Math::DivideByMultiple(SourceDesc.Extent.Y, BC_BLOCK_SIZE);
    const uint32 NumMips = Math::Min(Math::MipCountAboveMinSize(SourceDesc.Extent.X, SourceDesc.Extent.Y, BC_BLOCK_SIZE), SourceDesc.NumMipLevels);

    FRHITextureDesc CompressedTexDesc = FRHITextureDesc::CreateTexture2D(EFormat::R32G32B32A32_Uint, BlocksX, BlocksY, NumMips, 1, ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::CopySource);
    FRHITextureRef CompressedTex = RHI::CreateTexture(CompressedTexDesc, ERHIResourceState::UnorderedAccess);
    if (!CompressedTex)
    {
        LOG_ERROR("[FTextureCompressor] Failed to create temporary compressed texture");
        return false;
    }

    FRHITextureDesc OutputDesc = FRHITextureDesc::CreateTexture2D(OutputFormat, SourceDesc.Extent.X, SourceDesc.Extent.Y, NumMips, 1, ETextureUsageFlags::ShaderResourceTexture | ETextureUsageFlags::CopyDest);
    OutTexture = RHI::CreateTexture(OutputDesc, ERHIResourceState::CopyDest);
    if (!OutTexture)
    {
        LOG_ERROR("[FTextureCompressor] Failed to create compressed texture");
        return false;
    }

    TArray<FRHIShaderResourceViewRef> SourceSRVs;
    SourceSRVs.Reserve(NumMips);

    TArray<FRHIUnorderedAccessViewRef> CompressedUAVs;
    CompressedUAVs.Reserve(NumMips);

    for (uint32 Mip = 0; Mip < NumMips; Mip++)
    {
        const FRHIShaderResourceViewDesc SRVDesc = FRHIShaderResourceViewDesc::CreateTexture2D(SrcTexture->GetDesc().Format, uint8(Mip), 1);

        FRHIShaderResourceViewRef SourceSRV = RHI::CreateShaderResourceView(SrcTexture.Get(), SRVDesc);
        if (!SourceSRV)
        {
            LOG_ERROR("[FTextureCompressor] Failed to create source SRV for mip %u", Mip);
            return false;
        }

        SourceSRVs.Emplace(SourceSRV);

        const FRHIUnorderedAccessViewDesc UAVDesc = FRHIUnorderedAccessViewDesc::CreateTexture2D(EFormat::R32G32B32A32_Uint, uint8(Mip));

        FRHIUnorderedAccessViewRef CompressedUAV = RHI::CreateUnorderedAccessView(CompressedTex.Get(), UAVDesc);
        if (!CompressedUAV)
        {
            LOG_ERROR("[FTextureCompressor] Failed to create compressed UAV for mip %u", Mip);
            return false;
        }

        CompressedUAVs.Emplace(CompressedUAV);
    }

    CommandList.SetComputePipelineState(PSO);

    int32 MipWidth  = SourceDesc.Extent.X;
    int32 MipHeight = SourceDesc.Extent.Y;

    for (uint32 Mip = 0; Mip < NumMips; Mip++)
    {
        const int32 MipBlocksX = Math::DivideByMultiple(MipWidth, BC_BLOCK_SIZE);
        const int32 MipBlocksY = Math::DivideByMultiple(MipHeight, BC_BLOCK_SIZE);

        CommandList.SetShaderResourceView(Shader, SourceSRVs[Mip].Get(), 0);
        CommandList.SetUnorderedAccessView(Shader, CompressedUAVs[Mip].Get(), 0);

        FCompressionBufferHLSL Buffer;
        Buffer.TextureSizeInBlocks[0] = Math::AlignUp(MipBlocksX, BC_BLOCK_SIZE);
        Buffer.TextureSizeInBlocks[1] = Math::AlignUp(MipBlocksY, BC_BLOCK_SIZE);
        Buffer.TextureSizeRcp         = Vector2(1.0f / static_cast<float>(MipWidth), 1.0f / static_cast<float>(MipHeight));

        constexpr uint32 NumConstants = sizeof(FCompressionBufferHLSL) / sizeof(uint32);
        CommandList.SetShaderConstants(Shader, &Buffer, NumConstants);

        const int32 ThreadGroupsX = Math::DivideByMultiple(MipBlocksX, int32(CS_NUM_THREADS));
        const int32 ThreadGroupsY = Math::DivideByMultiple(MipBlocksY, int32(CS_NUM_THREADS));
        CommandList.Dispatch(ThreadGroupsX, ThreadGroupsY, 1);
        CommandList.UnorderedAccessBarrier(CompressedTex.Get());

        MipWidth  = Math::Max(MipWidth / 2, 1);
        MipHeight = Math::Max(MipHeight / 2, 1);
    }

    FRHITextureCopyDesc CopyDesc;
    CopyDesc.DstArraySlice  = 0;
    CopyDesc.DstMipSlice    = 0;
    CopyDesc.DstPosition    = IntVector3();
    CopyDesc.SrcArraySlice  = 0;
    CopyDesc.SrcMipSlice    = 0;
    CopyDesc.SrcPosition    = IntVector3();
    CopyDesc.Size.X         = BlocksX;
    CopyDesc.Size.Y         = BlocksY;
    CopyDesc.Size.Z         = 1;
    CopyDesc.NumArraySlices = 1;
    CopyDesc.NumMipLevels   = NumMips;

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(CompressedTex.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::CopySource));
    CommandList.CopyTextureRegion(OutTexture.Get(), CompressedTex.Get(), CopyDesc);
    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTextureModeChange(
        OutTexture.Get(), ERHIResourceState::CopyDest, ERHIResourceState::ShaderResource, ERHIResourceStateTrackingMode::Static));
    return true;
}

bool FTextureCompressor::CompressBC1(const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture)
{
    FRHICommandList CommandList;
    if (!CompressBC1(CommandList, SrcTexture, OutTexture))
    {
        return false;
    }

    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    return true;
}

bool FTextureCompressor::CompressBC1(FRHICommandList& CommandList, const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture)
{
    return CompressSinglePass64(CommandList, SrcTexture, OutTexture, BC1CompressionShader.Get(), BC1CompressionPSO.Get(), EFormat::BC1_UNorm);
}

bool FTextureCompressor::CompressBC2(const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture)
{
    FRHICommandList CommandList;
    if (!CompressBC2(CommandList, SrcTexture, OutTexture))
    {
        return false;
    }

    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    return true;
}

bool FTextureCompressor::CompressBC2(FRHICommandList& CommandList, const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture)
{
    return CompressSinglePass128(CommandList, SrcTexture, OutTexture, BC2CompressionShader.Get(), BC2CompressionPSO.Get(), EFormat::BC2_UNorm);
}

bool FTextureCompressor::CompressBC3(const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture)
{
    FRHICommandList CommandList;
    if (!CompressBC3(CommandList, SrcTexture, OutTexture))
    {
        return false;
    }

    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    return true;
}

bool FTextureCompressor::CompressBC3(FRHICommandList& CommandList, const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture)
{
    return CompressSinglePass128(CommandList, SrcTexture, OutTexture, BC3CompressionShader.Get(), BC3CompressionPSO.Get(), EFormat::BC3_UNorm);
}

bool FTextureCompressor::CompressBC4(const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture)
{
    FRHICommandList CommandList;
    if (!CompressBC4(CommandList, SrcTexture, OutTexture))
    {
        return false;
    }

    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    return true;
}

bool FTextureCompressor::CompressBC4(FRHICommandList& CommandList, const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture)
{
    return CompressSinglePass64(CommandList, SrcTexture, OutTexture, BC4CompressionShader.Get(), BC4CompressionPSO.Get(), EFormat::BC4_UNorm);
}

bool FTextureCompressor::CompressBC5(const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture)
{
    FRHICommandList CommandList;
    if (!CompressBC5(CommandList, SrcTexture, OutTexture))
    {
        return false;
    }

    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    return true;
}

bool FTextureCompressor::CompressBC5(FRHICommandList& CommandList, const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture)
{
    return CompressSinglePass128(CommandList, SrcTexture, OutTexture, BC5CompressionShader.Get(), BC5CompressionPSO.Get(), EFormat::BC5_UNorm);
}

bool FTextureCompressor::CompressBC6(const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture)
{
    FRHICommandList CommandList;

    const bool bResult = CompressCubeMapBC6(CommandList, SrcTexture, OutTexture);
    if (!bResult)
    {
        return false;
    }

    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    return true;
}

bool FTextureCompressor::CompressBC6(FRHICommandList& CommandList, const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture)
{
    const FRHITextureDesc SourceDesc = SrcTexture->GetDesc();
    if (!IsBlockCompressedAligned(SourceDesc.Extent.X) || !IsBlockCompressedAligned(SourceDesc.Extent.Y))
    {
        LOG_ERROR("[FTextureCompressor] Cannot compress a texture with dimensions that are not a multiple of 4");
        return false;
    }

    const int32 BlocksX  = Math::DivideByMultiple(SourceDesc.Extent.X, BC_BLOCK_SIZE);
    const int32 BlocksY  = Math::DivideByMultiple(SourceDesc.Extent.Y, BC_BLOCK_SIZE);
    const uint32 NumMips = Math::Min(Math::MipCountAboveMinSize(SourceDesc.Extent.X, SourceDesc.Extent.Y, BC_BLOCK_SIZE), SourceDesc.NumMipLevels);

    FRHITextureDesc CompressedTexDesc = FRHITextureDesc::CreateTexture2D(EFormat::R32G32B32A32_Uint, BlocksX, BlocksY, NumMips, 1, ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::CopySource);
    FRHITextureRef  CompressedTex     = RHI::CreateTexture(CompressedTexDesc, ERHIResourceState::UnorderedAccess);
    
    if (!CompressedTex)
    {
        LOG_ERROR("[FTextureCompressor] Failed to create temporary compressed texture");
        return false;
    }

    FRHITextureDesc OutputDesc = FRHITextureDesc::CreateTexture2D(EFormat::BC6H_UF16, SourceDesc.Extent.X, SourceDesc.Extent.Y, NumMips, 1, ETextureUsageFlags::ShaderResourceTexture | ETextureUsageFlags::CopyDest);
    OutTexture = RHI::CreateTexture(OutputDesc, ERHIResourceState::CopyDest);
    
    if (!OutTexture)
    {
        LOG_ERROR("[FTextureCompressor] Failed to create compressed texture");
        return false;
    }

    TArray<FRHIShaderResourceViewRef> SourceSRVs;
    SourceSRVs.Reserve(NumMips);

    TArray<FRHIUnorderedAccessViewRef> CompressedUAVs;
    CompressedUAVs.Reserve(NumMips);

    for (uint32 Mip = 0; Mip < NumMips; Mip++)
    {
        const FRHIShaderResourceViewDesc SRVDesc = FRHIShaderResourceViewDesc::CreateTexture2D(SrcTexture->GetDesc().Format, uint8(Mip), 1);

        FRHIShaderResourceViewRef SourceSRV = RHI::CreateShaderResourceView(SrcTexture.Get(), SRVDesc);
        if (!SourceSRV)
        {
            LOG_ERROR("[FTextureCompressor] Failed to create source SRV for mip %u", Mip);
            return false;
        }

        SourceSRVs.Emplace(SourceSRV);

        const FRHIUnorderedAccessViewDesc UAVDesc = FRHIUnorderedAccessViewDesc::CreateTexture2D(EFormat::R32G32B32A32_Uint, uint8(Mip));

        FRHIUnorderedAccessViewRef CompressedUAV = RHI::CreateUnorderedAccessView(CompressedTex.Get(), UAVDesc);
        if (!CompressedUAV)
        {
            LOG_ERROR("[FTextureCompressor] Failed to create compressed UAV for mip %u", Mip);
            return false;
        }

        CompressedUAVs.Emplace(CompressedUAV);
    }

    CommandList.SetComputePipelineState(BC6HCompressionPSO.Get());

    int32 MipWidth  = SourceDesc.Extent.X;
    int32 MipHeight = SourceDesc.Extent.Y;

    for (uint32 Mip = 0; Mip < NumMips; Mip++)
    {
        const int32 MipBlocksX = Math::DivideByMultiple(MipWidth, BC_BLOCK_SIZE);
        const int32 MipBlocksY = Math::DivideByMultiple(MipHeight, BC_BLOCK_SIZE);

        CommandList.SetShaderResourceView(BC6HCompressionShader.Get(), SourceSRVs[Mip].Get(), 0);
        CommandList.SetUnorderedAccessView(BC6HCompressionShader.Get(), CompressedUAVs[Mip].Get(), 0);

        FCompressionBufferHLSL Buffer;
        Buffer.TextureSizeInBlocks[0] = Math::AlignUp(MipBlocksX, BC_BLOCK_SIZE);
        Buffer.TextureSizeInBlocks[1] = Math::AlignUp(MipBlocksY, BC_BLOCK_SIZE);
        Buffer.TextureSizeRcp         = Vector2(1.0f / static_cast<float>(MipWidth), 1.0f / static_cast<float>(MipHeight));

        constexpr uint32 NumConstants = sizeof(FCompressionBufferHLSL) / sizeof(uint32);
        CommandList.SetShaderConstants(BC6HCompressionShader.Get(), &Buffer, NumConstants);

        const int32 ThreadGroupsX = Math::DivideByMultiple(MipBlocksX, int32(CS_NUM_THREADS));
        const int32 ThreadGroupsY = Math::DivideByMultiple(MipBlocksY, int32(CS_NUM_THREADS));
        CommandList.Dispatch(ThreadGroupsX, ThreadGroupsY, 1);
        CommandList.UnorderedAccessBarrier(CompressedTex.Get());

        MipWidth  = Math::Max(MipWidth / 2, 1);
        MipHeight = Math::Max(MipHeight / 2, 1);
    }

    FRHITextureCopyDesc CopyDesc;
    CopyDesc.DstArraySlice  = 0;
    CopyDesc.DstMipSlice    = 0;
    CopyDesc.DstPosition    = IntVector3();
    CopyDesc.SrcArraySlice  = 0;
    CopyDesc.SrcMipSlice    = 0;
    CopyDesc.SrcPosition    = IntVector3();
    CopyDesc.Size.X         = BlocksX;
    CopyDesc.Size.Y         = BlocksY;
    CopyDesc.Size.Z         = 1;
    CopyDesc.NumArraySlices = 1;
    CopyDesc.NumMipLevels   = NumMips;

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(CompressedTex.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::CopySource));
    CommandList.CopyTextureRegion(OutTexture.Get(), CompressedTex.Get(), CopyDesc);
    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTextureModeChange(
        OutTexture.Get(), ERHIResourceState::CopyDest, ERHIResourceState::ShaderResource, ERHIResourceStateTrackingMode::Static));
    return true;
}

bool FTextureCompressor::InitializeBC7()
{
    if (!AcquireComputePipeline<FBC7TryMode456CS>(BC7TryMode456Shader, BC7TryMode456PSO))
    {
        return false;
    }

    if (!AcquireComputePipeline<FBC7TryMode137CS>(BC7TryMode137Shader, BC7TryMode137PSO))
    {
        return false;
    }

    if (!AcquireComputePipeline<FBC7TryMode02CS>(BC7TryMode02Shader, BC7TryMode02PSO))
    {
        return false;
    }

    if (!AcquireComputePipeline<FBC7EncodeBlockCS>(BC7EncodeBlockShader, BC7EncodeBlockPSO))
    {
        return false;
    }

    return true;
}

bool FTextureCompressor::CompressBC7(const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture)
{
    FRHICommandList CommandList;
    if (!CompressBC7(CommandList, SrcTexture, OutTexture))
    {
        return false;
    }

    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    return true;
}

bool FTextureCompressor::CompressBC7(FRHICommandList& CommandList, const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture)
{
    const FRHITextureDesc SourceDesc = SrcTexture->GetDesc();
    if (!IsBlockCompressedAligned(SourceDesc.Extent.X) || !IsBlockCompressedAligned(SourceDesc.Extent.Y))
    {
        LOG_ERROR("[FTextureCompressor] BC7: Cannot compress a texture with dimensions that are not a multiple of 4");
        return false;
    }

    const int32 BlocksX         = Math::DivideByMultiple(SourceDesc.Extent.X, BC_BLOCK_SIZE);
    const int32 BlocksY         = Math::DivideByMultiple(SourceDesc.Extent.Y, BC_BLOCK_SIZE);
    const uint32 MaxTotalBlocks = static_cast<uint32>(BlocksX * BlocksY);
    const uint32 NumMips        = Math::Min(Math::MipCountAboveMinSize(SourceDesc.Extent.X, SourceDesc.Extent.Y, BC_BLOCK_SIZE), SourceDesc.NumMipLevels);

    // Structured buffers for mode-selection ping-pong (uint4 per block), sized for the largest mip
    const uint32 StructuredStride = sizeof(uint32) * 4;

    const FRHIBufferDesc BufferDesc = FRHIBufferDesc::CreateStructuredBuffer(StructuredStride,
        MaxTotalBlocks, EBufferFlags::Default | EBufferFlags::UnorderedAccessBuffer);

    FRHIBufferRef BufA = RHI::CreateBuffer(BufferDesc, ERHIResourceState::UnorderedAccess);
    FRHIBufferRef BufB = RHI::CreateBuffer(BufferDesc, ERHIResourceState::UnorderedAccess);

    if (!BufA || !BufB)
    {
        LOG_ERROR("[FTextureCompressor] BC7: Failed to create structured buffers for mode selection");
        return false;
    }

    const FRHIShaderResourceViewDesc  SrvInfoA = FRHIShaderResourceViewDesc::CreateBuffer(0, MaxTotalBlocks);
    const FRHIShaderResourceViewDesc  SrvInfoB = FRHIShaderResourceViewDesc::CreateBuffer(0, MaxTotalBlocks);
    const FRHIUnorderedAccessViewDesc UavInfoA = FRHIUnorderedAccessViewDesc::CreateBuffer(0, MaxTotalBlocks);
    const FRHIUnorderedAccessViewDesc UavInfoB = FRHIUnorderedAccessViewDesc::CreateBuffer(0, MaxTotalBlocks);

    FRHIShaderResourceViewRef  SrvA = RHI::CreateShaderResourceView(BufA.Get(), SrvInfoA);
    FRHIShaderResourceViewRef  SrvB = RHI::CreateShaderResourceView(BufB.Get(), SrvInfoB);
    FRHIUnorderedAccessViewRef UavA = RHI::CreateUnorderedAccessView(BufA.Get(), UavInfoA);
    FRHIUnorderedAccessViewRef UavB = RHI::CreateUnorderedAccessView(BufB.Get(), UavInfoB);

    if (!SrvA || !SrvB || !UavA || !UavB)
    {
        LOG_ERROR("[FTextureCompressor] BC7: Failed to create buffer SRV/UAV views");
        return false;
    }

    // Intermediate texture with full mip chain for EncodeBlockCS output
    FRHITextureDesc CompressedTexDesc = FRHITextureDesc::CreateTexture2D(EFormat::R32G32B32A32_Uint, BlocksX, BlocksY, NumMips, 1, ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::CopySource);
    FRHITextureRef  CompressedTex     = RHI::CreateTexture(CompressedTexDesc, ERHIResourceState::UnorderedAccess);
    
    if (!CompressedTex)
    {
        LOG_ERROR("[FTextureCompressor] BC7: Failed to create temporary compressed texture");
        return false;
    }

    FRHITextureDesc OutputDesc = FRHITextureDesc::CreateTexture2D(EFormat::BC7_UNorm, SourceDesc.Extent.X, SourceDesc.Extent.Y, NumMips, 1, ETextureUsageFlags::ShaderResourceTexture | ETextureUsageFlags::CopyDest);
    OutTexture = RHI::CreateTexture(OutputDesc, ERHIResourceState::CopyDest);
    
    if (!OutTexture)
    {
        LOG_ERROR("[FTextureCompressor] BC7: Failed to create output BC7 texture");
        return false;
    }

    // Per-mip source SRVs and intermediate UAVs
    TArray<FRHIShaderResourceViewRef> SourceSRVs;
    SourceSRVs.Reserve(NumMips);

    TArray<FRHIUnorderedAccessViewRef> CompressedUAVs;
    CompressedUAVs.Reserve(NumMips);

    for (uint32 Mip = 0; Mip < NumMips; Mip++)
    {
        const FRHIShaderResourceViewDesc SRVDesc = FRHIShaderResourceViewDesc::CreateTexture2D(SrcTexture->GetDesc().Format, uint8(Mip), 1);

        FRHIShaderResourceViewRef SourceSRV = RHI::CreateShaderResourceView(SrcTexture.Get(), SRVDesc);
        if (!SourceSRV)
        {
            LOG_ERROR("[FTextureCompressor] BC7: Failed to create source SRV for mip %u", Mip);
            return false;
        }

        SourceSRVs.Emplace(SourceSRV);

        const FRHIUnorderedAccessViewDesc UAVDesc = FRHIUnorderedAccessViewDesc::CreateTexture2D(EFormat::R32G32B32A32_Uint, uint8(Mip));

        FRHIUnorderedAccessViewRef CompressedUAV = RHI::CreateUnorderedAccessView(CompressedTex.Get(), UAVDesc);
        if (!CompressedUAV)
        {
            LOG_ERROR("[FTextureCompressor] BC7: Failed to create compressed UAV for mip %u", Mip);
            return false;
        }

        CompressedUAVs.Emplace(CompressedUAV);
    }

    FBC7CompressionBufferHLSL ConstBuffer;
    ConstBuffer.Format       = BC7_UNORM_FORMAT;
    ConstBuffer.StartBlockId = 0;
    ConstBuffer.AlphaWeight  = 1.0f;

    constexpr uint32 NumConstants = sizeof(FBC7CompressionBufferHLSL) / sizeof(uint32);

    FRHIShaderResourceView* CurrentSourceSRV = nullptr;

    auto DispatchTryModePass = [&](FRHIComputeShader* Shader, FRHIComputePipelineState* PSO, FRHIShaderResourceView* InSrv, FRHIUnorderedAccessView* InUav,
        FRHIBuffer* InBuf, FRHIBuffer* OutBuf, uint32 ModeId, uint32 BlocksPerGroup)
    {
        ConstBuffer.ModeId = ModeId;

        CommandList.SetComputePipelineState(PSO);
        CommandList.SetShaderResourceView(Shader, CurrentSourceSRV, 0);
        
        if (InSrv)
        {
            CommandList.SetShaderResourceView(Shader, InSrv, 1);
        }

        CommandList.SetUnorderedAccessView(Shader, InUav, 0);
        CommandList.SetShaderConstants(Shader, &ConstBuffer, NumConstants);

        const uint32 NumGroups = Math::DivideByMultiple(ConstBuffer.NumTotalBlocks, BlocksPerGroup);
        CommandList.Dispatch(NumGroups, 1, 1);

        CommandList.UnorderedAccessBarrier(OutBuf);
        if (InBuf)
        {
            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(OutBuf, ERHIResourceState::UnorderedAccess, ERHIResourceState::NonPixelShaderResource));
            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(InBuf, ERHIResourceState::NonPixelShaderResource, ERHIResourceState::UnorderedAccess));
        }
    };

    int32 MipWidth  = SourceDesc.Extent.X;
    int32 MipHeight = SourceDesc.Extent.Y;

    for (uint32 Mip = 0; Mip < NumMips; Mip++)
    {
        const int32  MipBlocksX     = Math::DivideByMultiple(MipWidth, BC_BLOCK_SIZE);
        const int32  MipBlocksY     = Math::DivideByMultiple(MipHeight, BC_BLOCK_SIZE);
        const uint32 MipTotalBlocks = static_cast<uint32>(MipBlocksX * MipBlocksY);

        ConstBuffer.TexWidth       = static_cast<uint32>(MipWidth);
        ConstBuffer.NumBlockX      = static_cast<uint32>(MipBlocksX);
        ConstBuffer.ModeId         = 0;
        ConstBuffer.NumTotalBlocks = MipTotalBlocks;

        CurrentSourceSRV = SourceSRVs[Mip].Get();

        // Pass 1: TryMode456CS -> BufA
        DispatchTryModePass(BC7TryMode456Shader.Get(), BC7TryMode456PSO.Get(),
            nullptr, UavA.Get(), nullptr, BufA.Get(), 0, BC7_BLOCKS_PER_GROUP_456);

        CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(BufA.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::NonPixelShaderResource));

        // Pass 2: TryMode137CS (mode=1) BufA -> BufB
        DispatchTryModePass(BC7TryMode137Shader.Get(), BC7TryMode137PSO.Get(),
            SrvA.Get(), UavB.Get(), BufA.Get(), BufB.Get(), 1, BC7_BLOCKS_PER_GROUP_137);

        // Pass 3: TryMode137CS (mode=3) BufB -> BufA
        DispatchTryModePass(BC7TryMode137Shader.Get(), BC7TryMode137PSO.Get(),
            SrvB.Get(), UavA.Get(), BufB.Get(), BufA.Get(), 3, BC7_BLOCKS_PER_GROUP_137);

        // Pass 4: TryMode137CS (mode=7) BufA -> BufB
        DispatchTryModePass(BC7TryMode137Shader.Get(), BC7TryMode137PSO.Get(),
            SrvA.Get(), UavB.Get(), BufA.Get(), BufB.Get(), 7, BC7_BLOCKS_PER_GROUP_137);

        // Pass 5: TryMode02CS (mode=0) BufB -> BufA
        DispatchTryModePass(BC7TryMode02Shader.Get(), BC7TryMode02PSO.Get(),
            SrvB.Get(), UavA.Get(), BufB.Get(), BufA.Get(), 0, BC7_BLOCKS_PER_GROUP_02);

        // Pass 6: TryMode02CS (mode=2) BufA -> BufB
        DispatchTryModePass(BC7TryMode02Shader.Get(), BC7TryMode02PSO.Get(),
            SrvA.Get(), UavB.Get(), BufA.Get(), BufB.Get(), 2, BC7_BLOCKS_PER_GROUP_02);

        // Pass 7: EncodeBlockCS reads BufB, writes to per-mip intermediate texture
        CommandList.SetComputePipelineState(BC7EncodeBlockPSO.Get());
        CommandList.SetShaderResourceView(BC7EncodeBlockShader.Get(), CurrentSourceSRV, 0);
        CommandList.SetShaderResourceView(BC7EncodeBlockShader.Get(), SrvB.Get(), 1);
        CommandList.SetUnorderedAccessView(BC7EncodeBlockShader.Get(), CompressedUAVs[Mip].Get(), 0);

        ConstBuffer.ModeId = 0;
        CommandList.SetShaderConstants(BC7EncodeBlockShader.Get(), &ConstBuffer, NumConstants);

        const uint32 EncodeGroups = Math::DivideByMultiple(MipTotalBlocks, static_cast<uint32>(BC7_BLOCKS_PER_GROUP_ENC));
        CommandList.Dispatch(EncodeGroups, 1, 1);
        CommandList.UnorderedAccessBarrier(CompressedTex.Get());

        if (Mip + 1 < NumMips)
        {
            CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateBuffer(BufB.Get(), ERHIResourceState::NonPixelShaderResource, ERHIResourceState::UnorderedAccess));
        }

        MipWidth  = Math::Max(MipWidth / 2, 1);
        MipHeight = Math::Max(MipHeight / 2, 1);
    }

    FRHITextureCopyDesc CopyDesc;
    CopyDesc.DstArraySlice  = 0;
    CopyDesc.DstMipSlice    = 0;
    CopyDesc.DstPosition    = IntVector3();
    CopyDesc.SrcArraySlice  = 0;
    CopyDesc.SrcMipSlice    = 0;
    CopyDesc.SrcPosition    = IntVector3();
    CopyDesc.Size.X         = BlocksX;
    CopyDesc.Size.Y         = BlocksY;
    CopyDesc.Size.Z         = 1;
    CopyDesc.NumArraySlices = 1;
    CopyDesc.NumMipLevels   = NumMips;

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(CompressedTex.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::CopySource));

    CommandList.CopyTextureRegion(OutTexture.Get(), CompressedTex.Get(), CopyDesc);

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTextureModeChange(
        OutTexture.Get(), ERHIResourceState::CopyDest, ERHIResourceState::ShaderResource, ERHIResourceStateTrackingMode::Static));
    return true;
}

bool FTextureCompressor::CompressCubeMapBC6(const FRHITextureRef& SrcCubeMap, FRHITextureRef& OutCubeMap)
{
    FRHICommandList CommandList;

    const bool bResult = CompressCubeMapBC6(CommandList, SrcCubeMap, OutCubeMap);
    if (!bResult)
    {
        return false;
    }

    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    return true;
}

bool FTextureCompressor::CompressCubeMapBC6(FRHICommandList& CommandList, const FRHITextureRef& SrcCubeMap, FRHITextureRef& OutCubeMap)
{
    const FRHITextureDesc SourceDesc = SrcCubeMap->GetDesc();
    if (!IsBlockCompressedAligned(SourceDesc.Extent.X) || !IsBlockCompressedAligned(SourceDesc.Extent.Y))
    {
        LOG_ERROR("[FTextureCompressor] Cannot compress a texture with dimensions that are not a multiple of 4");
        return false;
    }

    // Create temporary compressed texture
    FRHITextureDesc CompressedTexDesc = SourceDesc;
    CompressedTexDesc.Format     = EFormat::R32G32B32A32_Uint;
    CompressedTexDesc.UsageFlags = ETextureUsageFlags::UnorderedAccessTexture | ETextureUsageFlags::CopySource;
    CompressedTexDesc.Extent.X   = Math::DivideByMultiple(SourceDesc.Extent.X, BC_BLOCK_SIZE);
    CompressedTexDesc.Extent.Y   = Math::DivideByMultiple(SourceDesc.Extent.Y, BC_BLOCK_SIZE);

    // Calculate the amount of compressed miplevels
    constexpr uint32 NumMipsSkipped = 3;
    CompressedTexDesc.NumMipLevels = Math::Max<int32>(static_cast<int32>(SourceDesc.NumMipLevels) - NumMipsSkipped, 1);

    FRHITextureRef CompressedTex = RHI::CreateTexture(CompressedTexDesc, ERHIResourceState::UnorderedAccess);
    if (!CompressedTex)
    {
        LOG_ERROR("[FTextureCompressor] Failed to create temporary compressed texture");
        return false;
    }
    else
    {
        CompressedTex->SetDebugName("Temp Compressed Texture");
    }

    TArray<FRHIUnorderedAccessViewRef> CompressedUAVs;
    CompressedUAVs.Reserve(CompressedTexDesc.NumMipLevels);

    TArray<FRHIShaderResourceViewRef> SourceSRVs;
    SourceSRVs.Reserve(CompressedTexDesc.NumMipLevels);

    for (uint8 Index = 0; Index < CompressedTexDesc.NumMipLevels; Index++)
    {
        const FRHIUnorderedAccessViewDesc CompressedTexUAVDesc = FRHIUnorderedAccessViewDesc::CreateTextureCube(
            EFormat::R32G32B32A32_Uint, Index);

        FRHIUnorderedAccessViewRef CompressedTexUAV = RHI::CreateUnorderedAccessView(CompressedTex.Get(), CompressedTexUAVDesc);
        if (!CompressedTexUAV)
        {
            LOG_ERROR("[FTextureCompressor] Failed to create compressed texture UAV");
            return false;
        }

        CompressedUAVs.Emplace(CompressedTexUAV);

        const FRHIShaderResourceViewDesc SRVDesc = FRHIShaderResourceViewDesc::CreateTextureCube(SrcCubeMap->GetDesc().Format, Index, 1);

        FRHIShaderResourceViewRef SourceSRV = RHI::CreateShaderResourceView(SrcCubeMap.Get(), SRVDesc);
        if (!SourceSRV)
        {
            LOG_ERROR("[FTextureCompressor] Failed to create source SRV");
            return false;
        }

        SourceSRVs.Emplace(SourceSRV);
    }

    // Create the actual compressed texture
    FRHITextureDesc OutputDesc = CompressedTexDesc;
    OutputDesc.Format       = EFormat::BC6H_UF16;
    OutputDesc.UsageFlags   = ETextureUsageFlags::ShaderResourceTexture | ETextureUsageFlags::CopyDest;
    OutputDesc.Extent       = SourceDesc.Extent;
    OutputDesc.NumMipLevels = CompressedTexDesc.NumMipLevels;

    OutCubeMap = RHI::CreateTexture(OutputDesc, ERHIResourceState::CopyDest);
    if (!OutCubeMap)
    {
        LOG_ERROR("[FTextureCompressor] Failed to create compressed texture");
        return false;
    }
    else
    {
        OutCubeMap->SetDebugName("Compressed Texture");
    }

    // Compress the texture
    CommandList.SetComputePipelineState(BC6HCompressionCubePSO.Get());

    int32 CurrentFaceSize         = SourceDesc.Extent.X;
    int32 CurrentFaceSizeInBlocks = CompressedTexDesc.Extent.X;

    for (uint32 Index = 0; Index < CompressedTexDesc.NumMipLevels; Index++)
    {
        FRHIShaderResourceViewRef SourceSRV = SourceSRVs[Index];
        CommandList.SetShaderResourceView(BC6HCompressionCubeShader.Get(), SourceSRV.Get(), 0);

        FRHIUnorderedAccessViewRef CompressedTexUAV = CompressedUAVs[Index];
        CommandList.SetUnorderedAccessView(BC6HCompressionCubeShader.Get(), CompressedTexUAV.Get(), 0);

        FCompressionBufferHLSL Buffer;
        Buffer.TextureSizeInBlocks[0] = Math::AlignUp(CurrentFaceSizeInBlocks, BC_BLOCK_SIZE);
        Buffer.TextureSizeInBlocks[1] = Math::AlignUp(CurrentFaceSizeInBlocks, BC_BLOCK_SIZE);

        const float CurrentFaceSizeRcp = 1.0f / static_cast<float>(CurrentFaceSize);
        Buffer.TextureSizeRcp = Vector2(CurrentFaceSizeRcp);

        constexpr uint32 NumConstants = sizeof(FCompressionBufferHLSL) / sizeof(uint32);
        CommandList.SetShaderConstants(BC6HCompressionCubeShader.Get(), &Buffer, NumConstants);

        constexpr uint32 NumArraySlices = 6;
        const uint32 ThreadsX = Math::DivideByMultiple(CompressedTexDesc.Extent.X, CS_NUM_THREADS);
        const uint32 ThreadsY = Math::DivideByMultiple(CompressedTexDesc.Extent.Y, CS_NUM_THREADS);
        CommandList.Dispatch(ThreadsX, ThreadsY, NumArraySlices);

        CommandList.UnorderedAccessBarrier(CompressedTex.Get());

        CurrentFaceSize         = CurrentFaceSize / 2;
        CurrentFaceSizeInBlocks = CurrentFaceSizeInBlocks / 2;
    }

    FRHITextureCopyDesc CopyDesc;
    CopyDesc.DstPosition    = IntVector3();
    CopyDesc.DstArraySlice  = 0;
    CopyDesc.DstMipSlice    = 0;
    CopyDesc.SrcPosition    = IntVector3();
    CopyDesc.SrcArraySlice  = 0;
    CopyDesc.SrcMipSlice    = 0;
    CopyDesc.Size.X         = CompressedTexDesc.Extent.X;
    CopyDesc.Size.Y         = CompressedTexDesc.Extent.Y;
    CopyDesc.Size.Z         = CompressedTexDesc.Extent.Z;
    CopyDesc.NumMipLevels   = CompressedTexDesc.NumMipLevels;
    CopyDesc.NumArraySlices = RHI_NUM_CUBE_FACES;

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(CompressedTex.Get(), ERHIResourceState::UnorderedAccess, ERHIResourceState::CopySource));

    CommandList.CopyTextureRegion(OutCubeMap.Get(), CompressedTex.Get(), CopyDesc);

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTextureModeChange(
        OutCubeMap.Get(), ERHIResourceState::CopyDest, ERHIResourceState::ShaderResource, ERHIResourceStateTrackingMode::Static));
    return true;
}
