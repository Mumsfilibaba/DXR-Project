#pragma once
#include "Widget.h"
#include "Core/Containers/String.h"
#include "Core/Delegates/Delegate.h"
#include "CoreApplication/Generic/GenericWindow.h"

class FWindow;

/** Delegate called when the window is moved. */
DECLARE_DELEGATE(FOnWindowMoved, const FIntVector2&);

/** Delegate called when the window is resized. */
DECLARE_DELEGATE(FOnWindowResized, const FIntVector2&);

/** Delegate called when the window is closed. */
DECLARE_DELEGATE(FOnWindowClosed);

/** Delegate called when the window activation state changes. */
DECLARE_DELEGATE(FOnWindowActivationChanged);

/**
 * @brief Represents a window in the application, managing its content, overlay, and platform-specific window.
 */

class APPLICATION_API FWindow : public FWidget
{
public:

    /**
     * @brief Initialization parameters for an FWindow.
     */

    struct FInitializer
    {
        /**
         * @brief Default constructor initializes default window parameters.
         */

        FInitializer()
            : Title()
            , Size()
            , Position()
            , StyleFlags(EWindowStyleFlags::Default)
        {
        }

        FString           Title;       /**< @brief The title of the window. */
        FIntVector2       Size;        /**< @brief The size of the window (width, height). */
        FIntVector2       Position;    /**< @brief The position of the window (x, y). */
        EWindowStyleFlags StyleFlags;  /**< @brief Style flags for the window. */
    };

    /**
     * @brief Constructs an FWindow object.
     */
    FWindow();

    /**
     * @brief Destructor for FWindow.
     */
    virtual ~FWindow();

    /**
     * @brief Initializes the window with the specified parameters.
     * @param Initializer Initialization parameters.
     */
    void Initialize(const FInitializer& Initializer);

    /**
     * @brief Updates the window each frame.
     */
    virtual void Tick() override final;

    /**
     * @brief Checks if this widget is a window.
     * @return True if it is a window; false otherwise.
     */
    virtual bool IsWindow() const override final;

    /**
     * @brief Finds child widgets that contain the specified point.
     * @param Point The point to check.
     * @param OutParentWidgets Output parameter to store the widget path.
     */
    virtual void FindChildrenContainingPoint(const FIntVector2& Point, FWidgetPath& OutParentWidgets) override final;

    /**
     * @brief Sets a delegate to be called when the window is closed.
     * @param InOnWindowClosed The delegate to set.
     */
    void SetOnWindowClosed(const FOnWindowClosed& InOnWindowClosed);

    /**
     * @brief Sets a delegate to be called when the window is moved.
     * @param InOnWindowMoved The delegate to set.
     */
    void SetOnWindowMoved(const FOnWindowMoved& InOnWindowMoved);

    /**
     * @brief Sets a delegate to be called when the window is resized.
     * @param InOnWindowResized The delegate to set.
     */
    void SetOnWindowResized(const FOnWindowResized& InOnWindowResized);

    /**
     * @brief Called when the platform window is destroyed.
     * 
     * This function is called from the FApplicationInstance when the platform window is destroyed.
     */
    void OnWindowDestroyed();

    /**
     * @brief Called when the window activation state changes.
     * 
     * This function is called from the FApplicationInstance when the platform window's activation state changes.
     * This means that the user switches windows, changing focus, or the entire application loses focus.
     * The behavior may vary depending on the platform, but generally, the function is called when the window loses or gains focus.
     * @param bIsActive True if the window is now active; false if it is inactive.
     */
    void OnWindowActivationChanged(bool bIsActive);

    /**
     * @brief Called when the platform window is resized.
     * @param InSize The new size of the window.
     */
    void OnWindowResize(const FIntVector2& InSize);

    /**
     * @brief Called when the platform window is moved.
     * @param InPosition The new position of the window.
     */
    void OnWindowMoved(const FIntVector2& InPosition);

    /**
     * @brief Resizes the window to a new size.
     * 
     * This function also sets the platform window's size and updates the cached size.
     * @param InSize The new size for the window.
     */
    void Resize(const FIntVector2& InSize);

    /**
     * @brief Moves the window to a new position.
     * 
     * This function also sets the platform window's position and updates the cached position.
     * @param InPosition The new position for the window.
     */
    void MoveTo(const FIntVector2& InPosition);
        
    /**
     * @brief Sets the cached window size without modifying the platform window size.
     * @param InSize The new cached size.
     */
    void SetSize(const FIntVector2& InSize);
    
    /**
     * @brief Sets the cached window position without modifying the platform window position.
     * @param InPosition The new cached position.
     */
    void SetPosition(const FIntVector2& InPosition);

    /**
     * @brief Gets the current cached window size.
     * @return The size of the window.
     */
    FIntVector2 GetSize() const;
    
    /**
     * @brief Gets the current cached window position.
     * @return The position of the window.
     */
    FIntVector2 GetPosition() const;

    /**
     * @brief Gets the current cached window width.
     * @return The width of the window.
     */
    uint32 GetWidth() const;
    
    /**
     * @brief Gets the current cached window height.
     * @return The height of the window.
     */
    uint32 GetHeight() const;

    /**
     * @brief Gets the current overlay widget.
     * @return A shared pointer to the overlay widget.
     */
    TSharedPtr<FWidget> GetOverlay() const;
    
    /**
     * @brief Gets the current content widget.
     * @return A shared pointer to the content widget.
     */
    TSharedPtr<FWidget> GetContent() const;

    /**
     * @brief Sets the overlay widget.
     * 
     * This widget will receive events before the content widget, allowing the overlay to respond to events first.
     * @param InOverlay The overlay widget to set.
     */
    void SetOverlay(const TSharedPtr<FWidget>& InOverlay);
    
    /**
     * @brief Sets the content widget.
     * @param InContent The content widget to set.
     */
    void SetContent(const TSharedPtr<FWidget>& InContent);

    /**
     * @brief Shows the window, optionally setting focus to it.
     * 
     * This only occurs if there is a valid platform window.
     * @param bFocus If true, sets focus to this window when displaying it.
     */
    void Show(bool bFocus = true);
    
    /**
     * @brief Minimizes the window.
     * 
     * This only occurs if there is a valid platform window.
     */
    void Minimize();
    
    /**
     * @brief Maximizes the window.
     * 
     * This only occurs if there is a valid platform window.
     */
    void Maximize();
    
    /**
     * @brief Restores the window to its previous state.
     * 
     * Restores the window's position and size if it has been minimized or maximized.
     */
    void Restore();

    /**
     * @brief Checks if the window's platform window is the current active window.
     * @return True if the window is active; false otherwise.
     */
    bool IsActive() const;
    
    /**
     * @brief Checks if the window's platform window is currently minimized.
     * @return True if the window is minimized; false otherwise.
     */
    bool IsMinimized() const;
    
    /**
     * @brief Checks if the window's platform window is currently maximized.
     * @return True if the window is maximized; false otherwise.
     */
    bool IsMaximized() const;

    /**
     * @brief Gets the DPI scale of the monitor where this window is currently displayed.
     * @return The DPI scale factor.
     */
    float GetWindowDPIScale() const;

    /**
     * @brief Sets the window title.
     * 
     * Updates the cached title and sets the platform window's text if there is a valid platform window.
     * @param InTitle The new title for the window.
     */
    void SetTitle(const FString& InTitle);
    
    /**
     * @brief Sets the window style flags.
     * 
     * Updates the cached style flags and sets the platform window's style if there is a valid platform window.
     * @param InStyleFlags The new style flags.
     */
    void SetStyle(EWindowStyleFlags InStyleFlags);
    
    /**
     * @brief Sets the platform window.
     * 
     * Updates the cached variables based on the new platform window.
     * @param InPlatformWindow A shared reference to the new platform window.
     */
    void SetPlatformWindow(const TSharedRef<FGenericWindow>& InPlatformWindow);
    
    /**
     * @brief Sets focus to this window's platform window.
     */
    void SetFocus();
    
    /**
     * @brief Sets the opacity of the platform window.
     * @param Alpha The opacity value between 0.0 (fully transparent) and 1.0 (fully opaque).
     */
    void SetOpacity(float Alpha);

    /**
     * @brief Gets the platform window.
     * @return A shared reference to the platform window.
     */
    TSharedRef<FGenericWindow> GetPlatformWindow() { return PlatformWindow; }

    /**
     * @brief Gets the platform window (const version).
     * @return A shared reference to the platform window.
     */
    TSharedRef<const FGenericWindow> GetPlatformWindow() const { return PlatformWindow; }

    /**
     * @brief Gets the window title.
     * @return The title of the window.
     */
    const FString& GetTitle() const
    {
        return Title;
    }

    /**
     * @brief Gets the window style flags.
     * @return The style flags of the window.
     */
    EWindowStyleFlags GetStyle() const
    {
        return StyleFlags;
    }

private:

    /** @brief The title of the window. */
    FString Title;

    /** @brief Delegate called when the window is closed. */
    FOnWindowClosed OnWindowClosedDelegate;

    /** @brief Delegate called when the window is moved. */
    FOnWindowMoved OnWindowMovedDelegate;

    /** @brief Delegate called when the window is resized. */
    FOnWindowResized OnWindowResizedDelegate;

    /** @brief Delegate called when the window activation changes. */
    FOnWindowActivationChanged OnWindowActivationChangedDelegate;

    /** @brief Cached window position. */
    FIntVector2 CachedPosition;

    /** @brief Cached window size. */
    FIntVector2 CachedSize;

    /** @brief The overlay widget. */
    TSharedPtr<FWidget> Overlay;

    /** @brief The content widget. */
    TSharedPtr<FWidget> Content;

    /** @brief The platform-specific window. */
    TSharedRef<FGenericWindow> PlatformWindow;

    /** @brief Style flags of the window. */
    EWindowStyleFlags StyleFlags;
};
