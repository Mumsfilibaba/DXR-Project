#pragma once
#include "Core/RefCounted.h"
#include "Core/Containers/String.h"
#include "Core/Templates/EnumUtilities.h"

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

ENUM_CLASS_OPERATORS(EWindowStyleFlag);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SWindowStyle

struct SWindowStyle
{
    SWindowStyle() = default;

    FORCEINLINE SWindowStyle(uint32 InStyle)
        : Style(InStyle)
    { }

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
// SWindowShape

struct SWindowShape
{
    SWindowShape() = default;

    FORCEINLINE SWindowShape(uint32 InWidth, uint32 InHeight, int32 x, int32 y)
        : Width(InWidth)
        , Height(InHeight)
        , Position({ x, y })
    { }

    uint32 Width = 0;
    uint32 Height = 0;
    struct
    {
        int32 x = 0;
        int32 y = 0;
    } Position;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CGenericWindow

class CGenericWindow : public CRefCounted
{
protected:

    CGenericWindow() = default;
    ~CGenericWindow() = default;

public:

    /**
     * @brief: Initializes the window
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
     * @brief: Shows the window 
     * 
     * @param bMaximized: True if the window should be shown maximized
     */
    virtual void Show(bool bMaximized) { }

    /**
     * @brief: Minimizes the window 
     */
    virtual void Minimize() { }

    /**
     * @brief:  Maximizes the window
     */
    virtual void Maximize() { }

     /** @brief: Closes the window */
    virtual void Close() { }

     /** @brief: Restores the window after being minimized or maximized */
    virtual void Restore() { }

     /** @brief: Makes the window a borderless fullscreen window */
    virtual void ToggleFullscreen() { }

     /** @brief: Checks if the underlaying native handle of the window is valid */
    virtual bool IsValid() const { return false; }

     /** @brief: Checks if this window is the currently active window */
    virtual bool IsActiveWindow() const { return false; }

     /** @brief: Sets the title */
    virtual void SetTitle(const String& Title) { }

     /** @brief: Retrieve the window title */
    virtual void GetTitle(String& OutTitle) { }

     /** @brief: Set the position of the window */
    virtual void MoveTo(int32 x, int32 y) { }

     /** @brief: Set the shape of the window */
    virtual void SetWindowShape(const SWindowShape& Shape, bool bMove) { }

     /** @brief: Retrieve the shape of the window */
    virtual void GetWindowShape(SWindowShape& OutWindowShape) const { }

     /** @brief: Get the fullscreen information of the monitor that the window currently is on */
    virtual void GetFullscreenInfo(uint32& OutWidth, uint32& OutHeight) const { }

     /** @brief: Retrieve the width of the window */
    virtual uint32 GetWidth()  const { return 0; }

     /** @brief: Retrieve the height of the window */
    virtual uint32 GetHeight() const { return 0; }

     /** @brief: Set the native window handle */
    virtual void SetPlatformHandle(void* InPlatformHandle) { }

     /** @brief: Retrieve the native handle */
    virtual void* GetPlatformHandle() const { return nullptr; }

     /** @brief: Retrieve the style of the window */
    FORCEINLINE SWindowStyle GetStyle() const { return StyleParams; }

protected:
    SWindowStyle StyleParams;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
