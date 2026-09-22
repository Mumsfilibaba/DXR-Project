#pragma once
#include "Core/Containers/String.h"
#include "Core/Delegates/Delegate.h"
#include "CoreApplication/PlatformInterface/IPlatformWindow.h"
#include "Application/Draw/DrawTypes.h"
#include "Application/Elements/VisualElement.h"

class FMenuHost;

/** @brief Delegate called when the window is moved. */
DECLARE_DELEGATE(FOnWindowMoved, const IntVector2&);

/** @brief Delegate called when the window is resized. */
DECLARE_DELEGATE(FOnWindowResized, const IntVector2&);

/** @brief Delegate called when the window is closed. */
DECLARE_DELEGATE(FOnWindowClosed);

/** @brief Delegate called when the window focus state changes. */
DECLARE_DELEGATE(FOnWindowFocusChanged);

/** @brief Called to append the draw commands of something painting above the content pass rather than in it. */
DECLARE_RETURN_DELEGATE(FOnDeferredPaint, int32, FDrawCommandList& /*OutCommandList*/, int32 /*LayerId*/);

class APPLICATION_API FWindow final : public FVisualElement
{
public:
    struct FDesc
    {
        /** @brief The title of the window. */
        String Title;

        /** @brief Optional parent/owner window (used for owned popup/tool windows). */
        TSharedPtr<FWindow> ParentWindow = nullptr;

        /** @brief The size of the window (width, height). */
        IntVector2 Size;

        /** @brief The position of the window (x, y). */
        IntVector2 Position;

        /** @brief Style flags for the window. */
        EWindowStyleFlags StyleFlags = EWindowStyleFlags::Default;

        /** @brief Should the window be activated when we show the window. */
        bool bActivateOnShow : 1 = true;

        /** @brief False makes the window transparent to hit-testing, so input resolves to whatever is behind it. */
        bool bAcceptsInput : 1 = true;

        /**
         * @brief False leaves the window hidden once it is created, so it can be shown when its first frame
         * has reached the screen rather than while its surface is still empty.
         */
        bool bShowOnCreate : 1 = true;

        /**
         * @brief True when something other than the application renderer owns this window's swap chain, which
         * an ImGui viewport does. A second swap chain on one native window fights the first for the surface,
         * so the renderer leaves such a window alone.
         */
        bool bHasExternalSurface : 1 = false;
    };

public:
    static TSharedPtr<FWindow> Create(const FDesc& Desc);

public:
    
    FWindow();
    virtual ~FWindow();

    // FVisualElement Interface
    virtual IntVector2 PrepareDesiredSize() override final;
    virtual void Tick(const FRectangle& AssignedBounds) override final;
    virtual bool IsWindow() const override final;
    virtual int32 OnDraw(const FDrawGeometry& AllottedGeometry, FDrawCommandList& OutCommandList, int32 LayerId) const override final;
    virtual void GetChildren(TArray<TSharedPtr<FVisualElement>>& OutChildren) const override final;
    virtual void FindChildrenContainingPoint(const IntVector2& ClientPosition, FElementPath& OutParentElements) override final;
    virtual bool SupportsKeyboardFocus() const override final;

    /**
     * @brief Initializes the window with the specified parameters.
     * 
     * @param Desc Initialization parameters.
     */
    void Initialize(const FDesc& Desc);

    /**
     * @brief Sets a delegate to be called when the window is closed.
     * 
     * @param InOnWindowClosed The delegate to set.
     */
    void SetOnWindowClosed(const FOnWindowClosed& InOnWindowClosed);

    /**
     * @brief Sets a delegate to be called when the window is moved.
     * 
     * @param InOnWindowMoved The delegate to set.
     */
    void SetOnWindowMoved(const FOnWindowMoved& InOnWindowMoved);

    /**
     * @brief Sets a delegate to be called when the window is resized.
     * 
     * @param InOnWindowResized The delegate to set.
     */
    void SetOnWindowResized(const FOnWindowResized& InOnWindowResized);

    /**
     * @brief Sets a delegate to be called when the window focus is changed.
     * 
     * @param InOnWindowFocusChanged The delegate to set.
     */
    void SetOnWindowFocusChanged(const FOnWindowFocusChanged& InOnWindowFocusChanged);

    /** @brief Called by FApplication when the platform window is destroyed. */
    void OnWindowDestroyed();

    /**
     * @brief Called by FApplication when the window is switched to or away from, which the platforms
     * agree on for a window gaining or losing focus and disagree on in the corners around it.
     *
     * @param bIsActive True if the window is now active; false if it is inactive.
     */
    void OnWindowFocusChanged(bool bIsActive);

    /**
     * @brief Called when the platform window is resized.
     * 
     * @param InSize The new size of the window.
     */
    void OnWindowResize(const IntVector2& InSize);

    /**
     * @brief Called when the platform window is moved.
     * 
     * @param InPosition The new position of the window.
     */
    void OnWindowMoved(const IntVector2& InPosition);

    /**
     * @brief Resizes the window, and the platform window with it.
     *
     * @param InSize The new size for the window.
     */
    void Resize(const IntVector2& InSize);

    /**
     * @brief Moves the window, and the platform window with it.
     *
     * @param InPosition The new position for the window.
     */
    void MoveTo(const IntVector2& InPosition);
        
    /**
     * @brief Sets the cached window size without modifying the platform window size.
     * 
     * @param InSize The new cached size.
     */
    void SetSize(const IntVector2& InSize);
    
    /**
     * @brief Sets the cached window position without modifying the platform window position.
     * 
     * @param InPosition The new cached position.
     */
    void SetPosition(const IntVector2& InPosition);

    /**
     * @brief Gets the current cached window size.
     * 
     * @return The size of the window.
     */
    IntVector2 GetSize() const;
    
    /**
     * @brief Gets the current cached window position.
     * 
     * @return The position of the window.
     */
    IntVector2 GetPosition() const;

    /**
     * @brief Gets the current cached window width.
     * 
     * @return The width of the window.
     */
    uint32 GetWidth() const;
    
    /**
     * @brief Gets the current cached window height.
     * 
     * @return The height of the window.
     */
    uint32 GetHeight() const;

    /**
     * @brief Gets the current overlay element.
     * 
     * @return A shared pointer to the overlay element.
     */
    TSharedPtr<FVisualElement> GetOverlay() const;
    
    /**
     * @brief Gets the current content element.
     * 
     * @return A shared pointer to the content element.
     */
    TSharedPtr<FVisualElement> GetContent() const;

    /**
     * @brief Sets the overlay element, which is offered every event before the content is.
     *
     * @param InOverlay The overlay element to set.
     */
    void SetOverlay(const TSharedPtr<FVisualElement>& InOverlay);
    
    /**
     * @brief Sets the content element.
     * 
     * @param InContent The content element to set.
     */
    void SetContent(const TSharedPtr<FVisualElement>& InContent);

    /** @return The element open menus are drawn in, or null while none has been opened in this window. */
    NODISCARD TSharedPtr<FMenuHost> GetMenuHost() const;

    /**
     * @brief Gets the element open menus are drawn in, which sits above the overlay and takes the cursor
     * before it does.
     *
     * @return The host, created on the first call.
     */
    NODISCARD TSharedPtr<FMenuHost> GetOrCreateMenuHost();

    /**
     * @brief Asks for a subtree to be painted after the whole content pass, above its siblings. Called from
     * inside a draw, and the queue is emptied by the drain that follows it, so it is asked for every frame.
     *
     * @param Element  The element to paint.
     * @param Geometry The geometry to paint it with, in the same space the content pass used.
     */
    void QueueDeferredPainting(const TSharedPtr<FVisualElement>& Element, const FDrawGeometry& Geometry) const;

    /**
     * @brief Asks for draw commands to be appended after the whole content pass, for something that paints
     * above the tree without being in it.
     *
     * @param OnPaint What to call, handed the layer to draw on and returning the highest layer it used.
     */
    void QueueDeferredPainting(const FOnDeferredPaint& OnPaint) const;

    /**
     * @brief Paints everything queued during the content pass, in the order it was asked for, and empties
     * the queue.
     *
     * @param OutCommandList The list to append to.
     * @param LayerId        The layer the first of them draws on.
     * @return The highest layer any of them drew on.
     */
    int32 PaintDeferred(FDrawCommandList& OutCommandList, int32 LayerId) const;

    /**
     * @brief Shows the window, optionally setting focus to it.
     *
     * @param bFocus If true, sets focus to this window when displaying it.
     */
    void Show(bool bFocus = true);
    
    /** @brief Minimizes the window. */
    void Minimize();
    
    /** @brief Maximizes the window. */
    void Maximize();
    
    /** @brief Restores the position and size a minimized or maximized window had before. */
    void Restore();

    /**
     * @brief Checks if the window's platform window is the current active window.
     * 
     * @return True if the window is active; false otherwise.
     */
    bool IsActive() const;
    
    /**
     * @brief Checks if the window's platform window is currently minimized.
     * 
     * @return True if the window is minimized; false otherwise.
     */
    bool IsMinimized() const;
    
    /**
     * @brief Checks if the window's platform window is currently maximized.
     * 
     * @return True if the window is maximized; false otherwise.
     */
    bool IsMaximized() const;

    /**
     * @brief Gets the DPI scale of the monitor where this window is currently displayed.
     * 
     * @return The DPI scale factor.
     */
    float GetWindowDPIScale() const;

    /**
     * @brief Measures what the platform contributes to a custom title bar. The values follow the window's
     * DPI and style, so they are re-queried rather than cached.
     *
     * @return The metrics for this window, all zero when it has no custom title bar.
     */
    FWindowTitleBarMetrics GetTitleBarMetrics() const;

    /**
     * @brief Publishes the regions of the window that behave like a title bar. The platform answers OS
     * hit-tests from the most recently published set, so this is called every frame by whichever element
     * draws the title bar.
     *
     * @param InRegions The regions, in window-relative coordinates, with the origin at the top-left.
     */
    void SetTitleBarRegions(const FWindowTitleBarRegions& InRegions);

    /**
     * @brief Sets the window title, and the platform window's text with it.
     *
     * @param InTitle The new title for the window.
     */
    void SetTitle(const String& InTitle);
    
    /**
     * @brief Sets the window style flags, and the platform window's style with them.
     *
     * @param InStyleFlags The new style flags.
     */
    void SetStyle(EWindowStyleFlags InStyleFlags);
    
    /**
     * @brief Sets the platform window, re-reading the cached shape and title from it.
     *
     * @param InPlatformWindow A shared reference to the new platform window.
     */
    void SetPlatformWindow(const TSharedRef<IPlatformWindow>& InPlatformWindow);
    
    /** @brief Sets focus to this window's platform window. */
    void SetFocus();
    
    /**
     * @brief Sets the opacity of the platform window, remembering it so a window created later still gets it.
     *
     * @param Alpha The opacity value between 0.0 (fully transparent) and 1.0 (fully opaque).
     */
    void SetOpacity(float Alpha);

    /** @return The opacity last asked for, whether or not a platform window existed to receive it. */
    NODISCARD float GetOpacity() const;

    /**
     * @brief Controls whether the window takes part in hit-testing.
     *
     * @param bInAcceptsInput False makes the window click-through, so the OS resolves input to whatever is behind it.
     */
    void SetAcceptsInput(bool bInAcceptsInput);

    /**
     * @brief Gets the platform window.
     * 
     * @return A shared reference to the platform window.
     */
    TSharedRef<IPlatformWindow> GetPlatformWindow() { return PlatformWindow; }

    /**
     * @brief Gets the platform window (const version).
     * 
     * @return A shared reference to the platform window.
     */
    TSharedRef<const IPlatformWindow> GetPlatformWindow() const { return PlatformWindow; }

    /**
     * @brief Gets the window title.
     * 
     * @return The title of the window.
     */
    const String& GetTitle() const
    {
        return Title;
    }

    /**
     * @brief Gets the window style flags.
     * 
     * @return The style flags of the window.
     */
    EWindowStyleFlags GetStyle() const
    {
        return StyleFlags;
    }

    /**
     * @brief Retrieves the parent/owner window of this window, if any.
     * 
     * @return The parent window element, or nullptr if there is no parent.
     */
    TSharedPtr<FWindow> GetParentWindow() const
    {
        return ParentWindow;
    }

    /**
     * @brief Gets the window style flags.
     * 
     * @return The style flags of the window.
     */
    bool ActivateOnShow() const
    {
        return bActivateOnShow;
    }

    /**
     * @brief Gets whether the window takes part in hit-testing.
     * 
     * @return False if the window is click-through.
     */
    bool GetAcceptsInput() const
    {
        return bAcceptsInput;
    }

    /** @return True when something other than the application renderer owns this window's swap chain. */
    NODISCARD FORCEINLINE bool HasExternalSurface() const
    {
        return bHasExternalSurface;
    }

    /** @return False when the window is left hidden on creation, waiting to be shown once it has content. */
    NODISCARD FORCEINLINE bool ShowOnCreate() const
    {
        return bShowOnCreate;
    }

    /** @return True when the window has been resized since it was last arranged, so its content is the wrong size. */
    NODISCARD FORCEINLINE bool IsLayoutStale() const
    {
        return bLayoutIsStale;
    }

    /** @brief Marks the window as arranged at its current size. */
    FORCEINLINE void ClearLayoutIsStale()
    {
        bLayoutIsStale = false;
    }

private:
    struct FDeferredPaint
    {
        TSharedPtr<FVisualElement> Element;
        FDrawGeometry              Geometry;
        FOnDeferredPaint           OnPaint;
    };

    String                         Title;
    FOnWindowClosed                OnWindowClosedDelegate;
    FOnWindowMoved                 OnWindowMovedDelegate;
    FOnWindowResized               OnWindowResizedDelegate;
    FOnWindowFocusChanged          OnWindowFocusChangedDelegate;
    IntVector2                     CachedPosition;
    IntVector2                     CachedSize;
    float                          CachedOpacity;
    EWindowStyleFlags              StyleFlags;
    bool                           bActivateOnShow : 1;
    bool                           bAcceptsInput : 1;
    bool                           bShowOnCreate : 1;
    bool                           bHasExternalSurface : 1;
    bool                           bLayoutIsStale : 1;
    bool                           bCachedIsMaximized : 1;
    TSharedPtr<FVisualElement>     Overlay;
    TSharedPtr<FMenuHost>          MenuHost;
    TSharedPtr<FVisualElement>     Content;
    TSharedRef<IPlatformWindow>    PlatformWindow;
    TSharedPtr<FWindow>            ParentWindow;
    mutable TArray<FDeferredPaint> DeferredPaints;
};
