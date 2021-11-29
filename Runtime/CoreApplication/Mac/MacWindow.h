#pragma once

#if PLATFORM_MACOS
#include "Core/Containers/SharedRef.h"

#include "CoreApplication/Interface/PlatformWindow.h"

#if defined(__OBJC__)
@class CCocoaWindow;
@class CCocoaContentView;
#else
class CCocoaWindow;
class CCocoaContentView;
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/

class CMacApplication;

class CMacWindow final : public CPlatformWindow
{
public:

	static TSharedRef<CMacWindow> Make( CMacApplication* InApplication );
	
    /* Initializes the window */
    virtual bool Initialize( const CString& InTitle, uint32 InWidth, uint32 InHeight, int32 x, int32 y, SWindowStyle Style ) override final;

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
    virtual bool IsValid() const override final { return (Window != nullptr); }

    /* Checks if this window is the currently active window */
    virtual bool IsActiveWindow() const override final;

    /* Sets the title */
    virtual void SetTitle( const CString& Title ) override final;

    /* Retrieve the window title */
    virtual void GetTitle( CString& OutTitle ) override final;

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
    CCocoaWindow*      Window = nullptr;
    CCocoaContentView* View   = nullptr;
};

#endif
