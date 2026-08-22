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

    /** @brief The single child of this element, which may be null. */
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

    /** @brief The space between this element's bounds and its child. */
    NODISCARD FORCEINLINE const FMargin& GetPadding() const
    {
        return Padding;
    }

protected:
    TSharedPtr<FVisualElement> Content;
    FMargin                    Padding;
};
