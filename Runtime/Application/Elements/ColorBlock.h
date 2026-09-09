#pragma once
#include "Core/Math/Color.h"
#include "Application/Elements/Button.h"
#include "Application/Elements/InteractiveElement.h"

class APPLICATION_API FColorBlock final : public FInteractiveElement
{
public:
    struct FDesc
    {
        /** @brief The color the swatch shows, whose alpha is ignored so a transparent color still reads. */
        FFloatColor Color = FFloatColor::White;

        /** @brief How wide and tall the swatch is, in pixels. */
        int32 Extent = FUIStyle::GetDefault().Metrics.ButtonHeight;

        /** @brief Fired when the swatch is clicked, which is what opens a picker anchored to it. */
        FOnClicked OnClicked;
    };

public:
    static TSharedPtr<FColorBlock> Create(const FDesc& Desc);

public:
    FColorBlock();
    virtual ~FColorBlock();

    /**
     * @brief Initializes the swatch with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;

    /**
     * @brief Replaces the color shown, which is what a picker drag drives.
     *
     * @param InColor The new color.
     */
    void SetColor(const FFloatColor& InColor);

    /** @return The color the swatch shows. */
    NODISCARD FORCEINLINE const FFloatColor& GetColor() const
    {
        return Color;
    }

    /**
     * @brief Gets the delegate the swatch fires when clicked, which an owner binds after the fact when what
     * the click opens is anchored to the swatch and so cannot exist before it.
     *
     * @return The delegate.
     */
    NODISCARD FORCEINLINE FOnClicked& GetOnClicked()
    {
        return OnClickedDelegate;
    }

protected:

    // FInteractiveElement Interface
    virtual void OnClicked() override;

private:
    FFloatColor Color;
    int32       Extent;
    FOnClicked  OnClickedDelegate;
};
