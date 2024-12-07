#pragma once
#include "Core/RefCounted.h"
#include "Core/Math/IntVector2.h"
#include "Core/Containers/String.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Templates/TypeTraits.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FGenericWindow;

/**
 * @brief Flags representing window style options.
 */
enum class EWindowStyleFlags : uint16
{
    None          = 0,       /** @brief No style flags set. */
    Titled        = FLAG(1), /** @brief Window has a title bar. */
    Closable      = FLAG(2), /** @brief Window can be closed. */
    Minimizable   = FLAG(3), /** @brief Window can be minimized. */
    Maximizable   = FLAG(4), /** @brief Window can be maximized. */
    Resizable     = FLAG(5), /** @brief Window is resizable. */
    NoTaskBarIcon = FLAG(6), /** @brief Window does not show an icon in the taskbar. */
    TopMost       = FLAG(7), /** @brief Window stays above all non-topmost windows. */
    Opaque        = FLAG(8), /** @brief Window is opaque (non-transparent). */

    Default = Titled | Maximizable | Minimizable | Resizable | Closable | Opaque /** Default window style flags. */
};

ENUM_CLASS_OPERATORS(EWindowStyleFlags);

/**
 * @brief Represents the shape and position of a window.
 */

struct FWindowShape
{
    /**
     * @brief Default constructor initializes width and height to zero and position to (0,0).
     */
    FWindowShape()
        : Width(0)
        , Height(0)
        , Position(0, 0)
    {
    }

    /**
     * @brief Constructs a window shape with specified width and height, position defaults to (0,0).
     * 
     * @param InWidth Width of the window.
     * @param InHeight Height of the window.
     */
    FWindowShape(uint32 InWidth, uint32 InHeight)
        : Width(InWidth)
        , Height(InHeight)
        , Position(0, 0)
    {
    }

    /**
     * @brief Constructs a window shape with specified width, height, and position.
     * 
     * @param InWidth Width of the window.
     * @param InHeight Height of the window.
     * @param x X-coordinate of the window position.
     * @param y Y-coordinate of the window position.
     */
    FWindowShape(uint32 InWidth, uint32 InHeight, int32 x, int32 y)
        : Width(InWidth)
        , Height(InHeight)
        , Position(x, y)
    {
    }

    /**
     * @brief Checks if this window shape is equal to another.
     * 
     * @param Other The other window shape to compare.
     * @return True if equal, false otherwise.
     */
    bool operator==(const FWindowShape& Other) const
    {
        return Width == Other.Width && Height == Other.Height && Position == Other.Position;
    }

    /**
     * @brief Checks if this window shape is not equal to another.
     * 
     * @param Other The other window shape to compare.
     * @return True if not equal, false otherwise.
     */
    bool operator!=(const FWindowShape& Other) const
    {
        return !(*this == Other);
    }

    /** @brief Width of the window. */
    uint32 Width;

    /** @brief Height of the window. */
    uint32 Height;

    /** @brief Position of the window (x, y). */
    FIntVector2 Position;
};

/**
 * @brief Contains initialization parameters for a generic window.
 */

struct FGenericWindowInitializer
{
    /**
     * @brief Default constructor initializes default window parameters.
     */

    FGenericWindowInitializer()
        : Title()
        , Width(1280)
        , Height(720)
        , Position(0, 0)
        , Style(EWindowStyleFlags::Default)
        , ParentWindow(nullptr)
    {
    }

    /** @brief The title of the window. */
    FString Title;

    /** @brief The width of the window. */
    uint32 Width;

    /** @brief The height of the window. */
    uint32 Height;

    /** @brief The position of the window (x, y). */
    FIntVector2 Position;

    /** @brief Style flags for the window. */
    EWindowStyleFlags Style;

    /** @brief Pointer to the parent window, if any. */
    FGenericWindow* ParentWindow;
};

/**
 * @brief Represents a generic window interface for platform-specific implementations.
 */
class FGenericWindow : public FRefCounted
{
public:

    /**
     * @brief Virtual destructor for FGenericWindow.
     */
    virtual ~FGenericWindow() = default;

    /**
     * @brief Initializes the platform-specific window.
     * 
     * @param InInitializer Initialization parameters for the window.
     * @return True if initialization was successful, false otherwise.
     */
    virtual bool Initialize(const FGenericWindowInitializer& InInitializer) { return true; }

    /**
     * @brief Shows the window and optionally sets focus to it.
     * 
     * @param bFocus If true, sets focus to this window after showing it.
     */
    virtual void Show(bool bFocus = true) { }

    /**
     * @brief Minimizes the window.
     */
    virtual void Minimize() { }

    /**
     * @brief Maximizes the window.
     */
    virtual void Maximize() { }

    /**
     * @brief Destroys the platform-specific window. The FGenericWindow instance remains valid, but the platform handle becomes invalid.
     */
    virtual void Destroy() { }

    /**
     * @brief Restores the window from a minimized or maximized state to its previous state.
     */
    virtual void Restore() { }

    /**
     * @brief Toggles the window's fullscreen mode.
     */
    virtual void ToggleFullscreen() { }

    /**
     * @brief Checks if this is the current active window.
     * 
     * @return True if this window is active, false otherwise.
     */
    virtual bool IsActiveWindow() const { return false; }

    /**
     * @brief Sets the window position.
     * 
     * @param x The x-coordinate of the new window position.
     * @param y The y-coordinate of the new window position.
     */
    virtual void SetWindowPos(int32 x, int32 y) { }

    /**
     * @brief Checks if the window has a valid platform handle.
     * 
     * @return True if the platform handle is valid, false otherwise.
     */
    virtual bool IsValid() const { return false; }

    /**
     * @brief Checks if the window is currently minimized.
     * 
     * @return True if the window is minimized, false otherwise.
     */
    virtual bool IsMinimized() const { return false; }

    /**
     * @brief Checks if the window is currently maximized.
     * 
     * @return True if the window is maximized, false otherwise.
     */
    virtual bool IsMaximized() const { return false; }

    /**
     * @brief Checks if this window is a child window of the specified parent window.
     * 
     * @param ParentWindow A reference to the potential parent window.
     * @return True if this window is a child of the specified window, false otherwise.
     */
    virtual bool IsChildWindow(const TSharedRef<FGenericWindow>& ParentWindow) const { return false; }

    /**
     * @brief Sets focus to this window.
     */
    virtual void SetWindowFocus() { }

    /**
     * @brief Sets the title of the window.
     * 
     * @param Title The new title for the window.
     */
    virtual void SetTitle(const FString& Title) { }

    /**
     * @brief Retrieves the title of the window.
     * 
     * @param OutTitle String to store the window's title.
     */
    virtual void GetTitle(FString& OutTitle) const { }

    /**
     * @brief Sets the window's opacity.
     * 
     * @param Alpha Opacity value between 0.0 (fully transparent) and 1.0 (fully opaque).
     */
    virtual void SetWindowOpacity(float Alpha) { }

    /**
     * @brief Sets the shape of the window, optionally moving it.
     * 
     * @param Shape The new shape parameters for the window.
     * @param bMove If true, moves the window to the position specified in Shape.
     */
    virtual void SetWindowShape(const FWindowShape& Shape, bool bMove) { }

    /**
     * @brief Retrieves the shape of the window.
     * 
     * @param OutWindowShape Struct to store the window's shape parameters.
     */
    virtual void GetWindowShape(FWindowShape& OutWindowShape) const { }

    /**
     * @brief Retrieves the size the window would have when set to fullscreen mode.
     * 
     * @param OutWidth Variable to store the fullscreen width.
     * @param OutHeight Variable to store the fullscreen height.
     */
    virtual void GetFullscreenInfo(uint32& OutWidth, uint32& OutHeight) const { }

    /**
     * @brief Gets the DPI scale of the monitor where the window is displayed.
     * 
     * @return The DPI scale factor.
     */
    virtual float GetWindowDPIScale() const { return 0.0f; }

    /**
     * @brief Gets the current width of the window.
     * 
     * @return The width of the window.
     */
    virtual uint32 GetWidth() const { return 0; }

    /**
     * @brief Gets the current height of the window.
     * 
     * @return The height of the window.
     */
    virtual uint32 GetHeight() const { return 0; }

    /**
     * @brief Sets the platform-specific handle for the window.
     * 
     * @param InPlatformHandle The platform handle to set.
     */
    virtual void SetPlatformHandle(void* InPlatformHandle) { }

    /**
     * @brief Returns the platform-specific handle of the window.
     * 
     * @return The platform handle.
     */
    virtual void* GetPlatformHandle() const { return nullptr; }

    /**
     * @brief Sets the window style flags.
     * 
     * @param Style The style flags to set.
     */
    virtual void SetStyle(EWindowStyleFlags Style) { }

    /**
     * @brief Gets the window's style flags.
     * 
     * @return The current style flags of the window.
     */
    EWindowStyleFlags GetStyle() const
    { 
        return StyleParams;
    }

protected:
    EWindowStyleFlags StyleParams;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
