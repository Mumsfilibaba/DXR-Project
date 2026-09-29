#pragma once
#include "Application/Elements/VisualElement.h"

class APPLICATION_API FMenuHost final : public FVisualElement
{
public:
    static TSharedPtr<FMenuHost> Create();

public:
    FMenuHost();
    virtual ~FMenuHost();

    /**
     * @brief Adds a child at a fixed rectangle, on top of everything already there.
     *
     * @param InContent      The element to show.
     * @param InClientBounds Where to put it, in the client space of the window holding this host.
     * @param bInHitTestable False keeps the cursor from ever finding it, which a tool tip needs.
     */
    void AddChild(const TSharedPtr<FVisualElement>& InContent, const FRectangle& InClientBounds, bool bInHitTestable = true);

    /**
     * @brief Moves a child, which does nothing when the element is not one.
     *
     * @param InContent      The element to move.
     * @param InClientBounds Where to put it, in the client space of the window holding this host.
     */
    void SetChildBounds(const TSharedPtr<FVisualElement>& InContent, const FRectangle& InClientBounds);

    /**
     * @brief Removes a child, which does nothing when the element is not one.
     *
     * @param InContent The element to remove.
     */
    void RemoveChild(const TSharedPtr<FVisualElement>& InContent);

    /** @return True while the host holds nothing, which is how a window knows there is nothing to draw. */
    NODISCARD bool IsEmpty() const;

    /**
     * @brief Tests whether an open menu covers the point.
     *
     * A menu is the top layer of its window, so what it covers has to stay out of the cursor path
     * entirely. A tool tip is hosted here too but is never hit testable, so it never blocks.
     *
     * @param ClientPosition The point, in the client space of the window holding this host.
     * @return True when a hit testable menu is over the point.
     */
    NODISCARD bool CoversPoint(const IntVector2& ClientPosition) const;

    /**
     * @brief Hit tests the host the way its window does, which leaves the host out of the path unless a hit testable
     * child is under the point. The host spans the whole window, so a plain HitTest would put it on every path.
     *
     * @param ClientPosition The point, in the client space of the window holding this host.
     * @param OutPath        The path to append the host and the chain under the point to.
     */
    void HitTestHostedChild(const IntVector2& ClientPosition, FElementPath& OutPath);

public:

    // FVisualElement Interface
    virtual void OnArrange(const FRectangle& AllottedBounds) override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;

protected:
    virtual EChildVisit VisitChildren(FChildVisitor& Visitor, EChildOrder Order) const override;
    virtual void HitTestChildren(const IntVector2& ClientPosition, FElementPath& OutPath) override;

private:
    struct FHostedChild
    {
        FHostedChild()
            : Content(nullptr)
            , ClientBounds()
            , bHitTestable(true)
        {
        }

        TSharedPtr<FVisualElement> Content;
        FRectangle                 ClientBounds;
        bool                       bHitTestable;
    };

    NODISCARD const FHostedChild* FindChildAtPoint(const IntVector2& ClientPosition) const;

    TArray<FHostedChild> Children;
};
