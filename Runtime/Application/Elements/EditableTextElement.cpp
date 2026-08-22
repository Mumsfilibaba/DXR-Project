#include "Application/Elements/EditableTextElement.h"
#include "Application/Application.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Input/Keys.h"
#include "Core/Math/Math.h"
#include "Core/Platform/PlatformSystemClipboard.h"
#include "Core/Platform/PlatformTime.h"

/** @brief The share of a blink the text cursor is drawn for, which is ImGui's 0.8s out of 1.2s. */
static constexpr double GTextCursorVisibleFraction = 2.0 / 3.0;

TSharedPtr<FEditableTextElement> FEditableTextElement::Create(const FInitializer& Initializer)
{
    TSharedPtr<FEditableTextElement> NewElement = MakeSharedPtr<FEditableTextElement>();
    if (NewElement)
    {
        NewElement->Initialize(Initializer);
    }

    return NewElement;
}

FEditableTextElement::FEditableTextElement()
    : FVisualElement()
    , Text()
    , HintText()
    , Font(nullptr)
    , ForegroundColor(FFloatColor::White)
    , HintColor(0.5f, 0.5f, 0.5f, 1.0f)
    , TextCursorColor(FFloatColor::White)
    , SelectionColor(0.26f, 0.59f, 0.98f, 0.35f)
    , Padding(4, 2, 4, 2)
    , OnTextChanged()
    , OnTextCommitted()
    , OnKeyDownInterceptor()
    , TextCursorBlinkPeriod(1.2f)
    , TextCursorBlinkResetCounter(FPlatformTime::QueryPerformanceCounter())
    , TextCursorPosition(0)
    , SelectionAnchor(0)
    , bHasKeyboardFocus(false)
    , bIsSelectingWithMouse(false)
{
}

FEditableTextElement::~FEditableTextElement() = default;

void FEditableTextElement::Initialize(const FInitializer& Initializer)
{
    Text                  = Initializer.Text;
    HintText              = Initializer.HintText;
    Font                  = Initializer.Font;
    ForegroundColor       = Initializer.ForegroundColor;
    HintColor             = Initializer.HintColor;
    TextCursorColor       = Initializer.TextCursorColor;
    SelectionColor        = Initializer.SelectionColor;
    Padding               = Initializer.Padding;
    TextCursorBlinkPeriod = Math::Max(0.0f, Initializer.TextCursorBlinkPeriod);
    TextCursorPosition    = Text.Length();
    SelectionAnchor       = TextCursorPosition;

    ResetTextCursorBlink();

    // An editable line takes keyboard focus whenever its window becomes active
    SetActivationPolicy(EElementActivationPolicy::AutoFocusOnWindowActivate);
}

IntVector2 FEditableTextElement::ComputeDesiredSize() const
{
    if (!Font)
    {
        return IntVector2(Padding.GetTotalHorizontal(), Padding.GetTotalVertical());
    }

    // The hint is measured too, so an empty line does not collapse narrower than its placeholder
    const int32 TextWidth = Font->MeasureWidth(StringView(Text.Data(), Text.Length()));
    const int32 HintWidth = Font->MeasureWidth(StringView(HintText.Data(), HintText.Length()));

    return IntVector2(Math::Max(TextWidth, HintWidth) + Padding.GetTotalHorizontal(), GetTextBandHeight() + Padding.GetTotalVertical());
}

int32 FEditableTextElement::GetTextBandHeight() const
{
    return Font ? Font->GetTextBandHeight() : 0;
}

int32 FEditableTextElement::GetTextBandTop(const FRectangle& TextBounds) const
{
    return Font ? TextBounds.Position.Y + Font->GetTextBandOffset(TextBounds.Height) : TextBounds.Position.Y;
}

int32 FEditableTextElement::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FRectangle TextBounds = AllottedGeometry.Bounds.Deflate(Padding);

    if (HasSelection() && Font)
    {
        const int32 StartOffset = Font->MeasureWidth(StringView(Text.Data(), GetSelectionStart()));
        const int32 EndOffset   = Font->MeasureWidth(StringView(Text.Data(), GetSelectionEnd()));

        FRectangle SelectionBounds;
        SelectionBounds.Position.X = TextBounds.Position.X + StartOffset;
        SelectionBounds.Position.Y = GetTextBandTop(TextBounds);
        SelectionBounds.Width      = EndOffset - StartOffset;
        SelectionBounds.Height     = GetTextBandHeight();

        OutCommandList.AddBox(LayerId, SelectionBounds, SelectionColor);
    }

    if (Text.IsEmpty())
    {
        if (!HintText.IsEmpty())
        {
            OutCommandList.AddText(LayerId, TextBounds, HintText, Font.Get(), HintColor);
        }
    }
    else
    {
        OutCommandList.AddText(LayerId, TextBounds, Text, Font.Get(), ForegroundColor);
    }

    if (bHasKeyboardFocus && Font)
    {
        if (IsTextCursorVisibleAt(GetSecondsSinceTextCursorBlinkReset()))
        {
            const StringView TextBeforeCursor(Text.Data(), TextCursorPosition);

            FRectangle TextCursorBounds;
            TextCursorBounds.Position.X = TextBounds.Position.X + Font->MeasureWidth(TextBeforeCursor);
            TextCursorBounds.Position.Y = TextBounds.Position.Y + Font->GetTextCursorOffset(TextBounds.Height);
            TextCursorBounds.Width      = 1;
            TextCursorBounds.Height     = Font->GetTextCursorHeight(TextBounds.Height);

            OutCommandList.AddLine(LayerId + 1, TextCursorBounds, TextCursorColor);
        }

        // The layer is claimed whether or not the blink is on, so the sequence does not shift under
        // whatever is drawn after this element every time the text cursor goes dark.
        return LayerId + 1;
    }

    return LayerId;
}

bool FEditableTextElement::IsTextCursorVisibleAt(double ElapsedSeconds) const
{
    if (TextCursorBlinkPeriod <= 0.0f || ElapsedSeconds <= 0.0)
    {
        return true;
    }

    const double BlinkPeriod = static_cast<double>(TextCursorBlinkPeriod);
    return Math::FMod(ElapsedSeconds, BlinkPeriod) < (BlinkPeriod * GTextCursorVisibleFraction);
}

void FEditableTextElement::ResetTextCursorBlink()
{
    TextCursorBlinkResetCounter = FPlatformTime::QueryPerformanceCounter();
}

double FEditableTextElement::GetSecondsSinceTextCursorBlinkReset() const
{
    const uint64 Frequency = FPlatformTime::QueryPerformanceFrequency();
    if (Frequency == 0)
    {
        return 0.0;
    }

    const uint64 Now = FPlatformTime::QueryPerformanceCounter();
    return static_cast<double>(Now - TextCursorBlinkResetCounter) / static_cast<double>(Frequency);
}

FEventResponse FEditableTextElement::OnKeyChar(const FKeyEvent& KeyEvent)
{
    const CHAR Character = KeyEvent.GetAnsiChar();

    // Control characters arrive as key-down events instead, so they are dropped here
    if (Character < ' ' || Character == 0x7F)
    {
        return FEventResponse::Unhandled();
    }

    if (KeyEvent.GetModifierKeys().IsShortcutChordDown())
    {
        return FEventResponse::Unhandled();
    }

    InsertCharacter(Character);
    return FEventResponse::Handled();
}

FEventResponse FEditableTextElement::OnKeyDown(const FKeyEvent& KeyEvent)
{
    if (!KeyEvent.IsDown())
    {
        return FEventResponse::Unhandled();
    }

    if (OnKeyDownInterceptor.IsBound())
    {
        if (OnKeyDownInterceptor.Execute(KeyEvent) == EKeyInterceptResult::Handled)
        {
            return FEventResponse::Handled();
        }
    }

    const FKey               Key       = KeyEvent.GetKey();
    const FModifierKeyState& Modifiers = KeyEvent.GetModifierKeys();

    const bool bExtendSelection = Modifiers.IsShiftDown();
    const bool bIsLineJump      = Modifiers.IsSuperDown();
    const bool bIsWordJump      = !bIsLineJump && (Modifiers.IsAltDown() || Modifiers.IsCtrlDown());

    // The arrows resolve before the chords below, which would otherwise swallow Ctrl and Left together
    if (Key == Keys::Left)
    {
        if (bIsLineJump)
        {
            MoveTextCursorToStart(bExtendSelection);
        }
        else
        {
            const bool  bStartsFromSelection = !bExtendSelection && HasSelection();
            const int32 Origin               = bStartsFromSelection ? GetSelectionStart() : TextCursorPosition;

            if (bIsWordJump)
            {
                MoveTextCursor(FindWordBoundaryLeft(Origin), bExtendSelection);
            }
            else if (bStartsFromSelection)
            {
                MoveTextCursor(Origin, false);
            }
            else
            {
                MoveTextCursorLeft(bExtendSelection);
            }
        }

        return FEventResponse::Handled();
    }

    if (Key == Keys::Right)
    {
        if (bIsLineJump)
        {
            MoveTextCursorToEnd(bExtendSelection);
        }
        else
        {
            const bool  bStartsFromSelection = !bExtendSelection && HasSelection();
            const int32 Origin               = bStartsFromSelection ? GetSelectionEnd() : TextCursorPosition;

            if (bIsWordJump)
            {
                MoveTextCursor(FindWordBoundaryRight(Origin), bExtendSelection);
            }
            else if (bStartsFromSelection)
            {
                MoveTextCursor(Origin, false);
            }
            else
            {
                MoveTextCursorRight(bExtendSelection);
            }
        }

        return FEventResponse::Handled();
    }

    if (Modifiers.IsCommandDown())
    {
        if (Key == Keys::A)
        {
            SelectAll();
            return FEventResponse::Handled();
        }

        if (Key == Keys::C)
        {
            CopyToClipboard();
            return FEventResponse::Handled();
        }

        if (Key == Keys::X)
        {
            CutToClipboard();
            return FEventResponse::Handled();
        }

        if (Key == Keys::V)
        {
            PasteFromClipboard();
            return FEventResponse::Handled();
        }
    }

    if (Key == Keys::Home)
    {
        MoveTextCursorToStart(bExtendSelection);
        return FEventResponse::Handled();
    }

    if (Key == Keys::End)
    {
        MoveTextCursorToEnd(bExtendSelection);
        return FEventResponse::Handled();
    }

    if (Key == Keys::Backspace)
    {
        if (!DeleteSelection())
        {
            if (bIsWordJump)
            {
                const int32 WordStart = FindWordBoundaryLeft(TextCursorPosition);
                ReplaceRange(WordStart, TextCursorPosition - WordStart, StringView());
            }
            else
            {
                DeleteBackward();
            }
        }

        return FEventResponse::Handled();
    }

    if (Key == Keys::Delete)
    {
        if (!DeleteSelection())
        {
            if (bIsWordJump)
            {
                ReplaceRange(TextCursorPosition, FindWordBoundaryRight(TextCursorPosition) - TextCursorPosition, StringView());
            }
            else
            {
                DeleteForward();
            }
        }

        return FEventResponse::Handled();
    }

    if (Modifiers.IsShortcutChordDown())
    {
        return FEventResponse::Unhandled();
    }

    if (Key == Keys::Enter)
    {
        OnTextCommitted.ExecuteIfBound(Text);
        return FEventResponse::Handled();
    }

    return FEventResponse::Unhandled();
}

FEventResponse FEditableTextElement::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    if (CursorEvent.GetKey() != Keys::MouseButtonLeft || !Font)
    {
        return FEventResponse::Unhandled();
    }

    if (!GetContentRectangle().EncapsulatesPoint(CursorEvent.GetClientPosition()))
    {
        return FEventResponse::Unhandled();
    }

    MoveTextCursor(FindTextCursorPositionAt(CursorEvent.GetClientPosition()), CursorEvent.GetModifierKeys().IsShiftDown());

    bIsSelectingWithMouse = true;

    if (FApplication::IsInitialized())
    {
        FApplication::Get().CaptureMouse(AsSharedPtr());
    }

    return FEventResponse::Handled();
}

FEventResponse FEditableTextElement::OnMouseButtonUp(const FCursorEvent& CursorEvent)
{
    if (CursorEvent.GetKey() != Keys::MouseButtonLeft || !bIsSelectingWithMouse)
    {
        return FEventResponse::Unhandled();
    }

    bIsSelectingWithMouse = false;

    if (FApplication::IsInitialized())
    {
        FApplication::Get().ReleaseMouseCapture(AsSharedPtr());
    }

    return FEventResponse::Handled();
}

FEventResponse FEditableTextElement::OnMouseMove(const FCursorEvent& CursorEvent)
{
    if (!bIsSelectingWithMouse || !Font)
    {
        return FEventResponse::Unhandled();
    }

    MoveTextCursor(FindTextCursorPositionAt(CursorEvent.GetClientPosition()), true);
    return FEventResponse::Handled();
}

FEventResponse FEditableTextElement::OnFocusGained()
{
    bHasKeyboardFocus = true;

    ResetTextCursorBlink();
    return FEventResponse::Handled();
}

FEventResponse FEditableTextElement::OnFocusLost()
{
    bHasKeyboardFocus     = false;
    bIsSelectingWithMouse = false;

    ClearSelection();
    return FEventResponse::Handled();
}

bool FEditableTextElement::GetCursor(ECursor& OutCursor) const
{
    OutCursor = ECursor::TextInput;
    return true;
}

bool FEditableTextElement::SupportsKeyboardFocus() const
{
    return true;
}

void FEditableTextElement::SetText(const String& InText)
{
    Text               = InText;
    TextCursorPosition = Math::Clamp(TextCursorPosition, 0, Text.Length());

    ClearSelection();
    NotifyTextChanged();
}

void FEditableTextElement::SetTextSilently(const String& InText)
{
    Text               = InText;
    TextCursorPosition = Text.Length();

    ClearSelection();
    ResetTextCursorBlink();
}

void FEditableTextElement::ClearText()
{
    Text.Clear();
    
    TextCursorPosition = 0;

    ClearSelection();
    NotifyTextChanged();
}

void FEditableTextElement::InsertCharacter(CHAR Character)
{
    if (HasSelection())
    {
        const int32 SelectionStart = GetSelectionStart();
        ReplaceRange(SelectionStart, GetSelectionEnd() - SelectionStart, StringView(&Character, 1));
        return;
    }

    Text.Insert(Character, TextCursorPosition);
    TextCursorPosition++;

    ClearSelection();
    NotifyTextChanged();
}

void FEditableTextElement::InsertText(const StringView& InText)
{
    if (InText.IsEmpty())
    {
        return;
    }

    if (HasSelection())
    {
        const int32 SelectionStart = GetSelectionStart();
        ReplaceRange(SelectionStart, GetSelectionEnd() - SelectionStart, InText);
        return;
    }

    Text.Insert(InText.Data(), InText.Length(), TextCursorPosition);
    TextCursorPosition += InText.Length();

    ClearSelection();
    NotifyTextChanged();
}

void FEditableTextElement::ReplaceRange(int32 Position, int32 Count, const StringView& InText)
{
    const int32 ClampedPosition = Math::Clamp(Position, 0, Text.Length());
    const int32 ClampedCount    = Math::Clamp(Count, 0, Text.Length() - ClampedPosition);

    if (ClampedCount > 0)
    {
        Text.Remove(ClampedPosition, ClampedCount);
    }

    if (!InText.IsEmpty())
    {
        Text.Insert(InText.Data(), InText.Length(), ClampedPosition);
    }

    TextCursorPosition = ClampedPosition + InText.Length();

    ClearSelection();
    NotifyTextChanged();
}

bool FEditableTextElement::DeleteBackward()
{
    if (TextCursorPosition <= 0)
    {
        return false;
    }

    Text.Remove(TextCursorPosition - 1, 1);
    TextCursorPosition--;

    ClearSelection();
    NotifyTextChanged();
    return true;
}

bool FEditableTextElement::DeleteForward()
{
    if (TextCursorPosition >= Text.Length())
    {
        return false;
    }

    Text.Remove(TextCursorPosition, 1);

    ClearSelection();
    NotifyTextChanged();
    return true;
}

void FEditableTextElement::SetTextCursorPosition(int32 InTextCursorPosition)
{
    MoveTextCursor(InTextCursorPosition, false);
}

void FEditableTextElement::MoveTextCursorLeft(bool bExtendSelection)
{
    MoveTextCursor(TextCursorPosition - 1, bExtendSelection);
}

void FEditableTextElement::MoveTextCursorRight(bool bExtendSelection)
{
    MoveTextCursor(TextCursorPosition + 1, bExtendSelection);
}

void FEditableTextElement::MoveTextCursorToStart(bool bExtendSelection)
{
    MoveTextCursor(0, bExtendSelection);
}

void FEditableTextElement::MoveTextCursorToEnd(bool bExtendSelection)
{
    MoveTextCursor(Text.Length(), bExtendSelection);
}

void FEditableTextElement::MoveTextCursorWordLeft(bool bExtendSelection)
{
    MoveTextCursor(FindWordBoundaryLeft(TextCursorPosition), bExtendSelection);
}

void FEditableTextElement::MoveTextCursorWordRight(bool bExtendSelection)
{
    MoveTextCursor(FindWordBoundaryRight(TextCursorPosition), bExtendSelection);
}

bool FEditableTextElement::IsWordSeparator(CHAR Character)
{
    return Character == ' ' || Character == '\t' || Character == ',' || Character == ';';
}

int32 FEditableTextElement::FindWordBoundaryLeft(int32 From) const
{
    int32 Position = Math::Clamp(From, 0, Text.Length());

    while (Position > 0 && IsWordSeparator(Text.Data()[Position - 1]))
    {
        Position--;
    }

    while (Position > 0 && !IsWordSeparator(Text.Data()[Position - 1]))
    {
        Position--;
    }

    return Position;
}

int32 FEditableTextElement::FindWordBoundaryRight(int32 From) const
{
    const int32 Length   = Text.Length();
    int32       Position = Math::Clamp(From, 0, Length);

    while (Position < Length && IsWordSeparator(Text.Data()[Position]))
    {
        Position++;
    }

    while (Position < Length && !IsWordSeparator(Text.Data()[Position]))
    {
        Position++;
    }

    return Position;
}

bool FEditableTextElement::HasSelection() const
{
    return SelectionAnchor != TextCursorPosition;
}

int32 FEditableTextElement::GetSelectionStart() const
{
    return Math::Min(SelectionAnchor, TextCursorPosition);
}

int32 FEditableTextElement::GetSelectionEnd() const
{
    return Math::Max(SelectionAnchor, TextCursorPosition);
}

String FEditableTextElement::GetSelectedText() const
{
    if (!HasSelection())
    {
        return String();
    }

    return String(Text.Data() + GetSelectionStart(), GetSelectionEnd() - GetSelectionStart());
}

void FEditableTextElement::SelectAll()
{
    SelectionAnchor    = 0;
    TextCursorPosition = Text.Length();

    ResetTextCursorBlink();
}

void FEditableTextElement::ClearSelection()
{
    SelectionAnchor = TextCursorPosition;
}

bool FEditableTextElement::DeleteSelection()
{
    if (!HasSelection())
    {
        return false;
    }

    const int32 SelectionStart = GetSelectionStart();
    ReplaceRange(SelectionStart, GetSelectionEnd() - SelectionStart, StringView());
    return true;
}

void FEditableTextElement::CopyToClipboard() const
{
    FPlatformSystemClipboard::SetText(HasSelection() ? GetSelectedText() : Text);
}

void FEditableTextElement::CutToClipboard()
{
    CopyToClipboard();
    DeleteSelection();
}

void FEditableTextElement::PasteFromClipboard()
{
    String ClipboardText;
    FPlatformSystemClipboard::GetText(ClipboardText);

    if (!ClipboardText.IsEmpty())
    {
        InsertText(StringView(ClipboardText.Data(), ClipboardText.Length()));
    }
}

void FEditableTextElement::SetFont(const TSharedPtr<IFontFace>& InFont)
{
    Font = InFont;
}

void FEditableTextElement::NotifyTextChanged()
{
    ResetTextCursorBlink();
    OnTextChanged.ExecuteIfBound(Text);
}

void FEditableTextElement::MoveTextCursor(int32 NewTextCursorPosition, bool bExtendSelection)
{
    TextCursorPosition = Math::Clamp(NewTextCursorPosition, 0, Text.Length());

    if (!bExtendSelection)
    {
        ClearSelection();
    }

    ResetTextCursorBlink();
}

int32 FEditableTextElement::FindTextCursorPositionAt(const IntVector2& ClientPosition) const
{
    const FRectangle TextBounds = GetContentRectangle().Deflate(Padding);
    const int32      OffsetX    = ClientPosition.X - TextBounds.Position.X;

    return Font->FindCharacterIndexAtOffset(StringView(Text.Data(), Text.Length()), OffsetX);
}
