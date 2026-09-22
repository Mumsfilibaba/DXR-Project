#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/ArrayView.h"
#include "Core/Containers/String.h"
#include "Core/Containers/StringView.h"
#include "Application/Draw/DrawTypes.h"

class APPLICATION_API FDrawCommandList
{
public:
    static constexpr int32 InvalidIndex = -1;

    /** @brief The fewest segments a circle or an arc is tessellated into. */
    static constexpr int32 MinCircleSegments = 6;

    /** @brief The most segments a circle or an arc is tessellated into, whatever its radius. */
    static constexpr int32 MaxCircleSegments = 128;

    /** @brief The fewest segments a bezier is flattened into. */
    static constexpr int32 MinBezierSegments = 4;

    /** @brief The most segments a bezier is flattened into, whatever its extent. */
    static constexpr int32 MaxBezierSegments = 64;

public:
    FDrawCommandList();
    ~FDrawCommandList();

    /**
     * @brief Appends a filled rectangle.
     *
     * @param LayerId      The layer to draw on.
     * @param Bounds       The rectangle to fill.
     * @param Tint         The fill color.
     * @param CornerRadius How far each corner is rounded, in pixels, clamped to half the shorter side.
     */
    void AddBox(int32 LayerId, const FRectangle& Bounds, const FFloatColor& Tint, const FCornerRadii& CornerRadius = FCornerRadii());

    /**
     * @brief Appends the outline of a rectangle, stroked inward from its edge.
     *
     * @param LayerId      The layer to draw on.
     * @param Bounds       The rectangle to stroke.
     * @param Tint         The stroke color.
     * @param Thickness    The stroke width in pixels.
     * @param CornerRadius How far each corner is rounded, in pixels.
     */
    void AddBoxOutline(int32 LayerId, const FRectangle& Bounds, const FFloatColor& Tint, float Thickness, const FCornerRadii& CornerRadius = FCornerRadii());

    /**
     * @brief Appends a run of text on one line.
     *
     * @param LayerId The layer to draw on.
     * @param Bounds  The rectangle the text is anchored to at the top-left.
     * @param InText  The text to draw.
     * @param Font    The face the text was measured with, which may be null.
     * @param Tint    The text color.
     */
    void AddText(int32 LayerId, const FRectangle& Bounds, const StringView& InText, const IFontFace* Font, const FFloatColor& Tint);

    /**
     * @brief Appends a run of text on one line.
     *
     * @param LayerId The layer to draw on.
     * @param Bounds  The rectangle the text is anchored to at the top-left.
     * @param InText  The text to draw.
     * @param Font    The face the text was measured with, which may be null.
     * @param Tint    The text color.
     */
    FORCEINLINE void AddText(int32 LayerId, const FRectangle& Bounds, const String& InText, const IFontFace* Font, const FFloatColor& Tint)
    {
        AddText(LayerId, Bounds, StringView(InText), Font, Tint);
    }

    /**
     * @brief Appends a run of text on one line.
     *
     * A literal converts to both String and StringView, so it needs an overload of its own to call.
     *
     * @param LayerId The layer to draw on.
     * @param Bounds  The rectangle the text is anchored to at the top-left.
     * @param InText  The null-terminated text to draw.
     * @param Font    The face the text was measured with, which may be null.
     * @param Tint    The text color.
     */
    FORCEINLINE void AddText(int32 LayerId, const FRectangle& Bounds, const CHAR* InText, const IFontFace* Font, const FFloatColor& Tint)
    {
        AddText(LayerId, Bounds, StringView(InText), Font, Tint);
    }

    /**
     * @brief Appends a thin axis-aligned rule.
     *
     * @param LayerId The layer to draw on.
     * @param Bounds  The rectangle covered by the line.
     * @param Tint    The line color.
     */
    void AddLine(int32 LayerId, const FRectangle& Bounds, const FFloatColor& Tint);

    /**
     * @brief Appends a straight stroke between two points at any orientation.
     *
     * @param LayerId   The layer to draw on.
     * @param Start     Where the stroke begins.
     * @param End       Where the stroke ends.
     * @param Tint      The stroke color.
     * @param Thickness The stroke width in pixels.
     */
    void AddLine(int32 LayerId, const Vector2& Start, const Vector2& End, const FFloatColor& Tint, float Thickness);

    /**
     * @brief Appends a stroked run of points at any orientation.
     *
     * @param LayerId   The layer to draw on.
     * @param Points    The points, in order. Fewer than two draws nothing.
     * @param Tint      The stroke color.
     * @param Thickness The stroke width in pixels.
     * @param bClosed   True to join the last point back to the first.
     */
    void AddPolyline(int32 LayerId, TArrayView<const Vector2> Points, const FFloatColor& Tint, float Thickness, bool bClosed = false);

    /**
     * @brief Appends a filled convex polygon, tessellated as a fan around its first point.
     *
     * @param LayerId The layer to draw on.
     * @param Points  The points in winding order. Fewer than three draws nothing.
     * @param Tint    The fill color.
     */
    void AddConvexPolygon(int32 LayerId, TArrayView<const Vector2> Points, const FFloatColor& Tint);

    /**
     * @brief Appends the rounded frame of a panel so that it survives whatever the panel's content paints.
     *
     * Rounding a panel by filling a rounded rectangle underneath it does not work once the content
     * paints edge to edge, which a viewport image, a tree row fill or a tab strip all do: the square
     * content simply covers the rounded corners again. Clipping cannot fix it either, because the clip
     * stack is a rectangular scissor.
     *
     * So the frame is drawn from the outside in, on a layer above the content. Each corner gets a wedge
     * of backdrop filling the gap between the arc and the square corner, which cuts the content back to
     * the rounded silhouette, and a single stroke follows around the whole edge.
     *
     * @param LayerId      The layer to draw on, which must be above the content being framed.
     * @param Bounds       The panel rectangle to frame.
     * @param CornerRadius How far each corner is rounded, in pixels.
     * @param Thickness    The stroke width in pixels. Zero draws the wedges but no stroke.
     * @param BorderTint   The stroke color.
     * @param BackdropTint The color the corner wedges are filled with, which is what shows around the panel.
     */
    void AddPanelChrome(
        int32               LayerId,
        const FRectangle&   Bounds,
        const FCornerRadii& CornerRadius,
        float               Thickness,
        const FFloatColor&  BorderTint,
        const FFloatColor&  BackdropTint);

    /**
     * @brief Appends a filled triangle. Shorthand for a three-point convex polygon.
     *
     * @param LayerId The layer to draw on.
     * @param A       The first corner.
     * @param B       The second corner.
     * @param C       The third corner.
     * @param Tint    The fill color.
     */
    void AddTriangle(int32 LayerId, const Vector2& A, const Vector2& B, const Vector2& C, const FFloatColor& Tint);

    /**
     * @brief Appends a circle outline. Shorthand for a closed polyline.
     *
     * @param LayerId   The layer to draw on.
     * @param Center    The middle of the circle.
     * @param Radius    The radius in pixels.
     * @param Tint      The stroke color.
     * @param Thickness The stroke width in pixels.
     * @param Segments  How many segments to tessellate into, or zero to derive it from the radius.
     */
    void AddCircle(int32 LayerId, const Vector2& Center, float Radius, const FFloatColor& Tint, float Thickness, int32 Segments = 0);

    /**
     * @brief Appends a filled circle. Shorthand for a convex polygon.
     *
     * @param LayerId  The layer to draw on.
     * @param Center   The middle of the circle.
     * @param Radius   The radius in pixels.
     * @param Tint     The fill color.
     * @param Segments How many segments to tessellate into, or zero to derive it from the radius.
     */
    void AddCircleFilled(int32 LayerId, const Vector2& Center, float Radius, const FFloatColor& Tint, int32 Segments = 0);

    /**
     * @brief Appends a stroked arc, which is a circle walked between two angles.
     *
     * @param LayerId    The layer to draw on.
     * @param Center     The middle of the circle the arc lies on.
     * @param Radius     The radius in pixels.
     * @param StartAngle Where the arc begins, in radians.
     * @param EndAngle   Where the arc ends, in radians.
     * @param Tint       The stroke color.
     * @param Thickness  The stroke width in pixels.
     * @param Segments   How many segments to tessellate into, or zero to derive it from the sweep.
     */
    void AddArc(int32 LayerId, const Vector2& Center, float Radius, float StartAngle, float EndAngle, const FFloatColor& Tint, float Thickness, int32 Segments = 0);

    /**
     * @brief Appends a filled wedge, which is an arc closed back through its center.
     *
     * @param LayerId    The layer to draw on.
     * @param Center     The middle of the circle the wedge turns around.
     * @param Radius     The radius in pixels.
     * @param StartAngle Where the wedge begins, in radians.
     * @param EndAngle   Where the wedge ends, in radians.
     * @param Tint       The fill color.
     * @param Segments   How many segments to tessellate into, or zero to derive it from the sweep.
     */
    void AddArcFilled(int32 LayerId, const Vector2& Center, float Radius, float StartAngle, float EndAngle, const FFloatColor& Tint, int32 Segments = 0);

    /**
     * @brief Appends a cubic bezier, flattened to a polyline.
     *
     * @param LayerId   The layer to draw on.
     * @param P0        The start point.
     * @param P1        The first control point.
     * @param P2        The second control point.
     * @param P3        The end point.
     * @param Tint      The stroke color.
     * @param Thickness The stroke width in pixels.
     * @param Segments  How many segments to flatten into, or zero to derive it from the extent.
     */
    void AddBezier(int32 LayerId, const Vector2& P0, const Vector2& P1, const Vector2& P2, const Vector2& P3, const FFloatColor& Tint, float Thickness, int32 Segments = 0);

    /**
     * @brief Appends a textured rectangle.
     *
     * @param LayerId      The layer to draw on.
     * @param Bounds       The rectangle to fill.
     * @param Brush        The texture and the region of it to sample.
     * @param Tint         The color the sample is multiplied by.
     * @param CornerRadius How far the rectangle's corners are rounded, which a nine-sliced brush ignores
     * since its own corners are drawn from the texture.
     */
    void AddImage(int32 LayerId, const FRectangle& Bounds, const FUIBrush& Brush, const FFloatColor& Tint, const FCornerRadii& CornerRadius = FCornerRadii());

    /**
     * @brief Appends the bottom band of a rounded rectangle, with either end fading out.
     *
     * @param LayerId      The layer to draw on.
     * @param Bounds       The rounded rectangle the band is cut from.
     * @param CornerRadius The rectangle's corners, which decide where the band narrows.
     * @param Thickness    How tall the band is, measured up from the rectangle's bottom edge.
     * @param Tint         The band's color where it runs at full strength.
     * @param FadeWidth    How far in from either edge the alpha runs from nothing to full, in pixels.
     */
    void AddRoundedBottomBar(int32 LayerId, const FRectangle& Bounds, const FCornerRadii& CornerRadius, float Thickness, const FFloatColor& Tint, float FadeWidth);

    /**
     * @brief Appends a stroke round a rounded rectangle at an even width, held at full strength along the top
     * and faded back over the top corners to a trail that carries on round the rest, which is how an active
     * tab wears its accent.
     *
     * @param LayerId      The layer to draw on.
     * @param Bounds       The rounded rectangle the stroke follows.
     * @param CornerRadius The rectangle's corners, which are the curves the stroke turns.
     * @param Thickness    How wide the stroke is, measured inward from the rectangle's edge.
     * @param Tint         The stroke's color where it runs at full strength.
     * @param FadeFraction How much of each top corner the stroke spends fading back, as a share of that
     * corner's arc measured from the side edge. Half fades from the middle of the corner.
     * @param TrailAlpha   What the stroke keeps of that color once the fade is done, as a share of it, which
     * is what runs round the sides and the bottom. Zero leaves the top rule on its own.
     */
    void AddRoundedAccentRing(int32 LayerId, const FRectangle& Bounds, const FCornerRadii& CornerRadius, float Thickness,
        const FFloatColor& Tint, float FadeFraction, float TrailAlpha);

    /**
     * @brief Opens a clip region, intersected with whatever region is already open.
     *
     * @param LayerId       The layer the region belongs to.
     * @param ClipRectangle The region to clip to.
     */
    void PushClip(int32 LayerId, const FRectangle& ClipRectangle);

    /**
     * @brief Closes the region opened by the matching PushClip.
     *
     * @param LayerId The layer the region belongs to.
     */
    void PopClip(int32 LayerId);

    /** @brief Drops every command and the clip state, so the list can be filled again. */
    void Reset();

    /** @return True when no clip region is still open and no pop arrived without a push. */
    NODISCARD FORCEINLINE bool IsClipStackBalanced() const
    {
        return ClipStack.IsEmpty() && UnmatchedPopCount == 0;
    }

    /**
     * @brief Gets the innermost clip region, which is already intersected with the ones enclosing it.
     *
     * @return The region on top of the clip stack, or an empty rectangle when none is open.
     */
    NODISCARD FORCEINLINE const FRectangle& GetCurrentClipRectangle() const
    {
        return ClipStack.IsEmpty() ? EmptyClipRectangle : ClipRects[ClipStack.Last() - 1];
    }

    /** @return The number of commands the list holds. */
    NODISCARD FORCEINLINE int32 Size() const
    {
        return Commands.Size();
    }

    /** @return True when nothing has been appended since the list was created or reset. */
    NODISCARD FORCEINLINE bool IsEmpty() const
    {
        return Commands.IsEmpty();
    }

    /** @return The commands appended so far, in emission order rather than layer order. */
    NODISCARD FORCEINLINE const TArray<FDrawCommand>& GetCommands() const
    {
        return Commands;
    }

    /** @return The point pool every polyline or polygon command indexes into by offset and count. */
    NODISCARD FORCEINLINE const TArray<Vector2>& GetPoints() const
    {
        return Points;
    }

    /**
     * @brief The points one command owns, which is empty for a command that carries none.
     *
     * @param Command The command to read the range from.
     * @return A view over the command's points.
     */
    NODISCARD TArrayView<const Vector2> GetCommandPoints(const FDrawCommand& Command) const;

    /**
     * @brief The text one command owns, which is empty for anything but a text command that carries characters.
     *
     * The characters live in a pool shared by the whole list, so appending to the list or resetting it can move
     * them and leave the view dangling. Read it before emitting more commands, or copy it.
     *
     * @param Command The command to read the range from.
     * @return A view over the command's characters, which is not null terminated.
     */
    NODISCARD StringView GetCommandText(const FDrawCommand& Command) const;

    /**
     * @brief The brush one command owns, which only an image command has.
     *
     * The brushes live in a pool shared by the whole list, so appending to the list or resetting it can move them
     * and leave the pointer dangling.
     *
     * @param Command The command to read the brush from.
     * @return The command's brush, or null when the command is not an image or carries no brush.
     */
    NODISCARD const FUIBrush* GetCommandBrush(const FDrawCommand& Command) const;

    /**
     * @brief The rectangle one command is clipped to, which is how a batch decides what it may cover.
     *
     * @param Command The command to read the clip from.
     * @return The command's clip rectangle, or an empty rectangle when the command is not clipped.
     */
    NODISCARD const FRectangle& GetCommandClipRectangle(const FDrawCommand& Command) const;

    /**
     * @brief Counts the commands of one type, so a test can check what an element emitted.
     *
     * @param Type The command type to count.
     * @return The number of commands of that type.
     */
    NODISCARD int32 CountCommandsOfType(EDrawCommandType Type) const;

    /**
     * @brief Finds the first text command whose string matches.
     *
     * @param InText The text to look for.
     * @return The index of the command, or InvalidIndex when there is none.
     */
    NODISCARD int32 FindTextCommand(const StringView& InText) const;

    NODISCARD FORCEINLINE const FDrawCommand& operator[](int32 Index) const
    {
        return Commands[Index];
    }

private:
    NODISCARD static int32 ResolveCircleSegments(float Radius, float AngleSweep, int32 RequestedSegments);

    FDrawCommand& EmplaceCommand(EDrawCommandType Type, int32 LayerId);
    void StorePoints(FDrawCommand& Command, TArrayView<const Vector2> InPoints);
    void StoreText(FDrawCommand& Command, StringView InText);
    void BuildArcPoints(const Vector2& Center, float Radius, float StartAngle, float EndAngle, int32 Segments);

    TArray<FDrawCommand> Commands;
    TArray<Vector2>      Points;
    TArray<CHAR>         TextPool;
    TArray<FUIBrush>     Brushes;
    TArray<FRectangle>   ClipRects;
    TArray<Vector2>      ScratchPoints;
    TArray<uint16>       ClipStack;
    FRectangle           EmptyClipRectangle;
    int32                UnmatchedPopCount;
};
