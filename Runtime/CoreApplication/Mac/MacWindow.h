#pragma once
#include "Core/RefCountedBase.h"
#include "Core/Containers/SharedRef.h"
#include "CoreApplication/PlatformInterface/IPlatformWindow.h"

@class FCocoaWindow;
@class FCocoaWindowView;
@class NSScreen;

class FMacApplication;

class COREAPPLICATION_API FMacWindow final : public IPlatformWindow, public FRefCountedBase
{
public:
    static TSharedRef<FMacWindow> Create(FMacApplication* InApplication);
    
public:
    ~FMacWindow();

    // IRefCounted Interface Overrides
    virtual int32 AddRef() const override final
    {
        return FRefCountedBase::AddRef();
    }

    virtual int32 Release() const override final
    {
        return FRefCountedBase::Release();
    }

    virtual int32 GetRefCount() const override final
    {
        return FRefCountedBase::GetRefCount();
    }

    // IPlatformWindow Interface Overrides
    virtual bool Initialize(const FPlatformWindowDesc& InDesc) override final;
    virtual void Destroy() override final;

    virtual void Show(bool bFocus) override final;
    virtual void Minimize() override final;
    virtual void Maximize() override final;
    virtual void Restore() override final;
    virtual void ToggleFullscreen() override final;
    virtual void SetWindowFocus() override final;

    virtual bool IsValid() const override final;
    virtual bool IsActiveWindow() const override final;
    virtual bool IsMinimized() const override final;
    virtual bool IsMaximized() const override final;
    virtual bool IsChildWindow(const TSharedRef<IPlatformWindow>& ChildWindow) const override final;

    virtual void SetTitle(const String& Title) override final;
    virtual void GetTitle(String& OutTitle) const override final;

    virtual void SetWindowPos(int32 x, int32 y) override final;
    virtual void SetWindowShape(const FWindowShape& Shape, bool bMove) override final;
    virtual void GetWindowShape(FWindowShape& OutWindowShape) const override final;
    virtual uint32 GetWidth() const override final;
    virtual uint32 GetHeight() const override final;
    virtual void GetFullscreenInfo(uint32& OutWidth, uint32& OutHeight) const override final;
    virtual float GetWindowDPIScale() const override final;

    virtual void SetStyle(EWindowStyleFlags InStyle) override final;

    virtual EWindowStyleFlags GetStyle() const override final
    {
        return StyleParams;
    }

    virtual void SetWindowOpacity(float Alpha) override final;

    virtual void SetAcceptsInput(bool bInAcceptsInput) override final;

    virtual bool GetAcceptsInput() const override final
    {
        return bAcceptsInput;
    }

    virtual void SetPlatformHandle(void* InPlatformHandle) override final;

    virtual void* GetPlatformHandle() const override final
    {
        return reinterpret_cast<void*>(CocoaWindow);
    }
    
public:
    FORCEINLINE FCocoaWindow* GetCocoaWindow() const
    {
        return CocoaWindow;
    }

    FORCEINLINE FMacApplication* GetApplication() const
    {
        return Application;
    }

    FORCEINLINE void SetCachedPosition(const IntVector2& InPosition)
    {
        Position = InPosition;
    }

    FORCEINLINE const IntVector2& GetCachedPosition() const
    {
        return Position;
    }

private:
    FMacWindow(FMacApplication* InApplication);

    FMacApplication*  Application;
    FCocoaWindow*     CocoaWindow;
    FCocoaWindowView* CocoaWindowView;
    IntVector2        Position;
    EWindowStyleFlags StyleParams;
    bool              bAcceptsInput;
};
