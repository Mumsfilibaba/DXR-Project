#pragma once
#include "Core/RefCounted.h"
#include "Core/Containers/String.h"

#include "CoreApplication/CoreApplication.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EWindowStyleFlag - Window style flags

enum EWindowStyleFlag : uint32
{
    WindowStyleFlag_None        = 0x0,
    WindowStyleFlag_Titled      = FLAG(1),
    WindowStyleFlag_Closable    = FLAG(2),
    WindowStyleFlag_Minimizable = FLAG(3),
    WindowStyleFlag_Maximizable = FLAG(4),
    WindowStyleFlag_Resizeable  = FLAG(5),
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SWindowStyle - Struct for checking window style

struct SWindowStyle
{
    SWindowStyle() = default;

    FORCEINLINE SWindowStyle(uint32 InStyle)
        : Style(InStyle)
    {
    }

    FORCEINLINE bool IsTitled() const
    {
        return Style & WindowStyleFlag_Titled;
    }

    FORCEINLINE bool IsClosable() const
    {
        return Style & WindowStyleFlag_Closable;
    }

    FORCEINLINE bool IsMinimizable() const
    {
        return Style & WindowStyleFlag_Minimizable;
    }

    FORCEINLINE bool IsMaximizable() const
    {
        return Style & WindowStyleFlag_Maximizable;
    }

    FORCEINLINE bool IsResizeable() const
    {
        return Style & WindowStyleFlag_Resizeable;
    }

    uint32 Style = 0;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SWindowShape - Struct defining the shape of a window

struct SWindowShape
{
    SWindowShape() = default;

    FORCEINLINE SWindowShape(uint32 InWidth, uint32 InHeight, int32 x, int32 y)
        : Width(InWidth)
        , Height(InHeight)
        , Position({ x, y })
    {
    }

    uint32 Width = 0;
    uint32 Height = 0;
    struct
    {
        int32 x = 0;
        int32 y = 0;
    } Position;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CPlatformWindow - Platform interface for a window

typedef void* PlatformWindowHandle;

class CPlatformWindow : public CRefCounted
{
public:

    /**
     * Initializes the window
     * 
     * @param Title: Title of the window
     * @param InWidth: Width of the window
     * @param InHeight: Height of the window
     * @param x: x-coordinate of the window
     * @param y: y-coordinate of the window
     * @param Style: Style of the window
     * @return: Returns true if the initialization is successful, otherwise false
     */
    virtual bool Initialize(const String& Title, uint32 InWidth, uint32 InHeight, int32 x, int32 y, SWindowStyle Style) { return true; }

    /**
     * Shows the window 
     * 
     * @param bMaximized: True if the window should be shown maximized
     */
    virtual void Show(bool bMaximized) { }

    /**
     * Minimizes the window 
     */
    virtual void Minimize() { }

    /**
     *  Maximizes the window
     */
    virtual void Maximize() { }

    /**
     * Closes the window 
     */
    virtual void Close() { }

    /**
     * Restores the window after being minimized or maximized 
     */
    virtual void Restore() { }

    /**
     * Makes the window a borderless fullscreen window 
     */
    virtual void ToggleFullscreen() { }

    /**
     * Checks if the underlaying native handle of the window is valid 
     * 
     * @return: Returns true if the window is valid
     */
    virtual bool IsValid() const { return false; }

    /**
     * Checks if this window is the currently active window 
     * 
     * @return: Returns true if this window currently is the active Window
     */
    virtual bool IsActiveWindow() const { return false; }

    /**
     * Sets the title 
     * 
     * @param Title: The new title of the window
     */
    virtual void SetTitle(const String& Title) { }

    /**
     * Retrieve the window title 
     * 
     * @param OutTitle: String to store the current window-title in
     */
    virtual void GetTitle(String& OutTitle) { }

    /**
     * Set the position of the window 
     * 
     * @param x: The new x-coordinate of the window
     * @param y: The new y-coordinate of the window
     */
    virtual void MoveTo(int32 x, int32 y) { }

    /**
     * Set the shape of the window 
     * 
     * @param Shape: The new shape of the window
     * @param bMove: True if the window should be able to move when reshaping
     */
    virtual void SetWindowShape(const SWindowShape& Shape, bool bMove) { }

    /**
     * Retrieve the shape of the window 
     * 
     * @param OutWindowShape: Struct to store the current window-shape in
     */
    virtual void GetWindowShape(SWindowShape& OutWindowShape) const { }

    /**
     * Get the fullscreen information of the monitor that the window currently is on 
     * 
     * @param OutWidth: Variable to store the current width of the Window in
     * @param OutHeight: Variable to store the current height of the Window in
     */
    virtual void GetFullscreenInfo(uint32& OutWidth, uint32& OutHeight) const { }

    /**
     * Retrieve the width of the window 
     * 
     * @return: Returns the current width of the window
     */
    virtual uint32 GetWidth()  const { return 0; }

    /**
     * Retrieve the height of the window
     *
     * @return: Returns the current height of the window
     */
    virtual uint32 GetHeight() const { return 0; }

    /**
     * Set the native window handle 
     * 
     * @param InPlatformHandle: The new native window platform-handle
     */
    virtual void SetPlatformHandle(PlatformWindowHandle InPlatformHandle) { }

    /**
     * Retrieve the native handle
     * 
     * @return: Returns the platform-handle
     */
    virtual PlatformWindowHandle GetPlatformHandle() const { return nullptr; }

    /**
     * Retrieve the style of the window 
     * 
     * @return: Returns the window-style of the window
     */
    FORCEINLINE SWindowStyle GetStyle() const { return StyleParams; }

protected:

    CPlatformWindow() = default;
    ~CPlatformWindow() = default;

    SWindowStyle StyleParams;
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop

#endif
