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
    , MinValue(0.0f)
    , MaxValue(0.0f)
    , Value(0.0f)
    , ScrubSpeed(0.1f)
    , StepSize(0.0f)
    , ScrubStartValue(0.0f)
    , ScrubStartPosition()
    , Precision(2)
    , Prefix()
    , CornerRadius()
    , MinWidth(0)
    , MinHeight(0)
    , bIsTyping(false)
    , bHasScrubbed(false)
    , OnValueChangedDelegate()
    , OnValueCommittedDelegate()
{
}

FSpinBox::~FSpinBox() = default;

void FSpinBox::Initialize(const FDesc& Desc)
{
    MinValue                 = Desc.MinValue;
    MaxValue                 = Math::Max(Desc.MaxValue, Desc.MinValue);
    ScrubSpeed               = Desc.ScrubSpeed;
    StepSize                 = Math::Max(Desc.StepSize, 0.0f);
    Precision                = Math::Clamp(Desc.Precision, 0, 9);
    Prefix                   = Desc.Prefix;
    CornerRadius             = Desc.CornerRadius;
    MinWidth                 = Desc.MinWidth;
    MinHeight                = Desc.MinHeight;
    OnValueChangedDelegate   = Desc.OnValueChanged;
    OnValueCommittedDelegate = Desc.OnValueCommitted;

    Value = SanitizeValue(Desc.Value);

    SetPadding(Desc.Padding);

    FTextBlock::FDesc LabelDesc;
    LabelDesc.Font            = Desc.Font;
    LabelDesc.ColorAndOpacity = FUIStyle::GetDefault().Colors.Text;

    Label = FTextBlock::Create(LabelDesc);
    UpdateLabel();

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
    Content->Tick(FRectangle::AlignInBounds(Available, Content->GetCachedDesiredSize(), bIsTyping ? EHorizontalAlignment::Fill : EHorizontalAlignment::Left, EVerticalAlignment::Center));
}

int32 FSpinBox::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle&         Style = FUIStyle::GetDefault();
    const EInteractionState State = GetInteractionState();

    OutCommandList.AddBox(LayerId, AllottedGeometry.Bounds, Style.Colors.PanelBackground, CornerRadius);

    if (MinValue > -TNumericLimits<float>::Max() && MaxValue < TNumericLimits<float>::Max() && MaxValue > MinValue)
    {
        FRectangle Filled = AllottedGeometry.Bounds;
        Filled.Width      = Math::RoundToInt(Math::Saturate((Value - MinValue) / (MaxValue - MinValue)) * static_cast<float>(AllottedGeometry.Bounds.Width));

        if (Filled.Width > 0)
        {
            OutCommandList.AddBox(LayerId, Filled, Style.Colors.Accent, CornerRadius);
        }
    }

    OutCommandList.AddBoxOutline(LayerId, AllottedGeometry.Bounds, IsHovered() ? Style.Colors.Accent : Style.Colors.Border, Style.Metrics.BorderThickness, CornerRadius);

    if (Label)
    {
        Label->SetColorAndOpacity(Style.GetTextColor(State));
    }

    return FCompoundElement::OnDraw(AllottedGeometry, OutCommandList, LayerId);
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
        ScrubStartPosition = CursorEvent.GetClientPosition();
        ScrubStartValue    = Value;
        bHasScrubbed       = false;
    }

    return Response;
}

FEventResponse FSpinBox::OnMouseButtonUp(const FCursorEvent& CursorEvent)
{
    const bool bWasScrubbing = IsPressed() && bHasScrubbed;

    const FEventResponse Response = FInteractiveElement::OnMouseButtonUp(CursorEvent);
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
    Value = SanitizeValue(InValue);
    UpdateLabel();
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
    return String::Printf("%.*f", Precision, Value);
}

float FSpinBox::SanitizeValue(float InValue) const
{
    float Result = Math::Clamp(InValue, MinValue, MaxValue);

    if (StepSize > 0.0f)
    {
        const float Steps = Math::Round(Result / StepSize);
        Result            = Math::Clamp(Steps * StepSize, MinValue, MaxValue);
    }

    return Result;
}

void FSpinBox::ApplyValue(float InValue)
{
    const float NewValue = SanitizeValue(InValue);
    if (NewValue == Value)
    {
        return;
    }

    Value = NewValue;
    UpdateLabel();
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
    if (!bHasScrubbed)
    {
        BeginTyping();
    }
}

void FSpinBox::OnDragged(const FCursorEvent& CursorEvent)
{
    const int32 Travel = CursorEvent.GetClientPosition().X - ScrubStartPosition.X;
    if (!bHasScrubbed && Math::Abs(Travel) < ScrubThreshold)
    {
        return;
    }

    bHasScrubbed = true;
    ApplyValue(ScrubStartValue + (static_cast<float>(Travel) * ScrubSpeed));
}
