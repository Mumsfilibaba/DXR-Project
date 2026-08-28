#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Core/Math/Color.h"
#include "Core/Math/IntVector2.h"
#include "Application/Draw/DrawTypes.h"

struct IFontFace;
class FDrawCommandList;

struct FTextRun
{
    FTextRun()
        : Text()
        , Font(nullptr)
        , Tint(1.0f, 1.0f, 1.0f, 1.0f)
        , BackgroundTint(0.0f, 0.0f, 0.0f, 0.0f)
    {
    }

    FTextRun(const String& InText, const IFontFace* InFont, const FFloatColor& InTint)
        : Text(InText)
        , Font(InFont)
        , Tint(InTint)
        , BackgroundTint(0.0f, 0.0f, 0.0f, 0.0f)
    {
    }

    String           Text;
    const IFontFace* Font;
    FFloatColor      Tint;
    FFloatColor      BackgroundTint;
};

struct FTextLine
{
    FTextLine()
        : FirstRunIndex(0)
        , RunCount(0)
        , Width(0)
        , Height(0)
        , Baseline(0)
    {
    }

    int32 FirstRunIndex;
    int32 RunCount;
    int32 Width;
    int32 Height;
    int32 Baseline;
};

class APPLICATION_API FTextLayout
{
public:
    FTextLayout();
    ~FTextLayout();

    /**
     * @brief Appends a run to the end of the text, invalidating the current layout.
     *
     * @param Run The run to append.
     */
    void AppendRun(const FTextRun& Run);

    /** @brief Drops every run and line. */
    void Clear();

    /**
     * @brief Breaks the runs into lines, splitting a run where it crosses the wrap width.
     *
     * @param WrapWidth The width in pixels to wrap at. Zero or less leaves the text on one line per newline.
     */
    void WrapToWidth(int32 WrapWidth);

    /**
     * @brief Gets the extents of the wrapped text.
     *
     * @return The width of the widest line and the heights of every line added up, in pixels, both zero
     * until WrapToWidth has run.
     */
    NODISCARD FORCEINLINE IntVector2 GetSize() const
    {
        return CachedSize;
    }

    /** @return The lines the text was broken into by the last WrapToWidth call, top to bottom. */
    NODISCARD FORCEINLINE const TArray<FTextLine>& GetLines() const
    {
        return Lines;
    }

    /**
     * @brief Gets the runs the lines index into.
     *
     * @return The appended runs split at every wrap and newline, whose text does not carry the newlines
     * themselves.
     */
    NODISCARD FORCEINLINE const TArray<FTextRun>& GetRuns() const
    {
        return Runs;
    }

    /** @return The runs as appended, before wrapping split them, which the character indices count over. */
    NODISCARD FORCEINLINE const TArray<FTextRun>& GetSourceRuns() const
    {
        return SourceRuns;
    }

    /**
     * @brief Gets the width the text was last wrapped to.
     *
     * @return The width in pixels the last WrapToWidth call was given, which is zero before the first
     * of them and after a Clear.
     */
    NODISCARD FORCEINLINE int32 GetWrapWidth() const
    {
        return CachedWrapWidth;
    }

    /** @return How many characters the appended runs concatenate to, counting the newlines. */
    NODISCARD FORCEINLINE int32 GetCharacterCount() const
    {
        return SourceLength;
    }

    /**
     * @brief Finds the character nearest a point, for click-to-place-caret and drag selection.
     *
     * @param LocalPosition The point, relative to the top-left of the layout.
     * @return The index into the concatenated text.
     */
    NODISCARD int32 FindCharacterIndexAt(const IntVector2& LocalPosition) const;

    /**
     * @brief The rectangle one character occupies, for caret and selection drawing.
     *
     * @param CharacterIndex The index into the concatenated text.
     * @return The rectangle, relative to the top-left of the layout.
     */
    NODISCARD FRectangle GetCharacterBounds(int32 CharacterIndex) const;

    /**
     * @brief The line a character sits on, which is what a scroll-into-view needs.
     *
     * @param CharacterIndex The index into the concatenated text.
     * @return The line index, or zero when there are no lines.
     */
    NODISCARD int32 FindLineIndexForCharacter(int32 CharacterIndex) const;

    /**
     * @brief The span of the concatenated text a line covers, which is what a selection is clipped to.
     *
     * @param LineIndex The line to measure.
     * @param OutStart  Receives the index of its first character.
     * @param OutEnd    Receives the index one past its last.
     * @return True when the line exists.
     */
    NODISCARD bool GetLineCharacterRange(int32 LineIndex, int32& OutStart, int32& OutEnd) const;

    /** @return The text of every run concatenated, which is the string the indices refer to. */
    NODISCARD String GetText() const;

    /**
     * @brief Emits the wrapped text.
     *
     * @param AllottedGeometry The rectangle and scale to draw into.
     * @param OutCommandList   The list to append to.
     * @param LayerId          The layer to draw on.
     * @return The highest layer drawn on.
     */
    int32 Draw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const;

private:
    void StartLine();
    void FinishLine();
    void AppendPiece(const FTextRun& Style, const CHAR* Characters, int32 Count, int32 SourceOffset);
    void WrapSegment(const FTextRun& Style, const CHAR* Characters, int32 Count, int32 SourceOffset, int32 WrapWidth);

    NODISCARD static int32 MeasureRun(const FTextRun& Run);

    TArray<FTextRun>  SourceRuns;
    TArray<FTextRun>  Runs;
    TArray<int32>     RunSourceOffsets;
    TArray<FTextLine> Lines;
    IntVector2        CachedSize;
    int32             CachedWrapWidth;
    int32             SourceLength;
    int32             PendingLineWidth;
};
