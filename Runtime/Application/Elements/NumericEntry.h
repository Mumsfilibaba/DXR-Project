#pragma once
#include "Core/Containers/String.h"
#include "Core/Delegates/Delegate.h"
#include "Core/Math/Color.h"
#include "Core/Math/Math.h"
#include "Core/Platform/PlatformString.h"
#include "Core/Templates/NumericLimits.h"
#include "Application/Application.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/ElementPath.h"
#include "Application/Elements/EditableText.h"
#include "Application/Elements/VisualElement.h"
#include "Application/Input/Keys.h"
#include "Application/Style/UIStyle.h"
#include "Application/Text/IFontFace.h"

template<typename T>
struct TNumericEntryTraits;

template<>
struct TNumericEntryTraits<float>
{
    /**
     * @brief Writes the value out for the line of text it is edited in.
     *
     * @param Value The value to format.
     * @return The value at three decimal places.
     */
    NODISCARD static FORCEINLINE String Format(float Value)
    {
        return String::Printf("%.3f", Value);
    }

    /**
     * @brief Reads a value back out of what was typed.
     *
     * @param InText The text to parse.
     * @return The value the text names, or zero when it names none.
     */
    NODISCARD static FORCEINLINE float Parse(const String& InText)
    {
        return FPlatformString::Atof(InText.Data());
    }

    /** @return How far one pixel of drag moves the value, used when the description named no step. */
    NODISCARD static FORCEINLINE float GetDefaultStep()
    {
        return 0.01f;
    }
};

template<>
struct TNumericEntryTraits<int32>
{
    /**
     * @brief Writes the value out for the line of text it is edited in.
     *
     * @param Value The value to format.
     * @return The value in decimal.
     */
    NODISCARD static FORCEINLINE String Format(int32 Value)
    {
        return String::Printf("%d", Value);
    }

    /**
     * @brief Reads a value back out of what was typed.
     *
     * @param InText The text to parse.
     * @return The value the text names, or zero when it names none.
     */
    NODISCARD static FORCEINLINE int32 Parse(const String& InText)
    {
        return FPlatformString::Atoi(InText.Data());
    }

    /** @return How far one pixel of drag moves the value, used when the description named no step. */
    NODISCARD static FORCEINLINE int32 GetDefaultStep()
    {
        return 1;
    }
};

template<typename T>
class TNumericEntry final : public FVisualElement
{
public:
    using FOnValueChanged = TDelegate<void(T /*NewValue*/)>;

    struct FDesc
    {
        /** @brief Written in the coloured tag at the left, which is usually an axis name. */
        String Label;

        /** @brief The value the field starts at, clamped into the range. */
        T Value = T(0);

        /** @brief The lowest value the field can hold. */
        T MinValue = TNumericLimits<T>::Lowest();

        /** @brief The highest value the field can hold, raised to MinValue when it is set below it. */
        T MaxValue = TNumericLimits<T>::Max();

        /** @brief How far the value moves per pixel of horizontal drag. */
        T Step = TNumericEntryTraits<T>::GetDefaultStep();

        /** @brief The face the label and the value are measured and drawn with. */
        TSharedPtr<IFontFace> Font;

        /** @brief The width of the coloured tag at the left, in pixels. */
        int32 LabelWidth = 14;

        /** @brief Whether the coloured tag is drawn and given room at the left. */
        bool bShowLabel = true;

        /** @brief The fill of the coloured tag, which is what tells one axis from the next. */
        FFloatColor LabelColor = FFloatColor(0.62f, 0.20f, 0.22f, 1.0f);

        /** @brief Fired every time the value moves, which during a scrub is every frame the cursor moves. */
        FOnValueChanged OnValueChanged;
    };

public:

    /** @brief How far the cursor travels before a press counts as a scrub rather than a click. */
    static constexpr int32 ScrubThreshold = 3;

    /** @brief The narrowest the field asks to be, in pixels. */
    static constexpr int32 MinWidth = 64;

    /** @brief The space kept between the coloured tag and the value, in pixels. */
    static constexpr int32 TextInset = 4;

public:
    static TSharedPtr<TNumericEntry<T>> Create(const FDesc& Desc);

public:
    TNumericEntry();
    virtual ~TNumericEntry() = default;

    /**
     * @brief Initializes the numeric entry with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual void OnArrange(const FRectangle& AllottedBounds) override;
    virtual void GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const override;
    virtual void FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements) override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseButtonUp(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseMove(const FCursorEvent& CursorEvent) override;
    virtual bool GetCursor(ECursor& OutCursor) const override;

    /**
     * @brief Sets the value without firing the delegate, which is how a host pushes a model value in.
     *
     * @param InValue The value to move to, clamped to the range.
     */
    void SetValue(T InValue);

    /** @return The value the field holds, already clamped to the range. */
    NODISCARD FORCEINLINE T GetValue() const
    {
        return Value;
    }

    /** @return The line the value is typed into, which a caller can focus but does not own. */
    NODISCARD FORCEINLINE const TSharedPtr<FEditableText>& GetEditor() const
    {
        return Editor;
    }

    /** @return The delegate, which fires every time the value moves. */
    NODISCARD FORCEINLINE FOnValueChanged& GetOnValueChanged()
    {
        return OnValueChangedDelegate;
    }

    /** @return True while a press is being dragged, whether or not it has passed the scrub threshold. */
    NODISCARD FORCEINLINE bool IsScrubbing() const
    {
        return bIsScrubbing;
    }

    /**
     * @brief The coloured tag at the left.
     *
     * @param Bounds The rectangle the field was arranged into.
     * @return The tag, which is empty while the label is hidden.
     */
    NODISCARD FRectangle GetLabelRectangle(const FRectangle& Bounds) const;

    /**
     * @brief The space the value is drawn and edited in, which is what is left beside the tag.
     *
     * @param Bounds The rectangle the field was arranged into.
     * @return The rectangle handed to the line of text.
     */
    NODISCARD FRectangle GetEditorRectangle(const FRectangle& Bounds) const;

private:
    NODISCARD T SanitizeValue(T InValue) const;

    void ApplyValue(T InValue);
    void UpdateEditorText();
    void HandleTextCommitted(const String& InText);
    void BeginEditing(const IntVector2& ClientPosition);

    TSharedPtr<FEditableText> Editor;
    TSharedPtr<IFontFace>     Font;
    String                    Label;
    FFloatColor               LabelColor;
    T                         Value;
    T                         MinValue;
    T                         MaxValue;
    T                         Step;
    T                         ScrubStartValue;
    IntVector2                ScrubStartPosition;
    int32                     LabelWidth;
    bool                      bShowLabel;
    bool                      bIsScrubbing;
    bool                      bHasScrubbed;
    FOnValueChanged           OnValueChangedDelegate;
};

template<typename T>
TSharedPtr<TNumericEntry<T>> TNumericEntry<T>::Create(const FDesc& Desc)
{
    TSharedPtr<TNumericEntry<T>> NewNumericEntry = MakeSharedPtr<TNumericEntry<T>>();
    NewNumericEntry->Initialize(Desc);
    return NewNumericEntry;
}

template<typename T>
TNumericEntry<T>::TNumericEntry()
    : FVisualElement()
    , Editor(nullptr)
    , Font(nullptr)
    , Label()
    , LabelColor(0.62f, 0.20f, 0.22f, 1.0f)
    , Value(T(0))
    , MinValue(TNumericLimits<T>::Lowest())
    , MaxValue(TNumericLimits<T>::Max())
    , Step(TNumericEntryTraits<T>::GetDefaultStep())
    , ScrubStartValue(T(0))
    , ScrubStartPosition()
    , LabelWidth(14)
    , bShowLabel(true)
    , bIsScrubbing(false)
    , bHasScrubbed(false)
    , OnValueChangedDelegate()
{
}

template<typename T>
void TNumericEntry<T>::Initialize(const FDesc& Desc)
{
    Font                   = Desc.Font;
    Label                  = Desc.Label;
    LabelColor             = Desc.LabelColor;
    MinValue               = Desc.MinValue;
    MaxValue               = Math::Max(Desc.MaxValue, Desc.MinValue);
    Step                   = Desc.Step;
    LabelWidth             = Math::Max(0, Desc.LabelWidth);
    bShowLabel             = Desc.bShowLabel;
    OnValueChangedDelegate = Desc.OnValueChanged;

    Value = SanitizeValue(Desc.Value);

    FEditableText::FDesc EditorDesc;
    EditorDesc.Text            = TNumericEntryTraits<T>::Format(Value);
    EditorDesc.Font            = Desc.Font;
    EditorDesc.ForegroundColor = FUIStyle::GetDefault().Colors.Text;
    EditorDesc.Padding         = FMargin();

    Editor = FEditableText::Create(EditorDesc);
    Editor->SetParentElement(AsWeakPtr());
    Editor->SetActivationPolicy(EElementActivationPolicy::DoNotAutoFocusOnWindowActivate);
    Editor->GetOnTextCommitted().BindLambda([this](const String& InText)
    {
        HandleTextCommitted(InText);
    });
}

template<typename T>
IntVector2 TNumericEntry<T>::ComputeDesiredSize() const
{
    IntVector2 DesiredSize = Editor ? Editor->GetCachedDesiredSize() : IntVector2(0, 0);
    DesiredSize.X += TextInset * 2;
    DesiredSize.Y = Math::Max(DesiredSize.Y, FUIStyle::GetDefault().Metrics.RowHeight);

    if (bShowLabel)
    {
        DesiredSize.X += LabelWidth;
    }

    DesiredSize.X = Math::Max(DesiredSize.X, MinWidth);
    return DesiredSize;
}

template<typename T>
void TNumericEntry<T>::OnArrange(const FRectangle& AllottedBounds)
{
    if (Editor)
    {
        Editor->Tick(GetEditorRectangle(AllottedBounds));
    }
}

template<typename T>
void TNumericEntry<T>::GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const
{
    if (Editor)
    {
        OutChildren.Add(Editor);
    }
}

template<typename T>
void TNumericEntry<T>::FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements)
{
    FVisualElement::FindChildrenContainingPoint(ClientPosition, OutChildElements);

    if (Editor && Editor->HasKeyboardFocus())
    {
        Editor->FindChildrenContainingPoint(ClientPosition, OutChildElements);
    }
}

template<typename T>
int32 TNumericEntry<T>::OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const
{
    const FUIStyle&    Style        = FUIStyle::GetDefault();
    const FCornerRadii CornerRadius = FCornerRadii(Style.Metrics.CornerRadius);

    OutCommandList.AddBox(LayerId, AllottedGeometry.Bounds, Style.Colors.ControlNormal, CornerRadius);
    OutCommandList.AddBoxOutline(LayerId, AllottedGeometry.Bounds, Style.Colors.Border, Style.Metrics.BorderThickness, CornerRadius);

    const FRectangle LabelBounds = GetLabelRectangle(AllottedGeometry.Bounds);
    if (!LabelBounds.IsEmpty())
    {
        OutCommandList.AddBox(LayerId, LabelBounds, LabelColor, FCornerRadii(Style.Metrics.CornerRadius, 0.0f, 0.0f, Style.Metrics.CornerRadius));

        if (Font && !Label.IsEmpty())
        {
            const int32 LabelTextWidth = Font->MeasureWidth(StringView(Label.Data(), Label.Length()));

            FRectangle LabelTextBounds;
            LabelTextBounds.Position.X = LabelBounds.Position.X + Math::Max(0, (LabelBounds.Width - LabelTextWidth) / 2);
            LabelTextBounds.Position.Y = LabelBounds.Position.Y + Font->GetTextBandOffset(LabelBounds.Height);
            LabelTextBounds.Width      = LabelTextWidth;
            LabelTextBounds.Height     = Font->GetTextBandHeight();

            OutCommandList.AddText(LayerId, LabelTextBounds, Label, Font.Get(), Style.Colors.Text);
        }
    }

    int32 NextLayerId = LayerId;
    if (Editor && Editor->IsVisible())
    {
        const FDrawGeometry EditorGeometry(Editor->GetContentRectangle(), AllottedGeometry.Scale);
        NextLayerId = Math::Max(NextLayerId, Editor->OnDraw(EditorGeometry, OutCommandList, LayerId));
    }

    return NextLayerId;
}

template<typename T>
FEventResponse TNumericEntry<T>::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    if (CursorEvent.GetKey() != Keys::MouseButtonLeft)
    {
        return FEventResponse::Unhandled();
    }

    if (!GetContentRectangle().EncapsulatesPoint(CursorEvent.GetClientPosition()))
    {
        return FEventResponse::Unhandled();
    }

    bIsScrubbing       = true;
    bHasScrubbed       = false;
    ScrubStartValue    = Value;
    ScrubStartPosition = CursorEvent.GetClientPosition();

    if (FApplication::IsInitialized())
    {
        FApplication::Get().CaptureMouse(AsSharedPtr());
    }

    return FEventResponse::Handled();
}

template<typename T>
FEventResponse TNumericEntry<T>::OnMouseButtonUp(const FCursorEvent& CursorEvent)
{
    if (!bIsScrubbing || CursorEvent.GetKey() != Keys::MouseButtonLeft)
    {
        return FEventResponse::Unhandled();
    }

    bIsScrubbing = false;

    if (FApplication::IsInitialized())
    {
        FApplication::Get().ReleaseMouseCapture(AsSharedPtr());
    }

    if (!bHasScrubbed)
    {
        BeginEditing(CursorEvent.GetClientPosition());
    }

    return FEventResponse::Handled();
}

template<typename T>
FEventResponse TNumericEntry<T>::OnMouseMove(const FCursorEvent& CursorEvent)
{
    if (!bIsScrubbing)
    {
        return FEventResponse::Unhandled();
    }

    const int32 Travel = CursorEvent.GetClientPosition().X - ScrubStartPosition.X;
    if (!bHasScrubbed && Math::Abs(Travel) < ScrubThreshold)
    {
        return FEventResponse::Handled();
    }

    bHasScrubbed = true;
    ApplyValue(ScrubStartValue + (static_cast<T>(Travel) * Step));
    return FEventResponse::Handled();
}

template<typename T>
bool TNumericEntry<T>::GetCursor(ECursor& OutCursor) const
{
    if (Editor && Editor->HasKeyboardFocus())
    {
        return false;
    }

    OutCursor = ECursor::ResizeEW;
    return true;
}

template<typename T>
void TNumericEntry<T>::SetValue(T InValue)
{
    Value = SanitizeValue(InValue);
    UpdateEditorText();
}

template<typename T>
FRectangle TNumericEntry<T>::GetLabelRectangle(const FRectangle& Bounds) const
{
    if (!bShowLabel)
    {
        return FRectangle();
    }

    return FRectangle(Bounds.Position, Math::Min(LabelWidth, Bounds.Width), Bounds.Height);
}

template<typename T>
FRectangle TNumericEntry<T>::GetEditorRectangle(const FRectangle& Bounds) const
{
    const FRectangle LabelBounds = GetLabelRectangle(Bounds);
    const int32      Left        = (LabelBounds.IsEmpty() ? Bounds.Position.X : LabelBounds.GetRight()) + TextInset;

    return FRectangle(IntVector2(Left, Bounds.Position.Y), Math::Max(0, Bounds.GetRight() - TextInset - Left), Bounds.Height);
}

template<typename T>
T TNumericEntry<T>::SanitizeValue(T InValue) const
{
    return Math::Clamp(InValue, MinValue, MaxValue);
}

template<typename T>
void TNumericEntry<T>::ApplyValue(T InValue)
{
    const T NewValue = SanitizeValue(InValue);
    if (NewValue == Value)
    {
        return;
    }

    Value = NewValue;

    UpdateEditorText();
    OnValueChangedDelegate.ExecuteIfBound(Value);
}

template<typename T>
void TNumericEntry<T>::UpdateEditorText()
{
    if (Editor)
    {
        Editor->SetTextSilently(TNumericEntryTraits<T>::Format(Value));
    }
}

template<typename T>
void TNumericEntry<T>::HandleTextCommitted(const String& InText)
{
    ApplyValue(TNumericEntryTraits<T>::Parse(InText));
    UpdateEditorText();
}

template<typename T>
void TNumericEntry<T>::BeginEditing(const IntVector2& ClientPosition)
{
    if (!Editor)
    {
        return;
    }

    if (FApplication::IsInitialized())
    {
        FApplication::Get().SetFocusElement(Editor);
    }

    if (Font)
    {
        const String& EditorText = Editor->GetText();
        const int32   OffsetX    = ClientPosition.X - Editor->GetContentRectangle().Position.X;

        Editor->SetTextCursorPosition(Font->FindCharacterIndexAtOffset(StringView(EditorText.Data(), EditorText.Length()), OffsetX));
    }
}

using FNumericEntryFloat = TNumericEntry<float>;
using FNumericEntryInt   = TNumericEntry<int32>;
