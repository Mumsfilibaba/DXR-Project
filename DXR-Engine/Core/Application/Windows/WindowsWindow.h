#pragma once
#include "Windows.h"

#include "Core/Application/Generic/Window.h"

class WindowsWindow : public Window
{
public:
    WindowsWindow();
    ~WindowsWindow();

    bool Init(const std::string& Title, uint32 Width, uint32 Height, WindowStyle Style);

    virtual void Show(bool Maximized) override final;
    virtual void Close() override final;
    virtual void Minimize() override final;
    virtual void Maximize() override final;
    virtual void Restore() override final;
    virtual void ToggleFullscreen() override final;

    virtual bool IsValid() const override final;
    virtual bool IsActiveWindow() const override final;

    virtual void SetTitle(const std::string& Title) override final;
    virtual void GetTitle(std::string& OutTitle) override final;
    
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
    HWND  hWindow;
    DWORD Style;
    DWORD StyleEx;
    bool  IsFullscreen;
    WINDOWPLACEMENT StoredPlacement;
};