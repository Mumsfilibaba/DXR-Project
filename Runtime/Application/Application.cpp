#include "Application/Application.h"
#include "Application/InputHandler.h"
#include "Application/Input/Keys.h"
#include "Application/Input/InputMapper.h"
#include "Application/Widgets/Widget.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Modules/ModuleManager.h"
#include "CoreApplication/Platform/PlatformApplication.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "CoreApplication/Generic/InputDevice.h"
#include "RHI/RHICommandList.h"

IMPLEMENT_ENGINE_MODULE(IModule, Application);

/* ---------------------------------------------------------------------------------------------------------- */
// FEventDispatcher is a helper class that dispatches events with a specified dispatch policy.
// Policies include FLeafFirstPolicy, FLeafLastPolicy, and FDirectPolicy, which can be described
// as follows:
//  - FLeafFirstPolicy: Send the event to the top-level widget first and then send the events
//    along the widget path until a widget handles the event. In practice, this means
//    that the window will receive the event last, and the leaf child-widget will receive the
//    event first.
//  - FLeafLastPolicy: Send the event to the last widget first and then send the events
//    backward along the widget path until a widget handles the event. In practice, this means
//    that the window will receive the event first and then propagate to the rest of the widgets.
//  - FDirectPolicy: Send the events only to the first widget in the widget path. In practice,
//    this means that the window receives the event, and then the propagation stops there. This
//    policy gives a single widget the chance to process the event and stops even if the widget
//    does not handle the event.
/* ---------------------------------------------------------------------------------------------------------- */

struct FEventDispatcher
{
    class FLeafFirstPolicy : public FNonCopyAndNonMovable
    {
    public:
        FLeafFirstPolicy(FWidgetPath& InWidgets)
        : Widgets(InWidgets)
        , Index(static_cast<int32>(InWidgets.LastIndex()))
        {
        }
        
        bool ShouldProcess() const
        {
            return Index >= 0 && !Widgets.IsEmpty();
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

    class FLeafLastPolicy : public FNonCopyAndNonMovable
    {
    public:
        FLeafLastPolicy(FWidgetPath& InWidgets)
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

    class FDirectPolicy : public FNonCopyAndNonMovable
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
    static FEventResponse Dispatch(PolicyType Policy, const EventType& Event, PredicateType&& Predicate)
    {
        FEventResponse Response = FEventResponse::Unhandled();
        for (; !Response.IsEventHandled() && Policy.ShouldProcess(); Policy.Next())
        {
            Response = Predicate(Policy.GetWidget(), Event);
        }

        return Response;
    }
};

/* ---------------------------------------------------------------------------------------------------------- */
// FEventPreProcessor is a helper class for pre-processing input events. This allows input handlers
// to be notified about events before they are sent to the correct widgets. Events can have
// different policies that decide in what order they are pre-processed. Currently, the policy is:
//  - FPreProcessPolicy: Goes through the InputHandler array in order. This means that the
//    InputHandlers are looped through in order until an InputHandler returns that it has
//    handled the event.
/* ---------------------------------------------------------------------------------------------------------- */

struct FEventPreProcessor
{
    class FPreProcessPolicy : public FNonCopyAndNonMovable
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
    static FEventResponse PreProcess(FPreProcessPolicy Policy, const EventType& Event, PredicateType&& Predicate)
    {
        FEventResponse Response = FEventResponse::Unhandled();
        for (; Policy.ShouldProcess(); Policy.Next())
        {
            if (Predicate(Policy.GetPreProcessor(), Event))
            {
                Response = FEventResponse::Handled();
                break;
            }
        }

        return Response;
    }
};

TSharedPtr<FApplication> FApplication::GApplicationInstance = nullptr;

bool FApplication::Initialize()
{
    FInputMapper::Get().Initialize();

    TSharedPtr<FGenericApplication> PlatformApplication = FPlatformApplication::Create();
    if (!PlatformApplication)
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to create FPlatformApplication");
        return false;
    }

    GApplicationInstance = MakeSharedPtr<FApplication>(PlatformApplication);
    PlatformApplication->SetMessageHandler(GApplicationInstance);
    return true;
}

void FApplication::Release()
{
    if (GApplicationInstance)
    {
        GApplicationInstance->OverridePlatformApplication(nullptr);
        GApplicationInstance.Reset();
    }
}

FApplication::FApplication(TSharedPtr<FGenericApplication> InPlatformApplication)
    : PlatformApplication(InPlatformApplication)
    , PressedKeys()
    , PressedMouseButtons()
    , MonitorInfos()
    , bIsMonitorInfoValid(false)
    , bIsTrackingCursor(false)
    , FocusPath()
    , TrackedWidgets()
    , Windows()
    , InputHandlers()
    , OnMonitorConfigChangedEvent()
{
    UpdateMonitorInfo();
}

FApplication::~FApplication()
{
}

void FApplication::CreateWindow(const TSharedPtr<FWindowWidget>& InWindow)
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
    
    FGenericWindowInitializer WindowInitializer;
    WindowInitializer.Title    = InWindow->GetTitle();
    WindowInitializer.Style    = InWindow->GetStyle();
    WindowInitializer.Position = InWindow->GetPosition();
    
    if (TSharedPtr<FWindowWidget> ParentWindow = InWindow->GetParentWindow())
    {
        WindowInitializer.ParentWindow = ParentWindow->GetPlatformWindow().Get();
    }

    // Calculate the maximum position and size of the new window so that if fits in the main monitor bounds.
    const FMonitorInfo& MonitorInfo = MonitorInfos[PrimaryMonitorIndex];
    if (MonitorInfo.MainPosition.X > WindowInitializer.Position.X)
    {
        WindowInitializer.Position.X = MonitorInfo.MainPosition.X;
    }
    if (MonitorInfo.MainPosition.Y > WindowInitializer.Position.Y)
    {
        WindowInitializer.Position.Y = MonitorInfo.MainPosition.Y;
    }
    
    WindowInitializer.Width  = InWindow->GetWidth();
    WindowInitializer.Height = InWindow->GetHeight();
    
    const uint32 ScreenEndX = static_cast<uint32>(MonitorInfo.MainPosition.X + MonitorInfo.MainSize.X);
    const uint32 ScreenEndY = static_cast<uint32>(MonitorInfo.MainPosition.Y + MonitorInfo.MainSize.Y);
    const uint32 WindowEndX = WindowInitializer.Position.X + WindowInitializer.Width;
    const uint32 WindowEndY = WindowInitializer.Position.Y + WindowInitializer.Height;
    
    if (WindowEndX > ScreenEndX)
    {
        WindowInitializer.Width = ScreenEndX - WindowInitializer.Position.X;
    }
    if (WindowEndY > ScreenEndY)
    {
        WindowInitializer.Height = ScreenEndY - WindowInitializer.Position.Y;
    }

    if (PlatformWindow->Initialize(WindowInitializer))
    {
        InWindow->SetPlatformWindow(PlatformWindow);        
        Windows.Add(InWindow);

        PlatformWindow->Show(InWindow->ActivateOnShow());
    }
}

void FApplication::DestroyWindow(const TSharedPtr<FWindowWidget>& DestroyedWindow)
{
    if (DestroyedWindow)
    {
        TSharedRef<FGenericWindow> PlatformWindow = DestroyedWindow->GetPlatformWindow();
        DestroyedWindow->OnWindowDestroyed();
        Windows.Remove(DestroyedWindow);

        if (PlatformWindow == PlatformApplication->GetCapture())
        {
            // Give capture back to the first window so that we'll still receive the mouse-up event.
            TSharedPtr<FWindowWidget> NextWindow = Windows[0];
            PlatformApplication->SetCapture(NextWindow->GetPlatformWindow());
        }
    }
}

void FApplication::Tick(float Delta)
{
    ProcessEvents();

    ProcessDeferredEvents();

    PlatformApplication->Tick(Delta);

    UpdateInputDevices();

    for (const TSharedPtr<FWindowWidget>& CurrentWindow : Windows)
    {
        FRectangle WindowRectangle;
        WindowRectangle.Position = CurrentWindow->GetPosition();
        WindowRectangle.Width    = CurrentWindow->GetSize().X;
        WindowRectangle.Height   = CurrentWindow->GetSize().Y;

        CurrentWindow->Tick(WindowRectangle);
    }
}

void FApplication::ProcessEvents()
{
    PlatformApplication->ProcessEvents();
}

void FApplication::ProcessDeferredEvents()
{
    PlatformApplication->ProcessDeferredEvents();
}

void FApplication::UpdateInputDevices()
{
    PlatformApplication->UpdateInputDevices();
}

void FApplication::UpdateMonitorInfo()
{
    if (!bIsMonitorInfoValid)
    {
        PlatformApplication->QueryMonitorInfo(MonitorInfos);
        bIsMonitorInfoValid = true;
    }
}

void FApplication::RegisterInputHandler(const TSharedPtr<FInputHandler>& NewInputHandler)
{
    if (NewInputHandler)
    {
        InputHandlers.AddUnique(NewInputHandler);
    }
}

void FApplication::UnregisterInputHandler(const TSharedPtr<FInputHandler>& InputHandler)
{
    if (InputHandler)
    {
        InputHandlers.Remove(InputHandler);
    }
}

bool FApplication::OnGamepadButtonUp(EGamepadButtonName::Type Button, uint32 GamepadIndex)
{
    const FKeyEvent KeyEvent(EInputEventType::GamepadButtonUp, FInputMapper::Get().GetGamepadKey(Button), PlatformApplication->GetModifierKeyState(), 0, GamepadIndex, false, false);

    const FEventResponse PreProcessResponse = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), KeyEvent,
        [](const TSharedPtr<FInputHandler>& InputHandler, const FKeyEvent& KeyEvent)
        {
            return InputHandler->OnKeyUp(KeyEvent);
        });

    const FEventResponse WidgetResponse = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(FocusPath), KeyEvent,
        [](const TSharedPtr<FWidget>& Widget, const FKeyEvent& KeyEvent)
        {
            return Widget->OnKeyUp(KeyEvent);
        });

    return WidgetResponse.IsEventHandled();
}

bool FApplication::OnGamepadButtonDown(EGamepadButtonName::Type Button, uint32 GamepadIndex, bool bIsRepeat)
{
    const FKeyEvent KeyEvent(EInputEventType::GamepadButtonDown, FInputMapper::Get().GetGamepadKey(Button), PlatformApplication->GetModifierKeyState(), 0, GamepadIndex, bIsRepeat, true);

    const FEventResponse PreProcessResponse = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), KeyEvent,
        [](const TSharedPtr<FInputHandler>& InputHandler, const FKeyEvent& KeyEvent)
        {
            return InputHandler->OnKeyDown(KeyEvent);
        });

    if (PreProcessResponse.IsEventHandled())
    {
        return true;
    }

    const FEventResponse WidgetResponse = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(FocusPath), KeyEvent,
        [](const TSharedPtr<FWidget>& Widget, const FKeyEvent& KeyEvent)
        {
            return Widget->OnKeyDown(KeyEvent);
        });

    return WidgetResponse.IsEventHandled();
}

bool FApplication::OnAnalogGamepadChange(EAnalogSourceName::Type AnalogSource, uint32 GamepadIndex, float AnalogValue)
{
    const FAnalogGamepadEvent AnalogGamepadEvent(EInputEventType::GamepadAnalogSourceChanged, AnalogSource, GamepadIndex, PlatformApplication->GetModifierKeyState(), AnalogValue);

    const FEventResponse PreProcessResponse = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), AnalogGamepadEvent,
        [](const TSharedPtr<FInputHandler>& InputHandler, const FAnalogGamepadEvent& AnalogGamepadEvent)
        {
            return InputHandler->OnAnalogGamepadChange(AnalogGamepadEvent);
        });

    if (PreProcessResponse.IsEventHandled())
    {
        return true;
    }

    const FEventResponse WidgetResponse = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(FocusPath), AnalogGamepadEvent,
        [](const TSharedPtr<FWidget>& Widget, const FAnalogGamepadEvent& AnalogGamepadEvent)
        {
            return Widget->OnAnalogGamepadChange(AnalogGamepadEvent);
        });

    return WidgetResponse.IsEventHandled();
}

bool FApplication::OnKeyUp(EKeyboardKeyName::Type KeyCode, FModifierKeyState ModierKeyState)
{
    const FKeyEvent KeyEvent(EInputEventType::KeyUp, FInputMapper::Get().GetKeyboardKey(KeyCode), ModierKeyState, false, false);

    const FEventResponse PreProcessResponse = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), KeyEvent,
        [](const TSharedPtr<FInputHandler>& InputHandler, const FKeyEvent& KeyEvent)
        {
            return InputHandler->OnKeyUp(KeyEvent);
        });

    PressedKeys.Remove(KeyCode);

    const FEventResponse WidgetResponse = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(FocusPath), KeyEvent,
        [](const TSharedPtr<FWidget>& Widget, const FKeyEvent& KeyEvent)
        {
            return Widget->OnKeyUp(KeyEvent);
        });

    return PreProcessResponse.IsEventHandled() || WidgetResponse.IsEventHandled();
}

bool FApplication::OnKeyDown(EKeyboardKeyName::Type KeyCode, bool bIsRepeat, FModifierKeyState ModierKeyState)
{
    PressedKeys.Add(KeyCode);

    const FKeyEvent KeyEvent(EInputEventType::KeyDown, FInputMapper::Get().GetKeyboardKey(KeyCode), ModierKeyState, bIsRepeat, true);

    const FEventResponse PreProcessResponse = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), KeyEvent,
        [](const TSharedPtr<FInputHandler>& InputHandler, const FKeyEvent& KeyEvent)
        {
            return InputHandler->OnKeyDown(KeyEvent);
        });

    if (PreProcessResponse.IsEventHandled())
    {
        return true;
    }

    const FEventResponse WidgetResponse = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(FocusPath), KeyEvent,
        [](const TSharedPtr<FWidget>& Widget, const FKeyEvent& KeyEvent)
        {
            return Widget->OnKeyDown(KeyEvent);
        });

    return WidgetResponse.IsEventHandled();
}

bool FApplication::OnKeyChar(uint32 Character)
{
    const FKeyEvent KeyEvent(EInputEventType::KeyChar, Keys::Unknown, PlatformApplication->GetModifierKeyState(), Character, false, true);

    const FEventResponse PreProcessResponse = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), KeyEvent,
        [](const TSharedPtr<FInputHandler>& InputHandler, const FKeyEvent& KeyEvent)
        {
            return InputHandler->OnKeyChar(KeyEvent);
        });

    if (PreProcessResponse.IsEventHandled())
    {
        return true;
    }

    const FEventResponse WidgetResponse = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(FocusPath), KeyEvent,
        [](const TSharedPtr<FWidget>& Widget, const FKeyEvent& KeyEvent)
        {
            return Widget->OnKeyChar(KeyEvent);
        });

    return WidgetResponse.IsEventHandled();
}

bool FApplication::OnMouseMove(int32 MouseX, int32 MouseY)
{
    const FCursorEvent CursorEvent(EInputEventType::MouseMoved, IntVector2(MouseX, MouseY), PlatformApplication->GetModifierKeyState());

    const FEventResponse PreProcessResponse = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), CursorEvent,
        [](const TSharedPtr<FInputHandler>& InputHandler, const FCursorEvent& CursorEvent)
        {
            return InputHandler->OnMouseMove(CursorEvent);
        });

    if (PreProcessResponse.IsEventHandled())
    {
        return true;
    }

    FWidgetPath CursorPath;
    FindWidgetsUnderCursor(CursorEvent.GetCursorPos(), CursorPath);

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

    const FEventResponse MouseEnteredResponse = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(CursorPath), CursorEvent,
        [this](const TSharedPtr<FWidget>& Widget, const FCursorEvent& CursorEvent)
        {
            if (!TrackedWidgets.Contains(Widget))
            {
                TrackedWidgets.Add(EVisibility::Visible, Widget);
                Widget->OnMouseEntered(CursorEvent);
            }

            return FEventResponse::Unhandled();
        });

    const FEventResponse MouseMoveResponse = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(CursorPath), CursorEvent,
        [](const TSharedPtr<FWidget>& Widget, const FCursorEvent& CursorEvent)
        {
            return Widget->OnMouseMove(CursorEvent);
        });

    return MouseEnteredResponse.IsEventHandled() || MouseMoveResponse.IsEventHandled();
}

bool FApplication::OnMouseButtonDown(const TSharedRef<FGenericWindow>& PlatformWindow, EMouseButtonName::Type Button, FModifierKeyState ModierKeyState)
{
    PressedMouseButtons.Add(Button);

    // Set the mouse capture when the mouse is pressed
    PlatformApplication->SetCapture(PlatformWindow);
    bIsTrackingCursor = true;

    const FCursorEvent CursorEvent(EInputEventType::MouseButtonDown, FInputMapper::Get().GetMouseKey(Button), ModierKeyState, true);

    const FEventResponse PreProcessResponse = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), CursorEvent,
        [](const TSharedPtr<FInputHandler>& InputHandler, const FCursorEvent& CursorEvent)
        {
            return InputHandler->OnMouseButtonDown(CursorEvent);
        });

    if (PreProcessResponse.IsEventHandled())
    {
        return true;
    }

    FWidgetPath CursorPath;
    FindWidgetsUnderCursor(CursorPath);

    const FEventResponse WidgetResponse = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(CursorPath), CursorEvent,
        [this](const TSharedPtr<FWidget>& Widget, const FCursorEvent& CursorEvent)
        {
            const FEventResponse Response = Widget->OnMouseButtonDown(CursorEvent);
            if (Response.IsEventHandled() && !TrackedWidgets.Contains(Widget))
            {
                TrackedWidgets.Add(EVisibility::Visible, Widget);
            }

            return Response;
        });

    SetFocusWidgets(CursorPath);
    return WidgetResponse.IsEventHandled();
}

bool FApplication::OnMouseButtonUp(EMouseButtonName::Type Button, FModifierKeyState ModiferKeyState)
{
    PressedMouseButtons.Remove(Button);

    // Remove the mouse capture if there is a capture
    if (PressedMouseButtons.IsEmpty())
    {
        PlatformApplication->SetCapture(nullptr);
        bIsTrackingCursor = false;
    }

    const FCursorEvent CursorEvent(EInputEventType::MouseButtonUp, FInputMapper::Get().GetMouseKey(Button), ModiferKeyState, false);

    const FEventResponse PreProcessResponse = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), CursorEvent,
        [](const TSharedPtr<FInputHandler>& InputHandler, const FCursorEvent& CursorEvent)
        {
            return InputHandler->OnMouseButtonUp(CursorEvent);
        });

    if (PreProcessResponse.IsEventHandled())
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

    const FEventResponse WidgetResponse = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(CursorPath), CursorEvent,
        [](const TSharedPtr<FWidget>& Widget, const FCursorEvent& CursorEvent)
        {
            return Widget->OnMouseButtonUp(CursorEvent);
        });

    return WidgetResponse.IsEventHandled();
}

bool FApplication::OnMouseButtonDoubleClick(EMouseButtonName::Type Button, FModifierKeyState ModierKeyState)
{
    const FCursorEvent CursorEvent(EInputEventType::MouseButtonDoubleClick, FInputMapper::Get().GetMouseKey(Button), ModierKeyState, true);

    const FEventResponse PreProcessResponse = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), CursorEvent,
        [](const TSharedPtr<FInputHandler>& InputHandler, const FCursorEvent& CursorEvent)
        {
            return InputHandler->OnMouseButtonDown(CursorEvent);
        });

    if (PreProcessResponse.IsEventHandled())
    {
        return true;
    }

    FWidgetPath CursorPath;
    FindWidgetsUnderCursor(CursorPath);

    const FEventResponse WidgetResponse = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(CursorPath), CursorEvent,
        [](const TSharedPtr<FWidget>& Widget, const FCursorEvent& CursorEvent)
        {
            return Widget->OnMouseDoubleClick(CursorEvent);
        });

    SetFocusWidgets(CursorPath);
    return WidgetResponse.IsEventHandled();
}

bool FApplication::OnMouseScrolled(float WheelDelta, bool bVertical)
{
    const FCursorEvent CursorEvent(EInputEventType::MouseScrolled, PlatformApplication->GetModifierKeyState(), WheelDelta, bVertical);

    const FEventResponse PreProcessResponse = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), CursorEvent,
        [](const TSharedPtr<FInputHandler>& InputHandler, const FCursorEvent& CursorEvent)
        {
            return InputHandler->OnMouseScrolled(CursorEvent);
        });

    if (PreProcessResponse.IsEventHandled())
    {
        return true;
    }

    FWidgetPath CursorPath;
    FindWidgetsUnderCursor(CursorPath);

    const FEventResponse WidgetResponse = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(CursorPath), CursorEvent,
        [](const TSharedPtr<FWidget>& Widget, const FCursorEvent& CursorEvent)
        {
            return Widget->OnMouseScroll(CursorEvent);
        });

    return WidgetResponse.IsEventHandled();
}

bool FApplication::OnMouseEntered()
{
    const FCursorEvent CursorEvent(EInputEventType::MouseEntered, PlatformApplication->GetModifierKeyState());

    FWidgetPath CursorPath;
    FindWidgetsUnderCursor(CursorPath);

    const FEventResponse Response = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(CursorPath), CursorEvent,
        [this](const TSharedPtr<FWidget>& Widget, const FCursorEvent& CursorEvent)
        {
            if (!TrackedWidgets.Contains(Widget))
            {
                TrackedWidgets.Add(EVisibility::Visible, Widget);
                Widget->OnMouseEntered(CursorEvent);
            }

            return FEventResponse::Unhandled();
        });

    return Response.IsEventHandled();
}

bool FApplication::OnMouseLeft()
{
    const FCursorEvent CursorEvent(EInputEventType::MouseLeft, PlatformApplication->GetModifierKeyState());

    FWidgetPath CursorPath;
    FindWidgetsUnderCursor(CursorPath);

    FEventResponse Response = FEventResponse::Unhandled();

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

bool FApplication::OnHighPrecisionMouseInput(int32 MouseX, int32 MouseY)
{
    const FCursorEvent CursorEvent(EInputEventType::HighPrecisionMouse, IntVector2(MouseX, MouseY), PlatformApplication->GetModifierKeyState());

    const FEventResponse PreProcessResponse = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), CursorEvent,
        [](const TSharedPtr<FInputHandler>& InputHandler, const FCursorEvent& CursorEvent)
        {
            return InputHandler->OnHighPrecisionMouseInput(CursorEvent);
        });

    if (PreProcessResponse.IsEventHandled())
    {
        return true;
    }

    FWidgetPath CursorPath;
    FindWidgetsUnderCursor(CursorPath);

    const FEventResponse WidgetResponse = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(CursorPath), CursorEvent,
        [](const TSharedPtr<FWidget>& Widget, const FCursorEvent& CursorEvent)
        {
            return Widget->OnHighPrecisionMouseInput(CursorEvent);
        });

    return WidgetResponse.IsEventHandled();
}

bool FApplication::OnWindowResized(const TSharedRef<FGenericWindow>& PlatformWindow, uint32 Width, uint32 Height)
{
    bool bResult = false;

    if (TSharedPtr<FWindowWidget> Window = FindWindowFromGenericWindow(PlatformWindow))
    {
        IntVector2 NewScreenSize(Width, Height);
        Window->OnWindowResize(NewScreenSize);
        bResult = true;
    }

    return bResult;
}

bool FApplication::OnWindowResizing(const TSharedRef<FGenericWindow>&)
{
    // We wait for the GPU here to avoid weird resizing behavior
    FRHICommandListExecutor::Get().WaitForGPU();
    return true;
}

bool FApplication::OnWindowMoved(const TSharedRef<FGenericWindow>& PlatformWindow, int32 x, int32 y)
{
    bool bResult = false;

    if (TSharedPtr<FWindowWidget> Window = FindWindowFromGenericWindow(PlatformWindow))
    {
        IntVector2 NewScreenPosition(x, y);
        Window->OnWindowMoved(NewScreenPosition);
        bResult = true;
    }

    return bResult;
}

bool FApplication::OnWindowFocusLost(const TSharedRef<FGenericWindow>& PlatformWindow)
{
    bool bResult = false;

    if (TSharedPtr<FWindowWidget> Window = FindWindowFromGenericWindow(PlatformWindow))
    {
        Window->OnWindowFocusChanged(false);
        bResult = true;
    }

    SetFocusWidget(nullptr);
    return bResult;
}

bool FApplication::OnWindowFocusGained(const TSharedRef<FGenericWindow>& PlatformWindow)
{
    bool bResult = false;

    if (TSharedPtr<FWindowWidget> Window = FindWindowFromGenericWindow(PlatformWindow))
    {
        TSharedPtr<FWidget> FocusWidget = Window;

        if (TSharedPtr<FWidget> ContentWidget = Window->GetContent())
        {
            if (ContentWidget->GetActivationPolicy() == EWidgetActivationPolicy::AutoFocusOnWindowActivate)
            {
                FocusWidget = ContentWidget;
            }
        }

        SetFocusWidget(FocusWidget);

        Window->OnWindowFocusChanged(true);
        bResult = true;
    }

    return bResult;
}

bool FApplication::OnWindowClosed(const TSharedRef<FGenericWindow>& PlatformWindow)
{
    bool bResult = false;

    if (TSharedPtr<FWindowWidget> Window = FindWindowFromGenericWindow(PlatformWindow))
    {
        DestroyWindow(Window);
        bResult = true;
    }

    return bResult;
}

bool FApplication::OnMonitorConfigurationChange()
{
    // Invalidate the cached monitor-information.
    bIsMonitorInfoValid = false;

    // First update the cached monitor-information.
    UpdateMonitorInfo();

    // Then notify listeners that the monitor configuration has changed.
    OnMonitorConfigChangedEvent.Broadcast();
    return true;
}

bool FApplication::EnableHighPrecisionMouseForWindow(const TSharedPtr<FWindowWidget>& Window)
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

FModifierKeyState FApplication::GetModifierKeyState() const
{
    return PlatformApplication->GetModifierKeyState();
}

bool FApplication::SupportsHighPrecisionMouse() const 
{
    return PlatformApplication->SupportsHighPrecisionMouse();
}

void FApplication::SetCursorPosition(const IntVector2& Position)
{
    if (TSharedPtr<ICursor> Cursor = GetCursor())
    {
        Cursor->SetPosition(Position.X, Position.Y);
    }
}

IntVector2 FApplication::GetCursorPosition() const
{
    if (TSharedPtr<ICursor> Cursor = GetCursor())
    {
        return Cursor->GetPosition();
    }

    return IntVector2();
}

void FApplication::SetCursor(ECursor InCursor)
{
    if (TSharedPtr<ICursor> Cursor = GetCursor())
    {
        Cursor->SetCursor(InCursor);
    }
}

void FApplication::ShowCursor(bool bIsVisible)
{
    if (TSharedPtr<ICursor> Cursor = GetCursor())
    {
        Cursor->SetVisibility(bIsVisible);
    }
}

bool FApplication::IsCursorVisible() const
{
    if (TSharedPtr<ICursor> Cursor = GetCursor())
    {
        return Cursor->IsVisible();
    }

    return false;
}

bool FApplication::IsGamePadConnected() const
{
    if (FInputDevice* InputDevice = GetInputDevice())
    {
        return InputDevice->IsDeviceConnected();
    }

    return false;
}

TSharedPtr<FWindowWidget> FApplication::FindWindowFromGenericWindow(const TSharedRef<FGenericWindow>& PlatformWindow) const
{
    if (!PlatformWindow)
    {
        return nullptr;
    }

    for (TSharedPtr<FWindowWidget> CurrentWindow : Windows)
    {
        if (PlatformWindow == CurrentWindow->GetPlatformWindow())
        {
            return CurrentWindow;
        }
    }

    return nullptr;
}

void FApplication::OverridePlatformApplication(const TSharedPtr<FGenericApplication>& InPlatformApplication)
{
    if (PlatformApplication)
    {
        PlatformApplication->SetMessageHandler(MakeSharedPtr<FGenericApplicationMessageHandler>());
    }

    if (InPlatformApplication)
    {
        CHECK(PlatformApplication != InPlatformApplication);
        InPlatformApplication->SetMessageHandler(GApplicationInstance);
    }

    PlatformApplication = InPlatformApplication;
}

void FApplication::SetFocusWidget(const TSharedPtr<FWidget>& FocusWidget)
{
    FWidgetPath NewFocusPath;
    if (FocusWidget)
    {
        FocusWidget->FindParentWidgets(NewFocusPath);
    }

    SetFocusWidgets(NewFocusPath);
}

void FApplication::SetFocusWidgets(const FWidgetPath& NewFocusPath)
{
    // First we need to go through all the widgets that currently have focus and notify widgets that is not in the new widget-path that they have lost focus
    for (int32 Index = 0; Index < FocusPath.Size(); Index++)
    {
        const TSharedPtr<FWidget>& CurrentWidget = FocusPath[Index];
        if (!NewFocusPath.Contains(CurrentWidget))
        {
            CurrentWidget->OnFocusLost();
        }
    }

    // Then go through all the widgets in the new widget-path and notify them that they have gained focus, as long as they are not a part of the old path
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

TSharedPtr<FWindowWidget> FApplication::FindWindowWidget(const TSharedPtr<FWidget>& InWidget)
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
        return StaticCastSharedPtr<FWindowWidget>(ParentWidget.ToSharedPtr());
    }

    return nullptr;
}

TSharedPtr<FWindowWidget> FApplication::FindWindowUnderCursor() const
{
    if (TSharedRef<FGenericWindow> PlatformWindow = PlatformApplication->GetWindowUnderCursor())
    {
        return FindWindowFromGenericWindow(PlatformWindow);
    }

    return nullptr;
}

void FApplication::FindWidgetsUnderCursor(FWidgetPath& OutCursorPath)
{
    if (TSharedPtr<ICursor> Cursor = GetCursor())
    {
        FindWidgetsUnderCursor(Cursor->GetPosition(), OutCursorPath);
    }
}

void FApplication::FindWidgetsUnderCursor(const IntVector2& Point, FWidgetPath& OutCursorPath)
{
    if (TSharedRef<FGenericWindow> PlatformWindow = PlatformApplication->GetWindowUnderCursor())
    {
        if (TSharedPtr<FWindowWidget> CursorWindow = FindWindowFromGenericWindow(PlatformWindow))
        {
            CursorWindow->FindChildrenContainingPoint(Point, OutCursorPath);
        }
    }
}

void FApplication::GetDisplayInfo(TArray<FMonitorInfo>& OutMonitorInfo)
{
    if (!bIsMonitorInfoValid)
    {
        UpdateMonitorInfo();
    }

    OutMonitorInfo = MonitorInfos;
}

TSharedPtr<FWindowWidget> FApplication::GetFocusWindow() const
{
    if (TSharedRef<FGenericWindow> ActiveWindow = PlatformApplication->GetActiveWindow())
    {
        return FindWindowFromGenericWindow(ActiveWindow);
    }

    return nullptr;
}

TSharedPtr<FWidget> FApplication::GetFocusLeafWidget() const
{
    if (!FocusPath.IsEmpty())
    {
        return FocusPath[FocusPath.LastIndex()];
    }

    return nullptr;
}
