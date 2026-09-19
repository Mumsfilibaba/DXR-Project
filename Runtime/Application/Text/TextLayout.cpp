#include "Application/Text/TextLayout.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Style/UIStyle.h"
#include "Application/Text/IFontFace.h"
#include "Core/Math/Math.h"

void DrawTextWithSearchHighlight(FDrawCommandList& OutCommandList, int32 LayerId, const FRectangle& LabelBounds,
    const String& Label, int32 MatchOffset, int32 MatchLength, const IFontFace* Font, const FFloatColor& TextColor)
{
    if (!Font)
    {
        return;
    }

    const FUIStyle&        Style    = FUIStyle::GetDefault();
    const FUITreeRowStyle& RowStyle = Style.TreeRow;
    const StringView       LabelView(Label.Data(), Label.Length());

    const int32 MatchLeft  = Font->MeasureWidth(StringView(LabelView.Data(), MatchOffset));
    const int32 MatchWidth = Font->MeasureWidth(StringView(LabelView.Data() + MatchOffset, MatchLength));

    const FRectangle MatchBounds(IntVector2(LabelBounds.Position.X + MatchLeft, LabelBounds.Position.Y), MatchWidth, LabelBounds.Height);
    const FRectangle HighlightBounds = MatchBounds.Inflate(Style.Metrics.TextHighlightPadding);

    OutCommandList.AddBox(LayerId, HighlightBounds, RowStyle.SearchHighlight, FCornerRadii(Style.Metrics.TextHighlightCornerRadius));

    FRectangle RunBounds = LabelBounds;
    if (MatchOffset > 0)
    {
        OutCommandList.AddText(LayerId + 1, RunBounds, Label.SubString(0, MatchOffset), Font, TextColor);
    }

    RunBounds.Position.X = LabelBounds.Position.X + MatchLeft;
    OutCommandList.AddText(LayerId + 1, RunBounds, Label.SubString(MatchOffset, MatchLength), Font, RowStyle.SearchHighlightText);

    const int32 SuffixOffset = MatchOffset + MatchLength;
    if (SuffixOffset < Label.Length())
    {
        RunBounds.Position.X = LabelBounds.Position.X + MatchLeft + MatchWidth;
        OutCommandList.AddText(LayerId + 1, RunBounds, Label.SubString(SuffixOffset, Label.Length() - SuffixOffset), Font, TextColor);
    }
}

constexpr int32 FALLBACK_LINE_HEIGHT = 1;

FTextLayout::FTextLayout()
    : SourceRuns()
    , Runs()
    , RunSourceOffsets()
    , Lines()
    , CachedText()
    , CachedSize(0, 0)
    , CachedWrapWidth(0)
    , SourceLength(0)
    , PendingLineWidth(0)
    , bIsTextCacheValid(true)
{
}

FTextLayout::~FTextLayout() = default;

void FTextLayout::AppendRun(const FTextRun& Run)
{
    SourceLength += Run.Text.Length();
    SourceRuns.Add(Run);

    Runs.Clear();
    RunSourceOffsets.Clear();
    Lines.Clear();

    CachedSize        = IntVector2(0, 0);
    bIsTextCacheValid = false;
}

void FTextLayout::Clear()
{
    SourceRuns.Clear();
    Runs.Clear();
    RunSourceOffsets.Clear();
    Lines.Clear();
    CachedText.Clear();

    CachedSize        = IntVector2(0, 0);
    CachedWrapWidth   = 0;
    SourceLength      = 0;
    PendingLineWidth  = 0;
    bIsTextCacheValid = true;
}

void FTextLayout::WrapToWidth(int32 WrapWidth)
{
    Runs.Clear();
    RunSourceOffsets.Clear();
    Lines.Clear();

    CachedWrapWidth  = WrapWidth;
    CachedSize       = IntVector2(0, 0);
    PendingLineWidth = 0;

    StartLine();

    int32 SourceOffset = 0;

    for (const FTextRun& SourceRun : SourceRuns)
    {
        const CHAR* Characters = SourceRun.Text.Data();
        const int32 Length     = SourceRun.Text.Length();

        int32 Position = 0;
        while (Position <= Length)
        {
            int32 SegmentEnd = Position;
            while (SegmentEnd < Length && Characters[SegmentEnd] != '\n')
            {
                SegmentEnd++;
            }

            WrapSegment(SourceRun, Characters + Position, SegmentEnd - Position, SourceOffset + Position, WrapWidth);

            if (SegmentEnd >= Length)
            {
                break;
            }

            FinishLine();
            StartLine();

            Position = SegmentEnd + 1;
        }

        SourceOffset += Length;
    }

    FinishLine();

    int32 TotalHeight = 0;
    int32 WidestLine  = 0;

    for (const FTextLine& Line : Lines)
    {
        WidestLine   = Math::Max(WidestLine, Line.Width);
        TotalHeight += Line.Height;
    }

    CachedSize = IntVector2(WidestLine, TotalHeight);
}

void FTextLayout::StartLine()
{
    FTextLine& Line     = Lines.Emplace();
    Line.FirstRunIndex  = Runs.Size();
    Line.RunCount       = 0;
    PendingLineWidth    = 0;
}

void FTextLayout::FinishLine()
{
    if (Lines.IsEmpty())
    {
        return;
    }

    FTextLine& Line = Lines.Last();

    int32 Width     = 0;
    int32 Height    = 0;
    int32 Baseline  = 0;

    for (int32 Index = 0; Index < Line.RunCount; ++Index)
    {
        const FTextRun& Run = Runs[Line.FirstRunIndex + Index];
        Width += MeasureRun(Run);

        if (Run.Font)
        {
            Height   = Math::Max(Height, Run.Font->GetLineHeight());
            Baseline = Math::Max(Baseline, Run.Font->GetAscent());
        }
    }

    if (Height <= 0)
    {
        for (const FTextRun& Run : SourceRuns)
        {
            if (Run.Font)
            {
                Height   = Math::Max(Height, Run.Font->GetLineHeight());
                Baseline = Math::Max(Baseline, Run.Font->GetAscent());
            }
        }
    }

    Line.Width    = Width;
    Line.Height   = Math::Max(Height, FALLBACK_LINE_HEIGHT);
    Line.Baseline = Baseline;
}

void FTextLayout::AppendPiece(const FTextRun& Style, const CHAR* Characters, int32 Count, int32 SourceOffset)
{
    if (Count <= 0)
    {
        return;
    }

    FTextRun& Run = Runs.Emplace();
    Run.Text           = String(Characters, Count);
    Run.Font           = Style.Font;
    Run.Tint           = Style.Tint;
    Run.BackgroundTint = Style.BackgroundTint;

    RunSourceOffsets.Add(SourceOffset);

    PendingLineWidth += MeasureRun(Run);
    Lines.Last().RunCount++;
}

void FTextLayout::WrapSegment(const FTextRun& Style, const CHAR* Characters, int32 Count, int32 SourceOffset, int32 WrapWidth)
{
    if (Count <= 0)
    {
        return;
    }

    if (WrapWidth <= 0 || !Style.Font)
    {
        AppendPiece(Style, Characters, Count, SourceOffset);
        return;
    }

    int32 PieceStart = 0;

    while (PieceStart < Count)
    {
        const int32 Available = WrapWidth - PendingLineWidth;
        if (Available <= 0 && Lines.Last().RunCount > 0)
        {
            FinishLine();
            StartLine();
            continue;
        }

        const int32 Remaining      = Count - PieceStart;
        const int32 RemainingWidth = Style.Font->MeasureWidth(StringView(Characters + PieceStart, Remaining));

        if (RemainingWidth <= Available)
        {
            AppendPiece(Style, Characters + PieceStart, Remaining, SourceOffset + PieceStart);
            return;
        }

        int32 FitCount = Style.Font->FindCharacterIndexAtOffset(StringView(Characters + PieceStart, Remaining), Available);
        FitCount       = Math::Clamp(FitCount, 0, Remaining);

        while (FitCount > 0 && Style.Font->MeasureWidth(StringView(Characters + PieceStart, FitCount)) > Available)
        {
            FitCount--;
        }

        int32 BreakCount = FitCount;
        while (BreakCount > 0 && Characters[PieceStart + BreakCount - 1] != ' ' && Characters[PieceStart + BreakCount - 1] != '\t')
        {
            BreakCount--;
        }

        if (BreakCount > 0)
        {
            AppendPiece(Style, Characters + PieceStart, BreakCount, SourceOffset + PieceStart);
            PieceStart += BreakCount;
        }
        else if (Lines.Last().RunCount > 0)
        {
            FinishLine();
            StartLine();
            continue;
        }
        else
        {
            const int32 CharacterCount = Math::Max(FitCount, 1);
            AppendPiece(Style, Characters + PieceStart, CharacterCount, SourceOffset + PieceStart);
            PieceStart += CharacterCount;
        }

        if (PieceStart < Count)
        {
            FinishLine();
            StartLine();
        }
    }
}

int32 FTextLayout::MeasureRun(const FTextRun& Run)
{
    if (!Run.Font || Run.Text.IsEmpty())
    {
        return 0;
    }

    return Run.Font->MeasureWidth(StringView(Run.Text.Data(), Run.Text.Length()));
}

int32 FTextLayout::FindCharacterIndexAt(const IntVector2& LocalPosition) const
{
    if (Lines.IsEmpty())
    {
        return 0;
    }

    int32 LineTop   = 0;
    int32 LineIndex = Lines.Size() - 1;

    for (int32 Index = 0; Index < Lines.Size(); ++Index)
    {
        if (LocalPosition.Y < LineTop + Lines[Index].Height)
        {
            LineIndex = Index;
            break;
        }

        LineTop += Lines[Index].Height;
    }

    const FTextLine& Line = Lines[LineIndex];

    if (Line.RunCount <= 0)
    {
        return Line.FirstRunIndex < RunSourceOffsets.Size() 
            ? RunSourceOffsets[Line.FirstRunIndex] 
            : SourceLength;
    }

    int32 RunLeft = 0;

    for (int32 Index = 0; Index < Line.RunCount; ++Index)
    {
        const int32     RunIndex = Line.FirstRunIndex + Index;
        const FTextRun& Run      = Runs[RunIndex];
        const int32     RunWidth = MeasureRun(Run);

        const bool bIsLastRun = Index == Line.RunCount - 1;

        if (LocalPosition.X < RunLeft + RunWidth || bIsLastRun)
        {
            if (!Run.Font)
            {
                return RunSourceOffsets[RunIndex];
            }

            const int32 OffsetInRun = Run.Font->FindCharacterIndexAtOffset(
                StringView(Run.Text.Data(), Run.Text.Length()), LocalPosition.X - RunLeft);

            return RunSourceOffsets[RunIndex] + Math::Clamp(OffsetInRun, 0, Run.Text.Length());
        }

        RunLeft += RunWidth;
    }

    return SourceLength;
}

FRectangle FTextLayout::GetCharacterBounds(int32 CharacterIndex) const
{
    int32 LineTop = 0;

    for (const FTextLine& Line : Lines)
    {
        int32 RunLeft = 0;

        for (int32 Index = 0; Index < Line.RunCount; ++Index)
        {
            const int32     RunIndex   = Line.FirstRunIndex + Index;
            const FTextRun& Run        = Runs[RunIndex];
            const int32     RunStart   = RunSourceOffsets[RunIndex];
            const int32     RunLength  = Run.Text.Length();
            const int32     RunWidth   = MeasureRun(Run);
            const bool      bIsLastRun = Index == Line.RunCount - 1;
            const int32     UpperBound = bIsLastRun ? RunStart + RunLength : RunStart + RunLength - 1;

            if (CharacterIndex >= RunStart && CharacterIndex <= UpperBound)
            {
                const int32 OffsetInRun = CharacterIndex - RunStart;

                int32 PrefixWidth = 0;
                int32 Advance     = 0;

                if (Run.Font)
                {
                    Run.Font->GetCharacterPlacement(StringView(Run.Text.Data(), RunLength), OffsetInRun, PrefixWidth, Advance);
                }

                return FRectangle(IntVector2(RunLeft + PrefixWidth, LineTop), Advance, Line.Height);
            }

            RunLeft += RunWidth;
        }

        LineTop += Line.Height;
    }

    return FRectangle(IntVector2(0, 0), 0, Lines.IsEmpty() ? 0 : Lines[0].Height);
}

void FTextLayout::GatherRangeRectangles(const TArray<FTextRange>& Ranges, TArray<FRectangle>& OutRectangles) const
{
    if (Ranges.IsEmpty())
    {
        return;
    }

    int32 LineTop    = 0;
    int32 FirstRange = 0;

    for (const FTextLine& Line : Lines)
    {
        if (Line.RunCount <= 0)
        {
            LineTop += Line.Height;
            continue;
        }

        const int32 LastRunIndex = Line.FirstRunIndex + Line.RunCount - 1;
        const int32 LineStart    = RunSourceOffsets[Line.FirstRunIndex];
        const int32 LineEnd      = RunSourceOffsets[LastRunIndex] + Runs[LastRunIndex].Text.Length();

        while (FirstRange < Ranges.Size() && Ranges[FirstRange].End <= LineStart)
        {
            FirstRange++;
        }

        if (FirstRange >= Ranges.Size())
        {
            break;
        }

        const auto MeasureOffsetOnLine = [this, &Line](int32 CharacterIndex) -> int32
        {
            int32 RunLeft = 0;
            for (int32 Index = 0; Index < Line.RunCount; ++Index)
            {
                const int32     RunIndex  = Line.FirstRunIndex + Index;
                const FTextRun& Run       = Runs[RunIndex];
                const int32     RunStart  = RunSourceOffsets[RunIndex];
                const int32     RunLength = Run.Text.Length();

                if (CharacterIndex <= RunStart + RunLength)
                {
                    const int32 OffsetInRun = Math::Clamp(CharacterIndex - RunStart, 0, RunLength);
                    return RunLeft + (Run.Font ? Run.Font->MeasureWidth(StringView(Run.Text.Data(), OffsetInRun)) : 0);
                }

                RunLeft += MeasureRun(Run);
            }

            return RunLeft;
        };

        for (int32 Index = FirstRange; Index < Ranges.Size() && Ranges[Index].Start < LineEnd; ++Index)
        {
            const int32 SpanStart = Math::Max(Ranges[Index].Start, LineStart);
            const int32 SpanEnd   = Math::Min(Ranges[Index].End, LineEnd);

            if (SpanStart >= SpanEnd)
            {
                continue;
            }

            const int32 Left  = MeasureOffsetOnLine(SpanStart);
            const int32 Right = MeasureOffsetOnLine(SpanEnd);

            OutRectangles.Emplace(IntVector2(Left, LineTop), Right - Left, Line.Height);
        }

        LineTop += Line.Height;
    }
}

int32 FTextLayout::FindLineIndexForCharacter(int32 CharacterIndex) const
{
    for (int32 LineIndex = 0; LineIndex < Lines.Size(); ++LineIndex)
    {
        const FTextLine& Line = Lines[LineIndex];
        if (Line.RunCount <= 0)
        {
            continue;
        }

        const int32 LastRunIndex = Line.FirstRunIndex + Line.RunCount - 1;
        const int32 LineEnd      = RunSourceOffsets[LastRunIndex] + Runs[LastRunIndex].Text.Length();

        if (CharacterIndex <= LineEnd)
        {
            return LineIndex;
        }
    }

    return Lines.IsEmpty() ? 0 : Lines.Size() - 1;
}

bool FTextLayout::GetLineCharacterRange(int32 LineIndex, int32& OutStart, int32& OutEnd) const
{
    if (LineIndex < 0 || LineIndex >= Lines.Size())
    {
        return false;
    }

    const FTextLine& Line = Lines[LineIndex];
    if (Line.RunCount <= 0)
    {
        OutStart = Line.FirstRunIndex < RunSourceOffsets.Size() ? RunSourceOffsets[Line.FirstRunIndex] : SourceLength;
        OutEnd   = OutStart;
        return true;
    }

    const int32 LastRunIndex = Line.FirstRunIndex + Line.RunCount - 1;
    OutStart = RunSourceOffsets[Line.FirstRunIndex];
    OutEnd   = RunSourceOffsets[LastRunIndex] + Runs[LastRunIndex].Text.Length();
    return true;
}

const String& FTextLayout::GetText() const
{
    if (bIsTextCacheValid)
    {
        return CachedText;
    }

    CachedText.Clear();
    CachedText.Reserve(SourceLength);

    for (const FTextRun& Run : SourceRuns)
    {
        CachedText.Append(Run.Text);
    }

    bIsTextCacheValid = true;
    return CachedText;
}

int32 FTextLayout::Draw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const IntVector2 Origin        = AllottedGeometry.Bounds.Position;
    const FRectangle ClipRectangle = OutCommandList.GetCurrentClipRectangle();
    const bool       bIsClipped    = !ClipRectangle.IsEmpty();
    const int32      VisibleTop    = ClipRectangle.Position.Y;
    const int32      VisibleBottom = ClipRectangle.GetBottom();

    int32 HighestLayer = Lines.IsEmpty() ? LayerId : LayerId + 1;
    int32 LineTop      = 0;

    for (const FTextLine& Line : Lines)
    {
        if (bIsClipped)
        {
            const int32 LineBottom = Origin.Y + LineTop + Line.Height;
            if (LineBottom <= VisibleTop)
            {
                LineTop += Line.Height;
                continue;
            }

            if ((Origin.Y + LineTop) >= VisibleBottom)
            {
                break;
            }
        }

        int32 RunLeft = 0;
        for (int32 Index = 0; Index < Line.RunCount; ++Index)
        {
            const FTextRun&  Run       = Runs[Line.FirstRunIndex + Index];
            const int32      RunWidth  = MeasureRun(Run);
            const FRectangle RunBounds = FRectangle(IntVector2(Origin.X + RunLeft, Origin.Y + LineTop), RunWidth, Line.Height);

            if (Run.BackgroundTint.A > 0.0f && RunWidth > 0)
            {
                OutCommandList.AddBox(LayerId, RunBounds, Run.BackgroundTint);
            }

            if (!Run.Text.IsEmpty())
            {
                OutCommandList.AddText(LayerId + 1, RunBounds, Run.Text, Run.Font, Run.Tint);
            }

            RunLeft += RunWidth;
        }

        LineTop += Line.Height;
    }

    return HighestLayer;
}
