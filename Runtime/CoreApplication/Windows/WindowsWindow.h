#pragma once
#include "Core/Windows/Windows.h"
#include "Core/Containers/SharedRef.h"
#include "CoreApplication/Generic/GenericWindow.h"

class FWindowsApplication;

struct FWindowsWindowStyle
{
    FWindowsWindowStyle() = default;

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

    DWORD Style{0};
    DWORD StyleEx{0};
};

class COREAPPLICATION_API FWindowsWindow final : public FGenericWindow
{
public:
    FWindowsWindow(FWindowsApplication* InApplication);
    ~FWindowsWindow();

    static const CHAR* GetClassName();

public:
    virtual bool Initialize(
        const FString&  Title,
        uint32          InWidth,
        uint32          InHeight,
        int32           x,
        int32           y,
        FWindowStyle    Style,
        FGenericWindow* InParentWindow) override final;

    virtual void Show(bool bFocusOnActivate) override final;

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

    virtual void SetTitle(const FString& Title) override final;
    
    virtual void GetTitle(FString& OutTitle) const override final;

    virtual void SetWindowPos(int32 x, int32 y) override final;

    virtual void SetWindowOpacity(float Alpha) override final;

    virtual void SetWindowShape(const FWindowShape& Shape, bool bMove) override final;
    
    virtual void GetWindowShape(FWindowShape& OutWindowShape) const override final;

    virtual void GetFullscreenInfo(uint32& OutWidth, uint32& OutHeight) const override final;

    virtual float GetWindowDpiScale() const override final;

    virtual uint32 GetWidth() const override final;

    virtual uint32 GetHeight() const override final;

    virtual void SetPlatformHandle(void* InPlatformHandle) override final;

    virtual void* GetPlatformHandle() const override final 
    { 
        return reinterpret_cast<void*>(Window);
    }

    virtual void SetStyle(FWindowStyle InStyle) override final;

    FORCEINLINE HWND GetWindowHandle() const 
    { 
        return Window;
    }

private:
    FWindowsApplication* Application;
    HWND                 Window;
    FWindowsWindowStyle  Style;
    bool                 bIsFullscreen;
    WINDOWPLACEMENT      StoredPlacement;
};
