#pragma once

#if PLATFORM_WINDOWS
#include "Windows.h"

#include "Core/Containers/SharedRef.h"

#include "CoreApplication/CoreApplicationModule.h"
#include "CoreApplication/Interface/PlatformWindow.h"

class COREAPPLICATION_API CWindowsWindow final : public CPlatformWindow
{
public:

    /* Create a new window */
    static TSharedRef<CWindowsWindow> Make(class CWindowsApplication* InApplication);

    /* Initializes the window */
    virtual bool Initialize(const CString& Title, uint32 InWidth, uint32 InHeight, int32 x, int32 y, SWindowStyle Style) override final;

    /* Shows the window */
    virtual void Show(bool bMaximized) override final;

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
    virtual bool IsValid() const override final;

    /* Checks if this window is the currently active window */
    virtual bool IsActiveWindow() const override final;

    /* Sets the title */
    virtual void SetTitle(const CString& Title) override final;

    /* Retrieve the window title */
    virtual void GetTitle(CString& OutTitle) override final;

    /* Set the position of the window */
    virtual void MoveTo(int32 x, int32 y) override final;

    /* Set the shape of the window */
    virtual void SetWindowShape(const SWindowShape& Shape, bool bMove) override final;

    /* Retrieve the shape of the window */
    virtual void GetWindowShape(SWindowShape& OutWindowShape) const override final;

    /* Get the fullscreen information of the monitor that the window currently is on */
    virtual void GetFullscreenInfo(uint32& OutWidth, uint32& OutHeight) const override final;

    /* Retrieve the width of the window */
    virtual uint32 GetWidth()  const override final;

    /* Retrieve the height of the window */
    virtual uint32 GetHeight() const override final;

    /* Retrieve the native handle */
    virtual PlatformWindowHandle GetNativeHandle() const
    {
        return reinterpret_cast<PlatformWindowHandle>(Window);
    }

    FORCEINLINE HWND GetHandle() const
    {
        return Window;
    }

private:

    CWindowsWindow(CWindowsApplication* InApplication);
    ~CWindowsWindow();

    // Owning application
    CWindowsApplication* Application;

    HWND Window;

    bool bIsFullscreen;

    /* Used when toggling fullscreen */
    WINDOWPLACEMENT StoredPlacement;

    DWORD Style;
    DWORD StyleEx;
};

#endif