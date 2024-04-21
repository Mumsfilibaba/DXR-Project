#include "GPUTextureCompressor.h"
#include "Core/Templates/NumericLimits.h"
#include "RHI/RHI.h"
#include "RHI/ShaderCompiler.h"

#define BC_BLOCK_SIZE int32(4)
#define CS_NUM_THREADS (8)

struct FCompressionBuffer
{
    uint32   TextureSizeInBlocks[2];
    FVector2 TextureSizeRcp;
};


FGPUTextureCompressor::FGPUTextureCompressor()
    : BC6HCompressionShader(nullptr)
    , BC6HCompressionPSO(nullptr)
    , BC6HCompressionCubeShader(nullptr)
    , BC6HCompressionCubePSO(nullptr)
{
}

bool FGPUTextureCompressor::Initialize()
{
    TArray<uint8> ShaderCode;
    
    {
        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/BlockCompressionBC6H.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }
    }

    BC6HCompressionShader = RHICreateComputeShader(ShaderCode);
    if (!BC6HCompressionShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputePipelineStateInitializer PSOInitializer(BC6HCompressionShader.Get());
    BC6HCompressionPSO = RHICreateComputePipelineState(PSOInitializer);
    if (!BC6HCompressionPSO)
    {
        DEBUG_BREAK();
        return false;
    }

    TArray<FShaderDefine> CompressDefines =
    {
        { "ENABLE_CUBE_MAP", "(1)" }
    };

    {
        FShaderCompileInfo CompileInfo("Main", EShaderModel::SM_6_2, EShaderStage::Compute, CompressDefines);
        if (!FShaderCompiler::Get().CompileFromFile("Shaders/BlockCompressionBC6H.hlsl", CompileInfo, ShaderCode))
        {
            DEBUG_BREAK();
            return false;
        }
    }

    BC6HCompressionCubeShader = RHICreateComputeShader(ShaderCode);
    if (!BC6HCompressionCubeShader)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHIComputePipelineStateInitializer CubePSOInitializer(BC6HCompressionCubeShader.Get());
    BC6HCompressionCubePSO = RHICreateComputePipelineState(CubePSOInitializer);
    if (!BC6HCompressionCubePSO)
    {
        DEBUG_BREAK();
        return false;
    }

    FRHISamplerStateInfo Initializer;
    Initializer.AddressU = ESamplerMode::Wrap;
    Initializer.AddressV = ESamplerMode::Wrap;
    Initializer.AddressW = ESamplerMode::Wrap;
    Initializer.Filter   = ESamplerFilter::MinMagMipLinear;
    Initializer.MinLOD   = 0.0f;
    Initializer.MaxLOD   = TNumericLimits<float>::Max();

    PointSampler = RHICreateSamplerState(Initializer);
    if (!PointSampler)
    {
        return false;
    }

    return true;
}

bool FGPUTextureCompressor::CompressBC6(const FRHITextureRef& Source, FRHITextureRef& Output)
{
    const FRHITextureInfo SourceInfo = Source->GetInfo();
    if (!IsBlockCompressedAligned(SourceInfo.Extent.x) || !IsBlockCompressedAligned(SourceInfo.Extent.y))
    {
        LOG_ERROR("[FGPUTextureCompressor] Cannot compress a texture with dimensions that are not a multiple of 4");
        return false;
    }

    // Create temporary compressed texture
    FRHITextureInfo CompressedTexInfo = SourceInfo;
    CompressedTexInfo.Format       = EFormat::R32G32B32A32_Uint;
    CompressedTexInfo.UsageFlags   = ETextureUsageFlags::UnorderedAccess;
    CompressedTexInfo.Extent.x     = FMath::DivideByMultiple(SourceInfo.Extent.x, BC_BLOCK_SIZE);
    CompressedTexInfo.Extent.y     = FMath::DivideByMultiple(SourceInfo.Extent.y, BC_BLOCK_SIZE);
    CompressedTexInfo.NumMipLevels = 1;

    FRHITextureRef CompressedTex = RHICreateTexture(CompressedTexInfo, EResourceAccess::UnorderedAccess);
    if (!CompressedTex)
    {
        LOG_ERROR("[FGPUTextureCompressor] Failed to create temporary compressed texture");
        return false;
    }
    else
    {
        CompressedTex->SetDebugName("Temp Compressed Texture");
    }

    // Create the actual compressed texture
    FRHITextureInfo OutputInfo = CompressedTexInfo;
    OutputInfo.Format     = EFormat::BC6H_UF16;
    OutputInfo.UsageFlags = ETextureUsageFlags::ShaderResource;
    OutputInfo.Extent     = SourceInfo.Extent;

    Output = RHICreateTexture(OutputInfo, EResourceAccess::CopyDest);
    if (!Output)
    {
        LOG_ERROR("[FGPUTextureCompressor] Failed to create compressed texture");
        return false;
    }
    else
    {
        Output->SetDebugName("Compressed Texture");
    }

    // Compress the texture
    FRHICommandList CommandList;
    CommandList.SetComputePipelineState(BC6HCompressionPSO.Get());

    CommandList.SetShaderResourceView(BC6HCompressionShader.Get(), Source->GetShaderResourceView(), 0);
    CommandList.SetUnorderedAccessView(BC6HCompressionShader.Get(), CompressedTex->GetUnorderedAccessView(), 0);
    CommandList.SetSamplerState(BC6HCompressionShader.Get(), PointSampler.Get(), 0);

    const FVector2 TexSize = FVector2(static_cast<float>(SourceInfo.Extent.x), static_cast<float>(SourceInfo.Extent.y));

    FCompressionBuffer Buffer;
    Buffer.TextureSizeInBlocks[0] = FMath::AlignUp(CompressedTexInfo.Extent.x, BC_BLOCK_SIZE);
    Buffer.TextureSizeInBlocks[1] = FMath::AlignUp(CompressedTexInfo.Extent.y, BC_BLOCK_SIZE);
    Buffer.TextureSizeRcp         = FVector2(1.0f) / TexSize;

    CommandList.Set32BitShaderConstants(BC6HCompressionShader.Get(), &Buffer, FMath::BytesToNum32BitConstants(sizeof(FCompressionBuffer)));
    
    CommandList.TransitionTexture(Source.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource);

    const uint32 ThreadGroupsX = FMath::DivideByMultiple(CompressedTexInfo.Extent.x, CS_NUM_THREADS);
    const uint32 ThreadGroupsY = FMath::DivideByMultiple(CompressedTexInfo.Extent.y, CS_NUM_THREADS);
    CommandList.Dispatch(ThreadGroupsX, ThreadGroupsY, 1);
    CommandList.UnorderedAccessTextureBarrier(CompressedTex.Get());

    FTextureCopyInfo CopyDesc;
    CopyDesc.DstArraySlice = 0;
    CopyDesc.DstMipSlice   = 0;
    CopyDesc.DstPosition   = FIntVector3();

    CopyDesc.SrcArraySlice = 0;
    CopyDesc.SrcMipSlice   = 0;
    CopyDesc.SrcPosition   = FIntVector3();

    CopyDesc.Size.x         = CompressedTexInfo.Extent.x;
    CopyDesc.Size.y         = CompressedTexInfo.Extent.y;
    CopyDesc.Size.z         = CompressedTexInfo.Extent.z;
    CopyDesc.NumArraySlices = 1;
    CopyDesc.NumMipLevels   = 1;

    CommandList.TransitionTexture(CompressedTex.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::CopySource);
    CommandList.CopyTextureRegion(Output.Get(), CompressedTex.Get(), CopyDesc);
    CommandList.TransitionTexture(Output.Get(), EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource);

    CommandList.DestroyResource(CompressedTex.Get());

    GRHICommandExecutor.ExecuteCommandList(CommandList);
    return true;
}

bool FGPUTextureCompressor::CompressCubeMapBC6(const FRHITextureRef& Source, FRHITextureRef& Output)
{
    const FRHITextureInfo SourceInfo = Source->GetInfo();
    if (!IsBlockCompressedAligned(SourceInfo.Extent.x) || !IsBlockCompressedAligned(SourceInfo.Extent.y))
    {
        LOG_ERROR("[FGPUTextureCompressor] Cannot compress a texture with dimensions that are not a multiple of 4");
        return false;
    }

    // Create temporary compressed texture
    FRHITextureInfo CompressedTexInfo = SourceInfo;
    CompressedTexInfo.Format       = EFormat::R32G32B32A32_Uint;
    CompressedTexInfo.UsageFlags   = ETextureUsageFlags::UnorderedAccess;
    CompressedTexInfo.Extent.x     = FMath::DivideByMultiple(SourceInfo.Extent.x, BC_BLOCK_SIZE);
    CompressedTexInfo.Extent.y     = FMath::DivideByMultiple(SourceInfo.Extent.y, BC_BLOCK_SIZE);

    // When calculating NumMips we skip 3 miplevels since those are too small for the compressed texture 
    // since they are smaller than the compressed block-size.
    constexpr uint32 NumMipsSkipped = 3;

    // Calculate the amount of compressed miplevels
    CompressedTexInfo.NumMipLevels = FMath::Max<uint32>(FMath::Log2(SourceInfo.Extent.x) - NumMipsSkipped, 1u);

    FRHITextureRef CompressedTex = RHICreateTexture(CompressedTexInfo, EResourceAccess::UnorderedAccess);
    if (!CompressedTex)
    {
        LOG_ERROR("[FGPUTextureCompressor] Failed to create temporary compressed texture");
        return false;
    }
    else
    {
        CompressedTex->SetDebugName("Temp Compressed Texture");
    }

    TArray<FRHIUnorderedAccessViewRef> CompressedUAVs;
    CompressedUAVs.Reserve(CompressedTexInfo.NumMipLevels);

    for (uint8 Index = 0; Index < CompressedTexInfo.NumMipLevels; Index++)
    {
        FRHITextureUAVDesc CompressedTexUAVDesc;
        CompressedTexUAVDesc.Texture         = CompressedTex.Get();
        CompressedTexUAVDesc.Format          = EFormat::R32G32B32A32_Uint;
        CompressedTexUAVDesc.FirstArraySlice = 0;
        CompressedTexUAVDesc.MipLevel        = Index;
        CompressedTexUAVDesc.NumSlices       = 1;

        FRHIUnorderedAccessViewRef CompressedTexUAV = RHICreateUnorderedAccessView(CompressedTexUAVDesc);
        if (!CompressedTexUAV)
        {
            LOG_ERROR("[FGPUTextureCompressor] Failed to create compressed texture UAV");
            return false;
        }
        else
        {
            CompressedUAVs.Emplace(CompressedTexUAV);
        }
    }

    // Create the actual compressed texture
    FRHITextureInfo OutputInfo = CompressedTexInfo;
    OutputInfo.Format       = EFormat::BC6H_UF16;
    OutputInfo.UsageFlags   = ETextureUsageFlags::ShaderResource;
    OutputInfo.Extent       = SourceInfo.Extent;
    OutputInfo.NumMipLevels = CompressedTexInfo.NumMipLevels;

    Output = RHICreateTexture(OutputInfo, EResourceAccess::CopyDest);
    if (!Output)
    {
        LOG_ERROR("[FGPUTextureCompressor] Failed to create compressed texture");
        return false;
    }
    else
    {
        Output->SetDebugName("Compressed Texture");
    }

    // Compress the texture
    FRHICommandList CommandList;
    CommandList.SetComputePipelineState(BC6HCompressionCubePSO.Get());

    CommandList.SetShaderResourceView(BC6HCompressionCubeShader.Get(), Source->GetShaderResourceView(), 0);
    CommandList.SetSamplerState(BC6HCompressionCubeShader.Get(), PointSampler.Get(), 0);

    CommandList.TransitionTexture(Source.Get(), EResourceAccess::PixelShaderResource, EResourceAccess::NonPixelShaderResource);
    
    int32 CurrentFaceSize         = SourceInfo.Extent.x;
    int32 CurrentFaceSizeInBlocks = CompressedTexInfo.Extent.x;
    for (uint32 Index = 0; Index < CompressedTexInfo.NumMipLevels; Index++)
    {
        FRHIUnorderedAccessViewRef CompressedTexUAV = CompressedUAVs[Index];
        CommandList.SetUnorderedAccessView(BC6HCompressionCubeShader.Get(), CompressedTexUAV.Get(), 0);

        FCompressionBuffer Buffer;
        Buffer.TextureSizeInBlocks[0] = FMath::AlignUp(CurrentFaceSizeInBlocks, BC_BLOCK_SIZE);
        Buffer.TextureSizeInBlocks[1] = FMath::AlignUp(CurrentFaceSizeInBlocks, BC_BLOCK_SIZE);
        
        const float CurrentFaceSizeRcp = 1.0f / static_cast<float>(CurrentFaceSize);
        Buffer.TextureSizeRcp = FVector2(CurrentFaceSizeRcp);

        CommandList.Set32BitShaderConstants(BC6HCompressionCubeShader.Get(), &Buffer, FMath::BytesToNum32BitConstants(sizeof(FCompressionBuffer)));

        const uint32 NumArraySlices = 6;
        const uint32 ThreadGroupsX  = FMath::DivideByMultiple(CompressedTexInfo.Extent.x, CS_NUM_THREADS);
        const uint32 ThreadGroupsY  = FMath::DivideByMultiple(CompressedTexInfo.Extent.y, CS_NUM_THREADS);
        CommandList.Dispatch(ThreadGroupsX, ThreadGroupsY, NumArraySlices);

        CommandList.UnorderedAccessTextureBarrier(CompressedTex.Get());

        CommandList.DestroyResource(CompressedTexUAV.Get());

        CurrentFaceSize         = CurrentFaceSize / 2;
        CurrentFaceSizeInBlocks = CurrentFaceSizeInBlocks / 2;
    }

    FTextureCopyInfo CopyDesc;
    CopyDesc.DstPosition    = FIntVector3();
    CopyDesc.DstArraySlice  = 0;
    CopyDesc.DstMipSlice    = 0;
    CopyDesc.SrcPosition    = FIntVector3();
    CopyDesc.SrcArraySlice  = 0;
    CopyDesc.SrcMipSlice    = 0;
    CopyDesc.Size.x         = CompressedTexInfo.Extent.x;
    CopyDesc.Size.y         = CompressedTexInfo.Extent.y;
    CopyDesc.Size.z         = CompressedTexInfo.Extent.z;
    CopyDesc.NumMipLevels   = CompressedTexInfo.NumMipLevels;
    CopyDesc.NumArraySlices = 1;

    CommandList.TransitionTexture(CompressedTex.Get(), EResourceAccess::UnorderedAccess, EResourceAccess::CopySource);
    
    CommandList.CopyTextureRegion(Output.Get(), CompressedTex.Get(), CopyDesc);

    CommandList.TransitionTexture(Output.Get(), EResourceAccess::CopyDest, EResourceAccess::PixelShaderResource);
    CommandList.TransitionTexture(Source.Get(), EResourceAccess::NonPixelShaderResource, EResourceAccess::PixelShaderResource);

    CommandList.DestroyResource(CompressedTex.Get());
    CommandList.DestroyResource(Source.Get());

    GRHICommandExecutor.ExecuteCommandList(CommandList);
    return true;
}
