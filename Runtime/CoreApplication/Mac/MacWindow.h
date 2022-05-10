#pragma once
#include "Core/Containers/SharedRef.h"

#include "CoreApplication/Generic/GenericWindow.h"

@class CCocoaWindow;
@class CCocoaWindowView;

class CMacApplication;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMacWindow

class CMacWindow final : public CGenericWindow
{
private:

    CMacWindow(CMacApplication* InApplication);
    ~CMacWindow();

public:

	static CMacWindow* CreateMacWindow(CMacApplication* InApplication);

    CCocoaWindow* GetWindowHandle() const { return WindowHandle; }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CGenericWindow Interface

    virtual bool Initialize(const String& InTitle, uint32 InWidth, uint32 InHeight, int32 x, int32 y, SWindowStyle Style) override final;

    virtual void Show(bool bMaximized) override final;

    virtual void Minimize() override final;

    virtual void Maximize() override final;

    virtual void Close() override final;

    virtual void Restore() override final;

    virtual void ToggleFullscreen() override final;

    virtual bool IsValid() const override final { return (WindowHandle != nullptr); }

    virtual bool IsActiveWindow() const override final;

    virtual void SetTitle(const String& Title) override final;

    virtual void GetTitle(String& OutTitle) override final;

    virtual void SetWindowShape(const SWindowShape& Shape, bool bMove) override final;

    virtual void GetWindowShape(SWindowShape& OutWindowShape) const override final;

    virtual uint32 GetWidth() const override final;

    virtual uint32 GetHeight() const override final;

    virtual void SetPlatformHandle(void* InPlatformHandle) override final;

    virtual void* GetPlatformHandle() const override final { return reinterpret_cast<void*>(WindowHandle); }

private:
    CMacApplication*   Application;
    CCocoaWindow*      WindowHandle;
    CCocoaWindowView* View;
};
