#pragma once
#include "Core/Containers/String.h"
#include "Application/Draw/DrawTypes.h"
#include "Application/Elements/VisualElement.h"
#include "Application/Style/UIStyle.h"
#include "Application/Text/IFontFace.h"

class APPLICATION_API FProgressBar final : public FVisualElement
{
public:
    struct FDesc
    {
        /** @brief How full the bar starts, from empty at zero through full at one. */
        float Percent = 0.0f;

        /** @brief The text written over the track, which needs a font to be of any use. */
        String OverlayText;

        /** @brief The face the overlay text is drawn with. */
        TSharedPtr<IFontFace> Font = nullptr;

        /** @brief The color the filled part of the track is drawn in. */
        FFloatColor FillColor = FUIStyle::GetDefault().Colors.Accent;

        /** @brief The color the whole track is drawn in, behind the fill. */
        FFloatColor BackgroundColor = FUIStyle::GetDefault().Colors.ControlNormal;

        /** @brief The color the overlay text is drawn in. */
        FFloatColor TextColor = FUIStyle::GetDefault().Colors.Text;

        /** @brief How tall the bar asks to be, in pixels. */
        int32 PreferredHeight = 16;

        /** @brief How far each corner of the track is rounded, in pixels. */
        FCornerRadii CornerRadius = FCornerRadii(FUIStyle::GetDefault().Metrics.CornerRadius);
    };

public:
    static TSharedPtr<FProgressBar> Create(const FDesc& Desc);

public:
    FProgressBar();
    virtual ~FProgressBar();

    /**
     * @brief Initializes the bar with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;

    /**
     * @brief Sets how full the bar is.
     *
     * @param InPercent The fraction to fill to, clamped between zero and one.
     */
    void SetPercent(float InPercent);

    /** @return How full the bar is, from empty at zero through full at one. */
    NODISCARD FORCEINLINE float GetPercent() const
    {
        return Percent;
    }

    /**
     * @brief Sets the text written over the track.
     *
     * @param InText The text to write, which is drawn only when the bar has a font.
     */
    void SetOverlayText(const String& InText);

    /** @return The text written over the track, which is empty when there is none. */
    NODISCARD FORCEINLINE const String& GetOverlayText() const
    {
        return OverlayText;
    }

    /**
     * @brief Sets the color the filled part of the track is drawn in, which is how a bar warns about
     * what it measures rather than only reporting it, a VRAM bar turning red as it approaches full.
     *
     * @param InColor The color to fill with.
     */
    void SetFillColor(const FFloatColor& InColor);

private:
    String                OverlayText;
    TSharedPtr<IFontFace> Font;
    FFloatColor           FillColor;
    FFloatColor           BackgroundColor;
    FFloatColor           TextColor;
    FCornerRadii          CornerRadius;
    float                 Percent;
    int32                 PreferredHeight;
};
