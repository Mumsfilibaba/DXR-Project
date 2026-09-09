#pragma once
#include "Application/Draw/DrawTypes.h"
#include "Application/Elements/VisualElement.h"

class APPLICATION_API FImage final : public FVisualElement
{
public:
    struct FDesc
    {
        /** @brief The brush to sample, which may be set later. */
        FUIBrush Brush;

        /** @brief The size to ask for, which is nothing when the image takes whatever it is given. */
        IntVector2 DesiredSize;

        /** @brief The colour the brush is multiplied by. */
        FFloatColor Tint = FFloatColor::White;
    };

public:
    static TSharedPtr<FImage> Create(const FDesc& Desc);

public:
    FImage();
    virtual ~FImage();

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override final;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override final;

    /**
     * @brief Sets the brush the image samples.
     *
     * @param InBrush The brush, which draws nothing while it holds no texture.
     */
    void SetBrush(const FUIBrush& InBrush);

    /** @return The brush the image samples, which holds no texture until one is set. */
    NODISCARD FORCEINLINE const FUIBrush& GetBrush() const
    {
        return Brush;
    }

private:
    void Initialize(const FDesc& Desc);

    FUIBrush    Brush;
    IntVector2  DesiredSize;
    FFloatColor Tint;
};
