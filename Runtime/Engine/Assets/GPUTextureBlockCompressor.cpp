#include "Core/Math/Math.h"
#include "Engine/Assets/GPUTextureBlockCompressor.h"
#include "Engine/Assets/TextureAssetLoader.h"
#include "RHI/RHI.h"
#include "RendererCore/TextureFactory.h"
#include "RendererCore/TextureResourceData.h"

// Each row is copied on its own, to an offset D3D12 wants 512-byte aligned
static constexpr uint64 ReadbackRowAlignment = 512;

struct FMipReadback
{
    FRHITextureRef Blocks;
    uint64         Offset;
    uint64         RowStride;
    uint32         BlocksX;
    uint32         BlocksY;
};

bool FGPUTextureBlockCompressor::Compress(ETextureFormat SourceFormat, const TArray<FTextureMip>& Mips, ETextureFormat Format, TArray<FTextureMip>& OutMips, FString& OutError)
{
    const EFormat SourceRHIFormat = TextureAssetLoader::GetRHIFormat(SourceFormat, false);
    const EFormat RHIFormat       = TextureAssetLoader::GetRHIFormat(Format, false);

    if (SourceRHIFormat == EFormat::Unknown || !TextureAssets::IsBlockCompressed(Format))
    {
        OutError = FString::Printf(TEXT("%s can't be compressed to %s"), TextureAssets::GetFormatName(SourceFormat), TextureAssets::GetFormatName(Format));
        return false;
    }

    FTextureCompressor& Compressor = FTextureFactory::Get().GetTextureCompressor();
    FRHICommandList     CommandList;

    TArray<FRHITextureRef> Sources;
    TArray<FMipReadback>   Readbacks;
    uint64                 ReadbackSize = 0;

    for (int32 Index = 0; Index < Mips.Size(); ++Index)
    {
        const FTextureMip& Mip = Mips[Index];

        FTextureResourceData SourceData;
        SourceData.InitMipData(Mip.Data.Data(), TextureAssets::GetRowPitch(SourceFormat, Mip.Width), Mip.Data.Size());

        const FRHITextureDesc SourceDesc = FRHITextureDesc::CreateTexture2D(SourceRHIFormat, Mip.Width, Mip.Height, 1, 1, ETextureUsageFlags::ShaderResourceTexture);
        FRHITextureRef        Source     = RHI::CreateTexture(SourceDesc, ERHIResourceState::ShaderResource, &SourceData);

        FRHITextureRef Blocks;
        if (!Source || !Compressor.EncodeBlocks(CommandList, RHIFormat, Source, Blocks))
        {
            OutError = FString::Printf(TEXT("mip %d (%dx%d) couldn't be encoded on the GPU"), Index, Mip.Width, Mip.Height);
            return false;
        }

        FMipReadback& Readback = Readbacks.Emplace();
        Readback.Blocks    = Blocks;
        Readback.BlocksX   = Blocks->GetDesc().Extent.X;
        Readback.BlocksY   = Blocks->GetDesc().Extent.Y;
        Readback.RowStride = Math::AlignUp<uint64>(TextureAssets::GetRowPitch(Format, Mip.Width), ReadbackRowAlignment);
        Readback.Offset    = ReadbackSize;

        ReadbackSize += Readback.RowStride * Readback.BlocksY;
        Sources.Add(Source);
    }

    FRHIFenceRef  Fence          = RHI::CreateFence();
    FRHIBufferRef ReadbackBuffer = RHI::CreateBuffer(FRHIBufferDesc::CreateReadbackBuffer(ReadbackSize), ERHIResourceState::CopyDest, nullptr);

    if (!Fence || !ReadbackBuffer)
    {
        OutError = FString::Printf(TEXT("a %llu byte readback buffer couldn't be created"), ReadbackSize);
        return false;
    }

    for (const FMipReadback& Readback : Readbacks)
    {
        for (uint32 Row = 0; Row < Readback.BlocksY; ++Row)
        {
            CommandList.CopyTextureRegionToBuffer(ReadbackBuffer.Get(), Readback.Offset + Readback.RowStride * Row, Readback.Blocks.Get(), FTextureRegion2D(Readback.BlocksX, 1, 0, Row), 0);
        }
    }

    CommandList.WriteFence(Fence.Get());
    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);
    FRHICommandListExecutor::Get().WaitForCommands();

    if (!Fence->Wait())
    {
        OutError = FString(TEXT("the GPU never finished"));
        return false;
    }

    const uint8* Mapped = reinterpret_cast<const uint8*>(ReadbackBuffer->Map(0, ReadbackSize));
    if (!Mapped)
    {
        OutError = FString(TEXT("the readback buffer couldn't be mapped"));
        return false;
    }

    OutMips.Clear();
    for (int32 Index = 0; Index < Mips.Size(); ++Index)
    {
        const FMipReadback& Readback = Readbacks[Index];
        const int64         RowBytes = TextureAssets::GetRowPitch(Format, Mips[Index].Width);

        FTextureMip& Mip = OutMips.Emplace();
        Mip.Width  = Mips[Index].Width;
        Mip.Height = Mips[Index].Height;
        Mip.Data.ResizeUninitialized(static_cast<int32>(RowBytes * Readback.BlocksY));

        for (uint32 Row = 0; Row < Readback.BlocksY; ++Row)
        {
            Memory::Memcpy(Mip.Data.Data() + RowBytes * Row, Mapped + Readback.Offset + Readback.RowStride * Row, RowBytes);
        }
    }

    ReadbackBuffer->Unmap(0, ReadbackSize);
    return true;
}
