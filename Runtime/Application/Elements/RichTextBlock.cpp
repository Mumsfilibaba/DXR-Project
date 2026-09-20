#include "Application/Draw/DrawCommandList.h"
#include "Application/Elements/RichTextBlock.h"
#include "Application/Elements/ScrollBox.h"
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
    , SearchRanges()
    , WrapWidth(0)
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
    WrapWidth                  = Math::Max(0, Desc.WrapWidth);
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

    const FMargin&     HighlightPadding = Style.Metrics.TextHighlightPadding;
    const FCornerRadii HighlightRadii(Style.Metrics.TextHighlightCornerRadius);

    TArray<FRectangle> Rectangles;

    if (HasSelection())
    {
        TArray<FTextRange> SelectionRanges;
        SelectionRanges.Emplace(GetSelectionStart(), GetSelectionEnd());

        GatherRangeRectangles(SelectionRanges, Rectangles);

        for (const FRectangle& Rectangle : Rectangles)
        {
            OutCommandList.AddBox(LayerId, Rectangle.Inflate(HighlightPadding), Style.Colors.TextSelectionBackground, HighlightRadii);
        }
    }

    Rectangles.Clear();
    GatherRangeRectangles(SearchRanges, Rectangles);

    for (const FRectangle& Rectangle : Rectangles)
    {
        OutCommandList.AddBox(LayerId + 1, Rectangle.Inflate(HighlightPadding), Style.Colors.SearchTextHighlight, HighlightRadii);
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

bool FRichTextBlock::GetCursor(ECursor& OutCursor) const
{
    if (!bIsSelectable || !IsEnabled() || !IsHovered())
    {
        return false;
    }

    OutCursor = ECursor::TextInput;
    return true;
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

void FRichTextBlock::SetRunsAndSearchText(const TArray<FTextRun>& InRuns, const String& InSearchText)
{
    Layout.Clear();
    for (const FTextRun& Run : InRuns)
    {
        Layout.AppendRun(Run);
    }

    ClearSelection();

    SearchText = InSearchText;
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
    SearchRanges.Clear();
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

    FScrollBox* ScrollBox = nullptr;
    for (FVisualElement* Parent = GetParentElement().Get(); Parent; Parent = Parent->GetParentElement().Get())
    {
        ScrollBox = Parent->AsScrollBox();
        if (ScrollBox)
        {
            break;
        }
    }

    if (ScrollBox)
    {
        const FRectangle ViewBounds = ScrollBox->GetContentRectangle();
        const int32      CursorY    = CursorEvent.GetClientPosition().Y;
        const int32      Step       = FScrollBox::DefaultScrollAmountPerWheelStep;

        if (CursorY < ViewBounds.Position.Y)
        {
            ScrollBox->SetScrollOffset(ScrollBox->GetScrollOffset() - Step);
        }
        else if (CursorY > ViewBounds.GetBottom())
        {
            ScrollBox->SetScrollOffset(ScrollBox->GetScrollOffset() + Step);
        }
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
    const int32 TargetWrapWidth = (WrapWidth > 0) ? WrapWidth : (bAutoWrapText ? Math::Max(0, AvailableWidth) : 0);
    if (Layout.GetWrapWidth() == TargetWrapWidth && !Layout.GetLines().IsEmpty())
    {
        return;
    }

    Layout.WrapToWidth(TargetWrapWidth);
}

void FRichTextBlock::GatherRangeRectangles(const TArray<FTextRange>& Ranges, TArray<FRectangle>& OutRectangles) const
{
    const int32 FirstRectangle = OutRectangles.Size();
    Layout.GatherRangeRectangles(Ranges, OutRectangles);

    const IntVector2 Origin = GetTextBounds().Position;
    for (int32 Index = FirstRectangle; Index < OutRectangles.Size(); ++Index)
    {
        OutRectangles[Index].Position += Origin;
    }
}

void FRichTextBlock::RefreshSearchMatches()
{
    SearchMatches.Clear();
    SearchRanges.Clear();

    if (SearchText.IsEmpty())
    {
        return;
    }

    const String& Text        = Layout.GetText();
    const int32   MatchLength = SearchText.Length();
    const int32   TextLength  = Text.Length();

    if (MatchLength <= 0 || TextLength < MatchLength)
    {
        return;
    }

    for (int32 Index = Text.Find(SearchText.Data(), 0); Index >= 0 && Index + MatchLength <= TextLength;)
    {
        SearchMatches.Add(Index);
        SearchRanges.Emplace(Index, Index + MatchLength);

        const int32 NextIndex = Index + MatchLength;
        if (NextIndex + MatchLength > TextLength)
        {
            break;
        }

        Index = Text.Find(SearchText.Data(), NextIndex);
    }
}

FRectangle FRichTextBlock::GetTextBounds() const
{
    return GetContentRectangle().Deflate(Margin);
}
