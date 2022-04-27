#pragma once

#if PLATFORM_MACOS
#include "Core/Containers/SharedRef.h"

#include "CoreApplication/Generic/GenericWindow.h"

#if defined(__OBJC__)
@class CCocoaWindow;
@class CCocoaContentView;
#else
class CCocoaWindow;
class CCocoaContentView;
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Mac specific implementation for window interface 

class CMacApplication;

class CMacWindow final : public CGenericWindow
{
public:

	static TSharedRef<CMacWindow> Make(CMacApplication* InApplication);
	
     /** @brief: Initializes the window */
    virtual bool Initialize(const String& InTitle, uint32 InWidth, uint32 InHeight, int32 x, int32 y, SWindowStyle Style) override final;

     /** @brief: Shows the window */
    virtual void Show(bool bMaximized) override final;

     /** @brief: Minimizes the window */
    virtual void Minimize() override final;

     /** @brief: Maximizes the window */
    virtual void Maximize() override final;

     /** @brief: Closes the window */
    virtual void Close() override final;

     /** @brief: Restores the window after being minimized or maximized */
    virtual void Restore() override final;

     /** @brief: Makes the window a borderless fullscreen window */
    virtual void ToggleFullscreen() override final;

     /** @brief: Checks if the underlaying native handle of the window is valid */
    virtual bool IsValid() const override final { return (Window != nullptr); }

     /** @brief: Checks if this window is the currently active window */
    virtual bool IsActiveWindow() const override final;

     /** @brief: Sets the title */
    virtual void SetTitle(const String& Title) override final;

     /** @brief: Retrieve the window title */
    virtual void GetTitle(String& OutTitle) override final;

     /** @brief: Set the shape of the window */
    virtual void SetWindowShape(const SWindowShape& Shape, bool bMove) override final;

     /** @brief: Retrieve the shape of the window */
    virtual void GetWindowShape(SWindowShape& OutWindowShape) const override final;

     /** @brief: Retrieve the width of the window */
    virtual uint32 GetWidth()  const override final;

     /** @brief: Retrieve the height of the window */
    virtual uint32 GetHeight() const override final;

     /** @brief: Set the native handle */
    virtual void SetPlatformHandle(PlatformWindowHandle InPlatformHandle) override final;

     /** @brief: Retrieve the native handle */
    virtual PlatformWindowHandle GetPlatformHandle() const override final
    {
        return reinterpret_cast<PlatformWindowHandle>(Window);
    }

     /** @brief: Get the window */
    FORCEINLINE CCocoaWindow* GetCocoaWindow() const { return Window; }
     /** @brief: Get the content view */
    FORCEINLINE CCocoaWindow* GetCocoaContentView() const { return Window; }

private:

    CMacWindow(CMacApplication* InApplication);
    ~CMacWindow();

     /** @brief: Reference to the parent application */
    CMacApplication* Application = nullptr;

     /** @brief: The native window and view */
    CCocoaWindow*      Window = nullptr;
    CCocoaContentView* View   = nullptr;
};

#endif
