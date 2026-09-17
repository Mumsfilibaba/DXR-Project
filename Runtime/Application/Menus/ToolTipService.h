#pragma once
#include "Core/Containers/String.h"
#include "Core/Containers/UniquePtr.h"
#include "Application/Elements/CompoundElement.h"
#include "Application/Elements/Window.h"
#include "Application/Text/IFontFace.h"

enum class EToolTipPlacement : uint8
{
    /** @brief Just below and right of the cursor, and it moves with it. A hint about a point. */
    FollowCursor,

    /** @brief Under the element that asked for it, left edges flush. A hint about a control. */
    BelowAnchor,

    /** @brief Right of the element that asked for it, top edges flush, flipping to its left when there is no room. */
    RightOfAnchor,
};

class APPLICATION_API FToolTip final : public FCompoundElement
{
public:
    static TSharedPtr<FToolTip> Create(const String& InText, const TSharedPtr<IFontFace>& InFont);

public:
    FToolTip();
    virtual ~FToolTip();

    // FVisualElement Interface
    virtual IntVector2 ComputeDesiredSize() const override;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override;
    virtual void SetOuterCornerRadius(float InCornerRadius) override;

    /**
     * @brief Sets the text the tip shows.
     *
     * @param InText The text to show.
     */
    void SetText(const String& InText);

    /** @return The text the tip shows. */
    NODISCARD FORCEINLINE const String& GetText() const
    {
        return Text;
    }

private:
    String                Text;
    TSharedPtr<IFontFace> Font;
    float                 CornerRadius;
};

class APPLICATION_API FToolTipHost final : public FCompoundElement
{
public:

    /**
     * @brief Wraps an element in a plain text tip.
     *
     * @param InContent   The element the tip describes.
     * @param InText      The text to show.
     * @param InFont      The face to draw it with.
     * @param InPlacement Whether the tip follows the cursor or sits under the element.
     * @return The wrapper, which takes the place of the element it was handed.
     */
    static TSharedPtr<FToolTipHost> Create(
        const TSharedPtr<FVisualElement>& InContent,
        const String&                     InText,
        const TSharedPtr<IFontFace>&      InFont,
        EToolTipPlacement                 InPlacement = EToolTipPlacement::FollowCursor);

public:
    FToolTipHost();
    virtual ~FToolTipHost();

    // FVisualElement Interface
    virtual FEventResponse OnMouseEntered(const FCursorEvent& CursorEvent) override;
    virtual FEventResponse OnMouseLeft(const FCursorEvent& CursorEvent) override;

    /**
     * @brief Sets the text the tip shows.
     *
     * @param InText The text to show.
     */
    void SetToolTipText(const String& InText);

private:
    String                Text;
    TSharedPtr<IFontFace> Font;
    EToolTipPlacement     Placement;
};

class APPLICATION_API FToolTipService
{
public:

    /** @brief How long the cursor rests before a tip appears, in seconds. */
    static constexpr float DefaultDelay = 0.5f;

    /** @brief The gap between the cursor and a following tip, in pixels. */
    static constexpr int32 CursorOffset = 16;

    /** @brief The gap between the element a tip describes and the tip itself, in pixels. */
    static constexpr int32 AnchorGap = 2;

    /**
     * @brief Gets the process-wide service, which is what every element requests a tip through.
     *
     * @return The service, created on the first call.
     */
    NODISCARD static FToolTipService& Get();

    /** @brief Hides the tip and drops the process-wide service, which a host calls before the application goes. */
    static void Shutdown();

public:
    FToolTipService();
    ~FToolTipService();

    FToolTipService(const FToolTipService&) = delete;
    FToolTipService& operator=(const FToolTipService&) = delete;

    /**
     * @brief Asks for a tip once the cursor has rested on an element, replacing any other request.
     *
     * @param Owner        The element the tip describes, which is what cancels it when the cursor leaves.
     * @param Content      The element to show.
     * @param Placement    Whether the tip follows the cursor or sits under the element.
     * @param DelaySeconds How long the cursor has to rest before it appears.
     * @param AnchorBounds The rectangle to place the tip flush against, in screen coordinates. An empty
     *                     one measures the owner instead and leaves AnchorGap between the two, which is
     *                     what a caller wanting the element it hovered passes. A row inside a list gives
     *                     the list's rectangle here, so the tip clears the whole list rather than landing
     *                     on top of it, and grows the rectangle itself when it wants a gap.
     */
    void RequestToolTip(
        const TSharedPtr<FVisualElement>& Owner,
        const TSharedPtr<FVisualElement>& Content,
        EToolTipPlacement                 Placement    = EToolTipPlacement::FollowCursor,
        float                             DelaySeconds = DefaultDelay,
        const FRectangle&                 AnchorBounds = FRectangle());

    /**
     * @brief Asks for a plain text tip, which is what almost every caller wants.
     *
     * @param Owner        The element the tip describes.
     * @param Text         The text to show.
     * @param Font         The face to draw it with.
     * @param Placement    Whether the tip follows the cursor or sits under the element.
     * @param DelaySeconds How long the cursor has to rest before it appears.
     */
    void RequestTextToolTip(
        const TSharedPtr<FVisualElement>& Owner,
        const String&                     Text,
        const TSharedPtr<IFontFace>&      Font,
        EToolTipPlacement                 Placement    = EToolTipPlacement::FollowCursor,
        float                             DelaySeconds = DefaultDelay);

    /**
     * @brief Drops a request and hides the tip, which does nothing when another element owns it.
     *
     * @param Owner The element that asked for the tip.
     */
    void CancelToolTip(const TSharedPtr<FVisualElement>& Owner);

    /**
     * @brief Reports where the cursor is, which restarts the delay and moves a following tip.
     *
     * @param ScreenPosition The cursor position, in screen coordinates.
     */
    void NotifyCursorMoved(const IntVector2& ScreenPosition);

    /** @brief Hides the tip and forgets the request, which a click or a key press does. */
    void DismissToolTip();

    /** @return True while a tip window is on screen. */
    NODISCARD bool IsShowing() const;

    /** @return True while a tip has been asked for and is waiting out its delay, so it is not up yet. */
    NODISCARD bool IsPending() const;

    /** @return The element the shown or pending tip describes, or null when there is neither. */
    NODISCARD FORCEINLINE const TSharedPtr<FVisualElement>& GetOwner() const
    {
        return Owner;
    }

    /** @return The window the tip was given because it did not fit its host, or null in every other case. */
    NODISCARD FORCEINLINE const TSharedPtr<FWindow>& GetToolTipWindow() const
    {
        return ToolTipWindow;
    }

    /** @return Where the shown tip ended up, in screen coordinates, or an empty rectangle when none is up. */
    NODISCARD FORCEINLINE const FRectangle& GetToolTipBounds() const
    {
        return ToolTipBounds;
    }

    /**
     * @brief Advances the delay, showing whatever has waited long enough.
     *
     * @param DeltaSeconds Time since the last call.
     */
    void Tick(float DeltaSeconds);

private:
    void ShowToolTip();
    void MoveToolTip();

    NODISCARD FRectangle ResolveBounds(const IntVector2& ToolTipSize) const;
    NODISCARD FRectangle ResolveAnchorBounds() const;
    NODISCARD bool HasAnchorBoundsOverride() const;

    TSharedPtr<FVisualElement> Owner;
    TSharedPtr<FVisualElement> Content;
    TSharedPtr<FWindow>        HostWindow;
    TSharedPtr<FWindow>        ToolTipWindow;
    EToolTipPlacement          Placement;
    FRectangle                 AnchorBounds;
    FRectangle                 ClampArea;
    FRectangle                 ToolTipBounds;
    IntVector2                 CursorPosition;
    float                      RequestedDelay;
    float                      RemainingSeconds;
    bool                       bIsShowing;

    static TUniquePtr<FToolTipService> ToolTipService;
};
