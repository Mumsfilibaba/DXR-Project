#pragma once
#include "Core/Math/Math.h"
#include "Application/Draw/DrawCommandList.h"
#include "Application/ElementPath.h"
#include "Application/Elements/VisualElement.h"

class FFractionWidthBox final : public FVisualElement
{
public:

    /**
     * @brief Creates a box that hands its child part of the width.
     *
     * @param InChild    The element wrapped, which is arranged into the share and may be null.
     * @param InFraction How much of the allotted width the child is given, where one gives all of it.
     * @param InMinWidth The least width the child is given in pixels, which the allotted width still caps.
     * @return The box, which has taken the child over as its own.
     */
    NODISCARD static FORCEINLINE TSharedPtr<FFractionWidthBox> Create(const TSharedPtr<FVisualElement>& InChild, float InFraction, int32 InMinWidth)
    {
        TSharedPtr<FFractionWidthBox> NewBox = MakeSharedPtr<FFractionWidthBox>();
        NewBox->Child    = InChild;
        NewBox->Fraction = InFraction;
        NewBox->MinWidth = InMinWidth;

        if (InChild)
        {
            InChild->SetParentElement(NewBox->AsWeakPtr());
        }

        return NewBox;
    }

public:
    FFractionWidthBox()
        : Child(nullptr)
        , Fraction(1.0f)
        , MinWidth(0)
    {
    }

    virtual ~FFractionWidthBox() = default;

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override final
    {
        return Child ? Child->GetCachedDesiredSize() : IntVector2(0, 0);
    }

    virtual void OnArrange(const FRectangle& AllottedBounds) override final
    {
        if (!Child)
        {
            return;
        }

        const int32 Share = static_cast<int32>(static_cast<float>(AllottedBounds.Width) * Fraction);

        FRectangle ChildBounds = AllottedBounds;
        ChildBounds.Width      = Math::Min(AllottedBounds.Width, Math::Max(MinWidth, Share));

        Child->Tick(ChildBounds);
    }

    virtual void GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const override final
    {
        if (Child)
        {
            OutChildren.Add(Child);
        }
    }

    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override final
    {
        if (!Child)
        {
            return LayerId;
        }

        const FDrawGeometry ChildGeometry(Child->GetContentRectangle(), AllottedGeometry.Scale);
        return Child->OnDraw(ChildGeometry, OutCommandList, LayerId + 1);
    }

    virtual void FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements) override final
    {
        FVisualElement::FindChildrenContainingPoint(ClientPosition, OutChildElements);

        if (Child)
        {
            Child->FindChildrenContainingPoint(ClientPosition, OutChildElements);
        }
    }

private:
    TSharedPtr<FVisualElement> Child;
    float                      Fraction;
    int32                      MinWidth;
};
