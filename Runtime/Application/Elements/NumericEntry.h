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
     * @param Value     The value to format.
     * @param Precision How many decimal places to write.
     * @return The value at the given number of decimal places.
     */
    NODISCARD static FORCEINLINE String Format(float Value, int32 Precision)
    {
        return String::Printf("%.*f", Precision, Value);
    }

    /**
     * @brief Writes the value out at only the decimals it needs, so a whole number does not read as 1.000.
     *
     * @param Value The value to format.
     * @return The value at between one and four decimal places.
     */
    NODISCARD static FORCEINLINE String FormatDynamic(float Value)
    {
        static const float ScaleFactors[4] = { 10.0f, 100.0f, 1000.0f, 10000.0f };

        const float AbsoluteValue = Math::Abs(Value);

        int32 Decimals = 4;
        for (int32 Index = 0; Index < 4; ++Index)
        {
            const float Rounded   = Math::Round(AbsoluteValue * ScaleFactors[Index]) / ScaleFactors[Index];
            const float Tolerance = 1.0e-5f * (AbsoluteValue + 1.0f);

            if (Math::Abs(AbsoluteValue - Rounded) <= Tolerance)
            {
                Decimals = Index + 1;
                break;
            }
        }

        return String::Printf("%.*f", Decimals, Value);
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
     * @param Value     The value to format.
     * @param Precision Ignored, an integer having no decimals to place.
     * @return The value in decimal.
     */
    NODISCARD static FORCEINLINE String Format(int32 Value, int32 Precision)
    {
        UNREFERENCED_VARIABLE(Precision);
        return String::Printf("%d", Value);
    }

    /** @return The value in decimal, an integer having no decimals to drop. */
    NODISCARD static FORCEINLINE String FormatDynamic(int32 Value)
    {
        return Format(Value, 0);
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

        /**
         * @brief The colour of a thin rule down the field's left edge, which is the other way of marking an
         * axis. A fully transparent colour, which is the default, draws no rule.
         */
        FFloatColor AccentEdge = FFloatColor(0.0f, 0.0f, 0.0f, 0.0f);

        /** @brief Drawn after the value, which is what puts a degree sign on an angle. */
        String Suffix;

        /** @brief How many decimal places the value is written out at, which a float field ignores while bDynamicPrecision is set. */
        int32 Precision = 3;

        /** @brief Whether the value is written out at only the decimals it needs rather than at Precision. */
        bool bDynamicPrecision = false;

        /**
         * @brief Whether the field fills from its left edge to where the value sits between MinValue and
         * MaxValue, which is the shape ImGui's slider takes. A field left at the unbounded default range
         * draws no track, so this only takes effect once a range is set.
         */
        bool bShowFillTrack = false;

        /** @brief Where the value sits across the field, which a filled track wants centred. */
        EHorizontalAlignment TextAlignment = EHorizontalAlignment::Left;

        /** @brief Whether the field only reports a value, which stops it scrubbing and taking the caret. */
        bool bIsReadOnly = false;

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

    /** @brief How far the accent rule sits from the field's left edge, in pixels. */
    static constexpr int32 AccentEdgeOffset = 6;

    /** @brief How wide the accent rule is, in pixels. */
    static constexpr int32 AccentEdgeThickness = 2;

    /** @brief How far the accent rule stops short of the field's top and bottom edges, in pixels. */
    static constexpr int32 AccentEdgeInset = 5;

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
    NODISCARD bool HasFillTrack() const;
    NODISCARD T SanitizeValue(T InValue) const;
    NODISCARD String FormatValue() const;

    void ApplyValue(T InValue);
    void UpdateEditorText();
    void HandleTextCommitted(const String& InText);
    void BeginEditing(const IntVector2& ClientPosition);

    TSharedPtr<FEditableText> Editor;
    TSharedPtr<IFontFace>     Font;
    String                    Label;
    String                    Suffix;
    FFloatColor               LabelColor;
    FFloatColor               AccentEdge;
    T                         Value;
    T                         MinValue;
    T                         MaxValue;
    T                         Step;
    T                         ScrubStartValue;
    IntVector2                ScrubStartPosition;
    int32                     LabelWidth;
    int32                     Precision;
    bool                      bShowLabel;
    bool                      bDynamicPrecision;
    bool                      bShowFillTrack;
    bool                      bIsReadOnly;
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
    , Suffix()
    , LabelColor(0.62f, 0.20f, 0.22f, 1.0f)
    , AccentEdge(0.0f, 0.0f, 0.0f, 0.0f)
    , Value(T(0))
    , MinValue(TNumericLimits<T>::Lowest())
    , MaxValue(TNumericLimits<T>::Max())
    , Step(TNumericEntryTraits<T>::GetDefaultStep())
    , ScrubStartValue(T(0))
    , ScrubStartPosition()
    , LabelWidth(14)
    , Precision(3)
    , bShowLabel(true)
    , bDynamicPrecision(false)
    , bShowFillTrack(false)
    , bIsReadOnly(false)
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
    Suffix                 = Desc.Suffix;
    LabelColor             = Desc.LabelColor;
    AccentEdge             = Desc.AccentEdge;
    MinValue               = Desc.MinValue;
    MaxValue               = Math::Max(Desc.MaxValue, Desc.MinValue);
    Step                   = Desc.Step;
    LabelWidth             = Math::Max(0, Desc.LabelWidth);
    Precision              = Math::Max(0, Desc.Precision);
    bShowLabel             = Desc.bShowLabel;
    bDynamicPrecision      = Desc.bDynamicPrecision;
    bShowFillTrack         = Desc.bShowFillTrack;
    bIsReadOnly            = Desc.bIsReadOnly;
    OnValueChangedDelegate = Desc.OnValueChanged;

    Value = SanitizeValue(Desc.Value);

    FEditableText::FDesc EditorDesc;
    EditorDesc.Text            = FormatValue();
    EditorDesc.Font            = Desc.Font;
    EditorDesc.ForegroundColor = FUIStyle::GetDefault().Colors.Text;
    EditorDesc.Padding         = FMargin();
    EditorDesc.TextAlignment   = Desc.TextAlignment;

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
    DesiredSize.Y = Math::Max(DesiredSize.Y, FUIStyle::GetDefault().Metrics.FrameHeight);

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

    OutCommandList.AddBox(LayerId, AllottedGeometry.Bounds, Style.Colors.InputFieldFill, CornerRadius);

    if (HasFillTrack())
    {
        const float Fraction = Math::Clamp(static_cast<float>(Value - MinValue) / static_cast<float>(MaxValue - MinValue), 0.0f, 1.0f);

        FRectangle Filled = AllottedGeometry.Bounds;
        Filled.Width      = static_cast<int32>(static_cast<float>(Filled.Width) * Fraction);

        if (Filled.Width > 0)
        {
            OutCommandList.AddBox(LayerId, Filled, Style.NumericEntry.TrackFill, CornerRadius);
        }
    }

    OutCommandList.AddBoxOutline(LayerId, AllottedGeometry.Bounds, Style.Colors.InputFieldBorder, Style.Metrics.BorderThickness, CornerRadius);

    if (AccentEdge.A > 0.0f && AllottedGeometry.Bounds.Height > (AccentEdgeInset * 2))
    {
        const IntVector2 EdgePosition(
            AllottedGeometry.Bounds.Position.X + AccentEdgeOffset - (AccentEdgeThickness / 2),
            AllottedGeometry.Bounds.Position.Y + AccentEdgeInset);

        const FRectangle EdgeBounds(EdgePosition, AccentEdgeThickness, AllottedGeometry.Bounds.Height - (AccentEdgeInset * 2));
        OutCommandList.AddBox(LayerId, EdgeBounds, AccentEdge, FCornerRadii(1.0f));
    }

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
        OutCommandList.PushClip(LayerId, AllottedGeometry.Bounds);
        NextLayerId = Math::Max(NextLayerId, Editor->OnDraw(EditorGeometry, OutCommandList, LayerId));
        OutCommandList.PopClip(NextLayerId);
    }

    return NextLayerId;
}

template<typename T>
FEventResponse TNumericEntry<T>::OnMouseButtonDown(const FCursorEvent& CursorEvent)
{
    if (bIsReadOnly || CursorEvent.GetKey() != Keys::MouseButtonLeft)
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

    if (HasFillTrack())
    {
        const FRectangle Bounds = GetContentRectangle();
        const float      Alpha  = Bounds.Width > 0
            ? Math::Saturate(static_cast<float>(CursorEvent.GetClientPosition().X - Bounds.Position.X) / static_cast<float>(Bounds.Width))
            : 0.0f;

        ApplyValue(MinValue + static_cast<T>(static_cast<float>(MaxValue - MinValue) * Alpha));
        return FEventResponse::Handled();
    }

    ApplyValue(ScrubStartValue + (static_cast<T>(Travel) * Step));
    return FEventResponse::Handled();
}

template<typename T>
bool TNumericEntry<T>::GetCursor(ECursor& OutCursor) const
{
    if (bIsReadOnly || (Editor && Editor->HasKeyboardFocus()))
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
bool TNumericEntry<T>::HasFillTrack() const
{
    return bShowFillTrack
        && MaxValue > MinValue
        && MinValue > TNumericLimits<T>::Lowest()
        && MaxValue < TNumericLimits<T>::Max();
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

    int32 Left = LabelBounds.IsEmpty() ? Bounds.Position.X : LabelBounds.GetRight();
    if (LabelBounds.IsEmpty() && AccentEdge.A > 0.0f)
    {
        Left += AccentEdgeOffset + AccentEdgeThickness;
    }

    Left += TextInset;

    return FRectangle(IntVector2(Left, Bounds.Position.Y), Math::Max(0, Bounds.GetRight() - TextInset - Left), Bounds.Height);
}

template<typename T>
String TNumericEntry<T>::FormatValue() const
{
    String Text = bDynamicPrecision ? TNumericEntryTraits<T>::FormatDynamic(Value) : TNumericEntryTraits<T>::Format(Value, Precision);
    if (!Suffix.IsEmpty())
    {
        Text += Suffix;
    }

    return Text;
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
        Editor->SetTextSilently(FormatValue());
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
