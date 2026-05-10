#include "Core/Templates/NumericLimits.h"
#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"
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
    uint32   TextureSizeInBlocks[2];
    FVector2 TextureSizeRcp;
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

bool FTextureCompressor::CompileAndCreateShaderPSO(const FString& ShaderPath, const FRHIStaticSamplerInfo& StaticSampler, FRHIComputeShaderRef& OutShader, FRHIComputePipelineStateRef& OutPSO)
{
    TArray<uint8> ShaderCode;
    FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
    if (!FShaderCompiler::Get().CompileFromFile(ShaderPath, CompileInfo, ShaderCode))
    {
        LOG_ERROR("[FTextureCompressor] Failed to compile shader: %s", *ShaderPath);
        return false;
    }

    OutShader = RHI::CreateComputeShader(ShaderCode);
    if (!OutShader)
    {
        LOG_ERROR("[FTextureCompressor] Failed to create compute shader: %s", *ShaderPath);
        return false;
    }

    FRHIComputePipelineStateDesc PSODesc;
    PSODesc.Shader         = OutShader.Get();
    PSODesc.StaticSamplers = TArrayView<const FRHIStaticSamplerInfo>(&StaticSampler, 1);

    OutPSO = RHI::CreateComputePipelineState(PSODesc);
    if (!OutPSO)
    {
        LOG_ERROR("[FTextureCompressor] Failed to create PSO: %s", *ShaderPath);
        return false;
    }

    return true;
}

bool FTextureCompressor::CompileAndCreateShaderPSOEx(const FString& ShaderPath, const FString& EntryPoint, const TArrayView<FShaderDefine>& Defines, FRHIComputeShaderRef& OutShader, FRHIComputePipelineStateRef& OutPSO)
{
    TArray<uint8> ShaderCode;
    FShaderCompileInfo CompileInfo(EntryPoint, EShaderModel::SM_6_2, EShaderStage::Compute, Defines);
    if (!FShaderCompiler::Get().CompileFromFile(ShaderPath, CompileInfo, ShaderCode))
    {
        LOG_ERROR("[FTextureCompressor] Failed to compile shader: %s (entry: %s)", *ShaderPath, *EntryPoint);
        return false;
    }

    OutShader = RHI::CreateComputeShader(ShaderCode);
    if (!OutShader)
    {
        LOG_ERROR("[FTextureCompressor] Failed to create compute shader: %s (entry: %s)", *ShaderPath, *EntryPoint);
        return false;
    }

    FRHIComputePipelineStateDesc PSODesc;
    PSODesc.Shader = OutShader.Get();

    OutPSO = RHI::CreateComputePipelineState(PSODesc);
    if (!OutPSO)
    {
        LOG_ERROR("[FTextureCompressor] Failed to create PSO: %s (entry: %s)", *ShaderPath, *EntryPoint);
        return false;
    }

    return true;
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

    if (!CompileAndCreateShaderPSO("Shaders/BlockCompression/BlockCompressionBC1.hlsl", StaticSampler, BC1CompressionShader, BC1CompressionPSO))
    {
        return false;
    }

    if (!CompileAndCreateShaderPSO("Shaders/BlockCompression/BlockCompressionBC2.hlsl", StaticSampler, BC2CompressionShader, BC2CompressionPSO))
    {
        return false;
    }

    if (!CompileAndCreateShaderPSO("Shaders/BlockCompression/BlockCompressionBC3.hlsl", StaticSampler, BC3CompressionShader, BC3CompressionPSO))
    {
        return false;
    }

    if (!CompileAndCreateShaderPSO("Shaders/BlockCompression/BlockCompressionBC4.hlsl", StaticSampler, BC4CompressionShader, BC4CompressionPSO))
    {
        return false;
    }

    if (!CompileAndCreateShaderPSO("Shaders/BlockCompression/BlockCompressionBC5.hlsl", StaticSampler, BC5CompressionShader, BC5CompressionPSO))
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

    TArray<uint8> ShaderCode;

    FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/BlockCompression/BlockCompressionBC6H.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    BC6HCompressionShader = RHI::CreateComputeShader(ShaderCode);
    if (!BC6HCompressionShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputePipelineStateDesc PSODesc;
    PSODesc.Shader         = BC6HCompressionShader.Get();
    PSODesc.StaticSamplers = TArrayView<const FRHIStaticSamplerInfo>(&BC6HStaticSampler, 1);

    BC6HCompressionPSO = RHI::CreateComputePipelineState(PSODesc);
    if (!BC6HCompressionPSO)
    {
        DEBUG_BREAK();
        return false;
    }

    TArray<FShaderDefine> CompressDefines =
    {
        { "ENABLE_CUBE_MAP", "(1)" }
    };

    CompileInfo = FShaderCompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute, CompressDefines);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/BlockCompression/BlockCompressionBC6H.hlsl", CompileInfo, ShaderCode))
    {
        DEBUG_BREAK();
        return false;
    }

    BC6HCompressionCubeShader = RHI::CreateComputeShader(ShaderCode);
    if (!BC6HCompressionCubeShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputePipelineStateDesc BlockCompressionBC6H_PSODesc;
    BlockCompressionBC6H_PSODesc.Shader         = BC6HCompressionCubeShader.Get();
    BlockCompressionBC6H_PSODesc.StaticSamplers = TArrayView<const FRHIStaticSamplerInfo>(&BC6HStaticSampler, 1);

    BC6HCompressionCubePSO = RHI::CreateComputePipelineState(BlockCompressionBC6H_PSODesc);
    if (!BC6HCompressionCubePSO)
    {
        DEBUG_BREAK();
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

    FRHITextureDesc CompressedTexDesc = FRHITextureDesc::CreateTexture2D(EFormat::R32G32_Uint, BlocksX, BlocksY, NumMips, 1, ETextureUsageFlags::UnorderedAccessTexture);
    FRHITextureRef  CompressedTex     = RHI::CreateTexture(CompressedTexDesc, EResourceAccess::UnorderedAccess);

    if (!CompressedTex)
    {
        LOG_ERROR("[FTextureCompressor] Failed to create temporary compressed texture");
        return false;
    }

    FRHITextureDesc OutputDesc = FRHITextureDesc::CreateTexture2D(OutputFormat, SourceDesc.Extent.X, SourceDesc.Extent.Y, NumMips, 1, ETextureUsageFlags::ShaderResourceTexture);
    OutTexture = RHI::CreateTexture(OutputDesc, EResourceAccess::CopyDest);

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
        const FRHIShaderResourceViewDesc SRVDesc = FRHIShaderResourceViewDesc::CreateTexture2D(
            SrcTexture->GetDesc().Format, uint8(Mip), 1);

        FRHIShaderResourceViewRef SourceSRV = RHI::CreateShaderResourceView(SrcTexture.Get(), SRVDesc);
        if (!SourceSRV)
        {
            LOG_ERROR("[FTextureCompressor] Failed to create source SRV for mip %u", Mip);
            return false;
        }

        SourceSRVs.Emplace(SourceSRV);

        const FRHIUnorderedAccessViewDesc UAVDesc = FRHIUnorderedAccessViewDesc::CreateTexture2D(
            EFormat::R32G32_Uint, uint8(Mip));

        FRHIUnorderedAccessViewRef CompressedUAV = RHI::CreateUnorderedAccessView(CompressedTex.Get(), UAVDesc);
        if (!CompressedUAV)
        {
            LOG_ERROR("[FTextureCompressor] Failed to create compressed UAV for mip %u", Mip);
            return false;
        }

        CompressedUAVs.Emplace(CompressedUAV);
    }

    CommandList.SetComputePipelineState(PSO);
    CommandList.TransitionTextureState(SrcTexture.Get(), FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource));

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
        Buffer.TextureSizeRcp         = FVector2(1.0f / static_cast<float>(MipWidth), 1.0f / static_cast<float>(MipHeight));

        constexpr uint32 NumConstants = sizeof(FCompressionBufferHLSL) / sizeof(uint32);
        CommandList.SetShaderConstants(Shader, &Buffer, NumConstants);

        const int32 ThreadGroupsX = Math::DivideByMultiple(MipBlocksX, int32(CS_NUM_THREADS));
        const int32 ThreadGroupsY = Math::DivideByMultiple(MipBlocksY, int32(CS_NUM_THREADS));
        CommandList.Dispatch(ThreadGroupsX, ThreadGroupsY, 1);
        CommandList.UnorderedAccessTextureBarrier(CompressedTex.Get());

        MipWidth  = Math::Max(MipWidth / 2, 1);
        MipHeight = Math::Max(MipHeight / 2, 1);
    }

    FRHITextureCopyDesc CopyDesc;
    CopyDesc.DstArraySlice  = 0;
    CopyDesc.DstMipSlice    = 0;
    CopyDesc.DstPosition    = FIntVector3();
    CopyDesc.SrcArraySlice  = 0;
    CopyDesc.SrcMipSlice    = 0;
    CopyDesc.SrcPosition    = FIntVector3();
    CopyDesc.Size.X         = BlocksX;
    CopyDesc.Size.Y         = BlocksY;
    CopyDesc.Size.Z         = 1;
    CopyDesc.NumArraySlices = 1;
    CopyDesc.NumMipLevels   = NumMips;

    CommandList.TransitionTextureState(CompressedTex.Get(), FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::CopySource));
    CommandList.CopyTextureRegion(OutTexture.Get(), CompressedTex.Get(), CopyDesc);
    CommandList.TransitionTextureState(OutTexture.Get(), FRHITextureTransition::Make(EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource));
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

    FRHITextureDesc CompressedTexDesc = FRHITextureDesc::CreateTexture2D(EFormat::R32G32B32A32_Uint, BlocksX, BlocksY, NumMips, 1, ETextureUsageFlags::UnorderedAccessTexture);
    FRHITextureRef CompressedTex = RHI::CreateTexture(CompressedTexDesc, EResourceAccess::UnorderedAccess);
    if (!CompressedTex)
    {
        LOG_ERROR("[FTextureCompressor] Failed to create temporary compressed texture");
        return false;
    }

    FRHITextureDesc OutputDesc = FRHITextureDesc::CreateTexture2D(OutputFormat, SourceDesc.Extent.X, SourceDesc.Extent.Y, NumMips, 1, ETextureUsageFlags::ShaderResourceTexture);
    OutTexture = RHI::CreateTexture(OutputDesc, EResourceAccess::CopyDest);
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
        const FRHIShaderResourceViewDesc SRVDesc = FRHIShaderResourceViewDesc::CreateTexture2D(
            SrcTexture->GetDesc().Format, uint8(Mip), 1);

        FRHIShaderResourceViewRef SourceSRV = RHI::CreateShaderResourceView(SrcTexture.Get(), SRVDesc);
        if (!SourceSRV)
        {
            LOG_ERROR("[FTextureCompressor] Failed to create source SRV for mip %u", Mip);
            return false;
        }

        SourceSRVs.Emplace(SourceSRV);

        const FRHIUnorderedAccessViewDesc UAVDesc = FRHIUnorderedAccessViewDesc::CreateTexture2D(
            EFormat::R32G32B32A32_Uint, uint8(Mip));

        FRHIUnorderedAccessViewRef CompressedUAV = RHI::CreateUnorderedAccessView(CompressedTex.Get(), UAVDesc);
        if (!CompressedUAV)
        {
            LOG_ERROR("[FTextureCompressor] Failed to create compressed UAV for mip %u", Mip);
            return false;
        }

        CompressedUAVs.Emplace(CompressedUAV);
    }

    CommandList.SetComputePipelineState(PSO);
    CommandList.TransitionTextureState(SrcTexture.Get(), FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource));

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
        Buffer.TextureSizeRcp         = FVector2(1.0f / static_cast<float>(MipWidth), 1.0f / static_cast<float>(MipHeight));

        constexpr uint32 NumConstants = sizeof(FCompressionBufferHLSL) / sizeof(uint32);
        CommandList.SetShaderConstants(Shader, &Buffer, NumConstants);

        const int32 ThreadGroupsX = Math::DivideByMultiple(MipBlocksX, int32(CS_NUM_THREADS));
        const int32 ThreadGroupsY = Math::DivideByMultiple(MipBlocksY, int32(CS_NUM_THREADS));
        CommandList.Dispatch(ThreadGroupsX, ThreadGroupsY, 1);
        CommandList.UnorderedAccessTextureBarrier(CompressedTex.Get());

        MipWidth  = Math::Max(MipWidth / 2, 1);
        MipHeight = Math::Max(MipHeight / 2, 1);
    }

    FRHITextureCopyDesc CopyDesc;
    CopyDesc.DstArraySlice  = 0;
    CopyDesc.DstMipSlice    = 0;
    CopyDesc.DstPosition    = FIntVector3();
    CopyDesc.SrcArraySlice  = 0;
    CopyDesc.SrcMipSlice    = 0;
    CopyDesc.SrcPosition    = FIntVector3();
    CopyDesc.Size.X         = BlocksX;
    CopyDesc.Size.Y         = BlocksY;
    CopyDesc.Size.Z         = 1;
    CopyDesc.NumArraySlices = 1;
    CopyDesc.NumMipLevels   = NumMips;

    CommandList.TransitionTextureState(CompressedTex.Get(), FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::CopySource));
    CommandList.CopyTextureRegion(OutTexture.Get(), CompressedTex.Get(), CopyDesc);
    CommandList.TransitionTextureState(OutTexture.Get(), FRHITextureTransition::Make(EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource));
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

    FRHITextureDesc CompressedTexDesc = FRHITextureDesc::CreateTexture2D(EFormat::R32G32B32A32_Uint, BlocksX, BlocksY, NumMips, 1, ETextureUsageFlags::UnorderedAccessTexture);
    FRHITextureRef  CompressedTex     = RHI::CreateTexture(CompressedTexDesc, EResourceAccess::UnorderedAccess);
    
    if (!CompressedTex)
    {
        LOG_ERROR("[FTextureCompressor] Failed to create temporary compressed texture");
        return false;
    }

    FRHITextureDesc OutputDesc = FRHITextureDesc::CreateTexture2D(EFormat::BC6H_UF16, SourceDesc.Extent.X, SourceDesc.Extent.Y, NumMips, 1, ETextureUsageFlags::ShaderResourceTexture);
    OutTexture = RHI::CreateTexture(OutputDesc, EResourceAccess::CopyDest);
    
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
        const FRHIShaderResourceViewDesc SRVDesc = FRHIShaderResourceViewDesc::CreateTexture2D(
            SrcTexture->GetDesc().Format, uint8(Mip), 1);

        FRHIShaderResourceViewRef SourceSRV = RHI::CreateShaderResourceView(SrcTexture.Get(), SRVDesc);
        if (!SourceSRV)
        {
            LOG_ERROR("[FTextureCompressor] Failed to create source SRV for mip %u", Mip);
            return false;
        }

        SourceSRVs.Emplace(SourceSRV);

        const FRHIUnorderedAccessViewDesc UAVDesc = FRHIUnorderedAccessViewDesc::CreateTexture2D(
            EFormat::R32G32B32A32_Uint, uint8(Mip));

        FRHIUnorderedAccessViewRef CompressedUAV = RHI::CreateUnorderedAccessView(CompressedTex.Get(), UAVDesc);
        if (!CompressedUAV)
        {
            LOG_ERROR("[FTextureCompressor] Failed to create compressed UAV for mip %u", Mip);
            return false;
        }

        CompressedUAVs.Emplace(CompressedUAV);
    }

    CommandList.SetComputePipelineState(BC6HCompressionPSO.Get());
    CommandList.TransitionTextureState(SrcTexture.Get(), FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource));

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
        Buffer.TextureSizeRcp         = FVector2(1.0f / static_cast<float>(MipWidth), 1.0f / static_cast<float>(MipHeight));

        constexpr uint32 NumConstants = sizeof(FCompressionBufferHLSL) / sizeof(uint32);
        CommandList.SetShaderConstants(BC6HCompressionShader.Get(), &Buffer, NumConstants);

        const int32 ThreadGroupsX = Math::DivideByMultiple(MipBlocksX, int32(CS_NUM_THREADS));
        const int32 ThreadGroupsY = Math::DivideByMultiple(MipBlocksY, int32(CS_NUM_THREADS));
        CommandList.Dispatch(ThreadGroupsX, ThreadGroupsY, 1);
        CommandList.UnorderedAccessTextureBarrier(CompressedTex.Get());

        MipWidth  = Math::Max(MipWidth / 2, 1);
        MipHeight = Math::Max(MipHeight / 2, 1);
    }

    FRHITextureCopyDesc CopyDesc;
    CopyDesc.DstArraySlice  = 0;
    CopyDesc.DstMipSlice    = 0;
    CopyDesc.DstPosition    = FIntVector3();
    CopyDesc.SrcArraySlice  = 0;
    CopyDesc.SrcMipSlice    = 0;
    CopyDesc.SrcPosition    = FIntVector3();
    CopyDesc.Size.X         = BlocksX;
    CopyDesc.Size.Y         = BlocksY;
    CopyDesc.Size.Z         = 1;
    CopyDesc.NumArraySlices = 1;
    CopyDesc.NumMipLevels   = NumMips;

    CommandList.TransitionTextureState(CompressedTex.Get(), FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::CopySource));
    CommandList.CopyTextureRegion(OutTexture.Get(), CompressedTex.Get(), CopyDesc);
    CommandList.TransitionTextureState(OutTexture.Get(), FRHITextureTransition::Make(EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource));
    return true;
}

bool FTextureCompressor::InitializeBC7()
{
    const FString BC7ShaderPath = "Shaders/BlockCompression/BlockCompressionBC7.hlsl";

    TArray<FShaderDefine> NoDefines;
    if (!CompileAndCreateShaderPSOEx(BC7ShaderPath, "TryMode456CS", NoDefines, BC7TryMode456Shader, BC7TryMode456PSO))
    {
        return false;
    }

    if (!CompileAndCreateShaderPSOEx(BC7ShaderPath, "TryMode137CS", NoDefines, BC7TryMode137Shader, BC7TryMode137PSO))
    {
        return false;
    }

    if (!CompileAndCreateShaderPSOEx(BC7ShaderPath, "TryMode02CS", NoDefines, BC7TryMode02Shader, BC7TryMode02PSO))
    {
        return false;
    }

    TArray<FShaderDefine> EncodeDefines = { { "BC7_ENCODE_ONLY", "(1)" } };
    if (!CompileAndCreateShaderPSOEx(BC7ShaderPath, "EncodeBlockCS", EncodeDefines, BC7EncodeBlockShader, BC7EncodeBlockPSO))
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
    const uint64 BufferSize       = static_cast<uint64>(MaxTotalBlocks) * StructuredStride;

    FRHIBufferDesc BufferDesc;
    BufferDesc.Flags  = EBufferFlags::Default | EBufferFlags::RWBuffer;
    BufferDesc.Stride = StructuredStride;
    BufferDesc.Size   = BufferSize;

    FRHIBufferRef BufA = RHI::CreateBuffer(BufferDesc, EResourceAccess::UnorderedAccess);
    FRHIBufferRef BufB = RHI::CreateBuffer(BufferDesc, EResourceAccess::UnorderedAccess);
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
    FRHITextureDesc CompressedTexDesc = FRHITextureDesc::CreateTexture2D(EFormat::R32G32B32A32_Uint, BlocksX, BlocksY, NumMips, 1, ETextureUsageFlags::UnorderedAccessTexture);
    FRHITextureRef  CompressedTex     = RHI::CreateTexture(CompressedTexDesc, EResourceAccess::UnorderedAccess);
    
    if (!CompressedTex)
    {
        LOG_ERROR("[FTextureCompressor] BC7: Failed to create temporary compressed texture");
        return false;
    }

    FRHITextureDesc OutputDesc = FRHITextureDesc::CreateTexture2D(EFormat::BC7_UNorm, SourceDesc.Extent.X, SourceDesc.Extent.Y, NumMips, 1, ETextureUsageFlags::ShaderResourceTexture);
    OutTexture = RHI::CreateTexture(OutputDesc, EResourceAccess::CopyDest);
    
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
        const FRHIShaderResourceViewDesc SRVDesc = FRHIShaderResourceViewDesc::CreateTexture2D(
            SrcTexture->GetDesc().Format, uint8(Mip), 1);

        FRHIShaderResourceViewRef SourceSRV = RHI::CreateShaderResourceView(SrcTexture.Get(), SRVDesc);
        if (!SourceSRV)
        {
            LOG_ERROR("[FTextureCompressor] BC7: Failed to create source SRV for mip %u", Mip);
            return false;
        }

        SourceSRVs.Emplace(SourceSRV);

        const FRHIUnorderedAccessViewDesc UAVDesc = FRHIUnorderedAccessViewDesc::CreateTexture2D(
            EFormat::R32G32B32A32_Uint, uint8(Mip));

        FRHIUnorderedAccessViewRef CompressedUAV = RHI::CreateUnorderedAccessView(CompressedTex.Get(), UAVDesc);
        if (!CompressedUAV)
        {
            LOG_ERROR("[FTextureCompressor] BC7: Failed to create compressed UAV for mip %u", Mip);
            return false;
        }

        CompressedUAVs.Emplace(CompressedUAV);
    }

    CommandList.TransitionTextureState(SrcTexture.Get(), FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource));

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

        CommandList.UnorderedAccessBufferBarrier(OutBuf);
        if (InBuf)
        {
            CommandList.TransitionBufferState(OutBuf, EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);
            CommandList.TransitionBufferState(InBuf, EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);
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

        CommandList.TransitionBufferState(BufA.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::NonPixelShaderResource);

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
        CommandList.UnorderedAccessTextureBarrier(CompressedTex.Get());

        // After the 7-pass pipeline: BufA is in UAV state, BufB is in SRV state.
        // Transition BufB back to UAV for the next mip iteration.
        if (Mip + 1 < NumMips)
        {
            CommandList.TransitionBufferState(BufB.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::UnorderedAccess);
        }

        MipWidth  = Math::Max(MipWidth / 2, 1);
        MipHeight = Math::Max(MipHeight / 2, 1);
    }

    FRHITextureCopyDesc CopyDesc;
    CopyDesc.DstArraySlice  = 0;
    CopyDesc.DstMipSlice    = 0;
    CopyDesc.DstPosition    = FIntVector3();
    CopyDesc.SrcArraySlice  = 0;
    CopyDesc.SrcMipSlice    = 0;
    CopyDesc.SrcPosition    = FIntVector3();
    CopyDesc.Size.X         = BlocksX;
    CopyDesc.Size.Y         = BlocksY;
    CopyDesc.Size.Z         = 1;
    CopyDesc.NumArraySlices = 1;
    CopyDesc.NumMipLevels   = NumMips;

    CommandList.TransitionTextureState(CompressedTex.Get(), FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::CopySource));
    CommandList.CopyTextureRegion(OutTexture.Get(), CompressedTex.Get(), CopyDesc);
    CommandList.TransitionTextureState(OutTexture.Get(), FRHITextureTransition::Make(EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource));
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
    CompressedTexDesc.UsageFlags = ETextureUsageFlags::UnorderedAccessTexture;
    CompressedTexDesc.Extent.X   = Math::DivideByMultiple(SourceDesc.Extent.X, BC_BLOCK_SIZE);
    CompressedTexDesc.Extent.Y   = Math::DivideByMultiple(SourceDesc.Extent.Y, BC_BLOCK_SIZE);

    // When calculating NumMips we skip 3 miplevels since those are too small for the 
    // compressed texture since they are smaller than the compressed block-size.
    constexpr uint32 NumMipsSkipped = 3;

    // Calculate the amount of compressed miplevels
    CompressedTexDesc.NumMipLevels = Math::Max<int32>(static_cast<int32>(SourceDesc.NumMipLevels) - NumMipsSkipped, 1);

    FRHITextureRef CompressedTex = RHI::CreateTexture(CompressedTexDesc, EResourceAccess::UnorderedAccess);
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

        const FRHIShaderResourceViewDesc SRVDesc = FRHIShaderResourceViewDesc::CreateTextureCube(
            SrcCubeMap->GetDesc().Format, Index, 1);

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
    OutputDesc.UsageFlags   = ETextureUsageFlags::ShaderResourceTexture;
    OutputDesc.Extent       = SourceDesc.Extent;
    OutputDesc.NumMipLevels = CompressedTexDesc.NumMipLevels;

    OutCubeMap = RHI::CreateTexture(OutputDesc, EResourceAccess::CopyDest);
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

    CommandList.TransitionTextureState(SrcCubeMap.Get(), FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource));
    
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
        Buffer.TextureSizeRcp = FVector2(CurrentFaceSizeRcp);

        constexpr uint32 NumConstants = sizeof(FCompressionBufferHLSL) / sizeof(uint32);
        CommandList.SetShaderConstants(BC6HCompressionCubeShader.Get(), &Buffer, NumConstants);

        constexpr uint32 NumArraySlices = 6;
        const uint32 ThreadsX = Math::DivideByMultiple(CompressedTexDesc.Extent.X, CS_NUM_THREADS);
        const uint32 ThreadsY = Math::DivideByMultiple(CompressedTexDesc.Extent.Y, CS_NUM_THREADS);
        CommandList.Dispatch(ThreadsX, ThreadsY, NumArraySlices);

        CommandList.UnorderedAccessTextureBarrier(CompressedTex.Get());

        CurrentFaceSize         = CurrentFaceSize / 2;
        CurrentFaceSizeInBlocks = CurrentFaceSizeInBlocks / 2;
    }

    FRHITextureCopyDesc CopyDesc;
    CopyDesc.DstPosition    = FIntVector3();
    CopyDesc.DstArraySlice  = 0;
    CopyDesc.DstMipSlice    = 0;
    CopyDesc.SrcPosition    = FIntVector3();
    CopyDesc.SrcArraySlice  = 0;
    CopyDesc.SrcMipSlice    = 0;
    CopyDesc.Size.X         = CompressedTexDesc.Extent.X;
    CopyDesc.Size.Y         = CompressedTexDesc.Extent.Y;
    CopyDesc.Size.Z         = CompressedTexDesc.Extent.Z;
    CopyDesc.NumMipLevels   = CompressedTexDesc.NumMipLevels;
    CopyDesc.NumArraySlices = RHI_NUM_CUBE_FACES;

    CommandList.TransitionTextureState(CompressedTex.Get(), FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::CopySource));

    CommandList.CopyTextureRegion(OutCubeMap.Get(), CompressedTex.Get(), CopyDesc);

    CommandList.TransitionTextureState(OutCubeMap.Get(), FRHITextureTransition::Make(EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource));
    CommandList.TransitionTextureState(SrcCubeMap.Get(), FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource));
    return true;
}
