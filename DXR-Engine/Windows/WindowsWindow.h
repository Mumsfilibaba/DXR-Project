#pragma once
#include "Windows.h"

#include "Application/Generic/GenericWindow.h"

class WindowsApplication;

class WindowsWindow : public GenericWindow
{
public:
    WindowsWindow(WindowsApplication* InOwnerApplication);
    ~WindowsWindow();

    virtual bool Init(const WindowCreateInfo& InCreateInfo) override final;

    virtual void Show(bool Maximized) override final;
    virtual void Close()    override final;
    virtual void Minimize() override final;
    virtual void Maximize() override final;
    virtual void Restore()  override final;
    virtual void ToggleFullscreen() override final;

    virtual bool IsValid() const override final;
    virtual bool IsActiveWindow() const override final;

    virtual void SetTitle(const std::string& Title) override final;
    
    virtual void SetWindowShape(const WindowShape& Shape, bool Move) override final;
    virtual void GetWindowShape(WindowShape& OutWindowShape) const override final;

    virtual uint32 GetWidth()  const override final;
    virtual uint32 GetHeight() const override final;

    virtual void* GetNativeHandle() const override final
    {
        return reinterpret_cast<void*>(hWindow);
    }
    
    HWND GetHandle() const { return hWindow; }

private:
    WindowsApplication* OwnerApplication = nullptr;
    WINDOWPLACEMENT StoredPlacement;

    HWND  hWindow;
    DWORD Style;
    DWORD StyleEx;
    bool  IsFullscreen;
};