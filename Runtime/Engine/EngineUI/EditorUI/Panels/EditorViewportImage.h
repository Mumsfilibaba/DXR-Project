#pragma once
#include "Application/Draw/DrawTypes.h"
#include "Application/Elements/VisualElement.h"

class ENGINE_API FEditorViewportImage final : public FVisualElement
{
public:
    static TSharedPtr<FEditorViewportImage> Create();

public:
    FEditorViewportImage();
    virtual ~FEditorViewportImage();

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override final;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override final;

    /**
     * @brief Sets the texture the panel shows, which is the render target the scene was drawn into.
     *
     * @param InBrush The brush to sample, which draws nothing while it holds no texture.
     */
    void SetBrush(const FUIBrush& InBrush);

    /** @return The brush the panel samples, which holds no texture before the first render target exists. */
    NODISCARD FORCEINLINE const FUIBrush& GetBrush() const
    {
        return Brush;
    }

private:
    FUIBrush Brush;
};
