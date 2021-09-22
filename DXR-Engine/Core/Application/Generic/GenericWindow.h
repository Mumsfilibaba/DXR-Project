#pragma once
#include "Core/RefCounted.h"

// TODO: Remove
#include <string>

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

enum EWindowStyleFlag : uint32
{
    WindowStyleFlag_None = 0x0,
    WindowStyleFlag_Titled = FLAG( 1 ),
    WindowStyleFlag_Closable = FLAG( 2 ),
    WindowStyleFlag_Minimizable = FLAG( 3 ),
    WindowStyleFlag_Maximizable = FLAG( 4 ),
    WindowStyleFlag_Resizeable = FLAG( 5 ),
};

struct SWindowStyle
{
    SWindowStyle() = default;

    FORCEINLINE SWindowStyle( uint32 InStyle )
        : Style( InStyle )
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

struct SWindowShape
{
    SWindowShape() = default;

    FORCEINLINE SWindowShape( uint32 InWidth, uint32 InHeight, int32 x, int32 y )
        : Width( InWidth )
        , Height( InHeight )
        , Position( { x, y } )
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

typedef void* PlatformWindowHandle;

class CGenericWindow : public CRefCounted
{
public:

    /* Initializes the window */
    virtual bool Init( const std::string& Title, uint32 Width, uint32 Height, SWindowStyle Style ) = 0;

    /* Shows the window */
    virtual void Show( bool Maximized ) = 0;

    /* Minimizes the window */
    virtual void Minimize() = 0;

    /* Maximizes the window */
    virtual void Maximize() = 0;

    /* Closes the window */
    virtual void Close() = 0;

    /* Restores the window after being minimized or maximized */
    virtual void Restore() = 0;

    /* Makes the window a borderless fullscreen window */
    virtual void ToggleFullscreen() = 0;

    /* Checks if the underlaying native handle of the window is valid */
    virtual bool IsValid() const = 0;

    /* Checks if this window is the currently active window */
    virtual bool IsActiveWindow() const = 0;

    /* Sets the title */
    virtual void SetTitle( const std::string& Title ) = 0;

    /* Retrieve the window title */
    virtual void GetTitle( std::string& OutTitle ) = 0;

    /* Set the shape of the window */
    virtual void SetWindowShape( const SWindowShape& Shape, bool Move ) = 0;

    /* Retrieve the shape of the window */
    virtual void GetWindowShape( SWindowShape& OutWindowShape ) const = 0;

    /* Retrieve the width of the window */
    virtual uint32 GetWidth()  const = 0;

    /* Retrieve the height of the window */
    virtual uint32 GetHeight() const = 0;

    /* Retrieve the native handle */
    virtual PlatformWindowHandle GetNativeHandle() const
    {
        return nullptr;
    }

    /* Retrieve the style of the window */
    FORCEINLINE SWindowStyle GetStyle() const
    {
        return StyleParams;
    }

protected:

    CGenericWindow() = default;
    ~CGenericWindow() = default;

    SWindowStyle StyleParams;
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop

#endif