#pragma once
#include "Core/Math/Color.h"
#include "Application/Elements/VisualElement.h"
#include "Application/Layout/LayoutTypes.h"
#include "Application/Style/UIStyle.h"

class APPLICATION_API FSeparator final : public FVisualElement
{
public:
    struct FDesc
    {
        EOrientation Orientation = EOrientation::Horizontal;
        int32        Thickness = FUIStyle::GetDefault().Metrics.SeparatorThickness;
        FMargin      Padding;
        FFloatColor  Color = FUIStyle::GetDefault().Colors.Border;
    };

public:
    static TSharedPtr<FSeparator> Create(const FDesc& Desc);

    /**
     * @brief Creates a horizontal rule in the style's separator color and thickness.
     *
     * @return The new separator.
     */
    static TSharedPtr<FSeparator> CreateHorizontal();

    /**
     * @brief Creates a vertical rule in the style's separator color and thickness.
     *
     * @return The new separator.
     */
    static TSharedPtr<FSeparator> CreateVertical();

public:
    FSeparator();
    virtual ~FSeparator();

    /**
     * @brief Initializes the separator with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;

    /** @return The axis the line runs along, Horizontal spanning the width and Vertical the height. */
    NODISCARD FORCEINLINE EOrientation GetOrientation() const
    {
        return Orientation;
    }

private:
    EOrientation Orientation;
    int32        Thickness;
    FMargin      Padding;
    FFloatColor  Color;
};
