#pragma once
#include "Core/Containers/String.h"
#include "Core/Delegates/Delegate.h"
#include "Application/Draw/DrawTypes.h"
#include "Application/Elements/VisualElement.h"
#include "Application/Style/UIStyle.h"
#include "Application/Text/IFontFace.h"

/** @brief Called with the state the section moved to. */
DECLARE_DELEGATE(FOnExpanderStateChanged, bool /*bIsExpanded*/);

class APPLICATION_API FExpander final : public FVisualElement
{
public:
    struct FDesc
    {
        /** @brief The text the header shows. */
        String Label;

        /** @brief The face the header label is measured and drawn with. */
        TSharedPtr<IFontFace> Font = nullptr;

        /** @brief The element shown below the header while the section is open. */
        TSharedPtr<FVisualElement> Content = nullptr;

        /** @brief True to start with the content shown. */
        bool bIsExpanded = true;

        /** @brief The height of the header bar, in pixels. */
        int32 HeaderHeight = FUIStyle::GetDefault().Metrics.RowHeight;

        /** @brief The space kept around the content, whose left side is what reads as the indent. */
        FMargin ContentPadding = FMargin(12, 4, 4, 4);

        /** @brief The look of the header bar, which defaults to the shared header style. */
        FUIHeaderStyle Style = FUIStyle::GetDefault().Header;

        /** @brief Drawn at the left of an open header, an unset brush falling back to a drawn triangle. */
        FUIBrush ExpandedArrow;

        /** @brief Drawn at the left of a closed header, an unset brush falling back to a drawn triangle. */
        FUIBrush CollapsedArrow;

        /** @brief The size the arrow brush is drawn at, in pixels, which is square. */
        int32 ArrowSize = 16;

        /** @brief Whether a closed section draws the rule that separates it from the next one. */
        bool bDrawBottomBorderWhenClosed = true;

        /** @brief Fired with the state the section moved to. */
        FOnExpanderStateChanged OnStateChanged;
    };

public:
    static TSharedPtr<FExpander> Create(const FDesc& Desc);

public:
    FExpander();
    virtual ~FExpander();

    /**
     * @brief Initializes the expander with the specified parameters.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual void OnArrange(const FRectangle& AllottedBounds) override;
    virtual void GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const override;
    virtual void FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutChildElements) override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseEntered(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseMove(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseLeft(const FCursorEvent& CursorEvent) override;
    virtual bool GetCursor(ECursor& OutCursor) const override;

    /**
     * @brief Sets the element shown below the header and makes this element its parent. The content is
     * measured even while the section is closed, so opening it needs no further pass.
     *
     * @param InContent The element to show, which may be null.
     */
    void SetContent(const TSharedPtr<FVisualElement>& InContent);

    /** @return The element shown below the header, which is null when none has been set. */
    NODISCARD FORCEINLINE const TSharedPtr<FVisualElement>& GetContent() const
    {
        return Content;
    }

    /**
     * @brief Opens or closes the section, firing the delegate only when that moved it.
     *
     * @param bInIsExpanded True to show the content below the header.
     */
    void SetExpanded(bool bInIsExpanded);

    /** @return True while the content is shown below the header. */
    NODISCARD FORCEINLINE bool IsExpanded() const
    {
        return bIsExpanded;
    }

    /**
     * @brief Replaces the text the header shows.
     *
     * @param InLabel The new text.
     */
    void SetLabel(const String& InLabel);

    /** @return The text the header shows. */
    NODISCARD FORCEINLINE const String& GetLabel() const
    {
        return Label;
    }

private:
    NODISCARD FRectangle GetHeaderBounds(const FRectangle& AllottedBounds) const;
    NODISCARD int32 GetArrowExtent() const;

    TSharedPtr<FVisualElement> Content;
    TSharedPtr<IFontFace>      Font;
    String                     Label;
    FMargin                    ContentPadding;
    FUIHeaderStyle             Style;
    FUIBrush                   ExpandedArrow;
    FUIBrush                   CollapsedArrow;
    int32                      ArrowSize;
    int32                      HeaderHeight;
    bool                       bIsExpanded;
    bool                       bIsHeaderHovered;
    bool                       bDrawBottomBorderWhenClosed;
    FOnExpanderStateChanged    OnStateChangedDelegate;
};
