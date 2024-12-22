#pragma once
#include "Core/RefCounted.h"
#include "Core/Math/IntVector2.h"
#include "Core/Containers/String.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Templates/TypeTraits.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FGenericWindow;

/**
 * @enum EWindowStyleFlags
 * @brief Flags representing various window style options.
 *
 * These flags can be combined to create custom window styles. For instance,
 * a window might be resizable, closable, and have a title bar by combining
 * multiple flags. The Default style is a preset that includes common flags.
 */
enum class EWindowStyleFlags : uint16
{
    /** @brief No style flags set. */
    None = 0,

    /** @brief Window has a title bar. */
    Titled = FLAG(1),

    /** @brief Window can be closed. */
    Closable = FLAG(2),

    /** @brief Window can be minimized. */
    Minimizable = FLAG(3),

    /** @brief Window can be maximized. */
    Maximizable = FLAG(4),

    /** @brief Window is resizable. */
    Resizable = FLAG(5),

    /** @brief Window does not show an icon in the taskbar. */
    NoTaskBarIcon = FLAG(6),

    /** @brief Window stays above all non-topmost windows. */
    TopMost = FLAG(7),

    /** @brief Window is opaque (non-transparent). */
    Opaque = FLAG(8),

    /** @brief A default combination of style flags for most standard windows. */
    Default = Titled | Maximizable | Minimizable | Resizable | Closable | Opaque
};

ENUM_CLASS_OPERATORS(EWindowStyleFlags);

/**
 * @struct FWindowShape
 * @brief Represents the dimensions and on-screen position of a window.
 *
 * Contains width, height, and a position (x, y). This is used to set or query
 * the size and location of a window across various platforms.
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
     * @brief Constructs a window shape with specified width and height, defaulting position to (0,0).
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
     * @param InWidth Width of the window.
     * @param InHeight Height of the window.
     * @param x The X-coordinate of the window's position.
     * @param y The Y-coordinate of the window's position.
     */
    FWindowShape(uint32 InWidth, uint32 InHeight, int32 x, int32 y)
        : Width(InWidth)
        , Height(InHeight)
        , Position(x, y)
    {
    }

    /**
     * @brief Checks if this window shape is equal to another.
     * @param Other The other window shape to compare.
     * @return True if they have the same width, height, and position.
     */
    bool operator==(const FWindowShape& Other) const
    {
        return Width == Other.Width && Height == Other.Height && Position == Other.Position;
    }

    /**
     * @brief Checks if this window shape is not equal to another.
     * @param Other The other window shape to compare.
     * @return True if they differ in width, height, or position.
     */
    bool operator!=(const FWindowShape& Other) const
    {
        return !(*this == Other);
    }

    /** @brief The width of the window. */
    uint32 Width;

    /** @brief The height of the window. */
    uint32 Height;

    /** @brief The (x, y) position of the window. */
    FIntVector2 Position;
};

/**
 * @struct FGenericWindowInitializer
 * @brief Defines initialization parameters for creating a generic window.
 *
 * Contains common properties such as title, dimensions, position, style flags,
 * and an optional pointer to a parent window. Platform-specific window classes
 * can extend or use this struct to standardize window creation workflows.
 */
struct FGenericWindowInitializer
{
    /**
     * @brief Default constructor initializing typical default values.
     *
     * Defaults to a 1280x720 window at position (0,0) with a standard window style,
     * no title string, and no parent window.
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

    /** @brief The textual title of the window. */
    FString Title;

    /** @brief The desired width of the window. */
    uint32 Width;

    /** @brief The desired height of the window. */
    uint32 Height;

    /** @brief The initial on-screen position of the window (x, y). */
    FIntVector2 Position;

    /** @brief Style flags controlling the appearance and behavior of the window. */
    EWindowStyleFlags Style;

    /** @brief Pointer to a parent window, if this is a child window. */
    FGenericWindow* ParentWindow;
};

/**
 * @class FGenericWindow
 * @brief A base interface for platform-specific window implementations.
 *
 * FGenericWindow defines a reference-counted, platform-agnostic interface for
 * creating and managing windows. Derived classes should override virtual methods
 * to handle specific behaviors on each platform (e.g., Windows, macOS, Linux).
 */
class FGenericWindow : public FRefCounted
{
public:
    /**
     * @brief Virtual destructor for FGenericWindow.
     */
    virtual ~FGenericWindow() = default;

    /**
     * @brief Initializes the window using platform-specific logic.
     * 
     * @param InInitializer Contains the parameters needed to create and configure the window.
     * @return True if initialization was successful, otherwise false.
     */
    virtual bool Initialize(const FGenericWindowInitializer& InInitializer) { return true; }

    /**
     * @brief Shows the window and optionally sets input focus to it.
     * 
     * @param bFocus If true, the window will receive input focus after being shown.
     */
    virtual void Show(bool bFocus = true) { }

    /**
     * @brief Minimizes the window (iconifies it).
     */
    virtual void Minimize() { }

    /**
     * @brief Maximizes the window to fill the available work area.
     */
    virtual void Maximize() { }

    /**
     * @brief Destroys the platform-specific handle for this window.
     *
     * The FGenericWindow object may still exist, but the underlying OS window becomes invalid.
     */
    virtual void Destroy() { }

    /**
     * @brief Restores the window from either a minimized or maximized state to normal.
     */
    virtual void Restore() { }

    /**
     * @brief Toggles the window between windowed and fullscreen mode.
     */
    virtual void ToggleFullscreen() { }

    /**
     * @brief Checks if this window is currently active (focused).
     * 
     * @return True if the window is active, otherwise false.
     */
    virtual bool IsActiveWindow() const { return false; }

    /**
     * @brief Repositions the window on screen.
     * 
     * @param x The new X-coordinate of the window's position.
     * @param y The new Y-coordinate of the window's position.
     */
    virtual void SetWindowPos(int32 x, int32 y) { }

    /**
     * @brief Checks if the underlying platform handle for this window is valid.
     * 
     * @return True if the OS handle is valid, otherwise false.
     */
    virtual bool IsValid() const { return false; }

    /**
     * @brief Checks if the window is currently minimized (iconified).
     * 
     * @return True if minimized, otherwise false.
     */
    virtual bool IsMinimized() const { return false; }

    /**
     * @brief Checks if the window is currently maximized.
     * 
     * @return True if maximized, otherwise false.
     */
    virtual bool IsMaximized() const { return false; }

    /**
     * @brief Determines if this window is a child of the specified parent window.
     * 
     * @param ParentWindow A reference to the window that might be this window's parent.
     * @return True if this window is a child of ParentWindow, otherwise false.
     */
    virtual bool IsChildWindow(const TSharedRef<FGenericWindow>& ParentWindow) const { return false; }

    /**
     * @brief Sets input focus to this window.
     */
    virtual void SetWindowFocus() { }

    /**
     * @brief Updates the title bar text of the window.
     * 
     * @param Title The new title text.
     */
    virtual void SetTitle(const FString& Title) { }

    /**
     * @brief Retrieves the current title of the window.
     * 
     * @param OutTitle An FString that will receive the window's title.
     */
    virtual void GetTitle(FString& OutTitle) const { }

    /**
     * @brief Sets the overall window opacity.
     * 
     * @param Alpha A float from 0.0 (fully transparent) to 1.0 (fully opaque).
     */
    virtual void SetWindowOpacity(float Alpha) { }

    /**
     * @brief Adjusts the window's shape (width, height) and optionally its position.
     * 
     * @param Shape A struct containing the new shape parameters (position, width, height).
     * @param bMove If true, moves the window to Shape.Position as well as resizing it.
     */
    virtual void SetWindowShape(const FWindowShape& Shape, bool bMove) { }

    /**
     * @brief Retrieves the window's current shape (position, width, height).
     * 
     * @param OutWindowShape A struct that will be filled with the window's shape.
     */
    virtual void GetWindowShape(FWindowShape& OutWindowShape) const { }

    /**
     * @brief Obtains the resolution (width, height) used when the window goes fullscreen.
     * 
     * @param OutWidth A reference where the fullscreen width is stored.
     * @param OutHeight A reference where the fullscreen height is stored.
     */
    virtual void GetFullscreenInfo(uint32& OutWidth, uint32& OutHeight) const { }

    /**
     * @brief Returns the DPI scaling factor for this window, used for high-DPI displays.
     * 
     * @return A float representing the window's DPI scale (e.g., 1.0 = 100% scale).
     */
    virtual float GetWindowDPIScale() const { return 0.0f; }

    /**
     * @brief Retrieves the current width of the window (in screen pixels).
     * 
     * @return The width of the window.
     */
    virtual uint32 GetWidth() const { return 0; }

    /**
     * @brief Retrieves the current height of the window (in screen pixels).
     * 
     * @return The height of the window.
     */
    virtual uint32 GetHeight() const { return 0; }

    /**
     * @brief Sets the platform-specific handle for this window (if needed).
     * 
     * @param InPlatformHandle A pointer to the platform window handle.
     */
    virtual void SetPlatformHandle(void* InPlatformHandle) { }

    /**
     * @brief Retrieves the underlying platform-specific handle for this window.
     * 
     * @return A pointer to the OS handle, or nullptr if invalid.
     */
    virtual void* GetPlatformHandle() const { return nullptr; }

    /**
     * @brief Applies style flags to the window (e.g., resizable, closable).
     * 
     * @param Style The style flags to set.
     */
    virtual void SetStyle(EWindowStyleFlags Style) { }

    /**
     * @brief Retrieves the window's current style flags.
     * 
     * @return The EWindowStyleFlags that define this window's style.
     */
    EWindowStyleFlags GetStyle() const
    { 
        return StyleParams;
    }

protected:
    EWindowStyleFlags StyleParams;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
