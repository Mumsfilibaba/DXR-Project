#pragma once
#include "Core/Containers/String.h"
#include "Core/Delegates/Delegate.h"
#include "CoreApplication/PlatformInterface/IPlatformWindow.h"
#include "Application/Elements/VisualElement.h"

/** @brief Delegate called when the window is moved. */
DECLARE_DELEGATE(FOnWindowMoved, const IntVector2&);

/** @brief Delegate called when the window is resized. */
DECLARE_DELEGATE(FOnWindowResized, const IntVector2&);

/** @brief Delegate called when the window is closed. */
DECLARE_DELEGATE(FOnWindowClosed);

/** @brief Delegate called when the window focus state changes. */
DECLARE_DELEGATE(FOnWindowFocusChanged);

class APPLICATION_API FWindow final : public FVisualElement
{
public:

    struct FDesc
    {
        /** @brief Default constructor initializes default window parameters. */
        FDesc()
            : Title()
            , ParentWindow(nullptr)
            , Size()
            , Position()
            , StyleFlags(EWindowStyleFlags::Default)
            , bActivateOnShow(true)
            , bAcceptsInput(true)
        {
        }

        /** @brief The title of the window. */
        String Title;     
        
        /** @brief Optional parent/owner window (used for owned popup/tool windows). */
        TSharedPtr<FWindow> ParentWindow;

        /** @brief The size of the window (width, height). */
        IntVector2 Size;      
        
        /** @brief The position of the window (x, y). */
        IntVector2 Position;  
        
        /** @brief Style flags for the window. */
        EWindowStyleFlags StyleFlags;

        /** @brief Should the window be activated when we show the window. */
        bool bActivateOnShow;

        /** @brief False makes the window transparent to hit-testing, so input resolves to whatever is behind it. */
        bool bAcceptsInput;
    };

public:
    static TSharedPtr<FWindow> Create(const FDesc& Desc);

public:
    
    FWindow();
    virtual ~FWindow();

    // FVisualElement Interface
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
     * @brief Sets the opacity of the platform window.
     * 
     * @param Alpha The opacity value between 0.0 (fully transparent) and 1.0 (fully opaque).
     */
    void SetOpacity(float Alpha);

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

private:
    String                      Title;
    FOnWindowClosed             OnWindowClosedDelegate;
    FOnWindowMoved              OnWindowMovedDelegate;
    FOnWindowResized            OnWindowResizedDelegate;
    FOnWindowFocusChanged       OnWindowFocusChangedDelegate;
    IntVector2                  CachedPosition;
    IntVector2                  CachedSize;
    EWindowStyleFlags           StyleFlags;
    bool                        bActivateOnShow;
    bool                        bAcceptsInput;
    TSharedPtr<FVisualElement>  Overlay;
    TSharedPtr<FVisualElement>  Content;
    TSharedRef<IPlatformWindow> PlatformWindow;
    TSharedPtr<FWindow>         ParentWindow;
};
