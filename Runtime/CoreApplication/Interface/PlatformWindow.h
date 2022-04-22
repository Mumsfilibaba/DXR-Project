#pragma once
#include "Core/RefCounted.h"
#include "Core/Containers/String.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Templates/EnumUtilities.h"

#include "Core/Math/IntVector2.h"

#include "CoreApplication/CoreApplication.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedefs

typedef TSharedRef<class CPlatformWindow> CPlatformWindowRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EWindowStyleFlag

enum class EWindowStyleFlag : uint8
{
    None        = 0x0,
    Titled      = FLAG(1),
    Closable    = FLAG(2),
    Minimizable = FLAG(3),
    Maximizable = FLAG(4),
    Resizeable  = FLAG(5),
};

ENUM_CLASS_OPERATORS(EWindowStyleFlag);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SWindowStyle

struct SWindowStyle
{
    SWindowStyle()
        : Style(EWindowStyleFlag::None)
    { }

    FORCEINLINE SWindowStyle(EWindowStyleFlag InStyle)
        : Style(InStyle)
    { }

    FORCEINLINE bool IsTitled() const { return bool(Style & EWindowStyleFlag::Titled); }

    FORCEINLINE bool IsClosable() const { return bool(Style & EWindowStyleFlag::Closable); }

    FORCEINLINE bool IsMinimizable() const { return bool(Style & EWindowStyleFlag::Minimizable); }

    FORCEINLINE bool IsMaximizable() const { return bool(Style & EWindowStyleFlag::Maximizable); }

    FORCEINLINE bool IsResizeable() const { return bool(Style & EWindowStyleFlag::Resizeable); }

    operator bool() const
    {
        return (Style != EWindowStyleFlag::None);
    }

    bool operator==(SWindowStyle RHS) const
    {
        return (Style == RHS.Style);
    }

    bool operator!=(SWindowStyle RHS) const
    {
        return (Style != RHS.Style);
    }

    EWindowStyleFlag Style;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SWindowShape

struct SWindowShape
{
    SWindowShape() = default;

    SWindowShape(uint32 InWidth, uint32 InHeight, int32 x, int32 y)
        : Width(InWidth)
        , Height(InHeight)
        , Position(x, y)
    { }

    bool operator==(const SWindowShape& RHS) const
    {
        return (Width == RHS.Width) && (Height == RHS.Height) && (Position == RHS.Position);
    }

    bool operator!=(const SWindowShape& RHS) const
    {
        return !(*this == RHS);
    }

    uint32      Width  = 0;
    uint32      Height = 0;

    CIntVector2 Position;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CWindowInitializer

class CWindowInitializer
{
public:

    CWindowInitializer()
        : Title()
        , Width(0)
        , Height(0)
        , Position(0)
        , Style()
    { }

    CWindowInitializer(const String& InTitle, uint16 InWidth, uint16 InHeight, const CIntVector2& InPosition, SWindowStyle InStyle)
        : Title(InTitle)
        , Width(InWidth)
        , Height(InHeight)
        , Position(InPosition)
        , Style(InStyle)
    { }

    bool operator==(const CWindowInitializer& RHS) const
    {
        return (Title    == RHS.Title) 
            && (Width    == RHS.Width) 
            && (Height   == RHS.Height) 
            && (Position == RHS.Position) 
            && (Style    == RHS.Style);
    }

    bool operator!=(const CWindowInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    String       Title;

    uint16       Width;
    uint16       Height;

    CIntVector2  Position;

    SWindowStyle Style;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CPlatformWindow

class CPlatformWindow : public CRefCounted
{
protected:

    CPlatformWindow()  = default;
    ~CPlatformWindow() = default;

public:

    /**
     * @brief: Initializes the window
     * 
     * @param Initializer: Struct containing information about how to initialize the window
     * @return: Returns true if the initialization is successful, otherwise false
     */
    virtual bool Initialize(const CWindowInitializer& Initializer) { return true; }

    /**
     * @brief: Shows the window 
     * 
     * @param bMaximized: True if the window should be shown maximized
     */
    virtual void Show(bool bMaximized) { }

    /** @brief: Minimizes the window */
    virtual void Minimize() { }

    /** @brief: Maximizes the window */
    virtual void Maximize() { }

    /* @brief: Closes the window */
    virtual void Close() { }

    /* @brief: Restores the window after being minimized or maximized */
    virtual void Restore() { }

    /* @brief: Makes the window a borderless fullscreen window */
    virtual void ToggleFullscreen() { }

    /* @brief: Checks if the OS-handle is valid */
    virtual bool IsValid() const { return false; }

    /* @brief: Checks if this window is the currently active window */
    virtual bool IsActiveWindow() const { return false; }

    /* @brief: Sets the title */
    virtual void SetTitle(const String& Title) { }

    /* @brief: Retrieve the window title */
    virtual void GetTitle(String& OutTitle) { }

    /* @brief: Set the position of the window */
    virtual void MoveTo(int32 x, int32 y) { }

    /* @brief: Set the shape of the window */
    virtual void SetWindowShape(const SWindowShape& Shape, bool bMove) { }

    /* @brief: Retrieve the shape of the window */
    virtual void GetWindowShape(SWindowShape& OutWindowShape) const { }

    /* @brief: Get the fullscreen information of the monitor that the window currently is on */
    virtual void GetFullscreenInfo(uint32& OutWidth, uint32& OutHeight) const { }

    /* @brief: Retrieve the width of the window */
    virtual uint32 GetWidth()  const { return 0; }

    /* @brief: Retrieve the height of the window */
    virtual uint32 GetHeight() const { return 0; }

    /* @brief: Set the OS-window handle */
    virtual void SetOSHandle(void* InOSHandle) { }

    /* @brief: Retrieve the OS-window handle */
    virtual void* GetOSHandle() const { return nullptr; }

    /* @brief: Retrieve the style of the window */
    SWindowStyle GetStyle() const { return Style; }

protected:
    SWindowStyle Style;
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop

#endif
