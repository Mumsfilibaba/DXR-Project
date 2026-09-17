#pragma once
#include "Core/Delegates/Delegate.h"
#include "Core/Math/Color.h"
#include "Application/Elements/InteractiveElement.h"

/** @brief Called as the picker is dragged, rather than only once it is released. */
DECLARE_DELEGATE(FOnColorPicked, const FFloatColor& /*NewColor*/);

class APPLICATION_API FColorPicker final : public FInteractiveElement
{
public:
    struct FDesc
    {
        /** @brief The color the picker opens on. */
        FFloatColor Color = FFloatColor::White;

        /** @brief How wide and tall the saturation and value square is, in pixels. */
        int32 SquareExtent = 160;

        /** @brief How wide the hue bar beside that square is, in pixels. */
        int32 HueBarWidth = 16;

        /** @brief The space between the square and the hue bar, in pixels. */
        int32 Spacing = 8;

        /** @brief Fired as either the square or the bar is dragged. */
        FOnColorPicked OnColorPicked;
    };

public:
    static TSharedPtr<FColorPicker> Create(const FDesc& Desc);

public:
    FColorPicker();
    virtual ~FColorPicker();

    /**
     * @brief Initializes the picker with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override;

    /**
     * @brief Replaces the color the picker sits on, which leaves the hue where it is when the color is grey
     * and so carries no hue of its own.
     *
     * @param InColor The new color.
     */
    void SetColor(const FFloatColor& InColor);

    /** @return The color the picker currently names. */
    NODISCARD FFloatColor GetColor() const;

protected:

    // FInteractiveElement Interface
    virtual void OnDragged(const FCursorEvent& CursorEvent) override;

    virtual bool IsPressable() const override { return false; }

private:
    NODISCARD FRectangle GetSquareRectangle(const FRectangle& Bounds) const;
    NODISCARD FRectangle GetHueBarRectangle(const FRectangle& Bounds) const;

    void ApplyCursor(const IntVector2& ClientPosition);
    void NotifyColorPicked();

    float          Hue;
    float          Saturation;
    float          Brightness;
    int32          SquareExtent;
    int32          HueBarWidth;
    int32          Spacing;
    bool           bIsDraggingHue;
    FOnColorPicked OnColorPickedDelegate;
};
