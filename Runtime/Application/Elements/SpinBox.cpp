#include "Application/Elements/SpinBox.h"
#include "Application/Elements/EditableText.h"
#include "Application/Elements/TextBlock.h"
#include "Application/Application.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/Style/UIStyle.h"
#include "Core/Math/Math.h"
#include "Core/Platform/PlatformString.h"

TSharedPtr<FSpinBox> FSpinBox::Create(const FDesc& Desc)
{
    TSharedPtr<FSpinBox> NewSpinBox = MakeSharedPtr<FSpinBox>();
    NewSpinBox->Initialize(Desc);
    return NewSpinBox;
}

FSpinBox::FSpinBox()
    : FInteractiveElement()
    , Label(nullptr)
    , Editor(nullptr)
    , Range()
    , Scrubber()
    , Value(0.0f)
    , ScrubSpeed(0.1f)
    , Prefix()
    , CornerRadius()
    , MinWidth(0)
    , MinHeight(0)
    , bIsTyping(false)
    , OnValueChangedDelegate()
    , OnValueCommittedDelegate()
{
}

FSpinBox::~FSpinBox() = default;

void FSpinBox::Initialize(const FDesc& Desc)
{
    Range.Min                = Desc.MinValue;
    Range.Max                = Math::Max(Desc.MaxValue, Desc.MinValue);
    Range.Step               = Math::Max(Desc.StepSize, 0.0f);
    Range.Precision          = Math::Clamp(Desc.Precision, 0, 9);
    ScrubSpeed               = Desc.ScrubSpeed;
    Prefix                   = Desc.Prefix;
    CornerRadius             = Desc.CornerRadius;
    MinWidth                 = Desc.MinWidth;
    MinHeight                = Desc.MinHeight;
    OnValueChangedDelegate   = Desc.OnValueChanged;
    OnValueCommittedDelegate = Desc.OnValueCommitted;

    Value = Range.Sanitize(Desc.Value);

    SetPadding(Desc.Padding);

    FTextBlock::FDesc LabelDesc;
    LabelDesc.Font            = Desc.Font;
    LabelDesc.ColorAndOpacity = FUIStyle::GetDefault().Colors.Text;

    Label = FTextBlock::Create(LabelDesc);
    UpdateLabel();
    ApplyInteractionTextColor(Label.Get());

    FEditableText::FDesc EditorDesc;
    EditorDesc.Font            = Desc.Font;
    EditorDesc.ForegroundColor = FUIStyle::GetDefault().Colors.Text;
    EditorDesc.Padding         = FMargin();

    Editor = FEditableText::Create(EditorDesc);
    Editor->GetOnTextCommitted().BindLambda([this](const String& InText)
    {
        HandleTextCommitted(InText);
    });

    SetContent(Label);
}

IntVector2 FSpinBox::ComputeDesiredSize() const
{
    IntVector2 DesiredSize = FCompoundElement::ComputeDesiredSize();
    DesiredSize.X          = Math::Max(DesiredSize.X, MinWidth);
    DesiredSize.Y          = Math::Max(DesiredSize.Y, MinHeight);
    return DesiredSize;
}

void FSpinBox::OnArrange(const FRectangle& AllottedBounds)
{
    if (bIsTyping && Editor && !Editor->HasKeyboardFocus())
    {
        EndTyping(true);
    }

    if (!Content)
    {
        return;
    }

    const FRectangle Available = AllottedBounds.Deflate(GetPadding());
    Content->Arrange(FLayout::AlignInBounds(Available, Content->GetCachedDesiredSize(), bIsTyping ? EHorizontalAlignment::Fill : EHorizontalAlignment::Left, EVerticalAlignment::Center));
}

int32 FSpinBox::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle& Style = FUIStyle::GetDefault();

    OutCommandList.AddBox(LayerId, AllottedGeometry.Bounds, Style.Colors.ControlNormal, CornerRadius);

    if (Range.IsBounded())
    {
        FRectangle Filled = AllottedGeometry.Bounds;
        Filled.Width      = Math::RoundToInt(Range.GetFraction(Value) * static_cast<float>(AllottedGeometry.Bounds.Width));

        if (Filled.Width > 0)
        {
            OutCommandList.AddBox(LayerId, Filled, Style.Colors.Accent, CornerRadius);
        }
    }

    OutCommandList.AddBoxOutline(LayerId, AllottedGeometry.Bounds, IsHovered() ? Style.Colors.Accent : Style.Colors.Border, Style.Metrics.BorderThickness, CornerRadius);

    return FCompoundElement::OnDraw(AllottedGeometry, OutCommandList, LayerId);
}

void FSpinBox::OnInteractionStateChanged()
{
    FInteractiveElement::OnInteractionStateChanged();

    ApplyInteractionTextColor(Label.Get());
}

FEventResponse FSpinBox::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    if (bIsTyping)
    {
        return FEventResponse::Unhandled();
    }

    const FEventResponse Response = FInteractiveElement::OnMouseButtonDown(CursorEvent);
    if (IsPressed())
    {
        Scrubber.Begin(CursorEvent.GetClientPosition(), Value);
    }

    return Response;
}

FEventResponse FSpinBox::OnMouseButtonUp(const FCursorEvent& CursorEvent)
{
    const bool bWasScrubbing = IsPressed() && Scrubber.HasScrubbed();

    const FEventResponse Response = FInteractiveElement::OnMouseButtonUp(CursorEvent);
    Scrubber.End();

    if (bWasScrubbing)
    {
        OnValueCommittedDelegate.ExecuteIfBound(Value);
    }

    return Response;
}

bool FSpinBox::GetCursor(ECursor& OutCursor) const
{
    if (bIsTyping || !IsEnabled())
    {
        return false;
    }

    OutCursor = ECursor::ResizeEW;
    return true;
}

TSharedPtr<FVisualElement> FSpinBox::GetFocusTarget()
{
    return bIsTyping && Editor ? StaticCastSharedPtr<FVisualElement>(Editor) : FInteractiveElement::GetFocusTarget();
}

void FSpinBox::SetValue(float InValue)
{
    Value = Range.Sanitize(InValue);
    UpdateLabel();

    InvalidatePaint();
}

void FSpinBox::BeginTyping()
{
    if (bIsTyping || !Editor || !IsEnabled())
    {
        return;
    }

    bIsTyping = true;

    Editor->SetTextSilently(GetFormattedValue());
    Editor->SelectAll();
    SetContent(Editor);

    if (FApplication::IsInitialized())
    {
        FApplication::Get().SetFocusElement(Editor);
    }
}

void FSpinBox::EndTyping(bool bCommit)
{
    if (!bIsTyping)
    {
        return;
    }

    bIsTyping = false;
    SetContent(Label);

    if (bCommit && Editor)
    {
        const String& Typed = Editor->GetText();
        if (!Typed.IsEmpty())
        {
            ApplyValue(FPlatformString::Atof(Typed.Data()));
        }
    }

    UpdateLabel();
    OnValueCommittedDelegate.ExecuteIfBound(Value);
}

String FSpinBox::GetFormattedValue() const
{
    return Range.Format(Value);
}

void FSpinBox::ApplyValue(float InValue)
{
    const float NewValue = Range.Sanitize(InValue);
    if (NewValue == Value)
    {
        return;
    }

    Value = NewValue;
    UpdateLabel();
    InvalidatePaint();

    OnValueChangedDelegate.ExecuteIfBound(Value);
}

void FSpinBox::UpdateLabel()
{
    if (Label)
    {
        Label->SetText(Prefix + GetFormattedValue());
    }
}

void FSpinBox::HandleTextCommitted(const String& InText)
{
    UNREFERENCED_VARIABLE(InText);
    EndTyping(true);
}

void FSpinBox::OnClicked()
{
    if (!Scrubber.HasScrubbed())
    {
        BeginTyping();
    }
}

void FSpinBox::OnDragged(const FCursorEvent& CursorEvent)
{
    int32 Travel = 0;
    if (Scrubber.Update(CursorEvent.GetClientPosition(), ScrubThreshold, Travel))
    {
        ApplyValue(Scrubber.GetStartValue() + (static_cast<float>(Travel) * ScrubSpeed));
    }
}
