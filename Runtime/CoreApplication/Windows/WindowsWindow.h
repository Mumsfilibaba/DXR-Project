#pragma once

#if PLATFORM_WINDOWS
#include "Windows.h"

#include "Core/Containers/SharedRef.h"

#include "CoreApplication/CoreApplication.h"
#include "CoreApplication/Interface/PlatformWindow.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CWindowsWindow

class COREAPPLICATION_API CWindowsWindow final : public CPlatformWindow
{
private:

    CWindowsWindow(CWindowsApplication* InApplication);
    ~CWindowsWindow();

public:

    /* Create a new window */
    static TSharedRef<CWindowsWindow> Make(class CWindowsApplication* InApplication);

    HWND GetHandle() const { return Window; }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CPlatformWindow Interface

    virtual bool Initialize(const CWindowInitializer& Initializer) override final;
    virtual void Show(bool bMaximized) override final;

    virtual void Minimize() override final;
    virtual void Maximize() override final;

    virtual void Close() override final;
    virtual void Restore() override final;

    virtual void ToggleFullscreen() override final;

    virtual bool IsValid() const override final;
    virtual bool IsActiveWindow() const override final;

    virtual void SetTitle(const String& Title) override final;
    virtual void GetTitle(String& OutTitle) override final;

    virtual void MoveTo(int32 x, int32 y) override final;

    virtual void SetWindowShape(const SWindowShape& Shape, bool bMove) override final;
    virtual void GetWindowShape(SWindowShape& OutWindowShape) const override final;

    virtual void GetFullscreenInfo(uint32& OutWidth, uint32& OutHeight) const override final;

    virtual uint32 GetWidth()  const override final;
    virtual uint32 GetHeight() const override final;

    virtual void  SetOSHandle(void* InOSHandle) override final;
    virtual void* GetOSHandle() const override final { return reinterpret_cast<void*>(Window); }

private:
    CWindowsApplication* Application;

    HWND                 Window;

    bool                 bIsFullscreen;

    // Used when toggling fullscreen 
    WINDOWPLACEMENT      StoredPlacement;

    DWORD                DwStyle;
    DWORD                DwStyleEx;
};

#endif