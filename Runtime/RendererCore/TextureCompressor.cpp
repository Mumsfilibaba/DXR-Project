#include "Core/Templates/NumericLimits.h"
#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"
#include "RendererCore/TextureCompressor.h"

#define BC_BLOCK_SIZE int32(4)
#define CS_NUM_THREADS (8)

struct FCompressionBufferHLSL
{
    uint32   TextureSizeInBlocks[2];
    FVector2 TextureSizeRcp;
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
{
}

FTextureCompressor::~FTextureCompressor()
{
}

static bool CompileAndCreateShaderPSO(
    const FString& ShaderPath,
    const FRHIStaticSamplerInfo& StaticSampler,
    FRHIComputeShaderRef& OutShader,
    FRHIComputePipelineStateRef& OutPSO)
{
    TArray<uint8> ShaderCode;
    FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
    if (!FShaderCompiler::Get().CompileFromFile(ShaderPath, CompileInfo, ShaderCode))
    {
        LOG_ERROR("[FTextureCompressor] Failed to compile shader: %s", *ShaderPath);
        return false;
    }

    OutShader = FRHI::Get()->CreateComputeShader(ShaderCode);
    if (!OutShader)
    {
        LOG_ERROR("[FTextureCompressor] Failed to create compute shader: %s", *ShaderPath);
        return false;
    }

    FRHIComputePipelineStateInfo PSOInfo;
    PSOInfo.Shader         = OutShader.Get();
    PSOInfo.StaticSamplers = TArrayView<const FRHIStaticSamplerInfo>(&StaticSampler, 1);

    OutPSO = FRHI::Get()->CreateComputePipelineState(PSOInfo);
    if (!OutPSO)
    {
        LOG_ERROR("[FTextureCompressor] Failed to create PSO: %s", *ShaderPath);
        return false;
    }

    return true;
}

bool FTextureCompressor::Initialize()
{
    FRHISamplerStateInfo SamplerInfo;
    SamplerInfo.AddressU = ESamplerMode::Wrap;
    SamplerInfo.AddressV = ESamplerMode::Wrap;
    SamplerInfo.AddressW = ESamplerMode::Wrap;
    SamplerInfo.Filter   = ESamplerFilter::MinMagMipLinear;
    SamplerInfo.MinLOD   = 0.0f;
    SamplerInfo.MaxLOD   = TNumericLimits<float>::Max();

    PointSampler = FRHI::Get()->CreateSamplerState(SamplerInfo);
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

    BC6HCompressionShader = FRHI::Get()->CreateComputeShader(ShaderCode);
    if (!BC6HCompressionShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputePipelineStateInfo PSOInfo;
    PSOInfo.Shader         = BC6HCompressionShader.Get();
    PSOInfo.StaticSamplers = TArrayView<const FRHIStaticSamplerInfo>(&BC6HStaticSampler, 1);

    BC6HCompressionPSO = FRHI::Get()->CreateComputePipelineState(PSOInfo);
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

    BC6HCompressionCubeShader = FRHI::Get()->CreateComputeShader(ShaderCode);
    if (!BC6HCompressionCubeShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputePipelineStateInfo BlockCompressionBC6H_PSOInfo;
    BlockCompressionBC6H_PSOInfo.Shader         = BC6HCompressionCubeShader.Get();
    BlockCompressionBC6H_PSOInfo.StaticSamplers = TArrayView<const FRHIStaticSamplerInfo>(&BC6HStaticSampler, 1);

    BC6HCompressionCubePSO = FRHI::Get()->CreateComputePipelineState(BlockCompressionBC6H_PSOInfo);
    if (!BC6HCompressionCubePSO)
    {
        DEBUG_BREAK();
        return false;
    }

    return true;
}

bool FTextureCompressor::CompressSinglePass64(FRHICommandList& CommandList, const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture,
    FRHIComputeShader* Shader, FRHIComputePipelineState* PSO, EFormat OutputFormat)
{
    const FRHITextureInfo SourceInfo = SrcTexture->GetInfo();
    if (!IsBlockCompressedAligned(SourceInfo.Extent.X) || !IsBlockCompressedAligned(SourceInfo.Extent.Y))
    {
        LOG_ERROR("[FTextureCompressor] Cannot compress a texture with dimensions that are not a multiple of 4");
        return false;
    }

    const int32 BlocksX = Math::DivideByMultiple(SourceInfo.Extent.X, BC_BLOCK_SIZE);
    const int32 BlocksY = Math::DivideByMultiple(SourceInfo.Extent.Y, BC_BLOCK_SIZE);

    FRHITextureInfo CompressedTexInfo = FRHITextureInfo::CreateTexture2D(EFormat::R32G32_Uint, BlocksX, BlocksY, 1, 1, ETextureUsageFlags::UnorderedAccessTexture);
    FRHITextureRef CompressedTex = FRHI::Get()->CreateTexture(CompressedTexInfo, EResourceAccess::UnorderedAccess);
    if (!CompressedTex)
    {
        LOG_ERROR("[FTextureCompressor] Failed to create temporary compressed texture");
        return false;
    }

    FRHITextureInfo OutputInfo = FRHITextureInfo::CreateTexture2D(OutputFormat, SourceInfo.Extent.X, SourceInfo.Extent.Y, 1, 1, ETextureUsageFlags::ShaderResourceTexture);
    OutTexture = FRHI::Get()->CreateTexture(OutputInfo, EResourceAccess::CopyDest);
    if (!OutTexture)
    {
        LOG_ERROR("[FTextureCompressor] Failed to create compressed texture");
        return false;
    }

    CommandList.SetComputePipelineState(PSO);
    CommandList.SetShaderResourceView(Shader, SrcTexture->GetShaderResourceView(), 0);
    CommandList.SetUnorderedAccessView(Shader, CompressedTex->GetUnorderedAccessView(), 0);

    const FVector2 TexSize = FVector2(static_cast<float>(SourceInfo.Extent.X), static_cast<float>(SourceInfo.Extent.Y));

    FCompressionBufferHLSL Buffer;
    Buffer.TextureSizeInBlocks[0] = Math::AlignUp(BlocksX, BC_BLOCK_SIZE);
    Buffer.TextureSizeInBlocks[1] = Math::AlignUp(BlocksY, BC_BLOCK_SIZE);
    Buffer.TextureSizeRcp         = FVector2(1.0f) / TexSize;

    constexpr uint32 NumConstants = sizeof(FCompressionBufferHLSL) / sizeof(uint32);
    CommandList.SetShaderConstants(Shader, &Buffer, NumConstants);

    CommandList.TransitionTextureState(SrcTexture.Get(), FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource));

    const int32 ThreadGroupsX = Math::DivideByMultiple(BlocksX, int32(CS_NUM_THREADS));
    const int32 ThreadGroupsY = Math::DivideByMultiple(BlocksY, int32(CS_NUM_THREADS));
    CommandList.Dispatch(ThreadGroupsX, ThreadGroupsY, 1);
    CommandList.UnorderedAccessTextureBarrier(CompressedTex.Get());

    FTextureCopyInfo CopyDesc;
    CopyDesc.DstArraySlice = 0;
    CopyDesc.DstMipSlice   = 0;
    CopyDesc.DstPosition   = FIntVector3();
    CopyDesc.SrcArraySlice = 0;
    CopyDesc.SrcMipSlice   = 0;
    CopyDesc.SrcPosition   = FIntVector3();
    CopyDesc.Size.X         = BlocksX;
    CopyDesc.Size.Y         = BlocksY;
    CopyDesc.Size.Z         = 1;
    CopyDesc.NumArraySlices = 1;
    CopyDesc.NumMipLevels   = 1;

    CommandList.TransitionTextureState(CompressedTex.Get(), FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::CopySource));
    CommandList.CopyTextureRegion(OutTexture.Get(), CompressedTex.Get(), CopyDesc);
    CommandList.TransitionTextureState(OutTexture.Get(), FRHITextureTransition::Make(EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource));
    return true;
}

bool FTextureCompressor::CompressSinglePass128(FRHICommandList& CommandList, const FRHITextureRef& SrcTexture, FRHITextureRef& OutTexture,
    FRHIComputeShader* Shader, FRHIComputePipelineState* PSO, EFormat OutputFormat)
{
    const FRHITextureInfo SourceInfo = SrcTexture->GetInfo();
    if (!IsBlockCompressedAligned(SourceInfo.Extent.X) || !IsBlockCompressedAligned(SourceInfo.Extent.Y))
    {
        LOG_ERROR("[FTextureCompressor] Cannot compress a texture with dimensions that are not a multiple of 4");
        return false;
    }

    const int32 BlocksX = Math::DivideByMultiple(SourceInfo.Extent.X, BC_BLOCK_SIZE);
    const int32 BlocksY = Math::DivideByMultiple(SourceInfo.Extent.Y, BC_BLOCK_SIZE);

    FRHITextureInfo CompressedTexInfo = FRHITextureInfo::CreateTexture2D(EFormat::R32G32B32A32_Uint, BlocksX, BlocksY, 1, 1, ETextureUsageFlags::UnorderedAccessTexture);
    FRHITextureRef CompressedTex = FRHI::Get()->CreateTexture(CompressedTexInfo, EResourceAccess::UnorderedAccess);
    if (!CompressedTex)
    {
        LOG_ERROR("[FTextureCompressor] Failed to create temporary compressed texture");
        return false;
    }

    FRHITextureInfo OutputInfo = FRHITextureInfo::CreateTexture2D(OutputFormat, SourceInfo.Extent.X, SourceInfo.Extent.Y, 1, 1, ETextureUsageFlags::ShaderResourceTexture);
    OutTexture = FRHI::Get()->CreateTexture(OutputInfo, EResourceAccess::CopyDest);
    if (!OutTexture)
    {
        LOG_ERROR("[FTextureCompressor] Failed to create compressed texture");
        return false;
    }

    CommandList.SetComputePipelineState(PSO);
    CommandList.SetShaderResourceView(Shader, SrcTexture->GetShaderResourceView(), 0);
    CommandList.SetUnorderedAccessView(Shader, CompressedTex->GetUnorderedAccessView(), 0);

    const FVector2 TexSize = FVector2(static_cast<float>(SourceInfo.Extent.X), static_cast<float>(SourceInfo.Extent.Y));

    FCompressionBufferHLSL Buffer;
    Buffer.TextureSizeInBlocks[0] = Math::AlignUp(BlocksX, BC_BLOCK_SIZE);
    Buffer.TextureSizeInBlocks[1] = Math::AlignUp(BlocksY, BC_BLOCK_SIZE);
    Buffer.TextureSizeRcp         = FVector2(1.0f) / TexSize;

    constexpr uint32 NumConstants = sizeof(FCompressionBufferHLSL) / sizeof(uint32);
    CommandList.SetShaderConstants(Shader, &Buffer, NumConstants);

    CommandList.TransitionTextureState(SrcTexture.Get(), FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource));

    const int32 ThreadGroupsX = Math::DivideByMultiple(BlocksX, int32(CS_NUM_THREADS));
    const int32 ThreadGroupsY = Math::DivideByMultiple(BlocksY, int32(CS_NUM_THREADS));
    CommandList.Dispatch(ThreadGroupsX, ThreadGroupsY, 1);
    CommandList.UnorderedAccessTextureBarrier(CompressedTex.Get());

    FTextureCopyInfo CopyDesc;
    CopyDesc.DstArraySlice = 0;
    CopyDesc.DstMipSlice   = 0;
    CopyDesc.DstPosition   = FIntVector3();
    CopyDesc.SrcArraySlice = 0;
    CopyDesc.SrcMipSlice   = 0;
    CopyDesc.SrcPosition   = FIntVector3();
    CopyDesc.Size.X         = BlocksX;
    CopyDesc.Size.Y         = BlocksY;
    CopyDesc.Size.Z         = 1;
    CopyDesc.NumArraySlices = 1;
    CopyDesc.NumMipLevels   = 1;

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
    const FRHITextureInfo SourceInfo = SrcTexture->GetInfo();
    if (!IsBlockCompressedAligned(SourceInfo.Extent.X) || !IsBlockCompressedAligned(SourceInfo.Extent.Y))
    {
        LOG_ERROR("[FTextureCompressor] Cannot compress a texture with dimensions that are not a multiple of 4");
        return false;
    }

    // Create temporary compressed texture
    FRHITextureInfo CompressedTexInfo = SourceInfo;
    CompressedTexInfo.Format       = EFormat::R32G32B32A32_Uint;
    CompressedTexInfo.UsageFlags   = ETextureUsageFlags::UnorderedAccessTexture;
    CompressedTexInfo.Extent.X     = Math::DivideByMultiple(SourceInfo.Extent.X, BC_BLOCK_SIZE);
    CompressedTexInfo.Extent.Y     = Math::DivideByMultiple(SourceInfo.Extent.Y, BC_BLOCK_SIZE);
    CompressedTexInfo.NumMipLevels = 1;

    FRHITextureRef CompressedTex = FRHI::Get()->CreateTexture(CompressedTexInfo, EResourceAccess::UnorderedAccess);
    if (!CompressedTex)
    {
        LOG_ERROR("[FTextureCompressor] Failed to create temporary compressed texture");
        return false;
    }
    else
    {
        CompressedTex->SetDebugName("Temp Compressed Texture");
    }

    // Create the actual compressed texture
    FRHITextureInfo OutputInfo = CompressedTexInfo;
    OutputInfo.Format     = EFormat::BC6H_UF16;
    OutputInfo.UsageFlags = ETextureUsageFlags::ShaderResourceTexture;
    OutputInfo.Extent     = SourceInfo.Extent;

    OutTexture = FRHI::Get()->CreateTexture(OutputInfo, EResourceAccess::CopyDest);
    if (!OutTexture)
    {
        LOG_ERROR("[FTextureCompressor] Failed to create compressed texture");
        return false;
    }
    else
    {
        OutTexture->SetDebugName("Compressed Texture");
    }

    // Compress the texture
    CommandList.SetComputePipelineState(BC6HCompressionPSO.Get());

    CommandList.SetShaderResourceView(BC6HCompressionShader.Get(), SrcTexture->GetShaderResourceView(), 0);
    CommandList.SetUnorderedAccessView(BC6HCompressionShader.Get(), CompressedTex->GetUnorderedAccessView(), 0);

    const FVector2 TexSize = FVector2(static_cast<float>(SourceInfo.Extent.X), static_cast<float>(SourceInfo.Extent.Y));

    FCompressionBufferHLSL Buffer;
    Buffer.TextureSizeInBlocks[0] = Math::AlignUp(CompressedTexInfo.Extent.X, BC_BLOCK_SIZE);
    Buffer.TextureSizeInBlocks[1] = Math::AlignUp(CompressedTexInfo.Extent.Y, BC_BLOCK_SIZE);
    Buffer.TextureSizeRcp         = FVector2(1.0f) / TexSize;

    constexpr uint32 NumConstants = sizeof(FCompressionBufferHLSL) / sizeof(uint32);
    CommandList.SetShaderConstants(BC6HCompressionShader.Get(), &Buffer, NumConstants);
    
    CommandList.TransitionTextureState(SrcTexture.Get(), FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource));

    const uint32 ThreadGroupsX = Math::DivideByMultiple(CompressedTexInfo.Extent.X, CS_NUM_THREADS);
    const uint32 ThreadGroupsY = Math::DivideByMultiple(CompressedTexInfo.Extent.Y, CS_NUM_THREADS);
    CommandList.Dispatch(ThreadGroupsX, ThreadGroupsY, 1);
    CommandList.UnorderedAccessTextureBarrier(CompressedTex.Get());

    FTextureCopyInfo CopyDesc;
    CopyDesc.DstArraySlice = 0;
    CopyDesc.DstMipSlice   = 0;
    CopyDesc.DstPosition   = FIntVector3();

    CopyDesc.SrcArraySlice = 0;
    CopyDesc.SrcMipSlice   = 0;
    CopyDesc.SrcPosition   = FIntVector3();

    CopyDesc.Size.X         = CompressedTexInfo.Extent.X;
    CopyDesc.Size.Y         = CompressedTexInfo.Extent.Y;
    CopyDesc.Size.Z         = CompressedTexInfo.Extent.Z;
    CopyDesc.NumArraySlices = 1;
    CopyDesc.NumMipLevels   = 1;

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
    const FRHITextureInfo SourceInfo = SrcCubeMap->GetInfo();
    if (!IsBlockCompressedAligned(SourceInfo.Extent.X) || !IsBlockCompressedAligned(SourceInfo.Extent.Y))
    {
        LOG_ERROR("[FTextureCompressor] Cannot compress a texture with dimensions that are not a multiple of 4");
        return false;
    }

    // Create temporary compressed texture
    FRHITextureInfo CompressedTexInfo = SourceInfo;
    CompressedTexInfo.Format     = EFormat::R32G32B32A32_Uint;
    CompressedTexInfo.UsageFlags = ETextureUsageFlags::UnorderedAccessTexture;
    CompressedTexInfo.Extent.X   = Math::DivideByMultiple(SourceInfo.Extent.X, BC_BLOCK_SIZE);
    CompressedTexInfo.Extent.Y   = Math::DivideByMultiple(SourceInfo.Extent.Y, BC_BLOCK_SIZE);

    // When calculating NumMips we skip 3 miplevels since those are too small for the 
    // compressed texture since they are smaller than the compressed block-size.
    constexpr uint32 NumMipsSkipped = 3;

    // Calculate the amount of compressed miplevels
    CompressedTexInfo.NumMipLevels = Math::Max<int32>(static_cast<int32>(SourceInfo.NumMipLevels) - NumMipsSkipped, 1);

    FRHITextureRef CompressedTex = FRHI::Get()->CreateTexture(CompressedTexInfo, EResourceAccess::UnorderedAccess);
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
    CompressedUAVs.Reserve(CompressedTexInfo.NumMipLevels);

    TArray<FRHIShaderResourceViewRef> SourceSRVs;
    SourceSRVs.Reserve(CompressedTexInfo.NumMipLevels);

    for (uint8 Index = 0; Index < CompressedTexInfo.NumMipLevels; Index++)
    {
        FRHIUnorderedAccessViewInfo CompressedTexUAVInfo;
        CompressedTexUAVInfo.Type = FRHIUnorderedAccessViewInfo::EType::TextureUAV;
        CompressedTexUAVInfo.TextureUAV.Texture         = CompressedTex.Get();
        CompressedTexUAVInfo.TextureUAV.Format          = EFormat::R32G32B32A32_Uint;
        CompressedTexUAVInfo.TextureUAV.FirstArraySlice = 0;
        CompressedTexUAVInfo.TextureUAV.MipLevel        = Index;
        CompressedTexUAVInfo.TextureUAV.NumSlices       = 1;

        FRHIUnorderedAccessViewRef CompressedTexUAV = FRHI::Get()->CreateUnorderedAccessView(CompressedTexUAVInfo);
        if (!CompressedTexUAV)
        {
            LOG_ERROR("[FTextureCompressor] Failed to create compressed texture UAV");
            return false;
        }
        else
        {
            CompressedUAVs.Emplace(CompressedTexUAV);
        }

        FRHIShaderResourceViewInfo SRVInfo;
        SRVInfo.Type = FRHIShaderResourceViewInfo::EType::TextureSRV;
        SRVInfo.TextureSRV.Texture         = SrcCubeMap.Get();
        SRVInfo.TextureSRV.Format          = SrcCubeMap->GetFormat();
        SRVInfo.TextureSRV.FirstArraySlice = 0;
        SRVInfo.TextureSRV.NumSlices       = 1;
        SRVInfo.TextureSRV.FirstMipLevel   = Index;
        SRVInfo.TextureSRV.MinLODClamp     = 0;
        SRVInfo.TextureSRV.NumMips         = 1;

        FRHIShaderResourceViewRef SourceSRV = FRHI::Get()->CreateShaderResourceView(SRVInfo);
        if (!SourceSRV)
        {
            LOG_ERROR("[FTextureCompressor] Failed to create source SRV");
            return false;
        }
        else
        {
            SourceSRVs.Emplace(SourceSRV);
        }
    }

    // Create the actual compressed texture
    FRHITextureInfo OutputInfo = CompressedTexInfo;
    OutputInfo.Format       = EFormat::BC6H_UF16;
    OutputInfo.UsageFlags   = ETextureUsageFlags::ShaderResourceTexture;
    OutputInfo.Extent       = SourceInfo.Extent;
    OutputInfo.NumMipLevels = CompressedTexInfo.NumMipLevels;

    OutCubeMap = FRHI::Get()->CreateTexture(OutputInfo, EResourceAccess::CopyDest);
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
    
    int32 CurrentFaceSize         = SourceInfo.Extent.X;
    int32 CurrentFaceSizeInBlocks = CompressedTexInfo.Extent.X;
    for (uint32 Index = 0; Index < CompressedTexInfo.NumMipLevels; Index++)
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
        const uint32 ThreadsX = Math::DivideByMultiple(CompressedTexInfo.Extent.X, CS_NUM_THREADS);
        const uint32 ThreadsY = Math::DivideByMultiple(CompressedTexInfo.Extent.Y, CS_NUM_THREADS);
        CommandList.Dispatch(ThreadsX, ThreadsY, NumArraySlices);

        CommandList.UnorderedAccessTextureBarrier(CompressedTex.Get());

        CurrentFaceSize         = CurrentFaceSize / 2;
        CurrentFaceSizeInBlocks = CurrentFaceSizeInBlocks / 2;
    }

    FTextureCopyInfo CopyInfo;
    CopyInfo.DstPosition    = FIntVector3();
    CopyInfo.DstArraySlice  = 0;
    CopyInfo.DstMipSlice    = 0;
    CopyInfo.SrcPosition    = FIntVector3();
    CopyInfo.SrcArraySlice  = 0;
    CopyInfo.SrcMipSlice    = 0;
    CopyInfo.Size.X         = CompressedTexInfo.Extent.X;
    CopyInfo.Size.Y         = CompressedTexInfo.Extent.Y;
    CopyInfo.Size.Z         = CompressedTexInfo.Extent.Z;
    CopyInfo.NumMipLevels   = CompressedTexInfo.NumMipLevels;
    CopyInfo.NumArraySlices = 1;

    CommandList.TransitionTextureState(CompressedTex.Get(), FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::CopySource));

    CommandList.CopyTextureRegion(OutCubeMap.Get(), CompressedTex.Get(), CopyInfo);

    CommandList.TransitionTextureState(OutCubeMap.Get(), FRHITextureTransition::Make(EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource));
    CommandList.TransitionTextureState(SrcCubeMap.Get(), FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource));

    return true;
}
