#pragma once

#if defined(PLATFORM_MACOS)
#include "Core/Application/Generic/GenericWindow.h"

#if defined(__OBJC__)
@class CCocoaWindow;
@class CCocoaContentView;
#else
class CCocoaWindow;
class CCocoaContentView;
#endif

class CMacApplication;

class CMacWindow : public CGenericWindow
{
    friend class CMacApplication;

public:

    /* Initializes the window */
    virtual bool Init( const std::string& Title, uint32 Width, uint32 Height, SWindowStyle Style ) override final;

    /* Shows the window */
    virtual void Show( bool Maximized ) override final;

    /* Minimizes the window */
    virtual void Minimize() override final;

    /* Maximizes the window */
    virtual void Maximize() override final;

    /* Closes the window */
    virtual void Close() override final;

    /* Restores the window after being minimized or maximized */
    virtual void Restore() override final;

    /* Makes the window a borderless fullscreen window */
    virtual void ToggleFullscreen() override final;

    /* Checks if the underlaying native handle of the window is valid */
    virtual bool IsValid() const override final
    {
        return (Window != nullptr);
    }

    /* Checks if this window is the currently active window */
    virtual bool IsActiveWindow() const override final;

    /* Sets the title */
    virtual void SetTitle( const std::string& Title ) override final;

    /* Retrieve the window title */
    virtual void GetTitle( std::string& OutTitle ) override final;

    /* Set the shape of the window */
    virtual void SetWindowShape( const SWindowShape& Shape, bool Move ) override final;

    /* Retrieve the shape of the window */
    virtual void GetWindowShape( SWindowShape& OutWindowShape ) const override final;

    /* Retrieve the width of the window */
    virtual uint32 GetWidth()  const override final;

    /* Retrieve the height of the window */
    virtual uint32 GetHeight() const override final;

    /* Retrieve the native handle */
    virtual PlatformWindowHandle GetNativeHandle() const override final
    {
        return reinterpret_cast<void*>(Window);
    }

    /* Get the window */
    FORCEINLINE CCocoaWindow* GetCocoaWindow() const
    {
        return Window;
    }

    /* Get the content view */
    FORCEINLINE CCocoaWindow* GetCocoaContentView() const
    {
        return Window;
    }

private:

    CMacWindow( CMacApplication* InApplication );
    ~CMacWindow();

    /* Reference to the parent application */
    CMacApplication* Application = nullptr;

    /* The native window and view */
    CCocoaWindow* Window = nullptr;
    CCocoaContentView* View = nullptr;
};

#endif
