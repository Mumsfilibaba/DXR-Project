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
    : BC6HCompressionShader(nullptr)
    , BC6HCompressionPSO(nullptr)
    , BC6HCompressionCubeShader(nullptr)
    , BC6HCompressionCubePSO(nullptr)
{
}

FTextureCompressor::~FTextureCompressor()
{
}

bool FTextureCompressor::Initialize()
{
    TArray<uint8> ShaderCode;

    FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/BlockCompressionBC6H.hlsl", CompileInfo, ShaderCode))
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

    FRHIComputePipelineStateDesc PSOInfo;
    PSOInfo.Shader = BC6HCompressionShader.Get();

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
    if (!FShaderCompiler::Get().CompileFromFile("Shaders/BlockCompressionBC6H.hlsl", CompileInfo, ShaderCode))
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

    FRHIComputePipelineStateDesc BlockCompressionBC6H_PSOInfo;
    BlockCompressionBC6H_PSOInfo.Shader = BC6HCompressionCubeShader.Get();

    BC6HCompressionCubePSO = FRHI::Get()->CreateComputePipelineState(BlockCompressionBC6H_PSOInfo);
    if (!BC6HCompressionCubePSO)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHISamplerStateDesc SamplerDesc;
    SamplerDesc.AddressU = ESamplerMode::Wrap;
    SamplerDesc.AddressV = ESamplerMode::Wrap;
    SamplerDesc.AddressW = ESamplerMode::Wrap;
    SamplerDesc.Filter   = ESamplerFilter::MinMagMipLinear;
    SamplerDesc.MinLOD   = 0.0f;
    SamplerDesc.MaxLOD   = TNumericLimits<float>::Max();

    PointSampler = FRHI::Get()->CreateSamplerState(SamplerDesc);
    if (!PointSampler)
    {
        return false;
    }

    return true;
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

    // Create temporary compressed texture
    FRHITextureDesc CompressedTexDesc = SourceDesc;
    CompressedTexDesc.Format       = EFormat::R32G32B32A32_Uint;
    CompressedTexDesc.UsageFlags   = ETextureUsageFlags::UnorderedAccessTexture;
    CompressedTexDesc.Extent.X     = Math::DivideByMultiple(SourceDesc.Extent.X, BC_BLOCK_SIZE);
    CompressedTexDesc.Extent.Y     = Math::DivideByMultiple(SourceDesc.Extent.Y, BC_BLOCK_SIZE);
    CompressedTexDesc.NumMipLevels = 1;

    FRHITextureRef CompressedTex = FRHI::Get()->CreateTexture(CompressedTexDesc, EResourceAccess::UnorderedAccess);
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
    FRHITextureDesc OutputDesc = CompressedTexDesc;
    OutputDesc.Format     = EFormat::BC6H_UF16;
    OutputDesc.UsageFlags = ETextureUsageFlags::ShaderResourceTexture;
    OutputDesc.Extent     = SourceDesc.Extent;

    OutTexture = FRHI::Get()->CreateTexture(OutputDesc, EResourceAccess::CopyDest);
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
    CommandList.SetSamplerState(BC6HCompressionShader.Get(), PointSampler.Get(), 0);

    const FVector2 TexSize = FVector2(static_cast<float>(SourceDesc.Extent.X), static_cast<float>(SourceDesc.Extent.Y));

    FCompressionBufferHLSL Buffer;
    Buffer.TextureSizeInBlocks[0] = Math::AlignUp(CompressedTexDesc.Extent.X, BC_BLOCK_SIZE);
    Buffer.TextureSizeInBlocks[1] = Math::AlignUp(CompressedTexDesc.Extent.Y, BC_BLOCK_SIZE);
    Buffer.TextureSizeRcp         = FVector2(1.0f) / TexSize;

    constexpr uint32 NumConstants = sizeof(FCompressionBufferHLSL) / sizeof(uint32);
    CommandList.SetShaderConstants(BC6HCompressionShader.Get(), &Buffer, NumConstants);
    
    CommandList.TransitionTexture(SrcTexture.Get(), FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource));

    const uint32 ThreadGroupsX = Math::DivideByMultiple(CompressedTexDesc.Extent.X, CS_NUM_THREADS);
    const uint32 ThreadGroupsY = Math::DivideByMultiple(CompressedTexDesc.Extent.Y, CS_NUM_THREADS);
    CommandList.Dispatch(ThreadGroupsX, ThreadGroupsY, 1);
    CommandList.UnorderedAccessTextureBarrier(CompressedTex.Get());

    FRHITextureCopyDesc CopyDesc;
    CopyDesc.DstArraySlice = 0;
    CopyDesc.DstMipSlice   = 0;
    CopyDesc.DstPosition   = FIntVector3();

    CopyDesc.SrcArraySlice = 0;
    CopyDesc.SrcMipSlice   = 0;
    CopyDesc.SrcPosition   = FIntVector3();

    CopyDesc.Size.X         = CompressedTexDesc.Extent.X;
    CopyDesc.Size.Y         = CompressedTexDesc.Extent.Y;
    CopyDesc.Size.Z         = CompressedTexDesc.Extent.Z;
    CopyDesc.NumArraySlices = 1;
    CopyDesc.NumMipLevels   = 1;

    CommandList.TransitionTexture(CompressedTex.Get(), FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::CopySource));
    CommandList.CopyTextureRegion(OutTexture.Get(), CompressedTex.Get(), CopyDesc);
    CommandList.TransitionTexture(OutTexture.Get(), FRHITextureTransition::Make(EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource));
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

    FRHITextureRef CompressedTex = FRHI::Get()->CreateTexture(CompressedTexDesc, EResourceAccess::UnorderedAccess);
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
        FRHIUnorderedAccessViewDesc CompressedTexUAVDesc;
        CompressedTexUAVDesc.Type = FRHIUnorderedAccessViewDesc::EType::TextureUAV;
        CompressedTexUAVDesc.TextureUAV.Texture         = CompressedTex.Get();
        CompressedTexUAVDesc.TextureUAV.Format          = EFormat::R32G32B32A32_Uint;
        CompressedTexUAVDesc.TextureUAV.FirstArraySlice = 0;
        CompressedTexUAVDesc.TextureUAV.MipLevel        = Index;
        CompressedTexUAVDesc.TextureUAV.NumSlices       = 1;

        FRHIUnorderedAccessViewRef CompressedTexUAV = FRHI::Get()->CreateUnorderedAccessView(CompressedTexUAVDesc);
        if (!CompressedTexUAV)
        {
            LOG_ERROR("[FTextureCompressor] Failed to create compressed texture UAV");
            return false;
        }
        else
        {
            CompressedUAVs.Emplace(CompressedTexUAV);
        }

        FRHIShaderResourceViewDesc SRVDesc;
        SRVDesc.Type = FRHIShaderResourceViewDesc::EType::TextureSRV;
        SRVDesc.TextureSRV.Texture         = SrcCubeMap.Get();
        SRVDesc.TextureSRV.Format          = SrcCubeMap->GetFormat();
        SRVDesc.TextureSRV.FirstArraySlice = 0;
        SRVDesc.TextureSRV.NumSlices       = 1;
        SRVDesc.TextureSRV.FirstMipLevel   = Index;
        SRVDesc.TextureSRV.MinLODClamp     = 0;
        SRVDesc.TextureSRV.NumMips         = 1;

        FRHIShaderResourceViewRef SourceSRV = FRHI::Get()->CreateShaderResourceView(SRVDesc);
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
    FRHITextureDesc OutputDesc = CompressedTexDesc;
    OutputDesc.Format       = EFormat::BC6H_UF16;
    OutputDesc.UsageFlags   = ETextureUsageFlags::ShaderResourceTexture;
    OutputDesc.Extent       = SourceDesc.Extent;
    OutputDesc.NumMipLevels = CompressedTexDesc.NumMipLevels;

    OutCubeMap = FRHI::Get()->CreateTexture(OutputDesc, EResourceAccess::CopyDest);
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
    CommandList.SetSamplerState(BC6HCompressionCubeShader.Get(), PointSampler.Get(), 0);

    CommandList.TransitionTexture(SrcCubeMap.Get(), FRHITextureTransition::Make(EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource));
    
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
    CopyDesc.NumArraySlices = 1;

    CommandList.TransitionTexture(CompressedTex.Get(), FRHITextureTransition::Make(EResourceAccess::UnorderedAccess, EResourceAccess::CopySource));

    CommandList.CopyTextureRegion(OutCubeMap.Get(), CompressedTex.Get(), CopyDesc);

    CommandList.TransitionTexture(OutCubeMap.Get(), FRHITextureTransition::Make(EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource));
    CommandList.TransitionTexture(SrcCubeMap.Get(), FRHITextureTransition::Make(EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource));

    return true;
}
