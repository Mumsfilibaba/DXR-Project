#include "Application.h"
#include "InputHandler.h"
#include "Input/Keys.h"
#include "Input/InputMapper.h"
#include "Widgets/Widget.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Modules/ModuleManager.h"
#include "CoreApplication/Platform/PlatformApplication.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "CoreApplication/Generic/InputDevice.h"

IMPLEMENT_ENGINE_MODULE(FModuleInterface, Application);

// FEventDispatcher is a helper class that dispatches events with a specified dispatch policy.
// Policies include FLeafFirstPolicy, FLeafLastPolicy, and FDirectPolicy, which can be described
// as follows:
//  - FLeafFirstPolicy: Send the event to the first widget first and then send the events
//    along the widget path until a widget handles the event. In practice, this means
//    that the window will receive the event first and then propagate to the rest of the widgets.
//  - FLeafLastPolicy: Send the event to the last widget first and then send the events
//    backward along the widget path until a widget handles the event. In practice, this means
//    that the window will receive the event last.
//  - FDirectPolicy: Send the events only to the first widget in the widget path. In practice,
//    this means that the window receives the event, and then the propagation stops there. This
//    policy gives a single widget the chance to process the event and stops even if the widget
//    does not handle the event.

struct FEventDispatcher
{
    class FLeafFirstPolicy
    {
    public:
        FLeafFirstPolicy(FWidgetPath& InWidgets)
            : Widgets(InWidgets)
            , Index(0)
        {
        }

        bool ShouldProcess() const
        {
            return Index < static_cast<int32>(Widgets.Size());
        }

        void Next()
        {
            Index++;
        }

        const TSharedPtr<FWidget>& GetWidget() const
        {
            return Widgets[Index];
        }

    private:
        FWidgetPath& Widgets;
        int32        Index;
    };

    class FLeafLastPolicy
    {
    public:
        FLeafLastPolicy(FWidgetPath& InWidgets)
            : Widgets(InWidgets)
            , Index(static_cast<int32>(InWidgets.LastIndex()))
        {
        }

        bool ShouldProcess() const
        {
            return Index >= 0 && !Widgets.IsEmpty(); // Corrected from Index > 0
        }

        void Next()
        {
            Index--;
        }

        const TSharedPtr<FWidget>& GetWidget() const
        {
            return Widgets[Index];
        }

    private:
        FWidgetPath& Widgets;
        int32        Index;
    };

    class FDirectPolicy
    {
    public:
        FDirectPolicy(FWidgetPath& InWidgets)
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

        const TSharedPtr<FWidget>& GetWidget() const
        {
            return Widgets[0];
        }

    private:
        FWidgetPath& Widgets;
        bool         bIsProcessed;
    };

    template<typename PolicyType, typename EventType, typename PredicateType>
    static FResponse Dispatch(PolicyType Policy, const EventType& Event, PredicateType&& Predicate)
    {
        FResponse Response = FResponse::Unhandled();
        for (; !Response.IsEventHandled() && Policy.ShouldProcess(); Policy.Next())
        {
            Response = Predicate(Policy.GetWidget(), Event);
        }

        return Response;
    }
};

// FEventPreProcessor is a helper class for pre-processing input events. This allows input handlers
// to be notified about events before they are sent to the correct widgets. Events can have
// different policies that decide in what order they are pre-processed. Currently, the policy is:
//  - FPreProcessPolicy: Goes through the InputHandler array in order. This means that the
//    InputHandlers are looped through in order until an InputHandler returns that it has
//    handled the event.

struct FEventPreProcessor
{
    class FPreProcessPolicy
    {
    public:
        FPreProcessPolicy(TArray<TSharedPtr<FInputHandler>>& InInputPreProcessors)
            : InputPreProcessors(InInputPreProcessors)
            , Index(0)
        {
        }

        bool ShouldProcess() const
        {
            return Index < static_cast<int32>(InputPreProcessors.Size());
        }

        void Next()
        {
            Index++;
        }

        const TSharedPtr<FInputHandler>& GetPreProcessor() const
        {
            return InputPreProcessors[Index];
        }

    private:
        TArray<TSharedPtr<FInputHandler>>& InputPreProcessors;
        int32 Index;
    };

    template<typename EventType, typename PredicateType>
    static FResponse PreProcess(FPreProcessPolicy Policy, const EventType& Event, PredicateType&& Predicate)
    {
        FResponse Response = FResponse::Unhandled();
        for (; Policy.ShouldProcess(); Policy.Next())
        {
            if (Predicate(Policy.GetPreProcessor(), Event))
            {
                Response = FResponse::Handled();
                break;
            }
        }

        return Response;
    }
};

TSharedPtr<FGenericApplication>   FApplicationInterface::PlatformApplication = nullptr;
TSharedPtr<FApplicationInterface> FApplicationInterface::ApplicationInstance = nullptr;

bool FApplicationInterface::Create()
{
    // Initialize the Input mappings
    FInputMapper::Get().Initialize();

    PlatformApplication = FPlatformApplication::Create();
    if (!PlatformApplication)
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to create FPlatformApplication");
        return false;
    }

    ApplicationInstance = MakeSharedPtr<FApplicationInterface>();
    PlatformApplication->SetMessageHandler(ApplicationInstance);
    return true;
}

void FApplicationInterface::Destroy()
{
    if (ApplicationInstance)
    {
        ApplicationInstance->OverridePlatformApplication(nullptr);
        ApplicationInstance.Reset();
    }

    if (PlatformApplication)
    {
        PlatformApplication->SetMessageHandler(nullptr);
        PlatformApplication.Reset();
    }
}

FApplicationInterface::FApplicationInterface()
    : PressedKeys()
    , PressedMouseButtons()
    , MonitorInfos()
    , bIsMonitorInfoValid(false)
    , bIsTrackingCursor(false)
    , Windows()
    , FocusPath()
    , TrackedWidgets()
    , InputHandlers()
    , OnMonitorConfigChangedEvent()
{
    // Init monitor information
    UpdateMonitorInfo();
}

FApplicationInterface::~FApplicationInterface()
{
}

void FApplicationInterface::CreateWindow(const TSharedPtr<FWindow>& InWindow)
{
    if (!InWindow)
    {
        LOG_WARNING("Trying to register a null window");
        return;
    }

    if (Windows.Contains(InWindow))
    {
        LOG_WARNING("Window is already registered");
        return;
    }

    TSharedRef<FGenericWindow> PlatformWindow = GetPlatformApplication()->CreateWindow();
    if (!PlatformWindow)
    {
        return;
    }

    // Find the primary monitor
    int32 PrimaryMonitorIndex = -1;
    for (int32 Index = 0; Index < MonitorInfos.Size(); Index++)
    {
        const FMonitorInfo& MonitorInfo = MonitorInfos[Index];
        if (MonitorInfo.bIsPrimary)
        {
            PrimaryMonitorIndex = Index;
            break;
        }
    }
    
    if (PrimaryMonitorIndex < 0)
    {
        LOG_WARNING("No primary monitor detected");
        return;
    }
    
    // Calculate the maximum position and size of the new window
    const FMonitorInfo& MonitorInfo = MonitorInfos[PrimaryMonitorIndex];

    const uint32 MinWidth  = 0;
    const uint32 MinHeight = 0;
    
    const uint32 MaxWidth  = static_cast<uint32>(MonitorInfo.WorkSize.x);
    const uint32 MaxHeight = static_cast<uint32>(MonitorInfo.WorkSize.y);

    FGenericWindowInitializer WindowInitializer;
    WindowInitializer.Title    = InWindow->GetTitle();
    WindowInitializer.Position = InWindow->GetPosition();
    WindowInitializer.Style    = InWindow->GetStyle();
    WindowInitializer.Width    = FMath::Clamp<int32>(MinWidth, MaxWidth, InWindow->GetWidth());
    WindowInitializer.Height   = FMath::Clamp<int32>(MinHeight, MaxHeight, InWindow->GetHeight());

    if (PlatformWindow->Initialize(WindowInitializer))
    {
        InWindow->SetPlatformWindow(PlatformWindow);        
        Windows.Add(InWindow);

        PlatformWindow->Show(true);
    }
}

void FApplicationInterface::DestroyWindow(const TSharedPtr<FWindow>& DestroyedWindow)
{
    if (DestroyedWindow)
    {
        TSharedRef<FGenericWindow> PlatformWindow = DestroyedWindow->GetPlatformWindow();
        DestroyedWindow->OnWindowDestroyed();
        Windows.Remove(DestroyedWindow);

        if (PlatformWindow == PlatformApplication->GetCapture())
        {
            // Give capture back to the first window so that we'll still receive the MOUSEUP event.
            TSharedPtr<FWindow> NextWindow = Windows[0];
            PlatformApplication->SetCapture(NextWindow->GetPlatformWindow());
        }
    }
}

void FApplicationInterface::Tick(float Delta)
{
    PlatformApplication->Tick(Delta);

    UpdateInputDevices();

    // Tick all the windows, which in turn ticks their children
    for (const TSharedPtr<FWindow>& CurrentWindow : Windows)
    {
        CurrentWindow->Tick();
    }
}

void FApplicationInterface::UpdateInputDevices()
{
    PlatformApplication->UpdateInputDevices();
}

void FApplicationInterface::UpdateMonitorInfo()
{
    if (!bIsMonitorInfoValid)
    {
        PlatformApplication->QueryMonitorInfo(MonitorInfos);
        bIsMonitorInfoValid = true;
    }
}

void FApplicationInterface::RegisterInputHandler(const TSharedPtr<FInputHandler>& NewInputHandler)
{
    if (NewInputHandler)
    {
        InputHandlers.AddUnique(NewInputHandler);
    }
}

void FApplicationInterface::UnregisterInputHandler(const TSharedPtr<FInputHandler>& InputHandler)
{
    if (InputHandler)
    {
        InputHandlers.Remove(InputHandler);
    }
}

bool FApplicationInterface::OnGamepadButtonUp(EGamepadButtonName::Type Button, uint32 GamepadIndex)
{
    const FKeyEvent KeyEvent(FInputMapper::Get().GetGamepadKey(Button), FPlatformApplicationMisc::GetModifierKeyState(), 0, GamepadIndex, false, false);

    FResponse Response = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), KeyEvent,
        [](const TSharedPtr<FInputHandler>& InputHandler, const FKeyEvent& KeyEvent)
        {
            return InputHandler->OnKeyUp(KeyEvent);
        });

    if (Response.IsEventHandled())
    {
        return true;
    }

    Response = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(FocusPath), KeyEvent,
        [](const TSharedPtr<FWidget>& Widget, const FKeyEvent& KeyEvent)
        {
            return Widget->OnKeyUp(KeyEvent);
        });

    return Response.IsEventHandled();
}

bool FApplicationInterface::OnGamepadButtonDown(EGamepadButtonName::Type Button, uint32 GamepadIndex, bool bIsRepeat)
{
    const FKeyEvent KeyEvent(FInputMapper::Get().GetGamepadKey(Button), FPlatformApplicationMisc::GetModifierKeyState(), 0, GamepadIndex, bIsRepeat, true);

    FResponse Response = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), KeyEvent,
        [](const TSharedPtr<FInputHandler>& InputHandler, const FKeyEvent& KeyEvent)
        {
            return InputHandler->OnKeyDown(KeyEvent);
        });

    if (Response.IsEventHandled())
    {
        return true;
    }

    Response = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(FocusPath), KeyEvent,
        [](const TSharedPtr<FWidget>& Widget, const FKeyEvent& KeyEvent)
        {
            return Widget->OnKeyDown(KeyEvent);
        });

    return Response.IsEventHandled();
}

bool FApplicationInterface::OnAnalogGamepadChange(EAnalogSourceName::Type AnalogSource, uint32 GamepadIndex, float AnalogValue)
{
    const FAnalogGamepadEvent AnalogGamepadEvent(AnalogSource, GamepadIndex, FPlatformApplicationMisc::GetModifierKeyState(), AnalogValue);

    FResponse Response = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), AnalogGamepadEvent,
        [](const TSharedPtr<FInputHandler>& InputHandler, const FAnalogGamepadEvent& AnalogGamepadEvent)
        {
            return InputHandler->OnAnalogGamepadChange(AnalogGamepadEvent);
        });

    if (Response.IsEventHandled())
    {
        return true;
    }

    Response = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(FocusPath), AnalogGamepadEvent,
        [](const TSharedPtr<FWidget>& Widget, const FAnalogGamepadEvent& AnalogGamepadEvent)
        {
            return Widget->OnAnalogGamepadChange(AnalogGamepadEvent);
        });

    return Response.IsEventHandled();
}

bool FApplicationInterface::OnKeyUp(EKeyboardKeyName::Type KeyCode, FModifierKeyState ModierKeyState)
{
    const FKeyEvent KeyEvent(FInputMapper::Get().GetKeyboardKey(KeyCode), ModierKeyState, false, false);

    FResponse Response = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), KeyEvent,
        [](const TSharedPtr<FInputHandler>& InputHandler, const FKeyEvent& KeyEvent)
        {
            return InputHandler->OnKeyUp(KeyEvent);
        });

    // Remove the Key
    PressedKeys.Remove(KeyCode);

    if (Response.IsEventHandled())
    {
        return true;
    }

    Response = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(FocusPath), KeyEvent,
        [](const TSharedPtr<FWidget>& Widget, const FKeyEvent& KeyEvent)
        {
            return Widget->OnKeyUp(KeyEvent);
        });

    return Response.IsEventHandled();
}

bool FApplicationInterface::OnKeyDown(EKeyboardKeyName::Type KeyCode, bool bIsRepeat, FModifierKeyState ModierKeyState)
{
    const FKeyEvent KeyEvent(FInputMapper::Get().GetKeyboardKey(KeyCode), ModierKeyState, bIsRepeat, true);
    
    FResponse Response = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), KeyEvent,
        [](const TSharedPtr<FInputHandler>& InputHandler, const FKeyEvent& KeyEvent)
        {
            return InputHandler->OnKeyDown(KeyEvent);
        });

    if (Response.IsEventHandled())
    {
        return true;
    }

    // Add key
    PressedKeys.Add(KeyCode);

    Response = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(FocusPath), KeyEvent,
        [](const TSharedPtr<FWidget>& Widget, const FKeyEvent& KeyEvent)
        {
            return Widget->OnKeyDown(KeyEvent);
        });

    return Response.IsEventHandled();
}

bool FApplicationInterface::OnKeyChar(uint32 Character)
{
    const FKeyEvent KeyEvent(EKeys::Unknown, FPlatformApplicationMisc::GetModifierKeyState(), Character, false, true);
    
    FResponse Response = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), KeyEvent,
        [](const TSharedPtr<FInputHandler>& InputHandler, const FKeyEvent& KeyEvent)
        {
            return InputHandler->OnKeyChar(KeyEvent);
        });

    if (Response.IsEventHandled())
    {
        return true;
    }

    Response = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(FocusPath), KeyEvent,
        [](const TSharedPtr<FWidget>& Widget, const FKeyEvent& KeyEvent)
        {
            return Widget->OnKeyChar(KeyEvent);
        });

    return Response.IsEventHandled();
}

bool FApplicationInterface::OnMouseMove(int32 MouseX, int32 MouseY)
{
    const FCursorEvent CursorEvent(FIntVector2(MouseX, MouseY), FPlatformApplicationMisc::GetModifierKeyState());

    FResponse Response = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), CursorEvent,
        [](const TSharedPtr<FInputHandler>& InputHandler, const FCursorEvent& CursorEvent)
        {
            return InputHandler->OnMouseMove(CursorEvent);
        });

    if (Response.IsEventHandled())
    {
        return true;
    }
    
    // Retrieve all the widgets under the cursor which should receive events
    FWidgetPath CursorPath;
    FindWidgetsUnderCursor(CursorEvent.GetCursorPos(), CursorPath);

    // Remove the widget from any widget which is not tracked
    const bool bIsDragging = !PressedMouseButtons.IsEmpty();
    for (int32 Index = 0; Index < TrackedWidgets.Size();)
    {
        const TSharedPtr<FWidget>& CurrentWidget = TrackedWidgets[Index];
        if (!CursorPath.Contains(CurrentWidget) && !bIsDragging)
        {
            CurrentWidget->OnMouseLeft(CursorEvent);
            TrackedWidgets.RemoveAt(Index);
        }
        else
        {
            Index++;
        }
    }

    Response = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(CursorPath), CursorEvent,
        [this](const TSharedPtr<FWidget>& Widget, const FCursorEvent& CursorEvent)
        {
            if (!TrackedWidgets.Contains(Widget))
            {
                TrackedWidgets.Add(EVisibility::Visible, Widget);
                Widget->OnMouseEntered(CursorEvent);
            }

            return FResponse::Unhandled();
        });

    Response = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(CursorPath), CursorEvent,
        [](const TSharedPtr<FWidget>& Widget, const FCursorEvent& CursorEvent)
        {
            return Widget->OnMouseMove(CursorEvent);
        });

    return Response.IsEventHandled();
}

bool FApplicationInterface::OnMouseButtonDown(const TSharedRef<FGenericWindow>& PlatformWindow, EMouseButtonName::Type Button, FModifierKeyState ModierKeyState)
{
    // Set the mouse capture when the mouse is pressed
    PlatformApplication->SetCapture(PlatformWindow);
    bIsTrackingCursor = true;

    const FCursorEvent CursorEvent(FInputMapper::Get().GetMouseKey(Button), ModierKeyState, true);

    FResponse Response = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), CursorEvent,
        [](const TSharedPtr<FInputHandler>& InputHandler, const FCursorEvent& CursorEvent)
        {
            return InputHandler->OnMouseButtonDown(CursorEvent);
        });

    if (Response.IsEventHandled())
    {
        return true;
    }

    // Add the button to the pressed buttons
    PressedMouseButtons.Remove(Button);

    FWidgetPath CursorPath;
    FindWidgetsUnderCursor(CursorPath);

    Response = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(CursorPath), CursorEvent,
        [this](const TSharedPtr<FWidget>& Widget, const FCursorEvent& CursorEvent)
        {
            const FResponse Response = Widget->OnMouseButtonDown(CursorEvent);
            if (Response.IsEventHandled() && !TrackedWidgets.Contains(Widget))
            {
                TrackedWidgets.Add(EVisibility::Visible, Widget);
            }

            return Response;
        });

    SetFocusWidgets(CursorPath);
    return Response.IsEventHandled();
}

bool FApplicationInterface::OnMouseButtonUp(EMouseButtonName::Type Button, FModifierKeyState ModiferKeyState)
{
    PressedMouseButtons.Remove(Button);

    // Remove the mouse capture if there is a capture
    PlatformApplication->SetCapture(nullptr);
    bIsTrackingCursor = false;

    const FCursorEvent CursorEvent(FInputMapper::Get().GetMouseKey(Button), ModiferKeyState, false);
    
    FResponse Response = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), CursorEvent,
        [](const TSharedPtr<FInputHandler>& InputHandler, const FCursorEvent& CursorEvent)
        {
            return InputHandler->OnMouseButtonUp(CursorEvent);
        });

    if (Response.IsEventHandled())
    {
        return true;
    }

    FWidgetPath CursorPath;
    FindWidgetsUnderCursor(CursorPath);

    const bool bIsDragging = !PressedMouseButtons.IsEmpty();
    for (int32 Index = 0; Index < TrackedWidgets.Size();)
    {
        const TSharedPtr<FWidget>& CurrentWidget = TrackedWidgets[Index];
        if (!CursorPath.Contains(CurrentWidget) && !bIsDragging)
        {
            CurrentWidget->OnMouseLeft(CursorEvent);
            TrackedWidgets.RemoveAt(Index);
        }
        else
        {
            Index++;
        }
    }

    Response = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(CursorPath), CursorEvent,
        [](const TSharedPtr<FWidget>& Widget, const FCursorEvent& CursorEvent)
        {
            return Widget->OnMouseButtonUp(CursorEvent);
        });

    return Response.IsEventHandled();
}

bool FApplicationInterface::OnMouseButtonDoubleClick(EMouseButtonName::Type Button, FModifierKeyState ModierKeyState)
{
    const FCursorEvent CursorEvent(FInputMapper::Get().GetMouseKey(Button), ModierKeyState, true);

    FResponse Response = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), CursorEvent,
        [](const TSharedPtr<FInputHandler>& InputHandler, const FCursorEvent& CursorEvent)
        {
            return InputHandler->OnMouseButtonDown(CursorEvent);
        });

    if (Response.IsEventHandled())
    {
        return true;
    }

    // Add the button to the pressed buttons
    PressedMouseButtons.Remove(Button);

    FWidgetPath CursorPath;
    FindWidgetsUnderCursor(CursorPath);

    Response = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(CursorPath), CursorEvent,
        [](const TSharedPtr<FWidget>& Widget, const FCursorEvent& CursorEvent)
        {
            return Widget->OnMouseDoubleClick(CursorEvent);
        });

    SetFocusWidgets(CursorPath);
    return Response.IsEventHandled();
}

bool FApplicationInterface::OnMouseScrolled(float WheelDelta, bool bVertical)
{
    const FCursorEvent CursorEvent(FPlatformApplicationMisc::GetModifierKeyState(), WheelDelta, bVertical);

    FResponse Response = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), CursorEvent,
        [](const TSharedPtr<FInputHandler>& InputHandler, const FCursorEvent& CursorEvent)
        {
            return InputHandler->OnMouseScrolled(CursorEvent);
        });

    if (Response.IsEventHandled())
    {
        return true;
    }

    FWidgetPath CursorPath;
    FindWidgetsUnderCursor(CursorPath);

    Response = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(CursorPath), CursorEvent,
        [](const TSharedPtr<FWidget>& Widget, const FCursorEvent& CursorEvent)
        {
            return Widget->OnMouseScroll(CursorEvent);
        });

    return Response.IsEventHandled();
}

bool FApplicationInterface::OnMouseEntered()
{
    const FCursorEvent CursorEvent(FPlatformApplicationMisc::GetModifierKeyState());

    FWidgetPath CursorPath;
    FindWidgetsUnderCursor(CursorPath);

    FResponse Response = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(CursorPath), CursorEvent,
        [this](const TSharedPtr<FWidget>& Widget, const FCursorEvent& CursorEvent)
        {
            if (!TrackedWidgets.Contains(Widget))
            {
                TrackedWidgets.Add(EVisibility::Visible, Widget);
                Widget->OnMouseEntered(CursorEvent);
            }

            return FResponse::Unhandled();
        });

    return Response.IsEventHandled();
}

bool FApplicationInterface::OnMouseLeft()
{
    const FCursorEvent CursorEvent(FPlatformApplicationMisc::GetModifierKeyState());

    FWidgetPath CursorPath;
    FindWidgetsUnderCursor(CursorPath);

    FResponse Response = FResponse::Unhandled();

    const bool bIsDragging = !PressedMouseButtons.IsEmpty();
    for (int32 Index = 0; Index < TrackedWidgets.Size();)
    {
        const TSharedPtr<FWidget>& CurrentWidget = TrackedWidgets[Index];
        if (!CursorPath.Contains(CurrentWidget) && !bIsDragging)
        {
            Response = CurrentWidget->OnMouseLeft(CursorEvent);
            TrackedWidgets.RemoveAt(Index);
        }
        else
        {
            Index++;
        }
    }

    return Response.IsEventHandled();
}

bool FApplicationInterface::OnHighPrecisionMouseInput(int32 MouseX, int32 MouseY)
{
    const FCursorEvent CursorEvent(FIntVector2(MouseX, MouseY), FPlatformApplicationMisc::GetModifierKeyState());

    FResponse Response = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), CursorEvent,
        [](const TSharedPtr<FInputHandler>& InputHandler, const FCursorEvent& CursorEvent)
        {
            return InputHandler->OnHighPrecisionMouseInput(CursorEvent);
        });

    if (Response.IsEventHandled())
    {
        return true;
    }

    FWidgetPath CursorPath;
    FindWidgetsUnderCursor(CursorPath);

    Response = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(CursorPath), CursorEvent,
        [](const TSharedPtr<FWidget>& Widget, const FCursorEvent& CursorEvent)
        {
            return Widget->OnHighPrecisionMouseInput(CursorEvent);
        });

    return Response.IsEventHandled();
}

bool FApplicationInterface::OnWindowResized(const TSharedRef<FGenericWindow>& PlatformWindow, uint32 Width, uint32 Height)
{
    bool bResult = false;
    if (TSharedPtr<FWindow> Window = FindWindowFromGenericWindow(PlatformWindow))
    {
        FIntVector2 NewScreenSize(Width, Height);
        Window->OnWindowResize(NewScreenSize);
        bResult = true;
    }

    return bResult;
}

bool FApplicationInterface::OnWindowResizing(const TSharedRef<FGenericWindow>&)
{
    return false;
}

bool FApplicationInterface::OnWindowMoved(const TSharedRef<FGenericWindow>& PlatformWindow, int32 x, int32 y)
{
    bool bResult = false;
    if (TSharedPtr<FWindow> Window = FindWindowFromGenericWindow(PlatformWindow))
    {
        FIntVector2 NewScreenPosition(x, y);
        Window->OnWindowMoved(NewScreenPosition);
        bResult = true;
    }

    return bResult;
}

bool FApplicationInterface::OnWindowFocusLost(const TSharedRef<FGenericWindow>& PlatformWindow)
{
    bool bResult = false;
    if (TSharedPtr<FWindow> Window = FindWindowFromGenericWindow(PlatformWindow))
    {
        Window->OnWindowActivationChanged(false);
        bResult = true;
    }

    SetFocusWidget(nullptr);
    return bResult;
}

bool FApplicationInterface::OnWindowFocusGained(const TSharedRef<FGenericWindow>& PlatformWindow)
{
    bool bResult = false;
    if (TSharedPtr<FWindow> Window = FindWindowFromGenericWindow(PlatformWindow))
    {
        TSharedPtr<FWidget> FocusWidget = Window->GetContent();
        if (!FocusWidget)
        {
            FocusWidget = Window;
        }

        // Either set focus to the window, or to the content of the window if there are a 
        SetFocusWidget(FocusWidget);

        Window->OnWindowActivationChanged(true);
        bResult = true;
    }

    return bResult;
}

bool FApplicationInterface::OnWindowClosed(const TSharedRef<FGenericWindow>& PlatformWindow)
{
    bool bResult = false;
    
    if (TSharedPtr<FWindow> Window = FindWindowFromGenericWindow(PlatformWindow))
    {
        DestroyWindow(Window);
        bResult = true;
    }

    return bResult;
}

bool FApplicationInterface::OnMonitorConfigurationChange()
{
    // Invalidate the cached monitor-information
    bIsMonitorInfoValid = false;
    
    // First update the cached monitor-information...
    UpdateMonitorInfo();

    // ... then notify listeners that the monitor configuration has changed
    OnMonitorConfigChangedEvent.Broadcast();
    return true;
}

bool FApplicationInterface::EnableHighPrecisionMouseForWindow(const TSharedPtr<FWindow>& Window)
{ 
    if (Window)
    {
        if (TSharedRef<FGenericWindow> PlatformWindow = Window->GetPlatformWindow())
        {
            return PlatformApplication->EnableHighPrecisionMouseForWindow(PlatformWindow);
        }
    }

    return false;
}

bool FApplicationInterface::SupportsHighPrecisionMouse() const 
{
    return PlatformApplication->SupportsHighPrecisionMouse();
}

void FApplicationInterface::SetCursorPosition(const FIntVector2& Position)
{
    if (TSharedPtr<ICursor> Cursor = GetCursor())
    {
        Cursor->SetPosition(Position.x, Position.y);
    }
}

FIntVector2 FApplicationInterface::GetCursorPosition() const
{
    if (TSharedPtr<ICursor> Cursor = GetCursor())
    {
        return Cursor->GetPosition();
    }

    return FIntVector2();
}

void FApplicationInterface::SetCursor(ECursor InCursor)
{
    if (TSharedPtr<ICursor> Cursor = GetCursor())
    {
        Cursor->SetCursor(InCursor);
    }
}

void FApplicationInterface::ShowCursor(bool bIsVisible)
{
    if (TSharedPtr<ICursor> Cursor = GetCursor())
    {
        Cursor->SetVisibility(bIsVisible);
    }
}

bool FApplicationInterface::IsCursorVisibile() const
{
    if (TSharedPtr<ICursor> Cursor = GetCursor())
    {
        return Cursor->IsVisible();
    }

    return false;
}

bool FApplicationInterface::IsGamePadConnected() const
{
    if (FInputDevice* InputDevice = GetInputDeviceInterface())
    {
        return InputDevice->IsDeviceConnected();
    }

    return false;
}

TSharedPtr<FWindow> FApplicationInterface::FindWindowFromGenericWindow(const TSharedRef<FGenericWindow>& PlatformWindow) const
{
    if (!PlatformWindow)
    {
        return nullptr;
    }

    for (TSharedPtr<FWindow> CurrentWindow : Windows)
    {
        if (PlatformWindow == CurrentWindow->GetPlatformWindow())
        {
            return CurrentWindow;
        }
    }

    return nullptr;
}

void FApplicationInterface::OverridePlatformApplication(const TSharedPtr<FGenericApplication>& InPlatformApplication)
{
    // Set a MessageHandler to avoid any potential nullptr access
    if (PlatformApplication)
    {
        PlatformApplication->SetMessageHandler(MakeSharedPtr<FGenericApplicationMessageHandler>());
    }

    if (InPlatformApplication)
    {
        CHECK(PlatformApplication != InPlatformApplication);
        InPlatformApplication->SetMessageHandler(ApplicationInstance);
    }

    PlatformApplication = InPlatformApplication;
}

void FApplicationInterface::SetFocusWidget(const TSharedPtr<FWidget>& FocusWidget)
{
    FWidgetPath NewFocusPath;
    if (FocusWidget)
    {
        FocusWidget->FindParentWidgets(NewFocusPath);
    }

    SetFocusWidgets(NewFocusPath);
}

void FApplicationInterface::SetFocusWidgets(const FWidgetPath& NewFocusPath)
{
    // First we need to go through all the widgets that currently have focus and 
    // notify widgets that is not in the new widget-path that they have lost focus
    for (int32 Index = 0; Index < FocusPath.Size(); Index++)
    {
        const TSharedPtr<FWidget>& CurrentWidget = FocusPath[Index];
        if (!NewFocusPath.Contains(CurrentWidget))
        {
            CurrentWidget->OnFocusLost();
        }
    }

    // Then go through all the widgets in the new widget-path and notify them that 
    // they have gained focus, as long as they are not a part of the old path
    for (int32 Index = 0; Index < NewFocusPath.Size(); Index++)
    {
        const TSharedPtr<FWidget>& CurrentWidget = NewFocusPath[Index];
        if (!FocusPath.Contains(CurrentWidget))
        {
            CurrentWidget->OnFocusGained();
        }
    }

    FocusPath = NewFocusPath;
}

TSharedPtr<FWindow> FApplicationInterface::FindWindowWidget(const TSharedPtr<FWidget>& InWidget)
{
    TWeakPtr<FWidget> ParentWidget = InWidget;
    while (ParentWidget)
    {
        if (ParentWidget->IsWindow())
        {
            break;
        }

        ParentWidget = ParentWidget->GetParentWidget();
    }

    if (!ParentWidget.IsExpired())
    {
        return StaticCastSharedPtr<FWindow>(ParentWidget.ToSharedPtr());
    }

    return nullptr;
}

TSharedPtr<FWindow> FApplicationInterface::FindWindowUnderCursor() const
{
    if (TSharedRef<FGenericWindow> PlatformWindow = PlatformApplication->GetWindowUnderCursor())
    {
        return FindWindowFromGenericWindow(PlatformWindow);
    }
    
    return nullptr;
}

void FApplicationInterface::FindWidgetsUnderCursor(FWidgetPath& OutCursorPath)
{
    if (TSharedPtr<ICursor> Cursor = GetCursor())
    {
        FindWidgetsUnderCursor(Cursor->GetPosition(), OutCursorPath);
    }
}

void FApplicationInterface::FindWidgetsUnderCursor(const FIntVector2& CursorPosition, FWidgetPath& OutCursorPath)
{
    if (TSharedRef<FGenericWindow> PlatformWindow = PlatformApplication->GetWindowUnderCursor())
    {
        if (TSharedPtr<FWindow> CursorWindow = FindWindowFromGenericWindow(PlatformWindow))
        {
            CursorWindow->FindChildrenUnderCursor(CursorPosition, OutCursorPath);
        }
    }
}

void FApplicationInterface::GetDisplayInfo(TArray<FMonitorInfo>& OutMonitorInfo)
{
    if (!bIsMonitorInfoValid)
    {
        UpdateMonitorInfo();
    }

    // Copy the monitor-array
    OutMonitorInfo = MonitorInfos;
}

TSharedPtr<FWindow> FApplicationInterface::GetFocusWindow() const
{
    if (TSharedRef<FGenericWindow> ActiveWindow = PlatformApplication->GetActiveWindow())
    {
        return FindWindowFromGenericWindow(ActiveWindow);
    }

    return nullptr;
}
