#include "Application/Draw/DrawCommandList.h"
#include "Application/Elements/RichTextBlock.h"
#include "Application/Input/Keys.h"
#include "Application/Style/UIStyle.h"
#include "Core/Math/Math.h"
#include "Core/Platform/PlatformSystemClipboard.h"

TSharedPtr<FRichTextBlock> FRichTextBlock::Create(const FDesc& Desc)
{
    TSharedPtr<FRichTextBlock> NewBlock = MakeSharedPtr<FRichTextBlock>();
    NewBlock->Initialize(Desc);
    return NewBlock;
}

FRichTextBlock::FRichTextBlock()
    : FInteractiveElement()
    , Layout()
    , Margin()
    , SearchText()
    , SearchMatches()
    , SelectionAnchor(0)
    , SelectionCursor(0)
    , bIsSelectable(true)
    , bAutoWrapText(true)
    , OnSelectionChangedDelegate()
{
}

FRichTextBlock::~FRichTextBlock() = default;

void FRichTextBlock::Initialize(const FDesc& Desc)
{
    Margin                     = Desc.Margin;
    bIsSelectable              = Desc.bIsSelectable;
    bAutoWrapText              = Desc.bAutoWrapText;
    OnSelectionChangedDelegate = Desc.OnSelectionChanged;

    SetRuns(Desc.Runs);
}

IntVector2 FRichTextBlock::ComputeDesiredSize() const
{
    const int32 AvailableWidth = Math::Max(0, GetContentRectangle().Width - Margin.GetTotalHorizontal());
    RefreshLayout(AvailableWidth);

    const IntVector2 TextSize = Layout.GetSize();
    return IntVector2(TextSize.X + Margin.GetTotalHorizontal(), TextSize.Y + Margin.GetTotalVertical());
}

void FRichTextBlock::OnArrange(const FRectangle& AllottedBounds)
{
    RefreshLayout(Math::Max(0, AllottedBounds.Width - Margin.GetTotalHorizontal()));
}

int32 FRichTextBlock::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle&  Style      = FUIStyle::GetDefault();
    const FRectangle TextBounds = GetTextBounds();

    RefreshLayout(TextBounds.Width);

    TArray<FRectangle> Rectangles;
    for (int32 MatchStart : SearchMatches)
    {
        GatherRangeRectangles(MatchStart, MatchStart + SearchText.Length(), Rectangles);
    }

    for (const FRectangle& Rectangle : Rectangles)
    {
        OutCommandList.AddBox(LayerId, Rectangle, Style.Colors.Accent);
    }

    if (HasSelection())
    {
        Rectangles.Clear();
        GatherRangeRectangles(GetSelectionStart(), GetSelectionEnd(), Rectangles);

        for (const FRectangle& Rectangle : Rectangles)
        {
            OutCommandList.AddBox(LayerId + 1, Rectangle, Style.Colors.TextSelectionBackground);
        }
    }

    const FDrawGeometry TextGeometry(TextBounds, AllottedGeometry.Scale);
    return Layout.Draw(TextGeometry, OutCommandList, LayerId + 2);
}

FEventResponse FRichTextBlock::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    if (!bIsSelectable)
    {
        return FEventResponse::Unhandled();
    }

    const FEventResponse Response = FInteractiveElement::OnMouseButtonDown(CursorEvent);
    if (!Response.IsEventHandled())
    {
        return Response;
    }

    const int32 CharacterIndex = FindCharacterIndexAt(CursorEvent.GetClientPosition());
    SelectionAnchor = CharacterIndex;
    SelectionCursor = CharacterIndex;

    OnSelectionChangedDelegate.ExecuteIfBound(String());
    return Response;
}

FEventResponse FRichTextBlock::OnKeyDown(const FKeyEvent& KeyEvent)
{
    if (bIsSelectable && KeyEvent.IsDown() && KeyEvent.GetModifierKeys().IsCommandDown())
    {
        if (KeyEvent.GetKey() == Keys::A)
        {
            SelectAll();
            return FEventResponse::Handled();
        }

        if (KeyEvent.GetKey() == Keys::C)
        {
            CopyToClipboard();
            return FEventResponse::Handled();
        }
    }

    return FInteractiveElement::OnKeyDown(KeyEvent);
}

void FRichTextBlock::SetRuns(const TArray<FTextRun>& InRuns)
{
    Layout.Clear();
    for (const FTextRun& Run : InRuns)
    {
        Layout.AppendRun(Run);
    }

    ClearSelection();
    RefreshSearchMatches();
}

void FRichTextBlock::AppendRun(const FTextRun& Run)
{
    Layout.AppendRun(Run);
    RefreshSearchMatches();
}

void FRichTextBlock::ClearRuns()
{
    Layout.Clear();

    ClearSelection();
    SearchMatches.Clear();
}

void FRichTextBlock::SetSearchText(const String& InSearchText)
{
    if (SearchText == InSearchText)
    {
        return;
    }

    SearchText = InSearchText;
    RefreshSearchMatches();
}

bool FRichTextBlock::HasSelection() const
{
    return SelectionAnchor != SelectionCursor;
}

int32 FRichTextBlock::GetSelectionStart() const
{
    return Math::Min(SelectionAnchor, SelectionCursor);
}

int32 FRichTextBlock::GetSelectionEnd() const
{
    return Math::Max(SelectionAnchor, SelectionCursor);
}

String FRichTextBlock::GetSelectedText() const
{
    if (!HasSelection())
    {
        return String();
    }

    const String Text = Layout.GetText();
    return String(Text.Data() + GetSelectionStart(), GetSelectionEnd() - GetSelectionStart());
}

void FRichTextBlock::SelectAll()
{
    SetSelection(0, Layout.GetCharacterCount());
}

void FRichTextBlock::ClearSelection()
{
    if (!HasSelection())
    {
        return;
    }

    SelectionAnchor = 0;
    SelectionCursor = 0;

    OnSelectionChangedDelegate.ExecuteIfBound(String());
}

void FRichTextBlock::SetSelection(int32 StartIndex, int32 EndIndex)
{
    const int32 CharacterCount = Layout.GetCharacterCount();

    SelectionAnchor = Math::Clamp(StartIndex, 0, CharacterCount);
    SelectionCursor = Math::Clamp(EndIndex, 0, CharacterCount);

    OnSelectionChangedDelegate.ExecuteIfBound(GetSelectedText());
}

void FRichTextBlock::CopyToClipboard() const
{
    FPlatformSystemClipboard::SetText(HasSelection() ? GetSelectedText() : Layout.GetText());
}

String FRichTextBlock::GetText() const
{
    return Layout.GetText();
}

int32 FRichTextBlock::FindCharacterIndexAt(const IntVector2& ClientPosition) const
{
    const FRectangle TextBounds = GetTextBounds();
    RefreshLayout(TextBounds.Width);

    return Layout.FindCharacterIndexAt(ClientPosition - TextBounds.Position);
}

void FRichTextBlock::OnDragged(const FCursorEvent& CursorEvent)
{
    if (!bIsSelectable)
    {
        return;
    }

    const int32 CharacterIndex = FindCharacterIndexAt(CursorEvent.GetClientPosition());
    if (CharacterIndex == SelectionCursor)
    {
        return;
    }

    SelectionCursor = CharacterIndex;
    OnSelectionChangedDelegate.ExecuteIfBound(GetSelectedText());
}

void FRichTextBlock::RefreshLayout(int32 AvailableWidth) const
{
    const int32 WrapWidth = bAutoWrapText ? Math::Max(0, AvailableWidth) : 0;
    if (Layout.GetWrapWidth() == WrapWidth && !Layout.GetLines().IsEmpty())
    {
        return;
    }

    Layout.WrapToWidth(WrapWidth);
}

void FRichTextBlock::GatherRangeRectangles(int32 StartIndex, int32 EndIndex, TArray<FRectangle>& OutRectangles) const
{
    if (StartIndex >= EndIndex)
    {
        return;
    }

    const FRectangle TextBounds = GetTextBounds();
    for (int32 LineIndex = 0; LineIndex < Layout.GetLines().Size(); ++LineIndex)
    {
        int32 LineStart = 0;
        int32 LineEnd   = 0;

        if (!Layout.GetLineCharacterRange(LineIndex, LineStart, LineEnd))
        {
            continue;
        }

        const int32 SpanStart = Math::Max(StartIndex, LineStart);
        const int32 SpanEnd   = Math::Min(EndIndex, LineEnd);

        if (SpanStart >= SpanEnd)
        {
            continue;
        }

        const FRectangle FirstBounds = Layout.GetCharacterBounds(SpanStart);
        const FRectangle LastBounds  = Layout.GetCharacterBounds(SpanEnd - 1);

        FRectangle Rectangle;
        Rectangle.Position = TextBounds.Position + FirstBounds.Position;
        Rectangle.Width    = LastBounds.GetRight() - FirstBounds.Position.X;
        Rectangle.Height   = Math::Max(FirstBounds.Height, LastBounds.Height);

        OutRectangles.Add(Rectangle);
    }
}

void FRichTextBlock::RefreshSearchMatches()
{
    SearchMatches.Clear();

    if (SearchText.IsEmpty())
    {
        return;
    }

    const String Text        = Layout.GetText();
    const int32  MatchLength = SearchText.Length();

    for (int32 Index = 0; Index + MatchLength <= Text.Length(); ++Index)
    {
        bool bIsMatch = true;
        for (int32 Offset = 0; Offset < MatchLength && bIsMatch; ++Offset)
        {
            bIsMatch = Text[Index + Offset] == SearchText[Offset];
        }

        if (bIsMatch)
        {
            SearchMatches.Add(Index);
            Index += MatchLength - 1;
        }
    }
}

FRectangle FRichTextBlock::GetTextBounds() const
{
    return GetContentRectangle().Deflate(Margin);
}
