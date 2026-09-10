#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/String.h"
#include "Application/Elements/VisualElement.h"
#include "Application/Style/UIStyle.h"
#include "Application/Text/IFontFace.h"

class APPLICATION_API FHistogram final : public FVisualElement
{
public:

    /** @brief The index reported for the hovered bar while the cursor is over none of them. */
    static constexpr int32 InvalidSampleIndex = -1;

public:
    struct FDesc
    {
        /** @brief How many samples are kept before the oldest is dropped. */
        int32 Capacity = 128;

        /** @brief The value the bottom of the strip stands for. */
        float MinValue = 0.0f;

        /** @brief The value a full-height bar stands for, which auto-scaling raises to the running maximum. */
        float MaxValue = 0.0f;

        /** @brief True to scale against the tallest sample held rather than against MaxValue alone. */
        bool bAutoScale = true;

        /** @brief The face the label and the hovered value are drawn with. */
        TSharedPtr<IFontFace> Font = nullptr;

        /** @brief The text drawn at the top-left, which names what the strip is measuring. */
        String Label;

        /** @brief The color a bar below the warning threshold is drawn in. */
        FFloatColor BarColor = FUIStyle::GetDefault().Colors.Accent;

        /** @brief The color the strip is drawn in behind the bars. */
        FFloatColor BackgroundColor = FUIStyle::GetDefault().Colors.ControlNormal;

        /** @brief How tall the strip asks to be, in pixels. */
        int32 PreferredHeight = 64;

        /** @brief The value a bar has to pass to be drawn as a warning, or zero to never warn. */
        float WarningThreshold = 0.0f;

        /** @brief The color a bar past the warning threshold is drawn in. */
        FFloatColor WarningColor = FFloatColor(0.85f, 0.35f, 0.25f, 1.0f);
    };

public:
    static TSharedPtr<FHistogram> Create(const FDesc& Desc);

public:
    FHistogram();
    virtual ~FHistogram();

    /**
     * @brief Initializes the histogram with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual FEventResponse OnMouseMove(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseLeft(const FCursorEvent& CursorEvent) override;

    /**
     * @brief Appends a sample, dropping the oldest once the buffer is full.
     *
     * @param InValue The value to append, which is not clamped to the range.
     */
    void AddSample(float InValue);

    /** @brief Drops every sample, leaving the strip empty. */
    void Clear();

    /** @return How many samples are held, which never passes the capacity. */
    NODISCARD FORCEINLINE int32 GetNumSamples() const
    {
        return NumSamples;
    }

    /**
     * @brief Gets one of the samples held, oldest first.
     *
     * @param Index The sample to read, where zero is the oldest still held.
     * @return The value, or zero when the index is outside what is held.
     */
    NODISCARD float GetSample(int32 Index) const;

    /** @return The most recent sample, or zero when none is held. */
    NODISCARD float GetLatest() const;

    /** @return The mean of the samples held, or zero when none is held. */
    NODISCARD float GetAverage() const;

    /** @return The largest of the samples held, which auto-scaling scales against, or zero when none is held. */
    NODISCARD float GetMaximum() const;

    /**
     * @brief Sets the range the bars are scaled against.
     *
     * @param InMin The value the bottom of the strip stands for.
     * @param InMax The value a full-height bar stands for, raised to the minimum when it is set below it.
     */
    void SetRange(float InMin, float InMax);

    /**
     * @brief Sets whether the range's upper end follows the tallest sample held.
     *
     * @param bInAutoScale True to scale against the running maximum rather than against the range alone.
     */
    void SetAutoScale(bool bInAutoScale);

    /** @return The bar under the cursor, oldest first, or InvalidSampleIndex when the cursor is over none. */
    NODISCARD FORCEINLINE int32 GetHoveredSample() const
    {
        return HoveredSample;
    }

private:
    NODISCARD float ResolveUpperBound() const;
    NODISCARD int32 ResolveColumnWidth(int32 AvailableWidth) const;

    TSharedPtr<IFontFace> Font;
    String                Label;
    TArray<float>         Samples;
    FFloatColor           BarColor;
    FFloatColor           BackgroundColor;
    FFloatColor           WarningColor;
    float                 MinValue;
    float                 MaxValue;
    float                 WarningThreshold;
    int32                 Capacity;
    int32                 OldestSample;
    int32                 NumSamples;
    int32                 PreferredHeight;
    int32                 HoveredSample;
    bool                  bAutoScale;
};
