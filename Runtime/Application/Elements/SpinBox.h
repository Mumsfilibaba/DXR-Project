#pragma once
#include "Core/Containers/String.h"
#include "Core/Delegates/Delegate.h"
#include "Core/Templates/NumericLimits.h"
#include "Application/Draw/DrawTypes.h"
#include "Application/Elements/InteractiveElement.h"
#include "Application/Style/UIStyle.h"
#include "Application/Text/IFontFace.h"

/** @brief Called every time the value moves, which during a scrub is every frame the cursor moves. */
DECLARE_DELEGATE(FOnSpinBoxValueChanged, float /*NewValue*/);

/** @brief Called once the value settles, which is the end of a scrub or an accepted typed entry. */
DECLARE_DELEGATE(FOnSpinBoxValueCommitted, float /*FinalValue*/);

class APPLICATION_API FSpinBox final : public FInteractiveElement
{
public:
    struct FDesc
    {
        FDesc()
            : MinValue(-TNumericLimits<float>::Max())
            , MaxValue(TNumericLimits<float>::Max())
            , Value(0.0f)
            , ScrubSpeed(0.1f)
            , StepSize(0.0f)
            , Precision(2)
            , Prefix()
            , Font(nullptr)
            , Padding(FUIStyle::GetDefault().Metrics.ControlPadding)
            , CornerRadius(FUIStyle::GetDefault().Metrics.CornerRadius)
            , MinWidth(64)
            , MinHeight(FUIStyle::GetDefault().Metrics.FrameHeight)
            , OnValueChanged()
            , OnValueCommitted()
        {
        }

        /**
         * @brief Sets the range the value is clamped to.
         *
         * @param InMinValue The lowest value the field can hold.
         * @param InMaxValue The highest value the field can hold.
         * @return This desc, so the setters can be chained.
         */
        FORCEINLINE FDesc& SetRange(float InMinValue, float InMaxValue)
        {
            MinValue = InMinValue;
            MaxValue = InMaxValue;
            return *this;
        }

        /**
         * @brief Sets the value the field starts at.
         *
         * @param InValue The starting value.
         * @return This desc, so the setters can be chained.
         */
        FORCEINLINE FDesc& SetValue(float InValue)
        {
            Value = InValue;
            return *this;
        }

        /**
         * @brief Sets the face the value is measured and drawn with.
         *
         * @param InFont The face to use.
         * @return This desc, so the setters can be chained.
         */
        FORCEINLINE FDesc& SetFont(const TSharedPtr<IFontFace>& InFont)
        {
            Font = InFont;
            return *this;
        }

        /** @brief The lowest value the field can hold. */
        float MinValue;

        /** @brief The highest value the field can hold, raised to MinValue when it is set below it. */
        float MaxValue;

        /** @brief The value the field starts at, clamped into the range and snapped to the step. */
        float Value;

        /** @brief How far the value moves per pixel of horizontal drag. */
        float ScrubSpeed;

        /** @brief Distance between the values a scrub can rest on, or zero for a continuous field. */
        float StepSize;

        /** @brief Digits shown after the decimal point. */
        int32 Precision;

        /** @brief Written before the value, so a field can read as "X: 12.0". */
        String Prefix;

        /** @brief The face the value is measured and drawn with. */
        TSharedPtr<IFontFace> Font;

        /** @brief The space between the field's bounds and its text. */
        FMargin Padding;

        /** @brief How far the field's corners are rounded, in pixels. */
        FCornerRadii CornerRadius;

        /** @brief The narrowest the field asks to be, in pixels. */
        int32 MinWidth;

        /** @brief The shortest the field asks to be, in pixels. */
        int32 MinHeight;

        /** @brief Fired every time the value moves, which during a scrub is every frame the cursor moves. */
        FOnSpinBoxValueChanged OnValueChanged;

        /** @brief Fired once the value settles, which is the end of a scrub or an accepted typed entry. */
        FOnSpinBoxValueCommitted OnValueCommitted;
    };

public:
    /** @brief How far the cursor travels before a press counts as a scrub rather than a click. */
    static constexpr int32 ScrubThreshold = 3;

public:
    static TSharedPtr<FSpinBox> Create(const FDesc& Desc);

public:
    FSpinBox();
    virtual ~FSpinBox();

    /**
     * @brief Initializes the spin box with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual void OnArrange(const FRectangle& AllottedBounds) override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseButtonUp(const FCursorEvent& CursorEvent) override;
    virtual bool GetCursor(ECursor& OutCursor) const override;
    virtual TSharedPtr<FVisualElement> GetFocusTarget() override;

    /**
     * @brief Sets the value without firing either delegate, which is how a host pushes a model value in.
     *
     * @param InValue The value to move to, clamped to the range and snapped to the step.
     */
    void SetValue(float InValue);

    /** @return The value the field holds, already clamped to the range and snapped to the step. */
    NODISCARD FORCEINLINE float GetValue() const
    {
        return Value;
    }

    /** @return The lowest value the field can hold, the bottom of the range the value is clamped to. */
    NODISCARD FORCEINLINE float GetMinValue() const
    {
        return MinValue;
    }

    /** @return The highest value the field can hold, the top of the range and never below the lowest. */
    NODISCARD FORCEINLINE float GetMaxValue() const
    {
        return MaxValue;
    }

    /** @return True while typing mode has the text box open, so keystrokes edit rather than scrub. */
    NODISCARD FORCEINLINE bool IsTyping() const
    {
        return bIsTyping;
    }

    /** @brief Swaps the label for a text box seeded with the current value and takes focus. */
    void BeginTyping();

    /**
     * @brief Leaves typing mode and puts the label back.
     *
     * @param bCommit True to parse what was typed and fire the commit delegate, false to discard it.
     */
    void EndTyping(bool bCommit);

    /**
     * @brief Formats the value at the configured precision, which is what typing mode is seeded with.
     *
     * @return The formatted value, without the prefix the label puts in front of it.
     */
    NODISCARD String GetFormattedValue() const;

protected:

    // FInteractiveElement Interface
    virtual void OnClicked() override;
    virtual void OnDragged(const FCursorEvent& CursorEvent) override;

private:
    NODISCARD float SanitizeValue(float InValue) const;

    void ApplyValue(float InValue);
    void UpdateLabel();
    void HandleTextCommitted(const String& InText);

    TSharedPtr<class FTextBlock>    Label;
    TSharedPtr<class FEditableText> Editor;
    float                           MinValue;
    float                           MaxValue;
    float                           Value;
    float                           ScrubSpeed;
    float                           StepSize;
    float                           ScrubStartValue;
    IntVector2                      ScrubStartPosition;
    int32                           Precision;
    String                          Prefix;
    FCornerRadii                    CornerRadius;
    int32                           MinWidth;
    int32                           MinHeight;
    bool                            bIsTyping;
    bool                            bHasScrubbed;
    FOnSpinBoxValueChanged          OnValueChangedDelegate;
    FOnSpinBoxValueCommitted        OnValueCommittedDelegate;
};
