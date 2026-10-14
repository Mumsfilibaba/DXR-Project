#pragma once
#include "Application/Elements/VisualElement.h"
#include "Application/Style/UIStyle.h"

class APPLICATION_API FSpinner final : public FVisualElement
{
public:
    struct FDesc
    {
        /** @brief How wide and tall the spinner is, in pixels. */
        int32 Extent = 14;

        /** @brief The width of the turning arc, in pixels. */
        float Thickness = 2.0f;

        /** @brief The color of the turning arc. */
        FFloatColor Color = FUIStyle::GetDefault().Colors.Accent;

        /** @brief The color of the faint ring the arc turns on. */
        FFloatColor TrackColor = FUIStyle::GetDefault().Colors.ControlNormal;

        /** @brief How long one turn takes, in seconds. */
        float Period = 1.0f;

        /** @brief Whether it turns from the start; a stopped spinner draws nothing and takes no space. */
        bool bIsSpinning = true;
    };

public:
    static TSharedPtr<FSpinner> Create(const FDesc& Desc);

public:
    FSpinner();
    virtual ~FSpinner();

    /**
     * @brief Initializes the spinner with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual FIntVector2 ComputeDesiredSize() const override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;

    /**
     * @brief Starts or stops the spinner, which shows that work is running while it turns.
     *
     * @param bInIsSpinning Whether it turns.
     */
    void SetSpinning(bool bInIsSpinning);

    NODISCARD FORCEINLINE bool IsSpinning() const
    {
        return bIsSpinning;
    }

private:
    int32       Extent;
    float       Thickness;
    FFloatColor Color;
    FFloatColor TrackColor;
    float       Period;
    bool        bIsSpinning;
};
