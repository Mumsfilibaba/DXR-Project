#pragma once
#include "Core/Delegates/Delegate.h"
#include "Application/Elements/InteractiveElement.h"
#include "Application/Layout/LayoutTypes.h"
#include "Application/Text/IFontFace.h"

/** @brief Called every time the value moves, which during a drag is every frame the cursor moves. */
DECLARE_DELEGATE(FOnSliderValueChanged, float /*NewValue*/);

/** @brief Called once when the drag ends, which is where an expensive reaction belongs. */
DECLARE_DELEGATE(FOnSliderValueCommitted, float /*FinalValue*/);

class APPLICATION_API FSlider final : public FInteractiveElement
{
public:
    struct FDesc
    {
        FDesc()
            : MinValue(0.0f)
            , MaxValue(1.0f)
            , Value(0.0f)
            , StepSize(0.0f)
            , Orientation(EOrientation::Horizontal)
            , HandleSize(12)
            , TrackThickness(4)
            , MinLength(96)
            , bShowValueText(false)
            , Font(nullptr)
            , Precision(2)
            , OnValueChanged()
            , OnValueCommitted()
        {
        }

        /**
         * @brief Sets the range the value is clamped to.
         *
         * @param InMinValue The lowest value the handle can reach.
         * @param InMaxValue The highest value the handle can reach.
         * @return This desc, so the setters can be chained.
         */
        FORCEINLINE FDesc& SetRange(float InMinValue, float InMaxValue)
        {
            MinValue = InMinValue;
            MaxValue = InMaxValue;
            return *this;
        }

        /**
         * @brief Sets the value the slider starts at.
         *
         * @param InValue The starting value.
         * @return This desc, so the setters can be chained.
         */
        FORCEINLINE FDesc& SetValue(float InValue)
        {
            Value = InValue;
            return *this;
        }

        /** @brief The lowest value the handle can reach. */
        float MinValue;

        /** @brief The highest value the handle can reach, raised to MinValue when it is set below it. */
        float MaxValue;

        /** @brief The value the slider starts at, clamped into the range and snapped to the step. */
        float Value;

        /** @brief Distance between the values the handle can rest on, or zero for a continuous slider. */
        float StepSize;

        /** @brief Whether the track runs left to right or bottom to top. */
        EOrientation Orientation;

        /** @brief The side of the handle square, in pixels, half of which insets the travel at each end. */
        int32 HandleSize;

        /** @brief How thick the groove is across the track, in pixels. */
        int32 TrackThickness;

        /** @brief The shortest the slider asks to be along the track, in pixels, never less than one handle. */
        int32 MinLength;

        /** @brief True to write the value over the track, which needs a font to be of any use. */
        bool bShowValueText : 1;

        /** @brief The face the value text is drawn with. */
        TSharedPtr<IFontFace> Font;

        /** @brief Digits shown after the decimal point in the value text, clamped to nine. */
        int32 Precision;

        /** @brief Fired every time the value moves, which during a drag is every frame the cursor moves. */
        FOnSliderValueChanged OnValueChanged;

        /** @brief Fired once when the drag ends, which is where an expensive reaction belongs. */
        FOnSliderValueCommitted OnValueCommitted;
    };

public:
    static TSharedPtr<FSlider> Create(const FDesc& Desc);

public:
    FSlider();
    virtual ~FSlider();

    /**
     * @brief Initializes the slider with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseButtonUp(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnKeyDown(const FKeyEvent& KeyEvent) override;

    /**
     * @brief Sets the value without firing either delegate, which is how a host pushes a model value in.
     *
     * @param InValue The value to move to, clamped to the range and snapped to the step.
     */
    void SetValue(float InValue);

    /**
     * @brief Sets the range and re-clamps the current value into it.
     *
     * @param InMinValue The lowest value the handle can reach.
     * @param InMaxValue The highest value the handle can reach.
     */
    void SetRange(float InMinValue, float InMaxValue);

    /** @return The value the handle sits at, already clamped to the range and snapped to the step. */
    NODISCARD FORCEINLINE float GetValue() const
    {
        return Value;
    }

    /** @return The lowest value the handle can reach. */
    NODISCARD FORCEINLINE float GetMinValue() const
    {
        return MinValue;
    }

    /** @return The highest value the handle can reach, which is never below the lowest. */
    NODISCARD FORCEINLINE float GetMaxValue() const
    {
        return MaxValue;
    }

    /** @return Zero at the minimum through one at the maximum, and zero for an empty range. */
    NODISCARD float GetNormalizedValue() const;

    /** @return The axis the handle travels along, where a vertical slider carries its maximum at the top. */
    NODISCARD FORCEINLINE EOrientation GetOrientation() const
    {
        return Orientation;
    }

    /**
     * @brief Gets the rectangle the handle occupies, which is what a drag grabs.
     *
     * @return The handle rectangle, in the space the slider was arranged in.
     */
    NODISCARD FRectangle GetHandleBounds() const;

    /**
     * @brief Gets the span the handle's centre travels over.
     *
     * @return The slider's rectangle inset by half a handle at each end along the axis it runs down.
     */
    NODISCARD FRectangle GetTrackBounds() const;

    /** @return The value as written over the track, in fixed point to the precision asked for. */
    NODISCARD String GetFormattedValue() const;

protected:

    // FInteractiveElement Interface
    virtual void OnDragged(const FCursorEvent& CursorEvent) override;

    virtual bool IsPressable() const override { return false; }

private:
    NODISCARD float SanitizeValue(float InValue) const;

    void ApplyValue(float InValue);

    void SetValueFromPosition(const IntVector2& ClientPosition);

    NODISCARD FRectangle ComputeHandleBounds(const FRectangle& Bounds) const;
    NODISCARD FRectangle ComputeTrackBounds(const FRectangle& Bounds) const;

    float                   MinValue;
    float                   MaxValue;
    float                   Value;
    float                   StepSize;
    EOrientation            Orientation;
    int32                   HandleSize;
    int32                   TrackThickness;
    int32                   MinLength;
    int32                   Precision;
    bool                    bShowValueText;
    TSharedPtr<IFontFace>   Font;
    FOnSliderValueChanged   OnValueChangedDelegate;
    FOnSliderValueCommitted OnValueCommittedDelegate;
};
