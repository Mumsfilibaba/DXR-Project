#pragma once
#include "InputHandler.h"
#include "Widgets/Viewport.h"
#include "Widgets/Window.h"
#include "ViewportRenderer.h"
#include "Core/Core.h"
#include "Core/Containers/Set.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Pair.h"
#include "Core/Time/Timespan.h"
#include "Core/Math/IntVector2.h"
#include "CoreApplication/Generic/GenericApplication.h"

struct FPriorityInputHandler
{
    FPriorityInputHandler()
        : InputHandler(nullptr)
        , Priority(-1)
    {
    }

    FPriorityInputHandler(const TSharedPtr<FInputHandler>& InInputHandler, uint32 InPriority)
        : InputHandler(InInputHandler)
        , Priority(InPriority)
    {
    }

    bool operator==(const FPriorityInputHandler& Other) const
    {
        return (InputHandler == Other.InputHandler) && (Priority == Other.Priority);
    }

    bool operator!=(const FPriorityInputHandler& Other) const
    {
        return !(*this == Other);
    }

    TSharedPtr<FInputHandler> InputHandler;
    uint32                    Priority;
};


struct FWindowInitializer
{
    FWindowInitializer()
        : Title()
        , Width(1280)
        , Height(720)
        , Position(0, 0)
        , Style(FWindowStyle::Default())
        , ParentWindow(nullptr)
    {
    }

    INITIALIZER_ATTRIBUTE(FString, Title);
    INITIALIZER_ATTRIBUTE(uint32, Width);
    INITIALIZER_ATTRIBUTE(uint32, Height);
    INITIALIZER_ATTRIBUTE(FIntVector2, Position);
    INITIALIZER_ATTRIBUTE(FWindowStyle, Style);
    INITIALIZER_ATTRIBUTE(FGenericWindow*, ParentWindow);
};

class APPLICATION_API FWindowedApplication : public FGenericApplicationMessageHandler
{
public:
    FWindowedApplication();
    virtual ~FWindowedApplication();

    static bool Create();
    
    static void Destroy();

    static bool IsInitialized()
    { 
        return CurrentApplication.IsValid();
    }

    static FWindowedApplication& Get() 
    {
        CHECK(IsInitialized());
        return *CurrentApplication; 
    }

    bool InitializeRenderer();
    
    void ReleaseRenderer();

    void Tick(FTimespan DeltaTime);
    
    void PollInputDevices();
  
public: // FGenericApplicationMessageHandler Interface
    virtual bool OnControllerButtonUp(EControllerButton Button, uint32 ControllerIndex) override final;
    
    virtual bool OnControllerButtonDown(EControllerButton Button, uint32 ControllerIndex) override final;
    
    virtual bool OnControllerAnalog(EControllerAnalog AnalogSource, uint32 ControllerIndex, float AnalogValue) override final;

    virtual bool OnKeyUp(EKey KeyCode, FModifierKeyState ModierKeyState) override final;
    
    virtual bool OnKeyDown(EKey KeyCode, bool bIsRepeat, FModifierKeyState ModierKeyState) override final;
    
    virtual bool OnKeyChar(uint32 Character) override final;

    virtual bool OnMouseButtonUp(EMouseButton Button, FModifierKeyState ModierKeyState, int32 x, int32 y) override final;
    
    virtual bool OnMouseButtonDown(const TSharedRef<FGenericWindow>& Window, EMouseButton Button, FModifierKeyState ModierKeyState, int32 x, int32 y) override final;
    
    virtual bool OnMouseMove(int32 x, int32 y) override final;
    
    virtual bool OnMouseScrolled(float WheelDelta, bool bVertical, int32 x, int32 y) override final;

    virtual bool OnWindowResized(const TSharedRef<FGenericWindow>& Window, uint32 Width, uint32 Height) override final;
    
    virtual bool OnWindowMoved(const TSharedRef<FGenericWindow>& Window, int32 x, int32 y) override final;
    
    virtual bool OnWindowFocusLost(const TSharedRef<FGenericWindow>& Window) override final;
    
    virtual bool OnWindowFocusGained(const TSharedRef<FGenericWindow>& Window) override final;
    
    virtual bool OnWindowMouseLeft(const TSharedRef<FGenericWindow>& Window) override final;
    
    virtual bool OnWindowMouseEntered(const TSharedRef<FGenericWindow>& Window) override final;
    
    virtual bool OnWindowClosed(const TSharedRef<FGenericWindow>& Window) override final;

    virtual bool OnMonitorChange() override final;

public:
    TSharedRef<FGenericWindow> CreateWindow(const FWindowInitializer& Initializer);

    TSharedPtr<FViewport> CreateViewport(const FViewportInitializer& Initializer);

    void SetCursor(ECursor Cursor);

    void SetCursorPos(const FIntVector2& Position);

    FIntVector2 GetCursorPos() const;

    void ShowCursor(bool bIsVisible);

    bool IsCursorVisibile() const;

    bool IsGamePadConnected() const;

    bool EnableHighPrecisionMouseForWindow(const TSharedPtr<FWindow>& Window);

    void SetCapture(const TSharedRef<FGenericWindow>& CaptureWindow);

    void SetActiveWindow(const TSharedRef<FGenericWindow>& ActiveWindow);

    TSharedPtr<FWindow> GetActiveWindow() const;
    
    TSharedPtr<FWindow> GetWindowUnderCursor() const;

    TSharedRef<FGenericWindow> GetCapture() const;

    void AddInputHandler(const TSharedPtr<FInputHandler>& NewInputHandler, uint32 Priority);
    
    void RemoveInputHandler(const TSharedPtr<FInputHandler>& InputHandler);

    void RegisterMainViewport(const TSharedPtr<FViewport>& InViewport);

    void AddWindow(const TSharedPtr<FWindow>& Window);

    void RemoveWindow(const TSharedPtr<FWindow>& Window);

    void DrawWindows(class FRHICommandList& InCommandList);

    TSharedPtr<FWindow> FindWindowFromNativeWindow(const TSharedRef<FGenericWindow>& NativeWindow) const;

    TSharedPtr<FWindow> FindWindowUnderCursor();
    
    void FindWidgetsUnderCursor(FIntVector2 CursorPos, FFilteredWidgets& OutWidgets);

    void OverridePlatformApplication(const TSharedPtr<FGenericApplication>& InPlatformApplication);

    TSharedPtr<FViewport> GetMainViewport() const
    { 
        return MainViewport; 
    }

    TSharedRef<FGenericWindow> GetMainWindow () const
	{
		return MainWindow;
	}

    TSharedPtr<FGenericApplication> GetPlatformApplication()
    {
        return PlatformApplication;
    }

    TSharedPtr<ICursor> GetCursor() const
    {
        return PlatformApplication->Cursor;
    }

    FInputDevice* GetInputDeviceInterface() const
    {
        return PlatformApplication->GetInputDeviceInterface();
    }

    bool SupportsHighPrecisionMouse() const 
    { 
        return PlatformApplication->SupportsHighPrecisionMouse(); 
    }

    void* GetContext() const 
    { 
        return Context; 
    }

    FViewportRenderer* GetRenderer() const
    {
        return Renderer.Get();
    }

    bool IsTrackingMouse() const
    {
        return bIsTrackingMouse;
    }

protected:
    TUniquePtr<FViewportRenderer> Renderer;
    struct ImGuiContext* Context{nullptr};

    bool bIsTrackingMouse;

    TArray<FPriorityInputHandler>      InputHandlers;
    TArray<TSharedRef<FGenericWindow>> AllWindows;
    TArray<TSharedPtr<FViewport>>      Viewports;

    TSharedPtr<FViewport>      MainViewport;
    TSharedRef<FGenericWindow> MainWindow;
    TSharedRef<FGenericWindow> FocusWindow;

    TSet<EKey>         PressedKeys;
    TSet<EMouseButton> PressedMouseButtons;

    /* TODO: Remove */
    TArray<TSharedPtr<FWindow>> Windows;
    TArray<TSharedPtr<FWidget>> TrackedWidgets;
    FFilteredWidgets            FocusPath;
    /* TODO: Remove */

private:
    static TSharedPtr<FWindowedApplication> CurrentApplication;
    static TSharedPtr<FGenericApplication>  PlatformApplication;
};
