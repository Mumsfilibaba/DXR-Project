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

IMPLEMENT_ENGINE_MODULE(FModuleInterface, Application);

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

// FEventPreProcessor is a helper class for pre-processing input events. This allows input handlers
// to be notified about events before they are sent to the correct widgets. Events can have
// different policies that decide in what order they are pre-processed. Currently, the policy is:
//  - FPreProcessPolicy: Goes through the InputHandler array in order. This means that the
//    InputHandlers are looped through in order until an InputHandler returns that it has
//    handled the event.

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

TSharedPtr<FGenericApplication>   FApplication::GPlatformApplication = nullptr;
TSharedPtr<FApplication> FApplication::GApplication = nullptr;

bool FApplication::Create()
{
    // Initialize the Input mappings
    FInputMapper::Get().Initialize();

    GPlatformApplication = FPlatformApplication::Create();
    if (!GPlatformApplication)
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to create FPlatformApplication");
        return false;
    }

    GApplication = MakeSharedPtr<FApplication>();
    GPlatformApplication->SetMessageHandler(GApplication);
    return true;
}

void FApplication::Destroy()
{
    if (GApplication)
    {
        GApplication->OverridePlatformApplication(nullptr);
        GApplication.Reset();
    }

    if (GPlatformApplication)
    {
        GPlatformApplication->SetMessageHandler(nullptr);
        GPlatformApplication.Reset();
    }
}

FApplication::FApplication()
    : PressedKeys()
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
    // Init monitor information
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

        if (PlatformWindow == GPlatformApplication->GetCapture())
        {
            // Give capture back to the first window so that we'll still receive the MOUSEUP event.
            TSharedPtr<FWindowWidget> NextWindow = Windows[0];
            GPlatformApplication->SetCapture(NextWindow->GetPlatformWindow());
        }
    }
}

void FApplication::Tick(float Delta)
{
    ProcessEvents();

    ProcessDeferredEvents();

    GPlatformApplication->Tick(Delta);

    UpdateInputDevices();

    // Tick all the windows, which in turn ticks their children
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
    GPlatformApplication->ProcessEvents();
}

void FApplication::ProcessDeferredEvents()
{
    GPlatformApplication->ProcessDeferredEvents();
}

void FApplication::UpdateInputDevices()
{
    GPlatformApplication->UpdateInputDevices();
}

void FApplication::UpdateMonitorInfo()
{
    if (!bIsMonitorInfoValid)
    {
        GPlatformApplication->QueryMonitorInfo(MonitorInfos);
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
    const FKeyEvent KeyEvent(EInputEventType::GamepadButtonUp, FInputMapper::Get().GetGamepadKey(Button), GPlatformApplication->GetModifierKeyState(), 0, GamepadIndex, false, false);

    FEventResponse Response = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), KeyEvent,
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

bool FApplication::OnGamepadButtonDown(EGamepadButtonName::Type Button, uint32 GamepadIndex, bool bIsRepeat)
{
    const FKeyEvent KeyEvent(EInputEventType::GamepadButtonDown, FInputMapper::Get().GetGamepadKey(Button), GPlatformApplication->GetModifierKeyState(), 0, GamepadIndex, bIsRepeat, true);

    FEventResponse Response = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), KeyEvent,
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

bool FApplication::OnAnalogGamepadChange(EAnalogSourceName::Type AnalogSource, uint32 GamepadIndex, float AnalogValue)
{
    const FAnalogGamepadEvent AnalogGamepadEvent(EInputEventType::GamepadAnalogSourceChanged, AnalogSource, GamepadIndex, GPlatformApplication->GetModifierKeyState(), AnalogValue);

    FEventResponse Response = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), AnalogGamepadEvent,
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

bool FApplication::OnKeyUp(EKeyboardKeyName::Type KeyCode, FModifierKeyState ModierKeyState)
{
    const FKeyEvent KeyEvent(EInputEventType::KeyUp, FInputMapper::Get().GetKeyboardKey(KeyCode), ModierKeyState, false, false);

    FEventResponse Response = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), KeyEvent,
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

bool FApplication::OnKeyDown(EKeyboardKeyName::Type KeyCode, bool bIsRepeat, FModifierKeyState ModierKeyState)
{
    const FKeyEvent KeyEvent(EInputEventType::KeyDown, FInputMapper::Get().GetKeyboardKey(KeyCode), ModierKeyState, bIsRepeat, true);
    
    FEventResponse Response = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), KeyEvent,
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

bool FApplication::OnKeyChar(uint32 Character)
{
    const FKeyEvent KeyEvent(EInputEventType::KeyChar, EKeys::Unknown, GPlatformApplication->GetModifierKeyState(), Character, false, true);
    
    FEventResponse Response = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), KeyEvent,
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

bool FApplication::OnMouseMove(int32 MouseX, int32 MouseY)
{
    const FCursorEvent CursorEvent(EInputEventType::MouseMoved, FIntVector2(MouseX, MouseY), GPlatformApplication->GetModifierKeyState());

    FEventResponse Response = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), CursorEvent,
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

            return FEventResponse::Unhandled();
        });

    Response = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(CursorPath), CursorEvent,
        [](const TSharedPtr<FWidget>& Widget, const FCursorEvent& CursorEvent)
        {
            return Widget->OnMouseMove(CursorEvent);
        });

    return Response.IsEventHandled();
}

bool FApplication::OnMouseButtonDown(const TSharedRef<FGenericWindow>& PlatformWindow, EMouseButtonName::Type Button, FModifierKeyState ModierKeyState)
{
    // Set the mouse capture when the mouse is pressed
    GPlatformApplication->SetCapture(PlatformWindow);
    bIsTrackingCursor = true;

    const FCursorEvent CursorEvent(EInputEventType::MouseButtonDown, FInputMapper::Get().GetMouseKey(Button), ModierKeyState, true);

    FEventResponse Response = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), CursorEvent,
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
            const FEventResponse Response = Widget->OnMouseButtonDown(CursorEvent);
            if (Response.IsEventHandled() && !TrackedWidgets.Contains(Widget))
            {
                TrackedWidgets.Add(EVisibility::Visible, Widget);
            }

            return Response;
        });

    SetFocusWidgets(CursorPath);
    return Response.IsEventHandled();
}

bool FApplication::OnMouseButtonUp(EMouseButtonName::Type Button, FModifierKeyState ModiferKeyState)
{
    PressedMouseButtons.Remove(Button);

    // Remove the mouse capture if there is a capture
    GPlatformApplication->SetCapture(nullptr);
    bIsTrackingCursor = false;

    const FCursorEvent CursorEvent(EInputEventType::MouseButtonUp, FInputMapper::Get().GetMouseKey(Button), ModiferKeyState, false);
    
    FEventResponse Response = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), CursorEvent,
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

bool FApplication::OnMouseButtonDoubleClick(EMouseButtonName::Type Button, FModifierKeyState ModierKeyState)
{
    const FCursorEvent CursorEvent(EInputEventType::MouseButtonDoubleClick, FInputMapper::Get().GetMouseKey(Button), ModierKeyState, true);

    FEventResponse Response = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), CursorEvent,
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

bool FApplication::OnMouseScrolled(float WheelDelta, bool bVertical)
{
    const FCursorEvent CursorEvent(EInputEventType::MouseScrolled, GPlatformApplication->GetModifierKeyState(), WheelDelta, bVertical);

    FEventResponse Response = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), CursorEvent,
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

bool FApplication::OnMouseEntered()
{
    const FCursorEvent CursorEvent(EInputEventType::MouseEntered, GPlatformApplication->GetModifierKeyState());

    FWidgetPath CursorPath;
    FindWidgetsUnderCursor(CursorPath);

    FEventResponse Response = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(CursorPath), CursorEvent,
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
    const FCursorEvent CursorEvent(EInputEventType::MouseLeft, GPlatformApplication->GetModifierKeyState());

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
    const FCursorEvent CursorEvent(EInputEventType::HighPrecisionMouse, FIntVector2(MouseX, MouseY), GPlatformApplication->GetModifierKeyState());

    FEventResponse Response = FEventPreProcessor::PreProcess(FEventPreProcessor::FPreProcessPolicy(InputHandlers), CursorEvent,
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

bool FApplication::OnWindowResized(const TSharedRef<FGenericWindow>& PlatformWindow, uint32 Width, uint32 Height)
{
    bool bResult = false;
    
    if (TSharedPtr<FWindowWidget> Window = FindWindowFromGenericWindow(PlatformWindow))
    {
        FIntVector2 NewScreenSize(Width, Height);
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
        FIntVector2 NewScreenPosition(x, y);
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
        Window->OnWindowActivationChanged(false);
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
    // Invalidate the cached monitor-information
    bIsMonitorInfoValid = false;
    
    // First update the cached monitor-information...
    UpdateMonitorInfo();

    // ... then notify listeners that the monitor configuration has changed
    OnMonitorConfigChangedEvent.Broadcast();
    return true;
}

bool FApplication::EnableHighPrecisionMouseForWindow(const TSharedPtr<FWindowWidget>& Window)
{ 
    if (Window)
    {
        if (TSharedRef<FGenericWindow> PlatformWindow = Window->GetPlatformWindow())
        {
            return GPlatformApplication->EnableHighPrecisionMouseForWindow(PlatformWindow);
        }
    }

    return false;
}

FModifierKeyState FApplication::GetModifierKeyState() const
{
    return GPlatformApplication->GetModifierKeyState();
}

bool FApplication::SupportsHighPrecisionMouse() const 
{
    return GPlatformApplication->SupportsHighPrecisionMouse();
}

void FApplication::SetCursorPosition(const FIntVector2& Position)
{
    if (TSharedPtr<ICursor> Cursor = GetCursor())
    {
        Cursor->SetPosition(Position.X, Position.Y);
    }
}

FIntVector2 FApplication::GetCursorPosition() const
{
    if (TSharedPtr<ICursor> Cursor = GetCursor())
    {
        return Cursor->GetPosition();
    }

    return FIntVector2();
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
    // Set a MessageHandler to avoid any potential nullptr access
    if (GPlatformApplication)
    {
        GPlatformApplication->SetMessageHandler(MakeSharedPtr<FGenericApplicationMessageHandler>());
    }

    if (InPlatformApplication)
    {
        CHECK(GPlatformApplication != InPlatformApplication);
        InPlatformApplication->SetMessageHandler(GApplication);
    }

    GPlatformApplication = InPlatformApplication;
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
    if (TSharedRef<FGenericWindow> PlatformWindow = GPlatformApplication->GetWindowUnderCursor())
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

void FApplication::FindWidgetsUnderCursor(const FIntVector2& Point, FWidgetPath& OutCursorPath)
{
    if (TSharedRef<FGenericWindow> PlatformWindow = GPlatformApplication->GetWindowUnderCursor())
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

    // Copy the monitor-array
    OutMonitorInfo = MonitorInfos;
}

TSharedPtr<FWindowWidget> FApplication::GetFocusWindow() const
{
    if (TSharedRef<FGenericWindow> ActiveWindow = GPlatformApplication->GetActiveWindow())
    {
        return FindWindowFromGenericWindow(ActiveWindow);
    }

    return nullptr;
}
