#pragma once
#include "Core/Containers/SharedRef.h"
#include "CoreApplication/Generic/GenericWindow.h"

@class FCocoaWindow;
@class FCocoaWindowView;
@class NSScreen;

class FMacApplication;

/**
 * @class FMacWindow
 * @brief A macOS-specific implementation of the FGenericWindow interface.
 *
 * FMacWindow manages the lifecycle of a window on macOS platforms. It integrates
 * with native Objective-C classes (FCocoaWindow, FCocoaWindowView) to control
 * a window's creation, destruction, attributes, and interactions.
 */

class COREAPPLICATION_API FMacWindow final : public FGenericWindow
{
public:
    
    /**
     * @brief Creates a new FMacWindow instance and returns it as a shared reference.
     * 
     * This static factory function simplifies creation and ensures that each FMacWindow
     * is managed by a shared pointer, thereby avoiding manual memory management.
     * 
     * @param InApplication Pointer to the FMacApplication instance that manages this window.
     * @return A shared reference to the newly created FMacWindow instance.
     */
    static TSharedRef<FMacWindow> Create(FMacApplication* InApplication);
    
public:

    /** @brief Destructor */
    ~FMacWindow();

public:

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

    virtual void SetTitle(const FString& Title) override final;

    virtual void GetTitle(FString& OutTitle) const override final;

    virtual void SetWindowPos(int32 x, int32 y) override final;

    virtual void SetWindowOpacity(float Alpha) override final;

    virtual void SetWindowShape(const FWindowShape& Shape, bool bMove) override final;

    virtual void GetWindowShape(FWindowShape& OutWindowShape) const override final;

    virtual void GetFullscreenInfo(uint32& OutWidth, uint32& OutHeight) const override final;

    virtual float GetWindowDPIScale() const override final;

    virtual uint32 GetWidth() const override final;

    virtual uint32 GetHeight() const override final;

    virtual void SetStyle(EWindowStyleFlags InStyle) override final;

    virtual void SetPlatformHandle(void* InPlatformHandle) override final;

    virtual void* GetPlatformHandle() const override final
    {
        return reinterpret_cast<void*>(CocoaWindow);
    }
    
public:

    /**
     * @brief Retrieves the underlying FCocoaWindow instance.
     * 
     * @return A pointer to the native Objective-C window (FCocoaWindow).
     */
    FORCEINLINE FCocoaWindow* GetCocoaWindow() const
    {
        return CocoaWindow;
    }

    /**
     * @brief Retrieves the FMacApplication instance managing this window.
     * 
     * @return A pointer to the FMacApplication.
     */
    FORCEINLINE FMacApplication* GetApplication() const
    {
        return Application;
    }

    /**
     * @brief Caches the window's position locally to avoid redundant reposition events.
     * 
     * @param InPosition The new position as an FIntVector2.
     */
    FORCEINLINE void SetCachedPosition(const FIntVector2& InPosition)
    {
        Position = InPosition;
    }

    /**
     * @brief Retrieves the cached window position.
     * 
     * @return A const reference to the cached window position as an FIntVector2.
     */
    FORCEINLINE const FIntVector2& GetCachedPosition() const
    {
        return Position;
    }

private:
    FMacWindow(FMacApplication* InApplication);

    FMacApplication*  Application;
    FCocoaWindow*     CocoaWindow;
    FCocoaWindowView* CocoaWindowView;
    FIntVector2       Position;
};
