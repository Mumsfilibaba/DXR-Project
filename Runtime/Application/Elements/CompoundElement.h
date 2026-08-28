#pragma once
#include "Application/Elements/VisualElement.h"

class APPLICATION_API FCompoundElement : public FVisualElement
{
public:
    FCompoundElement();
    virtual ~FCompoundElement();

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual void OnArrange(const FRectangle& AllottedBounds) override;
    virtual void GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual void FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements) override;

    /**
     * @brief Sets the single child of this element and makes it the child's parent.
     *
     * @param InContent The element to place inside the padding, which may be null.
     */
    void SetContent(const TSharedPtr<FVisualElement>& InContent);

    /** @return The single child of this element, which is null when none has been set. */
    NODISCARD FORCEINLINE const TSharedPtr<FVisualElement>& GetContent() const
    {
        return Content;
    }

    /**
     * @brief Sets the space between this element's bounds and its child.
     *
     * @param InPadding The padding to apply.
     */
    void SetPadding(const FMargin& InPadding);

    /** @return The space between this element's bounds and its child, in pixels on each side. */
    NODISCARD FORCEINLINE const FMargin& GetPadding() const
    {
        return Padding;
    }

protected:
    TSharedPtr<FVisualElement> Content;
    FMargin                    Padding;
};
