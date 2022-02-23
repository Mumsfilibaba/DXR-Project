#pragma once

#if PLATFORM_WINDOWS
#include "Windows.h"

#include "Core/Containers/SharedRef.h"

#include "CoreApplication/CoreApplication.h"
#include "CoreApplication/Interface/PlatformWindow.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CWindowsWindow - Windows-specific interface for a window

class COREAPPLICATION_API CWindowsWindow final : public CPlatformWindow
{
public:

    /**
     * Create a WindowsWindow
     * 
     * @param InApplication: Application that created the Window
     * @return: Returns the newly created window
     */
    static TSharedRef<CWindowsWindow> CreateWindow(class CWindowsApplication* InApplication);

    /**
     * Retrieve the Window-handle
     * 
     * @return: Returns the Window-handle
     */
    FORCEINLINE HWND GetHandle() const 
    { 
        return Window;
    }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CPlatformWindow Interface

    virtual bool Initialize(const String& Title, uint32 InWidth, uint32 InHeight, int32 x, int32 y, SWindowStyle Style) override final;

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

    virtual void SetPlatformHandle(PlatformWindowHandle InPlatformHandle) override final;

    virtual PlatformWindowHandle GetPlatformHandle() const override final
    {
        return reinterpret_cast<PlatformWindowHandle>(Window);
    }

private:

    CWindowsWindow(CWindowsApplication* InApplication);
    ~CWindowsWindow();

    // Owning application
    CWindowsApplication* Application;

    HWND  Window;
    DWORD Style;
    DWORD StyleEx;

    // Used when toggling fullscreen
    WINDOWPLACEMENT StoredPlacement;

    bool bIsFullscreen;
};

#endif