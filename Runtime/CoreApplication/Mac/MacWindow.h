#pragma once

#if PLATFORM_MACOS
#include "CocoaContentView.h"
#include "CocoaWindow.h"

#include "Core/Containers/SharedRef.h"

#include "CoreApplication/Interface/PlatformWindow.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMacWindow - Mac specific implementation for window interface

class CMacApplication;

class CMacWindow final : public CPlatformWindow
{
public:

    /**
     * Create new MacWindow object
     *
     * @param InApplication: The application that created the window
     * @return: Returns the newly created window object
     */
    static TSharedRef<CMacWindow> CreateWindow(CMacApplication* InApplication);
    
    /**
     * Retrieve the native window object
     *
     * @return: Returns the native window object
     */
    FORCEINLINE CCocoaWindow* GetCocoaWindow() const
    {
        return Window;
    }

public:
    
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CPlatformWindow Interface
    
    virtual bool Initialize(const String& InTitle, uint32 InWidth, uint32 InHeight, int32 x, int32 y, SWindowStyle Style) override final;

    virtual void Show(bool bMaximized) override final;

    virtual void Minimize() override final;
    virtual void Maximize() override final;

    virtual void Close() override final;

    virtual void Restore() override final;

    virtual void ToggleFullscreen() override final;

    virtual bool IsValid() const override final { return (Window != nullptr); }
    virtual bool IsActiveWindow() const override final;

    virtual void SetTitle(const String& Title) override final;
    virtual void GetTitle(String& OutTitle) override final;

    virtual void SetWindowShape(const SWindowShape& Shape, bool bMove) override final;
    virtual void GetWindowShape(SWindowShape& OutWindowShape) const override final;

    virtual uint32 GetWidth()  const override final;
    virtual uint32 GetHeight() const override final;

    virtual void SetPlatformHandle(PlatformWindowHandle InPlatformHandle) override final;

    virtual PlatformWindowHandle GetPlatformHandle() const override final
    {
        return reinterpret_cast<PlatformWindowHandle>(Window);
    }

private:

    CMacWindow(CMacApplication* InApplication);
    ~CMacWindow();

    CMacApplication* Application = nullptr;

    CCocoaWindow*      Window = nullptr;
    CCocoaContentView* View   = nullptr;
};

#endif
