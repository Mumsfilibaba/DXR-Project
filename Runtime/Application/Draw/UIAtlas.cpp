#include "Application/Draw/UIAtlas.h"
#include "Core/Math/Math.h"
#include "Core/Memory/Memory.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "RHI/RHI.h"
#include "RHI/RHICommandList.h"

/** @brief How many bytes one texel of the atlas takes, which only ever holds RGBA8. */
static constexpr int32 GBytesPerTexel = 4;

FUIAtlas::FUIAtlas()
    : Pixels()
    , Texture(nullptr)
    , DebugName()
    , ShelfX(0)
    , ShelfY(0)
    , ShelfHeight(0)
    , UsedHeight(0)
    , bIsDirty(false)
{
}

FUIAtlas::~FUIAtlas()
{
    Release();
}

FUIBrush FUIAtlas::Add(const uint8* InPixels, int32 Width, int32 Height)
{
    if (!InPixels || Width <= 0 || Height <= 0)
    {
        return FUIBrush();
    }

    int32 X = 0;
    int32 Y = 0;

    if (!PackRegion(Width, Height, X, Y))
    {
        LOG_ERROR("[FUIAtlas]: A %dx%d image does not fit the %dx%d atlas", Width, Height, AtlasSize, AtlasSize);
        return FUIBrush();
    }

    if (!EnsureTexture())
    {
        return FUIBrush();
    }

    const int32 RequiredBytes = UsedHeight * AtlasSize * GBytesPerTexel;
    if (Pixels.Size() < RequiredBytes)
    {
        const int32 PreviousBytes = Pixels.Size();

        Pixels.ResizeUninitialized(RequiredBytes);
        Memory::Memzero(Pixels.Data() + PreviousBytes, static_cast<uint64>(RequiredBytes - PreviousBytes));
    }

    const int64 SourceRowBytes = static_cast<int64>(Width) * GBytesPerTexel;
    for (int32 Row = 0; Row < Height; ++Row)
    {
        uint8* Destination = Pixels.Data() + ((static_cast<int64>(Y + Row) * AtlasSize) + X) * GBytesPerTexel;
        Memory::Memcpy(Destination, InPixels + static_cast<int64>(Row) * SourceRowBytes, SourceRowBytes);
    }

    bIsDirty = true;

    constexpr float AtlasSizeF = static_cast<float>(AtlasSize);

    FUIBrush Brush(Texture.Get());
    Brush.MinTexCoord = Vector2(static_cast<float>(X) / AtlasSizeF, static_cast<float>(Y) / AtlasSizeF);
    Brush.MaxTexCoord = Vector2(static_cast<float>(X + Width) / AtlasSizeF, static_cast<float>(Y + Height) / AtlasSizeF);

    return Brush;
}

bool FUIAtlas::Build()
{
    if (!Texture)
    {
        return false;
    }

    if (!bIsDirty)
    {
        return true;
    }

    FRHICommandList CommandList;
    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Texture.Get(), ERHIResourceState::CopyDest));

    const FTextureRegion2D Region(static_cast<uint32>(AtlasSize), static_cast<uint32>(UsedHeight), 0, 0);
    CommandList.UpdateTexture2D(Texture.Get(), Region, 0, Pixels.Data(), static_cast<uint32>(AtlasSize * GBytesPerTexel));

    CommandList.TransitionBarrier(FRHITransitionBarrierDesc::CreateTexture(Texture.Get(), ERHIResourceState::PixelShaderResource));

    FRHICommandListExecutor::Get().ExecuteCommandList(CommandList);

    bIsDirty = false;
    return true;
}

void FUIAtlas::Release()
{
    Pixels.Clear();
    Texture.Reset();

    ShelfX      = 0;
    ShelfY      = 0;
    ShelfHeight = 0;
    UsedHeight  = 0;
    bIsDirty    = false;
}

void FUIAtlas::SetDebugName(const String& InDebugName)
{
    DebugName = InDebugName;

    if (Texture)
    {
        Texture->SetDebugName(DebugName);
    }
}

bool FUIAtlas::EnsureTexture()
{
    if (Texture)
    {
        return true;
    }

    if (!RHI::IsInitialized())
    {
        return false;
    }

    const ETextureUsageFlags Flags = ETextureUsageFlags::ShaderResourceTexture | ETextureUsageFlags::CopyDest;
    const FRHITextureDesc TextureDesc = FRHITextureDesc::CreateTexture2D(EFormat::R8G8B8A8_Unorm, static_cast<uint32>(AtlasSize),
        static_cast<uint32>(AtlasSize), 1, 1, Flags, FClearValue(), ERHIResourceStateTrackingMode::Tracked);

    Texture = RHI::CreateTexture(TextureDesc, ERHIResourceState::PixelShaderResource, nullptr);
    if (!Texture)
    {
        LOG_ERROR("[FUIAtlas]: Failed to create the atlas texture");
        return false;
    }

    Texture->SetDebugName(DebugName.IsEmpty() ? String("UI Atlas") : DebugName);
    return true;
}

bool FUIAtlas::PackRegion(int32 Width, int32 Height, int32& OutX, int32& OutY)
{
    if (Width > AtlasSize || Height > AtlasSize)
    {
        return false;
    }

    if (ShelfX + Width > AtlasSize)
    {
        ShelfY     += ShelfHeight;
        ShelfX      = 0;
        ShelfHeight = 0;
    }

    if (ShelfY + Height > AtlasSize)
    {
        return false;
    }

    OutX = ShelfX;
    OutY = ShelfY;

    ShelfX += Width + Padding;

    ShelfHeight = Math::Max(ShelfHeight, Height + Padding);
    UsedHeight  = Math::Max(UsedHeight, ShelfY + Height);
    return true;
}
