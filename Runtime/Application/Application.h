#pragma once
#include "InputHandler.h"
#include "Viewport.h"
#include "ViewportRenderer.h"
#include "Core/Core.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Pair.h"
#include "Core/Time/Timespan.h"
#include "Core/Math/IntVector2.h"
#include "Core/Delegates/Event.h"
#include "CoreApplication/ICursor.h"
#include "CoreApplication/Generic/GenericApplication.h"

class APPLICATION_API FApplication 
    : public FGenericApplicationMessageHandler
{
public:
    FApplication(const TSharedPtr<FGenericApplication>& InPlatformApplication);
    virtual ~FApplication();

    /** @brief - Create the application instance */
    static bool Create();

    /** @brief - Create the application instance from a child-class */
    static bool Create(const TSharedPtr<FApplication>& NewApplication);

    /** @brief - Releases the application instance */
    static void Destroy();

    /** @return - Returns true if the FApplication is initialized */
    static bool IsInitialized() { return GInstance.IsValid(); }

    /** @return - Returns a reference to the FApplication */
    static FApplication& Get() { return GInstance.Dereference(); }
    
public:

    DECLARE_EVENT(FExitEvent, FApplication, int32);
    FExitEvent GetExitEvent() const { return ExitEvent; };
    
    DECLARE_EVENT(FViewportChangedEvent, FApplication, FViewport*);
    FViewportChangedEvent GetViewportChangedEvent() const { return ViewportChangedEvent; }

    /** @return - Returns true if the initialization is true */
    virtual bool Initialize();

    /** @return - Returns true if the initialization of the RHI specific code succeeded. Must be called after the RHI layer is initialized */
    virtual bool InitializeRHI();

    /** @brief - Returns true if the initialization of the RHI specific code succeeded. Must be called after the RHI layer is initialized */
    virtual void ReleaseRHI();

    /** @return - Returns the newly created window */
    virtual FGenericWindowRef CreateWindow();

    /**
     * @brief - Tick and update the FApplication 
     */
    virtual void Tick(FTimespan DeltaTime);

    /**
     * @return - Returns true if the application is running
     */
    virtual bool IsRunning() const { return bIsRunning; }
    
    /**
     * @param Cursor - Cursor to set 
     */
    virtual void SetCursor(ECursor Cursor);

    /**
     * @brief - Set the global cursor position 
     */
    virtual void SetCursorPos(const FIntVector2& Position);

    /**
     * @brief - Set the cursor position relative to a window 
     */
    virtual void SetCursorPos(const FGenericWindowRef& RelativeWindow, const FIntVector2& Position);

    /**
     * @return - Returns  the current global cursor position
     */
    virtual FIntVector2 GetCursorPos() const;

    /**
     * @return - Returns the current cursor position relative to a window 
     */
    virtual FIntVector2 GetCursorPos(const FGenericWindowRef& RelativeWindow) const;

    /**
     * @brief - Set the visibility of the cursor
     */
    virtual void ShowCursor(bool bIsVisible);

    /**
     * @return - Returns true if the cursor is visible
     */
    virtual bool IsCursorVisibile() const;

    /**
     * @return - Returns true if high-precision mouse movement is supported 
     */
    virtual bool SupportsHighPrecisionMouse() const
    {
        return PlatformApplication->SupportsHighPrecisionMouse();
    }

    /**
     * @brief - Enables high-precision mouse movement for a certain window 
     */
    virtual bool EnableHighPrecisionMouseForWindow(const FGenericWindowRef& Window)
    {
        return PlatformApplication->EnableHighPrecisionMouseForWindow(Window);
    }

    /**
     * @brief - Sets the window that should have keyboard focus 
     */
    virtual void SetCapture(const FGenericWindowRef& CaptureWindow);

    /**
     * @brief - Sets the window that should be the active window 
     */
    virtual void SetActiveWindow(const FGenericWindowRef& ActiveWindow);

    /**
     * @return - Returns the window that is currently active
     */
    virtual FGenericWindowRef GetActiveWindow() const
    {
        return PlatformApplication->GetActiveWindow();
    }

    /** 
     * @return - Returns the window that currently is under the cursor 
     */
    virtual FGenericWindowRef GetWindowUnderCursor() const
    {
        return PlatformApplication->GetActiveWindow();
    }

    /**
     * @return - Returns the window that currently has the keyboard focus 
     */
    virtual FGenericWindowRef GetCapture() const
    {
        return PlatformApplication->GetCapture();
    }

    /**
     * @brief - Adds a InputHandler to the application, which gets processed before the main viewport 
     */
    virtual void AddInputHandler(const TSharedPtr<FInputHandler>& NewInputHandler, uint32 Priority);

    /**
     * @brief - Removes a InputHandler from the application 
     */
    virtual void RemoveInputHandler(const TSharedPtr<FInputHandler>& InputHandler);

    /**
     * @brief - Registers the main window of the application
     */
    virtual void RegisterMainViewport(const TSharedRef<FViewport>& NewMainViewport);

    /**
     * @brief - Registers the main window of the application 
     */
    virtual void RegisterViewport(const TSharedRef<FViewport>& NewViewport);

    /**
     * @brief - Registers the main window of the application 
     */
    virtual void UnregisterViewport(const TSharedRef<FViewport>& Viewport);

    /**
     * @brief - Register a window to add that should be drawn the next frame 
     */
    virtual void AddWidget(const TSharedRef<FWidget>& NewWindow);

    /**
     * @brief - Removes a window 
     */
    virtual void RemoveWidget(const TSharedRef<FWidget>& Window);

    /** 
     * @brief - Draws a string in the viewport during the current frame, the strings are reset every frame
     */
    virtual void DrawString(const FString& NewString);

    /**
     * @brief - Draw all InterfaceWindows 
     */
    virtual void DrawWindows(class FRHICommandList& InCommandList);

    /**
     * @return - Returns the Viewport associated with this window or nullptr if no Viewport is registered for this window
     */
    TSharedRef<FViewport> GetViewportFromWindow(const FGenericWindowRef& Window);

    /**
     * @brief - Sets the platform application used to dispatch messages from the platform 
     */
    virtual void SetPlatformApplication(const TSharedPtr<FGenericApplication>& InFPlatformApplication);
    
    /** 
     * @return - Returns the FPlatformApplication 
     */
    virtual TSharedPtr<FGenericApplication> GetPlatformApplication() const { return PlatformApplication; }

    /** 
     * @return - Returns the Viewport registered as the main viewport 
     */
    virtual TSharedRef<FViewport> GetMainViewport() const { return MainViewport; }

    /** 
     * @return - Returns the cursor interface
     */
    virtual TSharedPtr<ICursor> GetCursor() const { return PlatformApplication->Cursor; }

    /**
     * @return - Returns the Renderer
     */
    FViewportRenderer* GetRenderer() const { return Renderer; }

    /**
     * @return - Returns the Context for the UI
     */
    void* GetContext() const { return Context; }

public:
    virtual void OnKeyReleased(EKey KeyCode, FModifierKeyState ModierKeyState) override final;
    
    virtual void OnKeyPressed(EKey KeyCode, bool IsRepeat, FModifierKeyState ModierKeyState) override final;
    
    virtual void OnKeyChar(uint32 Character) override final;

    virtual void OnCursorMove(int32 x, int32 y) override final;
    
    virtual void OnCursorReleased(EMouseButton Button, FModifierKeyState ModierKeyState) override final;
    
    virtual void OnCursorPressed(EMouseButton Button, FModifierKeyState ModierKeyState)  override final;
    
    virtual void OnCursorScrolled(float HorizontalDelta, float VerticalDelta) override final;

    virtual void OnWindowResized(const FGenericWindowRef& Window, uint32 Width, uint32 Height) override final;
    
    virtual void OnWindowMoved(const FGenericWindowRef& Window, int32 x, int32 y) override final;

    virtual void OnWindowFocusLost(const FGenericWindowRef& Window) override final;

    virtual void OnWindowFocusGained(const FGenericWindowRef& Window) override final;
    
    virtual void OnWindowCursorLeft(const FGenericWindowRef& Window) override final;
    
    virtual void OnWindowCursorEntered(const FGenericWindowRef& Window) override final;
    
    virtual void OnWindowClosed(const FGenericWindowRef& Window) override final;

    virtual void OnApplicationExit(int32 ExitCode) override final;

protected:
    void RenderStrings();

    TSharedPtr<FGenericApplication> PlatformApplication;

    TSharedRef<FViewport>         MainViewport;
    TArray<TSharedRef<FViewport>> Viewports;
    FViewportRenderer*            Renderer;

    TArray<FString>             DebugStrings;
    TArray<TSharedRef<FWidget>> InterfaceWindows;

    TArray<TPair<TSharedPtr<FInputHandler>, uint32>> InputHandlers;

    FExitEvent            ExitEvent;
    FViewportChangedEvent ViewportChangedEvent;

    // Is false when the platform application reports that the application should exit
    bool bIsRunning = true;

    struct ImGuiContext* Context = nullptr;

    static TSharedPtr<FApplication> GInstance;
};

