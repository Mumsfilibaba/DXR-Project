#pragma once
#include "Core/RefCounted.h"
#include "Core/Math/IntVector2.h"
#include "Core/Containers/String.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Templates/TypeTraits.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FGenericWindow;

enum class EWindowStyleFlags : uint16
{
    None          = 0,
    Titled        = FLAG(1),
    Closable      = FLAG(2),
    Minimizable   = FLAG(3),
    Maximizable   = FLAG(4),
    Resizeable    = FLAG(5),
    NoTaskBarIcon = FLAG(6),
    TopMost       = FLAG(7),
    Opaque        = FLAG(8),

    Default = Titled | Maximizable | Minimizable | Resizeable | Closable | Opaque
};

ENUM_CLASS_OPERATORS(EWindowStyleFlags);

struct FWindowShape
{
    FWindowShape()
        : Width(0)
        , Height(0)
        , Position(0, 0)
    {
    }

    FWindowShape(uint32 InWidth, uint32 InHeight)
        : Width(InWidth)
        , Height(InHeight)
        , Position(0, 0)
    {
    }

    FWindowShape(uint32 InWidth, uint32 InHeight, int32 x, int32 y)
        : Width(InWidth)
        , Height(InHeight)
        , Position(x, y)
    {
    }

    bool operator==(const FWindowShape& Other) const
    {
        return Width == Other.Width && Height == Other.Height && Position == Other.Position;
    }

    bool operator!=(const FWindowShape& Other) const
    {
        return !(*this == Other);
    }

    uint32      Width;
    uint32      Height;
    FIntVector2 Position;
};

struct FGenericWindowInitializer
{
    FGenericWindowInitializer()
        : Title()
        , Width(1280)
        , Height(720)
        , Position(0, 0)
        , Style(EWindowStyleFlags::Default)
        , ParentWindow(nullptr)
    {
    }

    FString           Title;
    uint32            Width;
    uint32            Height;
    FIntVector2       Position;
    EWindowStyleFlags Style;
    FGenericWindow*   ParentWindow;
};

class FGenericWindow : public FRefCounted
{
public:
    virtual ~FGenericWindow() = default;

    // Initializes the PlatformWindow
    virtual bool Initialize(const FGenericWindowInitializer& InInitializer) { return true; }
    
    // Show the window and optionally set the window focus to this window
    virtual void Show(bool bFocus = true) { }
    
    // Minimize the window
    virtual void Minimize() { }
    
    // Maximize the window
    virtual void Maximize() { }
    
    // Destroy the PlatformWindow. This instance is still valid, but not the platform-handle.
    virtual void Destroy() { }
    
    // Restores the window to the previous window state if the window has been minimized or maximized.
    virtual void Restore() { }
    
    // Toggles the window into a fullscreen-mode.
    virtual void ToggleFullscreen() { }
    
    // Returns true if this is the current active window.
    virtual bool IsActiveWindow() const { return false; }
    
    // Sets the window postion
    virtual void SetWindowPos(int32 x, int32 y) { }
    
    // Returns true if this window has a valid platform-handle.
    virtual bool IsValid() const { return false; }
    
    // Returns true if the window is currently minimized
    virtual bool IsMinimized() const { return false; }
    
    // Returns true if the window is currently maximized
    virtual bool IsMaximized() const { return false; }
    
    // Returns true if the specified window is a parent-window to this window.
    virtual bool IsChildWindow(const TSharedRef<FGenericWindow>& ParentWindow) const { return false; }
    
    // Sets the window focus to be set to this window
    virtual void SetWindowFocus() { }
    
    // Sets the title of this window to the specified title
    virtual void SetTitle(const FString& Title) { }
    
    // Retrieve the title of this window
    virtual void GetTitle(FString& OutTitle) const { }
    
    // Sets the window opacity
    virtual void SetWindowOpacity(float Alpha) { }
    
    // Sets the shape of the window, it can optionally be moved
    virtual void SetWindowShape(const FWindowShape& Shape, bool bMove) { }
    
    // Retrieve the window-shape
    virtual void GetWindowShape(FWindowShape& OutWindowShape) const { }
    
    // Retrieve the size of the window if it would be set to fullscreen
    virtual void GetFullscreenInfo(uint32& OutWidth, uint32& OutHeight) const { }
    
    // Returns the DPI scale of the monitor that the window is currently displayed on
    virtual float GetWindowDPIScale() const { return 0.0f; }
    
    // Return the current width of the window
    virtual uint32 GetWidth() const { return 0; }
    
    // Return the current height of the window
    virtual uint32 GetHeight() const { return 0; }
    
    // Sets the platform-handle
    virtual void SetPlatformHandle(void* InPlatformHandle) { }
    
    // Retruns the platform-handle of this window
    virtual void* GetPlatformHandle() const { return nullptr; }
    
    // Sets the window style-flags
    virtual void SetStyle(EWindowStyleFlags Style) { }

    EWindowStyleFlags GetStyle() const
    { 
        return StyleParams;
    }

protected:
    EWindowStyleFlags StyleParams;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
