#pragma once
#include "Core/Windows/Windows.h"
#include "Core/Containers/SharedRef.h"
#include "CoreApplication/Generic/GenericWindow.h"

class FWindowsApplication;

struct FWindowsWindowStyle
{
    FWindowsWindowStyle()
        : Style(0)
        , StyleEx(0)
    {
    }

    FWindowsWindowStyle(DWORD InStyle, DWORD InStyleEx)
        : Style(InStyle)
        , StyleEx(InStyleEx)
    {
    }

    bool operator==(FWindowsWindowStyle Other) const
    {
        return Style == Other.Style && StyleEx == Other.StyleEx;
    }

    bool operator!=(FWindowsWindowStyle Other) const
    {
        return !(*this == Other);
    }

    /** @brief The standard Windows style flags (e.g., WS_OVERLAPPEDWINDOW). */
    DWORD Style;

    /** @brief The Windows extended style flags (e.g., WS_EX_APPWINDOW). */
    DWORD StyleEx;
};

class COREAPPLICATION_API FWindowsWindow final : public FGenericWindow
{
public:
    static TSharedRef<FWindowsWindow> Create(FWindowsApplication* InApplication);
    
    static const CHAR* GetClassName()
    {
        return "WindowClass";
    }

public:
    ~FWindowsWindow();

    // FGenericWindow Interface Overrides
    virtual bool Initialize(const FGenericWindowInitializer& InInitializer) override final;
    virtual void Show(bool bFocus) override final;
    virtual void Minimize() override final;
    virtual void Maximize() override final;
    virtual void Destroy() override final;
    virtual void Restore() override final;
    virtual void ToggleFullscreen() override final;
    virtual bool IsActiveWindow() const override final;
    virtual bool IsValid() const override final;
    virtual bool IsMinimized() const override final;
    virtual bool IsMaximized() const override final;
    virtual bool IsChildWindow(const TSharedRef<FGenericWindow>& ChildWindow) const override final;
    virtual void SetWindowFocus() override final;
    virtual void SetTitle(const String& Title) override final;
    virtual void GetTitle(String& OutTitle) const override final;
    virtual void SetWindowPos(int32 x, int32 y) override final;
    virtual void SetWindowOpacity(float Alpha) override final;
    virtual void SetWindowShape(const FWindowShape& Shape, bool bMove) override final;
    virtual void GetWindowShape(FWindowShape& OutWindowShape) const override final;
    virtual void GetFullscreenInfo(uint32& OutWidth, uint32& OutHeight) const override final;
    virtual float GetWindowDPIScale() const override final;
    virtual uint32 GetWidth() const override final;
    virtual uint32 GetHeight() const override final;
    virtual void SetStyle(EWindowStyleFlags InStyle) override final;

    virtual EWindowStyleFlags GetStyle() const override final
    {
        return StyleParams;
    }

    virtual void SetAcceptsInput(bool bInAcceptsInput) override final
    {
        bAcceptsInput = bInAcceptsInput;
    }

    virtual bool GetAcceptsInput() const override final
    {
        return bAcceptsInput;
    }

    virtual void SetPlatformHandle(void* InPlatformHandle) override final;

    virtual void* GetPlatformHandle() const override final 
    { 
        return reinterpret_cast<void*>(Window);
    }

    FORCEINLINE HWND GetWindowHandle() const 
    { 
        return Window;
    }

    FORCEINLINE FWindowsApplication* GetApplication() const
    {
        return Application;
    }

private:
    FWindowsWindow(FWindowsApplication* InApplication);

    FWindowsApplication* Application;
    HWND                 Window;
    FWindowsWindowStyle  Style;
    EWindowStyleFlags    StyleParams;
    bool                 bIsFullscreen;
    bool                 bAcceptsInput;
    WINDOWPLACEMENT      StoredPlacement;
};
