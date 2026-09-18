#pragma once
#include "Core/CoreDefines.h"
#include "Core/CoreTypes.h"

enum class EImageFormat
{
    /** @brief Red in the lowest byte, which is the order PNG itself stores. */
    R8G8B8A8,

    /** @brief Blue in the lowest byte, which is the order a swap-chain readback usually is. */
    B8G8R8A8,
};

struct FImageView
{
    /** @brief Every format here is four channels of eight bits. */
    static constexpr int32 BytesPerPixel = 4;

    FImageView()
        : Pixels(nullptr)
        , Width(0)
        , Height(0)
        , RowPitch(0)
        , Format(EImageFormat::R8G8B8A8)
    {
    }

    /**
     * @brief Describes an image in memory.
     *
     * @param InPixels   The top row first. Must outlive the view.
     * @param InWidth    Width in pixels.
     * @param InHeight   Height in pixels.
     * @param InFormat   The channel order the bytes are in.
     * @param InRowPitch Bytes from the start of one row to the start of the next. Zero means the
     *                   rows are tightly packed.
     */
    FImageView(const uint8* InPixels, int32 InWidth, int32 InHeight, EImageFormat InFormat = EImageFormat::R8G8B8A8, int32 InRowPitch = 0)
        : Pixels(InPixels)
        , Width(InWidth)
        , Height(InHeight)
        , RowPitch((InRowPitch > 0) ? InRowPitch : (InWidth * BytesPerPixel))
        , Format(InFormat)
    {
    }

    /** @return True when there are pixels to read and the pitch is wide enough to hold a row. */
    NODISCARD bool IsValid() const
    {
        return Pixels && Width > 0 && Height > 0 && RowPitch >= GetRowBytes();
    }

    /** @return The bytes of a row that carry pixels, which is short of the pitch when padded. */
    NODISCARD int32 GetRowBytes() const
    {
        return Width * BytesPerPixel;
    }

    /** @return The first byte of the given row, counting from the top. */
    NODISCARD const uint8* GetRow(int32 Y) const
    {
        return Pixels + (static_cast<int64>(Y) * RowPitch);
    }

    /** @return True when red and blue have to trade places to reach PNG's order. */
    NODISCARD bool NeedsRedBlueSwap() const
    {
        return Format == EImageFormat::B8G8R8A8;
    }

    const uint8* Pixels;
    int32        Width;
    int32        Height;
    int32        RowPitch;
    EImageFormat Format;
};
