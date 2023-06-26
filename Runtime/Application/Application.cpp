#include "Application.h"
#include "ImGuiModule.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Modules/ModuleManager.h"
#include "CoreApplication/Platform/PlatformApplication.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"

IMPLEMENT_ENGINE_MODULE(FModuleInterface, Application);

struct FEventDispatcher
{
    class FLowPriorityFirstPolicy
    {
    public:
        FLowPriorityFirstPolicy(TArray<FApplicationEventHandlerRef>& InEventHandlers)
            : EventHandlers(InEventHandlers)
            , Index(static_cast<int32>(InEventHandlers.LastElementIndex()))
        {
        }

        bool ShouldProcess() const
        {
            return (Index > 0) && (EventHandlers.Size() > 0);
        }

        void Next()
        {
            Index--;
        }
        
        FApplicationEventHandlerRef& GetEventHandler()
        {
            return EventHandlers[Index];
        }

        const FApplicationEventHandlerRef& GetEventHandler() const
        {
            return EventHandlers[Index];
        }

    private:
        TArray<FApplicationEventHandlerRef>& EventHandlers;
        int32 Index;
    };


    class FHighPriorityFirstPolicy
    {
    public:
        FHighPriorityFirstPolicy(TArray<FApplicationEventHandlerRef>& InEventHandlers)
            : EventHandlers(InEventHandlers)
            , Index(0)
        {
        }

        bool ShouldProcess() const
        {
            return Index < static_cast<int32>(EventHandlers.Size());
        }

        void Next()
        {
            Index++;
        }

        FApplicationEventHandlerRef& GetEventHandler()
        {
            return EventHandlers[Index];
        }

        const FApplicationEventHandlerRef& GetEventHandler() const
        {
            return EventHandlers[Index];
        }

    private:
        TArray<FApplicationEventHandlerRef>& EventHandlers;
        int32 Index;
    };


    class FDirectPolicy
    {
    public:
        FDirectPolicy(TArray<FApplicationEventHandlerRef>& InEventHandlers)
            : EventHandlers(InEventHandlers)
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

        FApplicationEventHandlerRef& GetEventHandler()
        {
            return EventHandlers[0];
        }

        const FApplicationEventHandlerRef& GetEventHandler() const
        {
            return EventHandlers[0];
        }

    private:
        TArray<FApplicationEventHandlerRef>& EventHandlers;
        bool bIsProcessed;
    };


    class FPreProcessPolicy
    {
    public:
        FPreProcessPolicy(TArray<FInputPreProcessorAndPriority>& InEventHandlers)
            : EventHandlers(InEventHandlers)
            , Index(0)
        {
        }

        bool ShouldProcess() const
        {
            return Index < static_cast<int32>(EventHandlers.Size());
        }

        void Next()
        {
            Index++;
        }

        FInputPreProcessorAndPriority& GetEventHandler()
        {
            return EventHandlers[Index];
        }

        const FInputPreProcessorAndPriority& GetEventHandler() const
        {
            return EventHandlers[Index];
        }

    private:
        TArray<FInputPreProcessorAndPriority>& EventHandlers;
        int32 Index;
    };


    template<typename PolicyType, typename EventType, typename PedicateType>
    static FResponse PreProcess(PolicyType Policy, const EventType& Event, PedicateType&& Predicate)
    {
        FResponse Response = FResponse::Unhandled();
        for (; !Response.IsEventHandled() && Policy.ShouldProcess(); Policy.Next())
        {
            if (Predicate(Policy.GetEventHandler(), Event))
            {
                Response = FResponse::Handled();
            }
        }

        return Response;
    }


    template<typename PolicyType, typename EventType, typename PedicateType>
    static FResponse Dispatch(PolicyType Policy, const EventType& Event, PedicateType&& Predicate)
    {
        FResponse Response = FResponse::Unhandled();
        for (; !Response.IsEventHandled() && Policy.ShouldProcess(); Policy.Next())
        {
            Response = Predicate(Policy.GetEventHandler(), Event);
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
    , MainViewport(nullptr)
    , MainWindow(nullptr)
    , FocusWindow(nullptr)
    , EventHandlers()
    , Widgets()
    , InputPreProcessors()
    , AllWindows()
    , DisplayInfo()
    , bIsTrackingMouse(false)
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

bool FApplication::InitializeRenderer()
{
    Renderer = MakeUnique<FImGuiRenderer>();
    if (!Renderer->Initialize())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to init ViewportRenderer ");
        return false;
    }

    return true;
}

void FApplication::ReleaseRenderer()
{
    // Ensure the renderer gets destroyed
    Renderer.Reset();

    // Tear down ImGui
    FImGui::DestroyContext();
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

    Widgets.Foreach([](FWidgetRef& Widget)
    {
        Widget->Paint();
    });

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

bool FApplication::OnControllerButtonUp(EGamepadButtonName Button, uint32 ControllerIndex)
{
    // Create the event
    const FControllerEvent ControllerEvent(Button, ControllerIndex, false, false);

    // Let the InputPreProcessors handle the event first
    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputPreProcessors), ControllerEvent,
        [](const FInputPreProcessorAndPriority& PreProcessor, const FControllerEvent& ControllerEvent)
        {
            return PreProcessor.InputHandler->OnControllerButtonUp(ControllerEvent);
        });

    if (Response.IsEventHandled())
    {
        return true;
    }

    Response = FImGui::OnGamepadButtonEvent(ControllerEvent.GetButton(), ControllerEvent.IsButtonDown());

    if (Response.IsEventHandled())
    {
        return true;
    }

    // Dispatch the events to the widgets in-focus
    Response = FEventDispatcher::Dispatch(FEventDispatcher::FHighPriorityFirstPolicy(EventHandlers), ControllerEvent,
        [](const FApplicationEventHandlerRef& EventHandler, const FControllerEvent& ControllerEvent)
        {
            return EventHandler->OnControllerButtonUp(ControllerEvent);
        });

    return Response.IsEventHandled();
}

bool FApplication::OnControllerButtonDown(EGamepadButtonName Button, uint32 ControllerIndex, bool bIsRepeat)
{
    // Create the event
    const FControllerEvent ControllerEvent(Button, ControllerIndex, true, bIsRepeat);

    // Let the InputPreProcessors handle the event first
    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputPreProcessors), ControllerEvent,
        [](const FInputPreProcessorAndPriority& PreProcessor, const FControllerEvent& ControllerEvent)
        {
            return PreProcessor.InputHandler->OnControllerButtonDown(ControllerEvent);
        });

    if (Response.IsEventHandled())
    {
        return true;
    }

    Response = FImGui::OnGamepadButtonEvent(ControllerEvent.GetButton(), ControllerEvent.IsButtonDown());
    
    if (Response.IsEventHandled())
    {
        return true;
    }

    // Dispatch the events to the widgets in-focus
    Response = FEventDispatcher::Dispatch(FEventDispatcher::FHighPriorityFirstPolicy(EventHandlers), ControllerEvent,
        [](const FApplicationEventHandlerRef& EventHandler, const FControllerEvent& ControllerEvent)
        {
            return EventHandler->OnControllerButtonDown(ControllerEvent);
        });

    return Response.IsEventHandled();
}

bool FApplication::OnControllerAnalog(EAnalogSourceName AnalogSource, uint32 ControllerIndex, float AnalogValue)
{
    // Create the event
    const FControllerEvent ControllerEvent(AnalogSource, ControllerIndex, AnalogValue);

    // Let the InputPreProcessors handle the event first
    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputPreProcessors), ControllerEvent,
        [](const FInputPreProcessorAndPriority& PreProcessor, const FControllerEvent& ControllerEvent)
        {
            return PreProcessor.InputHandler->OnControllerAnalog(ControllerEvent);
        });

    if (Response.IsEventHandled())
    {
        return true;
    }

    Response = FImGui::OnGamepadAnalogEvent(ControllerEvent.GetAnalogSource(), ControllerEvent.GetAnalogValue());
    
    if (Response.IsEventHandled())
    {
        return true;
    }

    // Dispatch the events to the widgets in-focus
    Response = FEventDispatcher::Dispatch(FEventDispatcher::FHighPriorityFirstPolicy(EventHandlers), ControllerEvent,
        [](const FApplicationEventHandlerRef& EventHandler, const FControllerEvent& ControllerEvent)
        {
            return EventHandler->OnControllerAnalog(ControllerEvent);
        });

    return Response.IsEventHandled();
}

bool FApplication::OnKeyUp(EKeyName::Type KeyCode, FModifierKeyState ModierKeyState)
{
    // Create the event
    const FKeyEvent KeyEvent(ModierKeyState, KeyCode, false, false);

    // Let the InputPreProcessors handle the event first
    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputPreProcessors), KeyEvent,
        [](const FInputPreProcessorAndPriority& PreProcessor, const FKeyEvent& KeyEvent)
        {
            return PreProcessor.InputHandler->OnKeyUp(KeyEvent);
        });

    // Remove the Key
    PressedKeys.erase(KeyCode);

    Response = FImGui::OnKeyEvent(KeyEvent.GetKey(), KeyEvent.IsDown());
    if (Response.IsEventHandled())
    {
        return true;
    }

    // Dispatch the events to the widgets in-focus
    Response = FEventDispatcher::Dispatch(FEventDispatcher::FHighPriorityFirstPolicy(EventHandlers), KeyEvent, 
        [](const FApplicationEventHandlerRef& EventHandler, const FKeyEvent& KeyEvent)
        {
            return EventHandler->OnKeyUp(KeyEvent);
        });

    return Response.IsEventHandled();
}

bool FApplication::OnKeyDown(EKeyName::Type KeyCode, bool bIsRepeat, FModifierKeyState ModierKeyState)
{
    // Create the event
    const FKeyEvent KeyEvent(ModierKeyState, KeyCode, bIsRepeat, true);
    
    // Let the InputPreProcessors handle the event first
    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputPreProcessors), KeyEvent,
        [](const FInputPreProcessorAndPriority& PreProcessor, const FKeyEvent& KeyEvent)
        {
            return PreProcessor.InputHandler->OnKeyDown(KeyEvent);
        });

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

    // Dispatch the events to the widgets in-focus
    Response = FEventDispatcher::Dispatch(FEventDispatcher::FHighPriorityFirstPolicy(EventHandlers), KeyEvent, 
        [](const FApplicationEventHandlerRef& EventHandler, const FKeyEvent& KeyEvent)
        {
            return EventHandler->OnKeyDown(KeyEvent);
        });

    return Response.IsEventHandled();
}

bool FApplication::OnKeyChar(uint32 Character)
{
    // Create the event
    const FKeyEvent KeyEvent(FPlatformApplicationMisc::GetModifierKeyState(), EKeyName::Unknown, Character, false, true);
    
    // Let the InputPreProcessors handle the event first
    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputPreProcessors), KeyEvent,
        [](const FInputPreProcessorAndPriority& PreProcessor, const FKeyEvent& KeyEvent)
        {
            return PreProcessor.InputHandler->OnKeyChar(KeyEvent);
        });

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

    // Dispatch the events to the widgets in-focus
    Response = FEventDispatcher::Dispatch(FEventDispatcher::FHighPriorityFirstPolicy(EventHandlers), KeyEvent, 
        [](const FApplicationEventHandlerRef& EventHandler, const FKeyEvent& KeyEvent)
        {
            return EventHandler->OnKeyChar(KeyEvent);
        });

    return Response.IsEventHandled();
}

bool FApplication::OnMouseMove(int32 x, int32 y)
{
    // Create the event
    const FMouseEvent MouseEvent(FIntVector2(x, y), FPlatformApplicationMisc::GetModifierKeyState());
    
    // Let the InputPreProcessors handle the event first
    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputPreProcessors), MouseEvent,
        [](const FInputPreProcessorAndPriority& PreProcessor, const FMouseEvent& MouseEvent)
        {
            return PreProcessor.InputHandler->OnMouseMove(MouseEvent);
        });

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

    // Dispatch the MouseEvent to the widgets under the cursor
    Response = FEventDispatcher::Dispatch(FEventDispatcher::FHighPriorityFirstPolicy(EventHandlers), MouseEvent,
        [](const FApplicationEventHandlerRef& EventHandler, const FMouseEvent& KeyEvent)
        {
            return EventHandler->OnMouseMove(KeyEvent);
        });

    return Response.IsEventHandled();
}

bool FApplication::OnMouseButtonUp(EMouseButtonName Button, FModifierKeyState ModiferKeyState, int32 x, int32 y)
{
    // Remove the mouse capture if there is a capture
    SetCapture(nullptr);

    // Create the event
    const FMouseEvent MouseEvent(FIntVector2(x, y), ModiferKeyState, Button, false);
    
    // Let the InputPreProcessors handle the event first
    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputPreProcessors), MouseEvent,
        [](const FInputPreProcessorAndPriority& PreProcessor, const FMouseEvent& MouseEvent)
        {
            return PreProcessor.InputHandler->OnMouseButtonUp(MouseEvent);
        });
    
    // Remove the Key
    PressedMouseButtons.erase(Button);

    // If the event is handled, abort the process
    Response = FImGui::OnMouseButtonEvent(MouseEvent.GetButton(), MouseEvent.IsDown());
    if (Response.IsEventHandled())
    {
        return true;
    }

    // Dispatch the MouseEvent to the widgets under the cursor
    Response = FEventDispatcher::Dispatch(FEventDispatcher::FHighPriorityFirstPolicy(EventHandlers), MouseEvent,
        [](const FApplicationEventHandlerRef& EventHandler, const FMouseEvent& MouseEvent)
        {
            return EventHandler->OnMouseButtonUp(MouseEvent);
        });

    return Response.IsEventHandled();
}

bool FApplication::OnMouseButtonDown(const TSharedRef<FGenericWindow>& Window, EMouseButtonName Button, FModifierKeyState ModierKeyState, int32 x, int32 y)
{
    // Set the mouse capture when the mouse is pressed
    SetCapture(Window);

    // Create the event
    const FMouseEvent MouseEvent(FIntVector2(x, y), ModierKeyState, Button, true);

    // Let the InputPreProcessors handle the event first
    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputPreProcessors), MouseEvent,
        [](const FInputPreProcessorAndPriority& PreProcessor, const FMouseEvent& MouseEvent)
        {
            return PreProcessor.InputHandler->OnMouseButtonDown(MouseEvent);
        });

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
    Response = FEventDispatcher::Dispatch(FEventDispatcher::FHighPriorityFirstPolicy(EventHandlers), MouseEvent,
        [](const FApplicationEventHandlerRef& EventHandler, const FMouseEvent& MouseEvent)
        {
            return EventHandler->OnMouseButtonDown(MouseEvent);
        });

    return Response.IsEventHandled();
}

bool FApplication::OnMouseScrolled(float WheelDelta, bool bVertical, int32 x, int32 y)
{
    // Create the event
    const FMouseEvent MouseEvent(FIntVector2(x, y), FPlatformApplicationMisc::GetModifierKeyState(), WheelDelta, bVertical);

    // Let the InputPreProcessors handle the event first
    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputPreProcessors), MouseEvent,
        [](const FInputPreProcessorAndPriority& PreProcessor, const FMouseEvent& MouseEvent)
        {
            return PreProcessor.InputHandler->OnMouseScrolled(MouseEvent);
        });

    if (Response.IsEventHandled())
    {
        return true;
    }

    Response = FImGui::OnMouseScrollEvent(MouseEvent.GetScrollDelta(), MouseEvent.IsVerticalScrollDelta());
    
    if (Response.IsEventHandled())
    {
        return true;
    }

    // Dispatch the MouseEvent to the widgets under the cursor
    Response = FEventDispatcher::Dispatch(FEventDispatcher::FHighPriorityFirstPolicy(EventHandlers), MouseEvent,
        [](const FApplicationEventHandlerRef& EventHandler, const FMouseEvent& MouseEvent)
        {
            return EventHandler->OnMouseScroll(MouseEvent);
        });

    return Response.IsEventHandled();
}

bool FApplication::OnWindowResized(const TSharedRef<FGenericWindow>& InWindow, uint32 Width, uint32 Height)
{
    const FWindowEvent WindowEvent(InWindow, Width, Height);

    FImGui::OnWindowResize(InWindow->GetPlatformHandle());

    FResponse Response = FEventDispatcher::Dispatch(FEventDispatcher::FHighPriorityFirstPolicy(EventHandlers), WindowEvent,
        [](const FApplicationEventHandlerRef& EventHandler, const FWindowEvent& WindowEvent)
        {
            return EventHandler->OnWindowResized(WindowEvent);
        });

    return Response.IsEventHandled();
}

bool FApplication::OnWindowMoved(const TSharedRef<FGenericWindow>& InWindow, int32 x, int32 y)
{
    const FWindowEvent WindowEvent(InWindow, FIntVector2(x, y));

    FImGui::OnWindowMoved(InWindow->GetPlatformHandle());

    FResponse Response = FEventDispatcher::Dispatch(FEventDispatcher::FHighPriorityFirstPolicy(EventHandlers), WindowEvent,
        [](const FApplicationEventHandlerRef& EventHandler, const FWindowEvent& WindowEvent)
        {
            return EventHandler->OnWindowMoved(WindowEvent);
        });

    return Response.IsEventHandled();
}

bool FApplication::OnWindowFocusLost(const TSharedRef<FGenericWindow>& InWindow)
{
    const FWindowEvent WindowEvent(InWindow);

    FImGui::OnFocusLost();

    FResponse Response = FEventDispatcher::Dispatch(FEventDispatcher::FHighPriorityFirstPolicy(EventHandlers), WindowEvent,
        [](const FApplicationEventHandlerRef& EventHandler, const FWindowEvent& WindowEvent)
        {
            return EventHandler->OnWindowFocusLost(WindowEvent);
        });

    return Response.IsEventHandled();
}

bool FApplication::OnWindowFocusGained(const TSharedRef<FGenericWindow>& InWindow)
{
    const FWindowEvent WindowEvent(InWindow);

    FImGui::OnFocusGained();

    FResponse Response = FEventDispatcher::Dispatch(FEventDispatcher::FHighPriorityFirstPolicy(EventHandlers), WindowEvent,
        [](const FApplicationEventHandlerRef& EventHandler, const FWindowEvent& WindowEvent)
        {
            return EventHandler->OnWindowFocusGained(WindowEvent);
        });

    return Response.IsEventHandled();
}

bool FApplication::OnWindowMouseLeft(const TSharedRef<FGenericWindow>& InWindow)
{
    const FWindowEvent WindowEvent(InWindow);

    FImGui::OnMouseLeft();

    FResponse Response = FEventDispatcher::Dispatch(FEventDispatcher::FHighPriorityFirstPolicy(EventHandlers), WindowEvent,
        [](const FApplicationEventHandlerRef& EventHandler, const FWindowEvent& WindowEvent)
        {
            return EventHandler->OnMouseLeft(WindowEvent);
        });

    return Response.IsEventHandled();
}

bool FApplication::OnWindowMouseEntered(const TSharedRef<FGenericWindow>& InWindow)
{
    const FWindowEvent WindowEvent(InWindow);

    FResponse Response = FEventDispatcher::Dispatch(FEventDispatcher::FHighPriorityFirstPolicy(EventHandlers), WindowEvent,
        [](const FApplicationEventHandlerRef& EventHandler, const FWindowEvent& WindowEvent)
        {
            return EventHandler->OnMouseEntered(WindowEvent);
        });

    return Response.IsEventHandled();
}

bool FApplication::OnWindowClosed(const TSharedRef<FGenericWindow>& InWindow)
{
    const FWindowEvent WindowEvent(InWindow);

    FImGui::OnWindowClose(InWindow->GetPlatformHandle());

    FResponse Response = FEventDispatcher::Dispatch(FEventDispatcher::FHighPriorityFirstPolicy(EventHandlers), WindowEvent,
        [](const FApplicationEventHandlerRef& EventHandler, const FWindowEvent& WindowEvent)
        {
            return EventHandler->OnWindowClosed(WindowEvent);
        });

    if (TSharedRef<FGenericWindow> Window = MainViewport->GetWindow())
    {
        if (Window == InWindow)
        {
            RequestEngineExit("Normal Exit");

            // TODO: This feels inconsistent, should this be done from engineloop? 
            RegisterMainViewport(nullptr);
        }
    }

    // Remove the window
    AllWindows.Remove(InWindow);
    return Response.IsEventHandled();
}

bool FApplication::OnMonitorChange()
{
    UpdateMonitorInfo();
    return true;
}

TSharedRef<FGenericWindow> FApplication::CreateWindow(const FGenericWindowInitializer& Initializer)
{
    if (TSharedRef<FGenericWindow> Window = PlatformApplication->CreateWindow())
    {
        if (Window->Initialize(Initializer))
        {
            AllWindows.Add(Window);
            return Window;
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

void FApplication::AddInputPreProcessor(const TSharedPtr<FInputPreProcessor>& NewInputHandler, uint32 NewPriority)
{
    FInputPreProcessorAndPriority NewPair(NewInputHandler, NewPriority);
    if (!InputPreProcessors.Contains(NewPair))
    {
        for (int32 Index = 0; Index < InputPreProcessors.Size(); )
        {
            const FInputPreProcessorAndPriority& Handler = InputPreProcessors[Index];
            if (NewPriority <= Handler.Priority)
            {
                Index++;
                InputPreProcessors.Insert(Index, NewPair);
                return;
            }
        }

        InputPreProcessors.Add(NewPair);
    }
}

void FApplication::RemoveInputHandler(const TSharedPtr<FInputPreProcessor>& InputHandler)
{
    for (int32 Index = 0; Index < InputPreProcessors.Size(); Index++)
    {
        const FInputPreProcessorAndPriority Handler = InputPreProcessors[Index];
        if (Handler.InputHandler == InputHandler)
        {
            InputPreProcessors.RemoveAt(Index);
            return;
        }
    }
}

void FApplication::AddEventHandler(const FApplicationEventHandlerRef& EventHandler)
{
    EventHandlers.AddUnique(EventHandler);
}

void FApplication::RemoveEventHandler(const FApplicationEventHandlerRef& EventHandler)
{
    EventHandlers.Remove(EventHandler);
}

void FApplication::AddWidget(const FWidgetRef& Widget)
{
    Widgets.AddUnique(Widget);
}

void FApplication::RemoveWidget(const FWidgetRef& Widget)
{
    Widgets.Remove(Widget);
}

void FApplication::RegisterMainViewport(const TSharedPtr<FViewport>& InViewport)
{
    if (MainViewport != InViewport)
    {
        if (MainViewport)
        {
            RemoveEventHandler(MainViewport);
        }

        MainViewport = InViewport;
        if (MainViewport)
        {
            AddEventHandler(MainViewport);
            MainWindow = MainViewport->GetWindow();
        }
        else
        {
            MainWindow = nullptr;
        }

        FImGui::SetupMainViewport(InViewport.Get());
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
