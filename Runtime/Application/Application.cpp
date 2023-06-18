#include "Application.h"
#include "ImGuiModule.h"
#include "Core/Modules/ModuleManager.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "CoreApplication/Platform/PlatformApplication.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"

IMPLEMENT_ENGINE_MODULE(FModuleInterface, Application);

struct FEventDispatcher
{
public:
    class FLeafLastPolicy
    {
    public:
        FLeafLastPolicy(FFilteredWidgets& InWidgets)
            : Widgets(InWidgets)
            , Index(static_cast<uint32>(InWidgets.LastIndex()))
        {
        }

        bool ShouldProcess() const
        {
            return (Index > 0) && (Widgets.NumChildren() > 0);
        }

        void Next()
        {
            Index--;
        }
        
        TSharedPtr<FWidget>& GetWidget()
        {
            return Widgets[Index];
        }

        const TSharedPtr<FWidget>& GetWidget() const
        {
            return Widgets[Index];
        }

    private:
        FFilteredWidgets& Widgets;
        int32             Index;
    };

    class FLeafFirstPolicy
    {
    public:
        FLeafFirstPolicy(FFilteredWidgets& InWidgets)
            : Widgets(InWidgets)
            , Index(0)
        {
        }

        bool ShouldProcess() const
        {
            return (Index < static_cast<int32>(Widgets.NumChildren()));
        }

        void Next()
        {
            Index++;
        }

        TSharedPtr<FWidget>& GetWidget()
        {
            return Widgets[Index];
        }

        const TSharedPtr<FWidget>& GetWidget() const
        {
            return Widgets[Index];
        }

    private:
        FFilteredWidgets& Widgets;
        int32             Index;
    };

    class FDirectPolicy
    {
    public:
        FDirectPolicy(FFilteredWidgets& InWidgets)
            : Widgets(InWidgets)
            , bIsProcessed(false)
        {
        }

        bool ShouldProcess() const
        {
            return !bIsProcessed;
        }

        void Next()
        {
            bIsProcessed = true;
        }

        TSharedPtr<FWidget>& GetWidget()
        {
            return Widgets[0];
        }

        const TSharedPtr<FWidget>& GetWidget() const
        {
            return Widgets[0];
        }

    private:
        FFilteredWidgets& Widgets;
        bool              bIsProcessed;
    };

public:
    template<typename PolicyType, typename EventType, typename PedicateType>
    static FResponse Dispatch(FApplication* Application, PolicyType Policy, const EventType& Event, PedicateType&& Predicate)
    {
        FResponse Response = FResponse::Unhandled();
        
        for (; !Response.IsEventHandled() && Policy.ShouldProcess(); Policy.Next())
        {
            Response = Predicate(Application, Policy.GetWidget(), Event);
        }

        return Response;
    }
};


TSharedPtr<FApplication>        FApplication::CurrentApplication  = nullptr;
TSharedPtr<FGenericApplication> FApplication::PlatformApplication = nullptr;

bool FApplication::Create()
{
    PlatformApplication = TSharedPtr<FGenericApplication>(FPlatformApplicationMisc::CreateApplication());
    if (!PlatformApplication)
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to create FPlatformApplication");
        return false;
    }

    CurrentApplication = MakeShared<FApplication>();
    PlatformApplication->SetMessageHandler(CurrentApplication);
    return true;
}

void FApplication::Destroy()
{
    if (CurrentApplication)
    {
        CurrentApplication->OverridePlatformApplication(nullptr);
        CurrentApplication.Reset();
    }

    if (PlatformApplication)
    {
        PlatformApplication->SetMessageHandler(nullptr);
        PlatformApplication.Reset();
    }
}

FApplication::FApplication()
    : Renderer(nullptr)
    , DisplayInfo()
    , bIsTrackingMouse(false)
    , InputHandlers()
    , AllWindows()
    , Viewports()
    , MainViewport(nullptr)
    , MainWindow(nullptr)
    , FocusWindow(nullptr)
    , PressedKeys()
    , PressedMouseButtons()
{
    // Create the Context
    FImGui::CreateContext();

    // Configure ImGui
    ImGuiIO& UIState = ImGui::GetIO();
    UIState.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    UIState.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Init monitor information
    UpdateMonitorInfo();

    // Setup the style
    FImGui::InitializeStyle();
}

FApplication::~FApplication()
{
    // Tear down ImGui
    FImGui::DestroyContext();
}

bool FApplication::InitializeRenderer()
{
    Renderer = MakeUnique<FViewportRenderer>();
    if (!Renderer->Initialize())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to init ViewportRenderer ");
        return false;
    }

    return true; 
}

void FApplication::ReleaseRenderer()
{
    Renderer.Reset();
}

void FApplication::Tick(FTimespan DeltaTime)
{
    // Update platform
    const float Delta = static_cast<float>(DeltaTime.AsMilliseconds());
    PlatformApplication->Tick(Delta);

    // TODO: Investigate if this is a problem
    if (!MainWindow)
    {
        return;
    }

    ImGuiIO& UIState = ImGui::GetIO();
    UIState.DeltaTime = static_cast<float>(DeltaTime.AsSeconds());
    
    // Setup the display size of the Main-Window
    UIState.DisplaySize = ImVec2(static_cast<float>(MainWindow->GetWidth()), static_cast<float>(MainWindow->GetHeight()));
    
    // Setup the display scale from the Main-Window
    const float WindowDpiScale = MainWindow->GetWindowDpiScale();
    UIState.FontGlobalScale    = WindowDpiScale;
    UIState.DisplayFramebufferScale = ImVec2(WindowDpiScale, WindowDpiScale);

    // Retrieve the current active window
    TSharedRef<FGenericWindow> ForegroundWindow = GetForegroundWindow();
    
    // Update Mouse
    ImGuiViewport* ForegroundViewport = nullptr;
    if (ForegroundWindow)
    {
        ForegroundViewport = ImGui::FindViewportByPlatformHandle(ForegroundWindow->GetPlatformHandle());
    }

    const bool bIsAppFocused = ForegroundWindow && (ForegroundWindow == MainWindow || MainWindow->IsChildWindow(ForegroundWindow) || ForegroundViewport);
    if (bIsAppFocused)
    {
        if (UIState.WantSetMousePos)
        {
            SetCursorPos(FIntVector2(static_cast<int32>(UIState.MousePos.x), static_cast<int32>(UIState.MousePos.y)));
        }
        else if (!UIState.WantSetMousePos && !bIsTrackingMouse)
        {
            const FIntVector2 CursorPos = GetCursorPos();
            UIState.AddMousePosEvent(static_cast<float>(CursorPos.x), static_cast<float>(CursorPos.y));
        }
    }

    ImGuiID MouseViewportID = 0;
    if (TSharedRef<FGenericWindow> WindowUnderCursor = GetWindowUnderCursor())
    {
        if (ImGuiViewport* Viewport = ImGui::FindViewportByPlatformHandle(WindowUnderCursor->GetPlatformHandle()))
        {
            MouseViewportID = Viewport->ID;
        }
    }

    UIState.AddMouseViewportEvent(MouseViewportID);

    // Update the cursor type
    const bool bNoMouseCursorChange = (UIState.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) != 0;
    if (!bNoMouseCursorChange)
    {
        ImGuiMouseCursor ImguiCursor = ImGui::GetMouseCursor();
        if (ImguiCursor == ImGuiMouseCursor_None || UIState.MouseDrawCursor)
        {
            SetCursor(ECursor::None);
        }
        else
        {
            ECursor Cursor = ECursor::Arrow;
            switch (ImguiCursor)
            {
            case ImGuiMouseCursor_Arrow:      Cursor = ECursor::Arrow;      break;
            case ImGuiMouseCursor_TextInput:  Cursor = ECursor::TextInput;  break;
            case ImGuiMouseCursor_ResizeAll:  Cursor = ECursor::ResizeAll;  break;
            case ImGuiMouseCursor_ResizeEW:   Cursor = ECursor::ResizeEW;   break;
            case ImGuiMouseCursor_ResizeNS:   Cursor = ECursor::ResizeNS;   break;
            case ImGuiMouseCursor_ResizeNESW: Cursor = ECursor::ResizeNESW; break;
            case ImGuiMouseCursor_ResizeNWSE: Cursor = ECursor::ResizeNWSE; break;
            case ImGuiMouseCursor_Hand:       Cursor = ECursor::Hand;       break;
            case ImGuiMouseCursor_NotAllowed: Cursor = ECursor::NotAllowed; break;
            }

            SetCursor(Cursor);
        }
    }

    // Poll input devices
    PollInputDevices();

    UIState.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
    if (IsGamePadConnected())
    {
        UIState.BackendFlags |= ImGuiBackendFlags_HasGamepad;
    }

    // Update all the UI windows
    ImGui::NewFrame();

    //Windows.Foreach([](TSharedRef<FWidget>& Window)
    //{
    //    Window->Tick();
    //});

    static bool ShowDemoWindow    = true;
    static bool ShowAnotherWindow = false;
    static ImVec4 ClearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    if (ShowDemoWindow)
    {
        ImGui::ShowDemoWindow();
    }

    {
        static float SliderValue = 0.0f;
        static int32 Counter     = 0;

        ImGui::Begin("Hello, world!");

        ImGui::Text("This is some useful text.");
        ImGui::Checkbox("Demo Window", &ShowDemoWindow);
        ImGui::Checkbox("Another Window", &ShowAnotherWindow);

        ImGui::SliderFloat("float", &SliderValue, 0.0f, 1.0f);
        ImGui::ColorEdit3("clear color", (float*)&ClearColor);

        if (ImGui::Button("Button"))
        {
            Counter++;
        }

        ImGui::SameLine();
        ImGui::Text("counter = %d", Counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();
    }

    if (ShowAnotherWindow)
    {
        ImGui::Begin("Another Window", &ShowAnotherWindow);
        ImGui::Text("Hello from another window!");

        if (ImGui::Button("Close Me"))
        {
            ShowAnotherWindow = false;
        }

        ImGui::End();
    }

    ImGui::EndFrame();
}

void FApplication::PollInputDevices()
{
    PlatformApplication->PollInputDevices();
}

void FApplication::UpdateMonitorInfo()
{
    PlatformApplication->GetDisplayInfo(DisplayInfo);

    for (FMonitorInfo& MonitorInfo : DisplayInfo.MonitorInfos)
    {
        ImGuiPlatformMonitor ImGuiMonitor;
        ImGuiMonitor.MainPos  = ImVec2(static_cast<float>(MonitorInfo.MainPosition.x), static_cast<float>(MonitorInfo.MainPosition.y));
        ImGuiMonitor.MainSize = ImVec2(static_cast<float>(MonitorInfo.MainSize.x), static_cast<float>(MonitorInfo.MainSize.y));
        ImGuiMonitor.WorkPos  = ImVec2(static_cast<float>(MonitorInfo.WorkPosition.x), static_cast<float>(MonitorInfo.WorkPosition.y));
        ImGuiMonitor.WorkSize = ImVec2(static_cast<float>(MonitorInfo.WorkSize.x), static_cast<float>(MonitorInfo.WorkSize.y));
        ImGuiMonitor.DpiScale = MonitorInfo.DisplayScaling;

        ImGuiPlatformIO& PlatformState = ImGui::GetPlatformIO();
        if (MonitorInfo.bIsPrimary)
        {
            PlatformState.Monitors.push_front(ImGuiMonitor);
        }
        else
        {
            PlatformState.Monitors.push_back(ImGuiMonitor);
        }
    }
}

bool FApplication::OnControllerButtonUp(EControllerButton Button, uint32 ControllerIndex)
{
    // Create the event
    const FControllerEvent ControllerEvent(Button, false, ControllerIndex);

    // Let the InputHandlers handle the event first
    FResponse Response = FResponse::Unhandled();
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const FPriorityInputHandler& Handler = InputHandlers[Index];
        if (Handler.InputHandler->OnControllerButtonUpEvent(ControllerEvent))
        {
            Response = FResponse::Handled();
            break;
        }
    }

    if (Response.IsEventHandled())
    {
        return true;
    }

    Response = FImGui::OnGamepadButtonEvent(ControllerEvent.GetButton(), ControllerEvent.IsButtonDown());

    if (Response.IsEventHandled())
    {
        return true;
    }

    DISABLE_UNREFERENCED_VARIABLE_WARNING

    // Dispatch the events to the widgets in-focus
    Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(FocusPath), ControllerEvent,
        [](FApplication* Application, const TSharedPtr<FWidget>& Widget, const FControllerEvent& ControllerEvent)
        {
            return Widget->OnControllerButtonUp(ControllerEvent);
        });

    ENABLE_UNREFERENCED_VARIABLE_WARNING
    return Response.IsEventHandled();
}

bool FApplication::OnControllerButtonDown(EControllerButton Button, uint32 ControllerIndex)
{
    // Create the event
    const FControllerEvent ControllerEvent(Button, true, ControllerIndex);

    // Let the InputHandlers handle the event first
    FResponse Response = FResponse::Unhandled();
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const FPriorityInputHandler& Handler = InputHandlers[Index];
        if (Handler.InputHandler->OnControllerButtonDownEvent(ControllerEvent))
        {
            Response = FResponse::Handled();
            break;
        }
    }

    if (Response.IsEventHandled())
    {
        return true;
    }

    Response = FImGui::OnGamepadButtonEvent(ControllerEvent.GetButton(), ControllerEvent.IsButtonDown());
    
    if (Response.IsEventHandled())
    {
        return true;
    }

    DISABLE_UNREFERENCED_VARIABLE_WARNING

    // Dispatch the events to the widgets in-focus
    Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(FocusPath), ControllerEvent,
        [](FApplication* Application, const TSharedPtr<FWidget>& Widget, const FControllerEvent& ControllerEvent)
        {
            return Widget->OnControllerButtonDown(ControllerEvent);
        });

    ENABLE_UNREFERENCED_VARIABLE_WARNING
    return Response.IsEventHandled();
}

bool FApplication::OnControllerAnalog(EControllerAnalog AnalogSource, uint32 ControllerIndex, float AnalogValue)
{
    // Create the event
    const FControllerEvent ControllerEvent(AnalogSource, AnalogValue, ControllerIndex);

    // Let the InputHandlers handle the event first
    FResponse Response = FResponse::Unhandled();
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const FPriorityInputHandler& Handler = InputHandlers[Index];
        if (Handler.InputHandler->OnControllerAnalogEvent(ControllerEvent))
        {
            Response = FResponse::Handled();
            break;
        }
    }

    if (Response.IsEventHandled())
    {
        return true;
    }

    Response = FImGui::OnGamepadAnalogEvent(ControllerEvent.GetAnalogSource(), ControllerEvent.GetAnalogValue());
    
    if (Response.IsEventHandled())
    {
        return true;
    }

    DISABLE_UNREFERENCED_VARIABLE_WARNING

    // Dispatch the events to the widgets in-focus
    Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(FocusPath), ControllerEvent,
        [](FApplication* Application, const TSharedPtr<FWidget>& Widget, const FControllerEvent& ControllerEvent)
        {
            return Widget->OnControllerButtonAnalog(ControllerEvent);
        });

    ENABLE_UNREFERENCED_VARIABLE_WARNING
    return Response.IsEventHandled();
}

bool FApplication::OnKeyUp(EKey KeyCode, FModifierKeyState ModierKeyState)
{
    // Create the event
    const FKeyEvent KeyEvent(ModierKeyState, KeyCode, false, false);

    // Let the InputHandlers handle the event first
    FResponse Response = FResponse::Unhandled();
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const FPriorityInputHandler& Handler = InputHandlers[Index];
        if (Handler.InputHandler->OnKeyDownEvent(KeyEvent))
        {
            Response = FResponse::Handled();
            break;
        }
    }

    // Remove the Key
    PressedKeys.erase(KeyCode);

    Response = FImGui::OnKeyEvent(KeyEvent.GetKey(), KeyEvent.IsDown());
    if (Response.IsEventHandled())
    {
        return true;
    }

    DISABLE_UNREFERENCED_VARIABLE_WARNING

    // Dispatch the events to the widgets in-focus
    Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(FocusPath), KeyEvent, 
        [](FApplication* Application, const TSharedPtr<FWidget>& Widget, const FKeyEvent& KeyEvent)
        {
            return Widget->OnKeyUp(KeyEvent);
        });

    ENABLE_UNREFERENCED_VARIABLE_WARNING
    return Response.IsEventHandled();
}

bool FApplication::OnKeyDown(EKey KeyCode, bool bIsRepeat, FModifierKeyState ModierKeyState)
{
    // Create the event
    const FKeyEvent KeyEvent(ModierKeyState, KeyCode, bIsRepeat, true);
    
    // Let the InputHandlers handle the event first
    FResponse Response = FResponse::Unhandled();
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const FPriorityInputHandler& Handler = InputHandlers[Index];
        if (Handler.InputHandler->OnKeyDownEvent(KeyEvent))
        {
            Response = FResponse::Handled();
            break;
        }
    }

    // Add the Key among the pressed keys
    PressedKeys.insert(KeyCode);

    // If the event is handled, abort the process
    if (Response.IsEventHandled())
    {
        return true;
    }

    Response = FImGui::OnKeyEvent(KeyEvent.GetKey(), KeyEvent.IsDown());

    if (Response.IsEventHandled())
    {
        return true;
    }

    DISABLE_UNREFERENCED_VARIABLE_WARNING

    // Dispatch the events to the widgets in-focus
    Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(FocusPath), KeyEvent, 
        [](FApplication* Application, const TSharedPtr<FWidget>& Widget, const FKeyEvent& KeyEvent)
        {
            return Widget->OnKeyDown(KeyEvent);
        });

    ENABLE_UNREFERENCED_VARIABLE_WARNING
    return Response.IsEventHandled();
}

bool FApplication::OnKeyChar(uint32 Character)
{
    // Create the event
    const FKeyEvent KeyEvent(FPlatformApplicationMisc::GetModifierKeyState(), Key_Unknown, Character, false, true);
    
    // Let the InputHandlers handle the event first
    FResponse Response = FResponse::Unhandled();
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const FPriorityInputHandler& Handler = InputHandlers[Index];
        if (Handler.InputHandler->OnKeyCharEvent(KeyEvent))
        {
            Response = FResponse::Handled();
            break;
        }
    }

    // If the event is handled, abort the process
    if (Response.IsEventHandled())
    {
        return true;
    }

    Response = FImGui::OnKeyCharEvent(KeyEvent.GetAnsiChar());

    if (Response.IsEventHandled())
    {
        return true;
    }

    DISABLE_UNREFERENCED_VARIABLE_WARNING

    // Dispatch the events to the widgets in-focus
    Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(FocusPath), KeyEvent, 
        [](FApplication* Application, const TSharedPtr<FWidget>& Widget, const FKeyEvent& KeyEvent)
        {
            return Widget->OnKeyChar(KeyEvent);
        });

    ENABLE_UNREFERENCED_VARIABLE_WARNING
    return Response.IsEventHandled();
}

bool FApplication::OnMouseMove(int32 x, int32 y)
{
    // Create the event
    const FMouseEvent MouseEvent(FIntVector2(x, y), FPlatformApplicationMisc::GetModifierKeyState());
    
    // Let the InputHandlers handle the event first
    FResponse Response = FResponse::Unhandled();
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const FPriorityInputHandler& Handler = InputHandlers[Index];
        if (Handler.InputHandler->OnMouseMove(MouseEvent))
        {
            Response = FResponse::Handled();
            break;
        }
    }

    // If the event is handled, abort the process
    if (Response.IsEventHandled())
    {
        return true;
    }

    Response = FImGui::OnMouseMoveEvent(x, y);

    if (Response.IsEventHandled())
    {
        return true;
    }

    DISABLE_UNREFERENCED_VARIABLE_WARNING

    // Dispatch the MouseEvent to the widgets under the cursor
    Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(FocusPath), MouseEvent,
        [](FApplication* Application, const TSharedPtr<FWidget>& Widget, const FMouseEvent& KeyEvent)
        {
            return Widget->OnMouseMove(KeyEvent);
        });

    ENABLE_UNREFERENCED_VARIABLE_WARNING
    return Response.IsEventHandled();
}

bool FApplication::OnMouseButtonUp(EMouseButton Button, FModifierKeyState ModiferKeyState, int32 x, int32 y)
{
    // Remove the mouse capture if there is a capture
    SetCapture(nullptr);

    // Create the event
    const FMouseEvent MouseEvent(FIntVector2(x, y), ModiferKeyState, Button, false);
    
    // Let the InputHandlers handle the event first
    FResponse Response = FResponse::Unhandled();
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const FPriorityInputHandler& Handler = InputHandlers[Index];
        if (Handler.InputHandler->OnMouseButtonUpEvent(MouseEvent))
        {
            Response = FResponse::Handled();
            break;
        }
    }
    
    // Remove the Key
    PressedMouseButtons.erase(Button);

    // If the event is handled, abort the process
    Response = FImGui::OnMouseButtonEvent(MouseEvent.GetButton(), MouseEvent.IsDown());
    if (Response.IsEventHandled())
    {
        return true;
    }

    DISABLE_UNREFERENCED_VARIABLE_WARNING

    // Dispatch the MouseEvent to the widgets under the cursor
    Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(FocusPath), MouseEvent,
        [](FApplication* Application, const TSharedPtr<FWidget>& Widget, const FMouseEvent& MouseEvent)
        {
            return Widget->OnMouseButtonUp(MouseEvent);
        });

    ENABLE_UNREFERENCED_VARIABLE_WARNING
    return Response.IsEventHandled();
}

bool FApplication::OnMouseButtonDown(const TSharedRef<FGenericWindow>& Window, EMouseButton Button, FModifierKeyState ModierKeyState, int32 x, int32 y)
{
    // Set the mouse capture when the mouse is pressed
    SetCapture(Window);

    // Create the event
    const FMouseEvent MouseEvent(FIntVector2(x, y), ModierKeyState, Button, true);

    // Let the InputHandlers handle the event first
    FResponse Response = FResponse::Unhandled();
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const FPriorityInputHandler& Handler = InputHandlers[Index];
        if (Handler.InputHandler->OnMouseButtonDownEvent(MouseEvent))
        {
            Response = FResponse::Handled();
            break;
        }
    }

    if (Response.IsEventHandled())
    {
        return true;
    }

    Response = FImGui::OnMouseButtonEvent(MouseEvent.GetButton(), MouseEvent.IsDown());

    // Remove the Key
    PressedMouseButtons.insert(Button);

    // If the event is handled, abort the process
    if (Response.IsEventHandled())
    {
        return true;
    }

    // Dispatch the MouseEvent to the widgets under the cursor
    Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(FocusPath), MouseEvent,
        [](FApplication* Application, const TSharedPtr<FWidget>& Widget, const FMouseEvent& MouseEvent)
        {
            return Widget->OnMouseButtonDown(MouseEvent);
        });

    return Response.IsEventHandled();
}

bool FApplication::OnMouseScrolled(float WheelDelta, bool bVertical, int32 x, int32 y)
{
    // Create the event
    const FMouseEvent MouseEvent(FIntVector2(x, y), FPlatformApplicationMisc::GetModifierKeyState(), WheelDelta, bVertical);

    // Let the InputHandlers handle the event first
    FResponse Response = FResponse::Unhandled();
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const FPriorityInputHandler& Handler = InputHandlers[Index];
        if (Handler.InputHandler->OnMouseScrolled(MouseEvent))
        {
            Response = FResponse::Handled();
            break;
        }
    }

    if (Response.IsEventHandled())
    {
        return true;
    }

    Response = FImGui::OnMouseScrollEvent(MouseEvent.GetScrollDelta(), MouseEvent.IsVerticalScrollDelta());
    
    if (Response.IsEventHandled())
    {
        return true;
    }

    DISABLE_UNREFERENCED_VARIABLE_WARNING

    // Dispatch the MouseEvent to the widgets under the cursor
    Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(FocusPath), MouseEvent,
        [](FApplication* Application, const TSharedPtr<FWidget>& Widget, const FMouseEvent& MouseEvent)
        {
            // TODO: return Widget->OnMouseScrolled(MouseEvent);
            return FResponse::Unhandled();
        });

    ENABLE_UNREFERENCED_VARIABLE_WARNING
    return Response.IsEventHandled();
}

bool FApplication::OnWindowResized(const TSharedRef<FGenericWindow>& InWindow, uint32 Width, uint32 Height)
{
    if (ImGuiViewport* Viewport = ImGui::FindViewportByPlatformHandle(InWindow->GetPlatformHandle()))
    {
        Viewport->PlatformRequestResize = true;
    }

    return false;
}

bool FApplication::OnWindowMoved(const TSharedRef<FGenericWindow>& InWindow, int32 x, int32 y)
{
    if (ImGuiViewport* Viewport = ImGui::FindViewportByPlatformHandle(InWindow->GetPlatformHandle()))
    {
        Viewport->PlatformRequestMove = true;
    }

    return false;
}

DISABLE_UNREFERENCED_VARIABLE_WARNING

bool FApplication::OnWindowFocusLost(const TSharedRef<FGenericWindow>& InWindow)
{
    // The state needs to be reset when the window loses focus
    ImGuiIO& UIState = ImGui::GetIO();
    UIState.AddFocusEvent(false);
    return true;
}

bool FApplication::OnWindowFocusGained(const TSharedRef<FGenericWindow>& InWindow)
{
    ImGuiIO& UIState = ImGui::GetIO();
    UIState.AddFocusEvent(true);
    return true;
}

bool FApplication::OnWindowMouseLeft(const TSharedRef<FGenericWindow>& InWindow)
{
    ImGuiIO& UIState = ImGui::GetIO();
    UIState.AddMousePosEvent(-TNumericLimits<float>::Max(), -TNumericLimits<float>::Max());
    return true;
}

bool FApplication::OnWindowMouseEntered(const TSharedRef<FGenericWindow>& InWindow)
{
    return false;
}

bool FApplication::OnWindowClosed(const TSharedRef<FGenericWindow>& InWindow)
{
    if (ImGuiViewport* Viewport = ImGui::FindViewportByPlatformHandle(InWindow->GetPlatformHandle()))
    {
        Viewport->PlatformRequestClose = true;
    }

    if (FGenericWindow* Window = MainViewport->GetWindow())
    {
        if (Window == InWindow)
        {
            RequestEngineExit("Normal Exit");
            MainViewport = nullptr;
        }
    }

    // Remove the viewport
    for (int32 Index = 0; Index < Viewports.Size(); Index++)
    {
        if (Viewports[Index]->GetWindow() == InWindow)
        {
            Viewports.RemoveAt(Index);
            break;
        }
    }

    // Remove the window
    AllWindows.Remove(InWindow);
    return true;
}

bool FApplication::OnMonitorChange()
{
    UpdateMonitorInfo();
    return true;
}

ENABLE_UNREFERENCED_VARIABLE_WARNING

TSharedRef<FGenericWindow> FApplication::CreateWindow(const FWindowInitializer& Initializer)
{
    TSharedRef<FGenericWindow> Window = PlatformApplication->CreateWindow();
    if (Window)
    {
        if (Window->Initialize(Initializer.Title, Initializer.Width, Initializer.Height, Initializer.Position.x, Initializer.Position.y, Initializer.Style, Initializer.ParentWindow))
        {
            AllWindows.Add(Window);
            return Window;
        }
    }

    return nullptr;
}

TSharedPtr<FViewport> FApplication::CreateViewport(const FViewportInitializer& Initializer)
{
    TSharedPtr<FViewport> Viewport = MakeShared<FViewport>();
    if (Viewport)
    {
        if (Viewport->InitializeRHI(Initializer))
        {
            Viewports.Add(Viewport);
            return Viewport;
        }
    }

    return nullptr;
}

void FApplication::SetCursor(ECursor InCursor)
{
    if (TSharedPtr<ICursor> Cursor = GetCursor())
    {
        Cursor->SetCursor(InCursor);
    }
}

void FApplication::SetCursorPos(const FIntVector2& Position)
{
    if (TSharedPtr<ICursor> Cursor = GetCursor())
    {
        Cursor->SetPosition(Position.x, Position.y);
    }
}

FIntVector2 FApplication::GetCursorPos() const
{
    if (TSharedPtr<ICursor> Cursor = GetCursor())
    {
        return Cursor->GetPosition();
    }

    return FIntVector2();
}

void FApplication::ShowCursor(bool bIsVisible)
{
    if (TSharedPtr<ICursor> Cursor = GetCursor())
    {
        Cursor->SetVisibility(bIsVisible);
    }
}

bool FApplication::IsCursorVisibile() const
{
    if (TSharedPtr<ICursor> Cursor = GetCursor())
    {
        return Cursor->IsVisible();
    }

    return false;
}

bool FApplication::IsGamePadConnected() const
{
    if (FInputDevice* InputDevice = GetInputDeviceInterface())
    {
        return InputDevice->IsDeviceConnected();
    }

    return false;
}

bool FApplication::EnableHighPrecisionMouseForWindow(const TSharedRef<FGenericWindow>& Window)
{ 
    if (Window)
    {
        return PlatformApplication->EnableHighPrecisionMouseForWindow(Window);
    }

    return false;
}

void FApplication::SetCapture(const TSharedRef<FGenericWindow>& CaptureWindow)
{
    PlatformApplication->SetCapture(CaptureWindow);
    if (CaptureWindow && !PressedMouseButtons.empty())
    {
        bIsTrackingMouse = true;
    }
    else
    {
        bIsTrackingMouse = false;
    }
}

void FApplication::SetActiveWindow(const TSharedRef<FGenericWindow>& ActiveWindow)
{
    if (ActiveWindow)
    {
        PlatformApplication->SetActiveWindow(ActiveWindow);
    }
}

TSharedRef<FGenericWindow> FApplication::GetActiveWindow() const
{
    CHECK(PlatformApplication != nullptr);
    return PlatformApplication->GetActiveWindow();
}

TSharedRef<FGenericWindow> FApplication::GetWindowUnderCursor() const
{
    CHECK(PlatformApplication != nullptr);
    return PlatformApplication->GetWindowUnderCursor();
}

TSharedRef<FGenericWindow> FApplication::GetCapture() const
{
    CHECK(PlatformApplication != nullptr);
    return PlatformApplication->GetCapture();
}

TSharedRef<FGenericWindow> FApplication::GetForegroundWindow() const
{
	CHECK(PlatformApplication != nullptr);
	return PlatformApplication->GetForegroundWindow();
}

void FApplication::AddInputHandler(const TSharedPtr<FInputHandler>& NewInputHandler, uint32 NewPriority)
{
    FPriorityInputHandler NewPair(NewInputHandler, NewPriority);
    if (!InputHandlers.Contains(NewPair))
    {
        for (int32 Index = 0; Index < InputHandlers.Size(); )
        {
            const FPriorityInputHandler& Handler = InputHandlers[Index];
            if (NewPriority <= Handler.Priority)
            {
                Index++;
                InputHandlers.Insert(Index, NewPair);
                return;
            }
        }

        InputHandlers.Add(NewPair);
    }
}

void FApplication::RemoveInputHandler(const TSharedPtr<FInputHandler>& InputHandler)
{
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const FPriorityInputHandler Handler = InputHandlers[Index];
        if (Handler.InputHandler == InputHandler)
        {
            InputHandlers.RemoveAt(Index);
            return;
        }
    }
}

void FApplication::RegisterMainViewport(const TSharedPtr<FViewport>& InViewport)
{
    if (MainViewport != InViewport)
    {
        MainViewport = InViewport;
        MainWindow   = MakeSharedRef<FGenericWindow>(MainViewport->GetWindow());

        if (MainWindow)
        {
            if (ImGuiViewport* Viewport = ImGui::GetMainViewport())
            {
                // Viewport is now created, make sure we get the initial information of the window
                Viewport->PlatformWindowCreated = true;
                Viewport->PlatformRequestMove   = true;
                Viewport->PlatformRequestResize = true;

                // Set native handles
                Viewport->PlatformUserData = MainWindow.Get();
                Viewport->RendererUserData = InViewport.Get();
                Viewport->PlatformHandle   = Viewport->PlatformHandleRaw = MainWindow->GetPlatformHandle();
            }
        }
    }
}

void FApplication::DrawWindows(FRHICommandList& CommandList)
{
    // NOTE: Renderer is not forced to be valid
    if (Renderer)
    {
        Renderer->Render(CommandList);
    }
}

void FApplication::OverridePlatformApplication(const TSharedPtr<FGenericApplication>& InPlatformApplication)
{
    // Set a MessageHandler to avoid any potential nullptr access
    PlatformApplication->SetMessageHandler(MakeShared<FGenericApplicationMessageHandler>());

    if (InPlatformApplication)
    {
        CHECK(PlatformApplication != InPlatformApplication);
        InPlatformApplication->SetMessageHandler(CurrentApplication);
    }

    PlatformApplication = InPlatformApplication;
}
