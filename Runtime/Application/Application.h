#pragma once
#include "InputHandler.h"
#include "Viewport.h"
#include "Window.h"
#include "ViewportRenderer.h"
#include "Core/Core.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Pair.h"
#include "Core/Time/Timespan.h"
#include "Core/Math/IntVector2.h"
#include "CoreApplication/Generic/GenericApplication.h"

class APPLICATION_API FApplication 
    : public FGenericApplicationMessageHandler
{
public:
    FApplication();
    virtual ~FApplication();

    static bool Create();
    static void Destroy();

    static bool IsInitialized() 
    { 
        return CurrentApplication.IsValid();
    }

    static FApplication& Get() 
    {
        CHECK(IsInitialized());
        return CurrentApplication.Dereference(); 
    }

    virtual void OnKeyUp(EKey KeyCode, FModifierKeyState ModierKeyState) override;
    virtual void OnKeyDown(EKey KeyCode, bool IsRepeat, FModifierKeyState ModierKeyState) override;
    virtual void OnKeyChar(uint32 Character) override;

    virtual void OnMouseMove(int32 x, int32 y) override;
    virtual void OnMouseUp(EMouseButton Button, FModifierKeyState ModierKeyState) override;
    virtual void OnMouseDown(EMouseButton Button, FModifierKeyState ModierKeyState) override;
    virtual void OnMouseScrolled(float HorizontalDelta, float VerticalDelta) override;

    virtual void OnWindowResized(const TSharedRef<FGenericWindow>& Window, uint32 Width, uint32 Height) override;
    virtual void OnWindowMoved(const TSharedRef<FGenericWindow>& Window, int32 x, int32 y) override;
    virtual void OnWindowFocusLost(const TSharedRef<FGenericWindow>& Window) override;
    virtual void OnWindowFocusGained(const TSharedRef<FGenericWindow>& Window) override;
    virtual void OnWindowMouseLeft(const TSharedRef<FGenericWindow>& Window) override;
    virtual void OnWindowMouseEntered(const TSharedRef<FGenericWindow>& Window) override;
    virtual void OnWindowClosed(const TSharedRef<FGenericWindow>& Window) override;

    bool InitializeRenderer();
    void ReleaseRenderer();

    void Tick(FTimespan DeltaTime);

    void SetCursor(ECursor Cursor);
    void SetCursorPos(const FIntVector2& Position);

    FIntVector2 GetCursorPos() const;

    void ShowCursor(bool bIsVisible);

    bool IsCursorVisibile() const;

    bool EnableHighPrecisionMouseForWindow(const TSharedPtr<FWindow>& Window);

    void SetCapture(const TSharedPtr<FWindow>& CaptureWindow);
    void SetActiveWindow(const TSharedPtr<FWindow>& ActiveWindow);

    TSharedPtr<FWindow> GetActiveWindow() const;
    TSharedPtr<FWindow> GetWindowUnderCursor() const;
    TSharedPtr<FWindow> GetCapture() const;

    void AddInputHandler(const TSharedPtr<FInputHandler>& NewInputHandler, uint32 Priority);
    void RemoveInputHandler(const TSharedPtr<FInputHandler>& InputHandler);

    void RegisterMainViewport(const TSharedPtr<FViewport>& NewMainViewport);

    void AddWindow(const TSharedPtr<FWindow>& Window);
    void RemoveWindow(const TSharedPtr<FWindow>& Window);

    void DrawWindows(class FRHICommandList& InCommandList);

    TSharedPtr<FWindow> FindWindowFromNativeWindow(const TSharedRef<FGenericWindow>& NativeWindow) const;

    void OverridePlatformApplication(const TSharedPtr<FGenericApplication>& InPlatformApplication);

    TSharedPtr<FViewport> GetMainViewport() const 
    { 
        return MainViewport; 
    }

    TSharedPtr<FGenericApplication> GetPlatformApplication()
    {
        return PlatformApplication;
    }

    TSharedPtr<ICursor> GetCursor() const
    {
        return PlatformApplication->Cursor;
    }

    bool SupportsHighPrecisionMouse() const 
    { 
        return PlatformApplication->SupportsHighPrecisionMouse(); 
    }

    void* GetContext() const 
    { 
        return Context; 
    }

protected:
    TUniquePtr<FViewportRenderer> Renderer;
    struct ImGuiContext* Context  = nullptr;

    TArray<TSharedPtr<FWindow>> Windows;
    TSharedPtr<FViewport>       MainViewport;

    typedef TPair<TSharedPtr<FInputHandler>, uint32> FInputHandlerPair; 
    TArray<FInputHandlerPair> InputHandlers;

private:
    static TSharedPtr<FApplication>        CurrentApplication;
    static TSharedPtr<FGenericApplication> PlatformApplication;
};

