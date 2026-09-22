#pragma once
#include "Core/Math/Color.h"
#include "Core/Math/Vector2.h"
#include "Core/Templates/Utility/EnumOperators.h"
#include "Application/Layout/LayoutTypes.h"

struct IFontFace;
class FRHITexture;

enum class EDrawCommandType : uint8
{
    /** A filled axis-aligned rectangle. */
    Box,

    /** The outline of an axis-aligned rectangle, stroked inward from its edge. */
    BoxOutline,

    /** A run of text on one line, anchored at the top-left of the command bounds. */
    Text,

    /** A thin axis-aligned rule, used for the text cursor and for separators. */
    Line,

    /** A stroked run of points at any orientation, optionally closed into a ring. */
    Polyline,

    /** A filled convex polygon, tessellated as a fan around its first point. */
    ConvexPolygon,

    /** A textured axis-aligned rectangle. */
    Image,

    /** The bottom band of a rounded rectangle, narrowed by its corners and faded out at either end. */
    RoundedBottomBar,

    /** A stroke round a rounded rectangle, full strength along the top and faded back over the top corners. */
    RoundedAccentRing,

    /** Opens a clip region. Kept for tests and dumps; the list no longer records these as commands. */
    ClipPush,

    /** Closes the region opened by the matching ClipPush. Kept for tests and dumps. */
    ClipPop,
};

enum class EDrawCommandFlags : uint8
{
    None = 0,

    /** The command is confined to the rectangle its ClipId names. */
    Clipped = FLAG(1),

    /** A polyline whose last point joins back to its first, so the stroke closes into a ring. */
    Closed = FLAG(2),
};

ENUM_CLASS_OPERATORS(EDrawCommandFlags);

struct FCornerRadii
{
    /**
     * @brief Rounds the top two corners only, which is what a title bar meeting a body wants.
     *
     * @param Radius How far the top-left and top-right corners are rounded, in pixels.
     * @return The radii, with the bottom two corners left square.
     */
    NODISCARD static FORCEINLINE FCornerRadii Top(float Radius)
    {
        return FCornerRadii(Radius, Radius, 0.0f, 0.0f);
    }

    /**
     * @brief Rounds the bottom two corners only.
     *
     * @param Radius How far the bottom-left and bottom-right corners are rounded, in pixels.
     * @return The radii, with the top two corners left square.
     */
    NODISCARD static FORCEINLINE FCornerRadii Bottom(float Radius)
    {
        return FCornerRadii(0.0f, 0.0f, Radius, Radius);
    }

    /**
     * @brief Rounds the left two corners only, which is what the first button of a fused row wants.
     *
     * @param Radius How far the top-left and bottom-left corners are rounded, in pixels.
     * @return The radii, with the right two corners left square.
     */
    NODISCARD static FORCEINLINE FCornerRadii Left(float Radius)
    {
        return FCornerRadii(Radius, 0.0f, 0.0f, Radius);
    }

    /**
     * @brief Rounds the right two corners only, which is what the last button of a fused row wants.
     *
     * @param Radius How far the top-right and bottom-right corners are rounded, in pixels.
     * @return The radii, with the left two corners left square.
     */
    NODISCARD static FORCEINLINE FCornerRadii Right(float Radius)
    {
        return FCornerRadii(0.0f, Radius, Radius, 0.0f);
    }

    FCornerRadii()
        : TopLeft(0.0f)
        , TopRight(0.0f)
        , BottomRight(0.0f)
        , BottomLeft(0.0f)
    {
    }

    FCornerRadii(float UniformRadius)
        : TopLeft(UniformRadius)
        , TopRight(UniformRadius)
        , BottomRight(UniformRadius)
        , BottomLeft(UniformRadius)
    {
    }

    FCornerRadii(float InTopLeft, float InTopRight, float InBottomRight, float InBottomLeft)
        : TopLeft(InTopLeft)
        , TopRight(InTopRight)
        , BottomRight(InBottomRight)
        , BottomLeft(InBottomLeft)
    {
    }

    /**
     * @brief Gets whether the box has square corners, so the tessellator can take the four-vertex path.
     *
     * @return True when no corner has a radius above zero.
     */
    NODISCARD FORCEINLINE bool IsZero() const
    {
        return TopLeft <= 0.0f && TopRight <= 0.0f && BottomRight <= 0.0f && BottomLeft <= 0.0f;
    }

    /**
     * @brief Gets the largest of the four radii, which is what a bound has to allow for.
     *
     * @return The largest radius, in pixels.
     */
    NODISCARD FORCEINLINE float GetLargest() const
    {
        return Math::Max(Math::Max(TopLeft, TopRight), Math::Max(BottomRight, BottomLeft));
    }

    /**
     * @brief Returns the radii with every corner clamped so opposing corners cannot overlap.
     *
     * @param Bounds The rectangle the radii are applied to.
     * @return The clamped radii.
     */
    NODISCARD FCornerRadii ClampToBounds(const FRectangle& Bounds) const
    {
        const float Limit = static_cast<float>(Math::Min(Bounds.Width, Bounds.Height)) * 0.5f;

        FCornerRadii Result;
        Result.TopLeft     = Math::Clamp(TopLeft, 0.0f, Limit);
        Result.TopRight    = Math::Clamp(TopRight, 0.0f, Limit);
        Result.BottomRight = Math::Clamp(BottomRight, 0.0f, Limit);
        Result.BottomLeft  = Math::Clamp(BottomLeft, 0.0f, Limit);
        return Result;
    }

    NODISCARD FORCEINLINE bool operator==(const FCornerRadii& Other) const
    {
        return TopLeft == Other.TopLeft && TopRight == Other.TopRight && BottomRight == Other.BottomRight && BottomLeft == Other.BottomLeft;
    }

    NODISCARD FORCEINLINE bool operator!=(const FCornerRadii& Other) const
    {
        return !(*this == Other);
    }

    float TopLeft;
    float TopRight;
    float BottomRight;
    float BottomLeft;
};

struct FUIBrush
{
    FUIBrush()
        : Texture(nullptr)
        , MinTexCoord(0.0f, 0.0f)
        , MaxTexCoord(1.0f, 1.0f)
        , Margin()
    {
    }

    explicit FUIBrush(FRHITexture* InTexture)
        : Texture(InTexture)
        , MinTexCoord(0.0f, 0.0f)
        , MaxTexCoord(1.0f, 1.0f)
        , Margin()
    {
    }

    /** @return True when a texture is set for the brush to sample. */
    NODISCARD FORCEINLINE bool IsValid() const
    {
        return Texture != nullptr;
    }

    /**
     * @brief Gets whether the brush is drawn as nine stretched patches rather than one.
     *
     * @return True when the nine-slice margin covers any pixels on either axis.
     */
    NODISCARD FORCEINLINE bool IsNineSlice() const
    {
        return Margin.GetTotalHorizontal() > 0 || Margin.GetTotalVertical() > 0;
    }

    /** @brief The texture sampled, which is null for an untextured fill. */
    FRHITexture* Texture;

    /** @brief The top-left of the sampled region, in normalized coordinates. */
    Vector2 MinTexCoord;

    /** @brief The bottom-right of the sampled region, in normalized coordinates. */
    Vector2 MaxTexCoord;

    /** @brief The nine-slice border in pixels. A zero margin stretches the whole image. */
    FMargin Margin;
};

struct FDrawGeometry
{
    FDrawGeometry()
        : Bounds()
        , Scale(1.0f)
    {
    }

    FDrawGeometry(const FRectangle& InBounds, float InScale)
        : Bounds(InBounds)
        , Scale(InScale)
    {
    }

    FRectangle Bounds;
    float      Scale;
};

struct FDrawCommand
{
    EDrawCommandType  Type;
    EDrawCommandFlags Flags;
    uint16            ClipId;
    int32            LayerId;
    FRectangle       Bounds;
    uint32           PackedColor;
    FCornerRadii     CornerRadius;
    float            Thickness;
    float            FadeWidth;
    float            FadeFraction;
    float            TrailAlpha;
    int32            PayloadOffset;
    int32            PayloadCount;
    const IFontFace* Font;

    NODISCARD FORCEINLINE bool IsClipped() const
    {
        return (Flags & EDrawCommandFlags::Clipped) != EDrawCommandFlags::None;
    }

    NODISCARD FORCEINLINE bool IsClosed() const
    {
        return (Flags & EDrawCommandFlags::Closed) != EDrawCommandFlags::None;
    }

    NODISCARD FORCEINLINE bool HasTint(const FFloatColor& Color) const
    {
        return PackedColor == Color.ToPackedRGBA();
    }

    NODISCARD FORCEINLINE uint8 PackedAlpha() const
    {
        return static_cast<uint8>(PackedColor >> 24);
    }
};
