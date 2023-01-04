#pragma once
#include "Core/Windows/Windows.h"
#include "Core/Containers/SharedRef.h"
#include "CoreApplication/CoreApplication.h"
#include "CoreApplication/Generic/GenericWindow.h"

class FWindowsApplication;

typedef TSharedRef<class FWindowsWindow> FWindowsWindowRef;

class COREAPPLICATION_API FWindowsWindow final 
    : public FGenericWindow
{
public:
    FWindowsWindow(FWindowsApplication* InApplication);
    ~FWindowsWindow();

    static const CHAR* GetClassName();

public:
    virtual bool Initialize(
        const FString& Title,
        uint32 InWidth,
        uint32 InHeight,
        int32 x,
        int32 y,
        FWindowStyle Style) override final;

    virtual void Show(bool bMaximized) override final;

    virtual void Minimize() override final;

    virtual void Maximize() override final;

    virtual void Close() override final;

    virtual void Restore() override final;

    virtual void ToggleFullscreen() override final;

    virtual bool IsActiveWindow() const override final;
    
    virtual bool IsValid() const override final;

    virtual void SetTitle(const FString& Title) override final;
    
    virtual void GetTitle(FString& OutTitle) override final;

    virtual void MoveTo(int32 x, int32 y) override final;

    virtual void SetWindowShape(const FWindowShape& Shape, bool bMove) override final;
    
    virtual void GetWindowShape(FWindowShape& OutWindowShape) const override final;

    virtual void GetFullscreenInfo(uint32& OutWidth, uint32& OutHeight) const override final;

    virtual uint32 GetWidth() const override final;

    virtual uint32 GetHeight() const override final;

    virtual void SetPlatformHandle(void* InPlatformHandle) override final;

    virtual void* GetPlatformHandle() const override final { return reinterpret_cast<void*>(Window); }

    FORCEINLINE HWND GetWindowHandle() const 
    { 
        return Window;
    }

private:
    FWindowsApplication* Application;

    HWND            Window;

    DWORD           Style;
    DWORD           StyleEx;

    bool            bIsFullscreen;

    WINDOWPLACEMENT StoredPlacement;
};
