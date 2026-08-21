#include "Application/Elements/EditableTextElement.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Input/Keys.h"
#include "Core/Math/Math.h"
#include "Core/Platform/PlatformSystemClipboard.h"

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
    , Padding(4, 2, 4, 2)
    , OnTextChanged()
    , OnTextCommitted()
    , OnKeyDownInterceptor()
    , TextCursorPosition(0)
    , bHasKeyboardFocus(false)
{
}

FEditableTextElement::~FEditableTextElement() = default;

void FEditableTextElement::Initialize(const FInitializer& Initializer)
{
    Text               = Initializer.Text;
    HintText           = Initializer.HintText;
    Font               = Initializer.Font;
    ForegroundColor    = Initializer.ForegroundColor;
    HintColor          = Initializer.HintColor;
    TextCursorColor    = Initializer.TextCursorColor;
    Padding            = Initializer.Padding;
    TextCursorPosition = Text.Length();

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

    return IntVector2(Math::Max(TextWidth, HintWidth) + Padding.GetTotalHorizontal(), Font->GetLineHeight() + Padding.GetTotalVertical());
}

int32 FEditableTextElement::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FRectangle TextBounds = AllottedGeometry.Bounds.Deflate(Padding);

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
        const StringView TextBeforeCursor(Text.Data(), TextCursorPosition);

        FRectangle TextCursorBounds;
        TextCursorBounds.Position.X = TextBounds.Position.X + Font->MeasureWidth(TextBeforeCursor);
        TextCursorBounds.Position.Y = TextBounds.Position.Y;
        TextCursorBounds.Width      = 1;
        TextCursorBounds.Height     = Font->GetLineHeight();

        OutCommandList.AddLine(LayerId + 1, TextCursorBounds, TextCursorColor);
        return LayerId + 1;
    }

    return LayerId;
}

FEventResponse FEditableTextElement::OnKeyChar(const FKeyEvent& KeyEvent)
{
    const CHAR Character = KeyEvent.GetAnsiChar();

    // Control characters arrive as key-down events instead, so they are dropped here
    if (Character < ' ' || Character == 0x7F)
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

    // Cmd on macOS and Ctrl on Windows both drive copy and paste
    const bool bIsCommandDown = Modifiers.IsCtrlDown() || Modifiers.IsSuperDown();
    if (bIsCommandDown)
    {
        if (Key == Keys::C)
        {
            CopyToClipboard();
            return FEventResponse::Handled();
        }

        if (Key == Keys::V)
        {
            PasteFromClipboard();
            return FEventResponse::Handled();
        }

        return FEventResponse::Unhandled();
    }

    if (Key == Keys::Left)
    {
        MoveTextCursorLeft();
        return FEventResponse::Handled();
    }

    if (Key == Keys::Right)
    {
        MoveTextCursorRight();
        return FEventResponse::Handled();
    }

    if (Key == Keys::Home)
    {
        MoveTextCursorToStart();
        return FEventResponse::Handled();
    }

    if (Key == Keys::End)
    {
        MoveTextCursorToEnd();
        return FEventResponse::Handled();
    }

    if (Key == Keys::Backspace)
    {
        DeleteBackward();
        return FEventResponse::Handled();
    }

    if (Key == Keys::Delete)
    {
        DeleteForward();
        return FEventResponse::Handled();
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

    const FRectangle TextBounds = GetContentRectangle().Deflate(Padding);
    const int32      OffsetX    = CursorEvent.GetCursorPos().X - TextBounds.Position.X;

    TextCursorPosition = Font->FindCharacterIndexAtOffset(StringView(Text.Data(), Text.Length()), OffsetX);
    return FEventResponse::Handled();
}

FEventResponse FEditableTextElement::OnFocusGained()
{
    bHasKeyboardFocus = true;
    return FEventResponse::Handled();
}

FEventResponse FEditableTextElement::OnFocusLost()
{
    bHasKeyboardFocus = false;
    return FEventResponse::Handled();
}

void FEditableTextElement::SetText(const String& InText)
{
    Text               = InText;
    TextCursorPosition = Math::Clamp(TextCursorPosition, 0, Text.Length());
    NotifyTextChanged();
}

void FEditableTextElement::SetTextSilently(const String& InText)
{
    Text               = InText;
    TextCursorPosition = Text.Length();
}

void FEditableTextElement::ClearText()
{
    Text.Clear();
    TextCursorPosition = 0;
    NotifyTextChanged();
}

void FEditableTextElement::InsertCharacter(CHAR Character)
{
    Text.Insert(Character, TextCursorPosition);
    TextCursorPosition++;
    NotifyTextChanged();
}

void FEditableTextElement::InsertText(const StringView& InText)
{
    if (InText.IsEmpty())
    {
        return;
    }

    Text.Insert(InText.Data(), InText.Length(), TextCursorPosition);
    TextCursorPosition += InText.Length();
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
    NotifyTextChanged();
    return true;
}

void FEditableTextElement::SetTextCursorPosition(int32 InTextCursorPosition)
{
    TextCursorPosition = Math::Clamp(InTextCursorPosition, 0, Text.Length());
}

void FEditableTextElement::MoveTextCursorLeft()
{
    TextCursorPosition = Math::Max(0, TextCursorPosition - 1);
}

void FEditableTextElement::MoveTextCursorRight()
{
    TextCursorPosition = Math::Min(Text.Length(), TextCursorPosition + 1);
}

void FEditableTextElement::MoveTextCursorToStart()
{
    TextCursorPosition = 0;
}

void FEditableTextElement::MoveTextCursorToEnd()
{
    TextCursorPosition = Text.Length();
}

void FEditableTextElement::CopyToClipboard() const
{
    FPlatformSystemClipboard::SetText(Text);
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
    OnTextChanged.ExecuteIfBound(Text);
}
