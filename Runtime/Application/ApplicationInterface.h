#pragma once
#include "InputHandler.h"
#include "WindowMessageHandler.h"
#include "User.h"
#include "IViewportRenderer.h"

#include "Core/Core.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Pair.h"
#include "Core/Time/Timespan.h"
#include "Core/Math/IntVector2.h"
#include "Core/Delegates/Event.h"

#include "CoreApplication/ICursor.h"
#include "CoreApplication/Generic/GenericApplication.h"

class APPLICATION_API FApplicationInterface 
    : public FGenericApplicationMessageHandler
{
protected:
    FApplicationInterface() = default;

public:
    virtual ~FApplicationInterface() = default;

    /** @brief - Create the application instance */
    static bool Create();

    /** @brief - Releases the application instance */
    static void Release();

    /** @return - Returns true if the FApplicationInterface is initialized */
    static bool IsInitialized() { return GInstance.IsValid(); }

    /** @return - Returns a reference to the FApplicationInterface */
    static FApplicationInterface& Get() { return GInstance.Dereference(); }
    
public:
    DECLARE_EVENT(FExitEvent, FApplicationInterface, int32);
    FExitEvent GetExitEvent() const { return ExitEvent; };
    
    DECLARE_EVENT(FViewportChangedEvent, FApplicationInterface, const FGenericWindowRef&);
    FViewportChangedEvent GetViewportChangedEvent() const { return ViewportChangedEvent; };

    /** @return - Returns the newly created window */
    virtual FGenericWindowRef CreateWindow() = 0;

    /** @brief - Tick and update the FApplicationInterface */
    virtual void Tick(FTimespan DeltaTime) = 0;

    /** @return - Returns true if the application is running */
    virtual bool IsRunning() const = 0;
    
    /** @param Cursor - Cursor to set */
    virtual void SetCursor(ECursor Cursor) = 0;

    /** @brief - Set the global cursor position */
    virtual void SetCursorPos(const FIntVector2& Position) = 0;

    /** @brief - Set the cursor position relative to a window */
    virtual void SetCursorPos(const FGenericWindowRef& RelativeWindow, const FIntVector2& Position) = 0;

    /** @return - Returns  the current global cursor position */
    virtual FIntVector2 GetCursorPos() const = 0;

    /** @return - Returns the current cursor position relative to a window */
    virtual FIntVector2 GetCursorPos(const FGenericWindowRef& RelativeWindow) const = 0;

    /** @brief - Set the visibility of the cursor */
    virtual void ShowCursor(bool bIsVisible) = 0;

    /** @return - Returns true if the cursor is visible */
    virtual bool IsCursorVisibile() const = 0;

    /** @return - Returns true if high-precision mouse movement is supported */
    virtual bool SupportsHighPrecisionMouse() const = 0;

    /** @brief - Enables high-precision mouse movement for a certain window */
    virtual bool EnableHighPrecisionMouseForWindow(const FGenericWindowRef& Window) = 0;

    /** @brief - Sets the window that should have keyboard focus */
    virtual void SetCapture(const FGenericWindowRef& CaptureWindow) = 0;

    /** @brief - Sets the window that should be the active window */
    virtual void SetActiveWindow(const FGenericWindowRef& ActiveWindow) = 0;

    /** @return - Returns the window that is currently active */
    virtual FGenericWindowRef GetActiveWindow() const = 0;

    /** @return - Returns the window that currently is under the cursor */
    virtual FGenericWindowRef GetWindowUnderCursor() const = 0;

    /** @return - Returns the window that currently has the keyboard focus */
    virtual FGenericWindowRef GetCapture() const = 0;

    /** @brief - Adds a InputHandler to the application, which gets processed before the main viewport */
    virtual void AddInputHandler(const TSharedPtr<FInputHandler>& NewInputHandler, uint32 Priority) = 0;

    /** @brief - Removes a InputHandler from the application */
    virtual void RemoveInputHandler(const TSharedPtr<FInputHandler>& InputHandler) = 0;

    /** @brief - Registers the main window of the application */
    virtual void RegisterMainViewport(const FGenericWindowRef& NewMainViewport) = 0;

    /** @brief -  Sets the interface renderer */
    virtual void SetRenderer(const TSharedRef<IViewportRenderer>& NewRenderer) = 0;

    /** @brief - Register a window to add that should be drawn the next frame */
    virtual void AddWindow(const TSharedRef<FWindow>& NewWindow) = 0;

    /** @brief - Removes a window */
    virtual void RemoveWindow(const TSharedRef<FWindow>& Window) = 0;

    /** @brief - Draws a string in the viewport during the current frame, the strings are reset every frame */
    virtual void DrawString(const FString& NewString) = 0;

    /** @brief - Draw all InterfaceWindows */
    virtual void DrawWindows(class FRHICommandList& InCommandList) = 0;

    /** @brief - Sets the platform application used to dispatch messages from the platform */
    virtual void SetPlatformApplication(const TSharedPtr<FGenericApplication>& InFPlatformApplication) = 0;

    /** @brief - Adds a WindowMessageHandler to the application, which gets processed before the application module */
    virtual void AddWindowMessageHandler(const TSharedPtr<FWindowMessageHandler>& NewWindowMessageHandler, uint32 Priority) = 0;

    /** @brief - Removes a WindowMessageHandler from the application */
    virtual void RemoveWindowMessageHandler(const TSharedPtr<FWindowMessageHandler>& WindowMessageHandler) = 0;

    /** @return - Returns the FPlatformApplication */
    virtual TSharedPtr<FGenericApplication> GetPlatformApplication() const = 0;

    /** @return - Returns the renderer for the Application */
    virtual TSharedRef<IViewportRenderer> GetRenderer() const = 0;

    /** @return - Returns the window registered as the main viewport */
    virtual FGenericWindowRef GetMainViewport() const = 0;

    /** @return - Returns the cursor interface */
    virtual TSharedPtr<ICursor> GetCursor() const = 0;

public:

     /** @brief - Get the number of registered users */
    virtual uint32 GetNumUsers() const = 0;

     /** @brief - Register a new user to the application */
    virtual void RegisterUser(const TSharedPtr<FUser>& NewUser) = 0;

     /** @brief - Retrieve the first user */
    virtual TSharedPtr<FUser> GetFirstUser() const = 0;

    /** @brief - Retrieve a user from user index */
    virtual TSharedPtr<FUser> GetUserFromIndex(uint32 UserIndex) const = 0;

protected:
    void ForwardExitEvent(int32 ExitCode) { ExitEvent.Broadcast(ExitCode); }
    void ForwardViewportChangedEvent(const FGenericWindowRef& NewMainViewport) { ViewportChangedEvent.Broadcast(NewMainViewport); }

	FExitEvent            ExitEvent;
	FViewportChangedEvent ViewportChangedEvent;

    static TSharedPtr<FApplicationInterface> GInstance;
};

