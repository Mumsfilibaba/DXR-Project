#pragma once
#include "Core/Containers/String.h"
#include "Application/Draw/DrawTypes.h"
#include "Application/Elements/CompoundElement.h"
#include "Application/Elements/InteractiveElement.h"
#include "Application/Text/IFontFace.h"
#include "CoreApplication/PlatformInterface/IPlatformWindow.h"

class FHorizontalBox;
class FOverlay;
class FSpacer;
class FTextBlock;
class FWindow;

enum class ECaptionButtonKind : uint8
{
    Minimize,
    Maximize,
    Close,
};

class APPLICATION_API FCaptionButton final : public FInteractiveElement
{
public:

    /**
     * @brief Creates a caption button, which acts on whichever window it ends up in.
     *
     * @param InKind The window command the button carries out.
     * @return The new button.
     */
    static TSharedPtr<FCaptionButton> Create(ECaptionButtonKind InKind);

public:
    FCaptionButton();
    virtual ~FCaptionButton();

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual bool GetCursor(ECursor& OutCursor) const override;
    virtual FEventResponse OnMouseEntered(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseLeft(const FCursorEvent& CursorEvent) override;

    /**
     * @brief Sets the size the button asks for, which follows the window's DPI.
     *
     * @param InSize The width and height to ask for.
     */
    void SetButtonSize(const IntVector2& InSize);

    /** @return The window command the button carries out, which is the kind it was created as. */
    NODISCARD FORCEINLINE ECaptionButtonKind GetKind() const
    {
        return Kind;
    }

protected:

    // FInteractiveElement Interface
    virtual void OnClicked() override;

private:
    /**
     * @brief Starts the hover fade again from whatever alpha it had reached.
     *
     * Called before the base class takes the state change, so the alpha it captures is the one the
     * fade was at when the cursor moved rather than the one it is heading for.
     */
    void RestartHoverFade();

    /** @return Seconds since the hover last changed. */
    NODISCARD double GetSecondsSinceHoverFadeStart() const;

    /**
     * @return The alpha the hover fill should draw at, interpolated from the alpha held when the
     *         hover last changed towards the one the current state calls for.
     *
     * Derived from a timestamp rather than accumulated per tick so that the first draw after the
     * cursor arrives is already correct, without waiting on a tick to hand it a delta.
     */
    NODISCARD float GetHoverFillAlpha() const;

    ECaptionButtonKind Kind;
    IntVector2         ButtonSize;

    /** @brief The alpha the hover fill held when the hover last changed, which the fade starts from. */
    float HoverFadeStartAlpha;

    /** @brief When the hover last changed, which the fade is measured from. */
    uint64 HoverFadeStartCounter;
};

class APPLICATION_API FTitleBar final : public FCompoundElement
{
public:
    struct FDesc
    {
        FDesc()
            : Title()
            , Font(nullptr)
            , Icon()
            , bShowCaptionButtons(false)
            , Content(nullptr)
        {
        }

        /**
         * @brief Sets the caption text.
         *
         * @param InTitle The title to show.
         * @return This desc, so the setters can be chained.
         */
        FORCEINLINE FDesc& SetTitle(const String& InTitle)
        {
            Title = InTitle;
            return *this;
        }

        /**
         * @brief Sets the face the caption text is measured and drawn with.
         *
         * @param InFont The face to use.
         * @return This desc, so the setters can be chained.
         */
        FORCEINLINE FDesc& SetFont(const TSharedPtr<IFontFace>& InFont)
        {
            Font = InFont;
            return *this;
        }

        /**
         * @brief Sets what lives in the caption beside the title.
         *
         * @param InContent The element to place there.
         * @return This desc, so the setters can be chained.
         */
        FORCEINLINE FDesc& SetContent(const TSharedPtr<FVisualElement>& InContent)
        {
            Content = InContent;
            return *this;
        }

        /** @brief The caption text, drawn to the right of the icon. */
        String Title;

        /** @brief The face the caption text is measured and drawn with. */
        TSharedPtr<IFontFace> Font;

        /** @brief Drawn at the leading edge, and reserves no space when it holds no texture. */
        FUIBrush Icon;

        /** @brief Draw our own minimize, maximize and close. False on macOS, where the OS draws them. */
        bool bShowCaptionButtons : 1;

        /** @brief A FMenuBar, a FTabStrip, or anything else living in the caption. */
        TSharedPtr<FVisualElement> Content;
    };

public:
    static TSharedPtr<FTitleBar> Create(const FDesc& Desc);

public:
    FTitleBar();
    virtual ~FTitleBar();

    /**
     * @brief Fills the caption row, which cannot happen before the bar has a shared reference.
     *
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    // FVisualElement Interface
    virtual IntVector2 PrepareDesiredSize() override;
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual void OnArrange(const FRectangle& AllottedBounds) override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;

    /**
     * @brief Replaces the caption text.
     *
     * @param InTitle The title to show.
     */
    void SetTitle(const String& InTitle);

    /**
     * @brief Places a tab strip (or anything else) in the caption in place of the title text.
     *
     * @param InContent The element to host, or null to restore the title.
     */
    void SetLeadingContent(const TSharedPtr<FVisualElement>& InContent);

    /** @return What SetLeadingContent last placed, or null when the title is showing. */
    NODISCARD FORCEINLINE const TSharedPtr<FVisualElement>& GetLeadingContent() const
    {
        return LeadingContent;
    }

    /** @return The caption text. */
    NODISCARD FORCEINLINE const String& GetTitle() const
    {
        return Title;
    }

    /**
     * @brief Gets what the platform last said it contributes to the caption.
     *
     * @return The metrics read in the last measure, which are the defaults until the window is resolved.
     */
    NODISCARD FORCEINLINE const FWindowTitleBarMetrics& GetMetrics() const
    {
        return Metrics;
    }

    /**
     * @brief Gets the regions published to the platform in the last arrange.
     *
     * @return The caption, interactive and maximize rectangles, which are empty before the first arrange.
     */
    NODISCARD FORCEINLINE const FWindowTitleBarRegions& GetRegions() const
    {
        return Regions;
    }

    /**
     * @brief Gets the window command buttons the bar draws itself.
     *
     * @return Minimize, maximize and close in that order, or nothing when the desc turned them off.
     */
    NODISCARD FORCEINLINE const TArray<TSharedPtr<FCaptionButton>>& GetCaptionButtons() const
    {
        return CaptionButtons;
    }

private:
    void RefreshMetrics();
    void PublishRegions();
    void GatherInteractiveRects(const TSharedPtr<FVisualElement>& Element, TArray<FWindowRect>& OutRects) const;

    FWindowTitleBarMetrics             Metrics;
    FWindowTitleBarRegions             Regions;
    String                             Title;
    FUIBrush                           Icon;
    bool                               bShowCaptionButtons;
    TWeakPtr<FWindow>                  OwningWindow;
    TSharedPtr<FHorizontalBox>         Panel;
    TSharedPtr<FSpacer>                LeadingSpacer;
    TSharedPtr<FSpacer>                IconSpacer;
    TSharedPtr<FOverlay>               LeadingHost;
    int32                              LeadingHostSlotIndex;
    int32                              FlexibleSpacerSlotIndex;
    TSharedPtr<FSpacer>                TrailingSpacer;
    TSharedPtr<FTextBlock>             TitleLabel;
    TSharedPtr<FVisualElement>         LeadingContent;
    TSharedPtr<FHorizontalBox>         CaptionButtonRow;
    TArray<TSharedPtr<FCaptionButton>> CaptionButtons;
};
