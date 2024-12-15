#pragma once
#include "Core/Windows/Windows.h"
#include "Core/Containers/SharedRef.h"
#include "CoreApplication/Generic/GenericWindow.h"

class FWindowsApplication;

/**
 * @struct FWindowsWindowStyle
 * @brief Encapsulates Windows-specific style flags for a window.
 *
 * This structure holds both the standard and extended style flags (Style and StyleEx)
 * used by the Windows API to define the appearance and behavior of a window. Common
 * values include WS_OVERLAPPEDWINDOW (standard) and WS_EX_APPWINDOW (extended).
 */
struct FWindowsWindowStyle
{
    /**
     * @brief Default constructor, initializes style flags to zero.
     */
    FWindowsWindowStyle()
        : Style(0)
        , StyleEx(0)
    {
    }

    /**
     * @brief Constructs a FWindowsWindowStyle with given standard and extended style flags.
     * 
     * @param InStyle The standard Windows style flags.
     * @param InStyleEx The Windows extended style flags.
     */
    FWindowsWindowStyle(DWORD InStyle, DWORD InStyleEx)
        : Style(InStyle)
        , StyleEx(InStyleEx)
    {
    }

    /**
     * @brief Equality operator to compare two window styles.
     * 
     * @param Other The other FWindowsWindowStyle to compare against.
     * @return True if both the Style and StyleEx fields match.
     */
    bool operator==(FWindowsWindowStyle Other) const
    {
        return Style == Other.Style && StyleEx == Other.StyleEx;
    }

    /**
     * @brief Inequality operator to compare two window styles.
     * 
     * @param Other The other FWindowsWindowStyle to compare against.
     * @return True if either the Style or StyleEx fields differ.
     */
    bool operator!=(FWindowsWindowStyle Other) const
    {
        return !(*this == Other);
    }

    /** @brief The standard Windows style flags (e.g., WS_OVERLAPPEDWINDOW). */
    DWORD Style;

    /** @brief The Windows extended style flags (e.g., WS_EX_APPWINDOW). */
    DWORD StyleEx;
};

/**
 * @class FWindowsWindow
 * @brief A Windows-specific implementation of a generic engine window.
 *
 * FWindowsWindow provides methods to create, show, hide, resize, and otherwise manage a
 * window on the Windows platform. It wraps the native HWND handle while conforming to the
 * FGenericWindow interface used by the engine.
 */
class COREAPPLICATION_API FWindowsWindow final : public FGenericWindow
{
public:

    /**
     * @brief Creates a new FWindowsWindow instance and returns it as a shared reference.
     * 
     * This static factory function simplifies creation and ensures that each FWindowsWindow
     * is managed by a shared pointer, helping to maintain clean resource management.
     * 
     * @param InApplication A pointer to the FWindowsApplication that manages this window.
     * @return A shared reference to the new FWindowsWindow instance.
     */
    static TSharedRef<FWindowsWindow> Create(FWindowsApplication* InApplication);
    
    /**
     * @brief Retrieves the class name used when registering the window class.
     * 
     * This class name is used by the Windows API when creating the window. The class name
     * must be registered before the window can be created.
     * 
     * @return The class name as a const C-string.
     */
    static const CHAR* GetClassName() { return "WindowClass"; }

public:

    /** @brief Destructor */
    ~FWindowsWindow();

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
        return reinterpret_cast<void*>(Window);
    }

public:
    
    /**
     * @brief Gets the underlying native HWND handle for this window.
     * 
     * @return The HWND for this window.
     */
    FORCEINLINE HWND GetWindowHandle() const 
    { 
        return Window;
    }

    /**
     * @brief Retrieves the application that manages this window.
     * 
     * @return A pointer to the FWindowsApplication instance.
     */
    FORCEINLINE FWindowsApplication* GetApplication() const
    {
        return Application;
    }

private:
    FWindowsWindow(FWindowsApplication* InApplication);

    FWindowsApplication* Application;
    HWND                 Window;
    FWindowsWindowStyle  Style;
    bool                 bIsFullscreen;
    WINDOWPLACEMENT      StoredPlacement;
};
