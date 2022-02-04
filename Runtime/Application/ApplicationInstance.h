#pragma once
#include "InputHandler.h"
#include "WindowMessageHandler.h"
#include "ApplicationUser.h"
#include "IInterfaceRenderer.h"

#include "Core/Core.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/Pair.h"
#include "Core/Time/Timestamp.h"
#include "Core/Math/IntVector2.h"
#include "Core/Delegates/Event.h"

#include "CoreApplication/ICursor.h"
#include "CoreApplication/Interface/PlatformApplication.h"

/*/////////////////////////////////////////////////////////////////////////////////////////////////*/
// CApplicationInstance - Application class for the engine

class APPLICATION_API CApplicationInstance : public CPlatformApplicationMessageHandler
{
public:

    /**
     * Creates a standard main application 
     * 
     * @return: Returns true if the creation was successful
     */
    static bool Make();

    /**
     * Initializes the singleton from an existing application - Used for classes inheriting from CApplicationInstance
     * 
     * @param InApplication: Existing application
     * @return: Returns true if the initialization was successful
     */
    static bool Make(const TSharedPtr<CApplicationInstance>& InApplication);

    /**
     * Releases the global application instance
     */
    static void Release();

    /**
     * Retrieve the singleton application instance 
     * 
     * @return: Returns a reference to the InterfaceApplication instance
     */
    static FORCEINLINE CApplicationInstance& Get() 
    { 
        return *Instance;
    }

    /**
     * Check if the InterfaceApplication is initialized
     * 
     * @return: Returns true if the InterfaceApplication is initialized
     */
    static FORCEINLINE bool IsInitialized() 
    { 
        return Instance.IsValid(); 
    }

    /** Delegate for when the application is about to exit */
    DECLARE_EVENT(CExitEvent, CApplicationInstance, int32);
    CExitEvent GetExitEvent() const { return ExitEvent; }

    /** Delegate for when the application gets a new main-viewport */
    DECLARE_EVENT(CMainViewportChange, CApplicationInstance, const TSharedRef<CPlatformWindow>&);
    CMainViewportChange GetMainViewportChange() const { return MainViewportChange; }

    /**
     * Creates a new window 
     * 
     * @return: Returns the newly created window
     */
    TSharedRef<CPlatformWindow> MakeWindow();

    /**
     * Tick and update the InterfaceApplication
     * 
     * @param DeltaTime: Time between this and the previous update
     */
    void Tick(CTimestamp DeltaTime);
    
    /**
     * Set the current cursor type 
     * 
     * @param Cursor: Cursor to set
     */
    void SetCursor(ECursor Cursor);

    /**
     * Set the global cursor position
     * 
     * @param Position: New position to the cursor
     */
    void SetCursorPos(const CIntVector2& Position);

    /**
     * Set the cursor position relative to a window
     *
     * @param RelativeWindow: Window that the position should be relative to
     * @param Position: New position to the cursor
     */
    void SetCursorPos(const TSharedRef<CPlatformWindow>& RelativeWindow, const CIntVector2& Position);

    /**
     * Retrieve the current global cursor position 
     * 
     * @return: Returns the global cursor position
     */
    CIntVector2 GetCursorPos() const;

    /**
     * Retrieve the current cursor position relative to a window
     *
     * @param RelativeWindow: Window that the position should be relative to
     * @return: Returns the global cursor position
     */
    CIntVector2 GetCursorPos(const TSharedRef<CPlatformWindow>& RelativeWindow) const;

    /**
     * Set the visibility of the cursor 
     * 
     * @param bIsVisible: Should the cursor be visible or not
     */
    void ShowCursor(bool bIsVisible);

    /**
     *  Check the cursor visibility
     * 
     * @return: Returns true if the cursor is visible
     */
    bool IsCursorVisibile() const;

    /**
     * Check if the application supports high-precision mouse movement 
     * 
     * @return: Returns true if the application
     */
    FORCEINLINE bool SupportsHighPrecisionMouse() const 
    { 
        return PlatformApplication->SupportsHighPrecisionMouse(); 
    }

    /**
     * Enables high-precision mouse movement for a certain window
     *
     * @param Window: Window to enable high-precision mouse movement for
     * @return: Returns true if the window now uses high-precision mouse movement
     */
    FORCEINLINE bool EnableHighPrecisionMouseForWindow(const TSharedRef<CPlatformWindow>& Window)
    { 
        return PlatformApplication->EnableHighPrecisionMouseForWindow(Window); 
    }

    /**
     * Sets the window that should have keyboard focus 
     * 
     * @param CaptureWindow: Window that should have keyboard focus
     */
    void SetCapture(const TSharedRef<CPlatformWindow>& CaptureWindow);

    /**
     * Sets the window that should be the active window
     *
     * @param ActiveWindow: Window that should be the active window
     */
    void SetActiveWindow(const TSharedRef<CPlatformWindow>& ActiveWindow);

    /**
     * Retrieves the window that currently has the keyboard focus, can return nullptr
     * 
     * @return: Returns the window that currently has the keyboard focus
     */
    FORCEINLINE TSharedRef<CPlatformWindow> GetCapture() const 
    { 
        return PlatformApplication->GetCapture(); 
    }

    /**
     * Retrieves the window that is currently active
     * 
     * @return: Returns the currently active window
     */
    FORCEINLINE TSharedRef<CPlatformWindow> GetActiveWindow() const 
    { 
        return PlatformApplication->GetActiveWindow(); 
    }

    /**
     * Retrieves the window under the cursor 
     * 
     * @return: Returns the window that currently is under the cursor
     */
    FORCEINLINE TSharedRef<CPlatformWindow> GetWindowUnderCursor() const 
    { 
        return PlatformApplication->GetActiveWindow(); 
    }

    /**
     * Adds a InputHandler to the application, which gets processed before the main viewport
     *
     * @param NewInputHandler: InputHandler to add
     * @param Priority: Priority of the InputHandler
     */
    void AddInputHandler(const TSharedPtr<CInputHandler>& NewInputHandler, uint32 Priority);

    /**
     * Removes a InputHandler from the application 
     * 
     * @param InputHandler: InputHandler to remove
     */
    void RemoveInputHandler(const TSharedPtr<CInputHandler>& InputHandler);

    /**
     * Registers the main window of the application 
     * 
     * @param NewMainViewport: Viewport to be the new main-viewport
     */
    void RegisterMainViewport(const TSharedRef<CPlatformWindow>& NewMainViewport);

    /**
     *  Sets the interface renderer 
     * 
     * @param NewRenderer: New renderer for the InterfaceApplication
     */
    void SetRenderer(const TSharedRef<IInterfaceRenderer>& NewRenderer);

    /**
     * Register a interface window 
     * 
     * @param NewWindow: Window to add that should be drawn the next frame
     */
    void AddWindow(const TSharedRef<IWindow>& NewWindow);

    /**
     * Removes a interface window
     *
     * @param NewWindow: Window to remove
     */
    void RemoveWindow(const TSharedRef<IWindow>& Window);

    /**
     * Draws a string in the viewport during the current frame, the strings are reset every frame 
     * 
     * @param NewString: String to start drawing
     */
    void DrawString(const String& NewString);

    /**
     * Draw all InterfaceWindows
     * 
     * @param InCommandList: CommandList to submit draw-commands into
     */
    void DrawWindows(class CRHICommandList& InCommandList);

    /**
     * Sets the platform application used to dispatch messages from the platform
     * 
     * @param InPlatformApplication: New PlatformApplication
     */
    void SetPlatformApplication(const TSharedPtr<CPlatformApplication>& InPlatformApplication);

    /**
     * Adds a WindowMessageHandler to the application, which gets processed before the application module 
     * 
     * @param NewWindowMessageHandler: WindowMessageHandler to add
     * @param Priority: Priority of the InputHandler
     */
    void AddWindowMessageHandler(const TSharedPtr<CWindowMessageHandler>& NewWindowMessageHandler, uint32 Priority);

    /**
     * Removes a WindowMessageHandler from the application
     *
     * @param WindowMessageHandler: WindowMessageHandler to remove
     */
    void RemoveWindowMessageHandler(const TSharedPtr<CWindowMessageHandler>& WindowMessageHandler);

    /**
     * Retrieve the native application 
     * 
     * @return: Returns the PlatformApplication
     */
    FORCEINLINE TSharedPtr<CPlatformApplication> GetPlatformApplication() const 
    { 
        return PlatformApplication;
    }

    /**
     * Retrieve the renderer for the Application 
     * 
     * @return: Returns the renderer
     */
    FORCEINLINE TSharedRef<IInterfaceRenderer> GetRenderer() const 
    { 
        return Renderer;
    }

    /**
     *  Retrieve the window registered as the main viewport 
     * 
     * @return: Returns the main viewport
     */
    FORCEINLINE TSharedRef<CPlatformWindow> GetMainViewport() const
    { 
        return MainViewport;
    }

    /**
     * Retrieve the cursor interface 
     * 
     * @return: Returns the cursor interface
     */
    FORCEINLINE TSharedPtr<ICursor> GetCursor() const
    { 
        return PlatformApplication->GetCursor();
    }

    /**
     * Check if the application is currently running 
     * 
     * @return: Returns true if the application is running
     */
    FORCEINLINE bool IsRunning() const
    { 
        return bIsRunning;
    }

    /* Get the number of registered users */
    FORCEINLINE uint32 GetNumUsers() const { return static_cast<uint32>(RegisteredUsers.Size()); }

    /* Register a new user to the application */
    FORCEINLINE void RegisterUser(const TSharedPtr<CApplicationUser>& NewUser) { RegisteredUsers.Push(NewUser); }

    /* Retrieve the first user */
    FORCEINLINE TSharedPtr<CApplicationUser> GetFirstUser() const
    {
        if (!RegisteredUsers.IsEmpty())
        {
            return RegisteredUsers.FirstElement();
        }
        else
        {
            return nullptr;
        }
    }

    /* Retrieve a user from user index */
    FORCEINLINE TSharedPtr<CApplicationUser> GetUserFromIndex(uint32 UserIndex) const
    {
        if (UserIndex < (uint32)RegisteredUsers.Size())
        {
            return RegisteredUsers[UserIndex];
        }
        else
        {
            return nullptr;
        }
    }

public:

    /*/////////////////////////////////////////////////////////////////////////////////////////////////*/
    // CPlatformApplicationMessageHandler Interface

    virtual void HandleKeyReleased(EKey KeyCode, SModifierKeyState ModierKeyState) override;
    virtual void HandleKeyPressed(EKey KeyCode, bool IsRepeat, SModifierKeyState ModierKeyState) override;
    virtual void HandleKeyChar(uint32 Character) override;

    virtual void HandleMouseMove(int32 x, int32 y) override;
    virtual void HandleMouseReleased(EMouseButton Button, SModifierKeyState ModierKeyState) override;
    virtual void HandleMousePressed(EMouseButton Button, SModifierKeyState ModierKeyState) override;
    virtual void HandleMouseScrolled(float HorizontalDelta, float VerticalDelta) override;

    virtual void HandleWindowResized(const TSharedRef<CPlatformWindow>& Window, uint32 Width, uint32 Height) override;
    virtual void HandleWindowMoved(const TSharedRef<CPlatformWindow>& Window, int32 x, int32 y) override;
    virtual void HandleWindowFocusChanged(const TSharedRef<CPlatformWindow>& Window, bool HasFocus) override;
    virtual void HandleWindowMouseLeft(const TSharedRef<CPlatformWindow>& Window) override;
    virtual void HandleWindowMouseEntered(const TSharedRef<CPlatformWindow>& Window) override;
    virtual void HandleWindowClosed(const TSharedRef<CPlatformWindow>& Window) override;

    virtual void HandleApplicationExit(int32 ExitCode) override;

protected:

    friend struct TDefaultDelete<CApplicationInstance>;

    CApplicationInstance(const TSharedPtr<CPlatformApplication>& InPlatformApplication);
    virtual ~CApplicationInstance();

    bool CreateContext();

    void HandleKeyEvent(const SKeyEvent& KeyEvent);
    void HandleMouseButtonEvent(const SMouseButtonEvent& MouseButtonEvent);
    void HandleWindowFrameMouseEvent(const SWindowFrameMouseEvent& WindowFrameMouseEvent);

    template<typename MessageHandlerType>
    static void InsertMessageHandler(TArray<TPair<TSharedPtr<MessageHandlerType>, uint32>>& OutMessageHandlerArray, const TSharedPtr<MessageHandlerType>& NewMessageHandler, uint32 NewPriority);

    /** Render all the debug strings and clear the array */
    void RenderStrings();

    TSharedPtr<CPlatformApplication> PlatformApplication;

    TSharedRef<CPlatformWindow>    MainViewport;
    TSharedRef<IInterfaceRenderer> Renderer;

    TArray<String>             DebugStrings;
    TArray<TSharedRef<IWindow>> InterfaceWindows;

    TArray<TPair<TSharedPtr<CInputHandler>, uint32>>         InputHandlers;
    TArray<TPair<TSharedPtr<CWindowMessageHandler>, uint32>> WindowMessageHandlers;

    TArray<TSharedPtr<CApplicationUser>> RegisteredUsers;

    CExitEvent          ExitEvent;
    CMainViewportChange MainViewportChange;

    // Is false when the platform application reports that the application should exit
    bool bIsRunning = true;

    struct ImGuiContext* Context = nullptr;

    static TSharedPtr<CApplicationInstance> Instance;
};

