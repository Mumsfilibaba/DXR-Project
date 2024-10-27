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

struct FEventDispatcher
{
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
            return Index > 0 && !Widgets.IsEmpty();
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

    template<typename EventType, typename PedicateType>
    static FResponse PreProcess(FPreProcessPolicy Policy, const EventType& Event, PedicateType&& Predicate)
    {
        FResponse Response = FResponse::Unhandled();
        for (; Policy.ShouldProcess(); Policy.Next())
        {
            if (Predicate(Policy.GetPreProcessor(), Event))
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
            Response = Predicate(Policy.GetWidget(), Event);
        }

        return Response;
    }
};

TSharedPtr<FGenericApplication>   FApplicationInterface::PlatformApplication = nullptr;
TSharedPtr<FApplicationInterface> FApplicationInterface::ApplicationInstance = nullptr;

FApplicationInterface::FApplicationInterface()
    : PressedKeys()
    , PressedMouseButtons()
    , DisplayInfo()
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

void FApplicationInterface::CreateWindow(const TSharedPtr<FWindow>& InWindow)
{
    if (!InWindow)
    {
        return;
    }

    if (Windows.Contains(InWindow))
    {
        return;
    }

    TSharedRef<FGenericWindow> PlatformWindow = GetPlatformApplication()->CreateWindow();
    if (!PlatformWindow)
    {
        return;
    }

    const float PrimaryDisplayWidth  = static_cast<float>(DisplayInfo.PrimaryDisplayWidth);
    const float PrimaryDisplayHeight = static_cast<float>(DisplayInfo.PrimaryDisplayHeight);

    float DisplayScaling = 1.0f;
    for (const FMonitorInfo& MonitorInfo : DisplayInfo.MonitorInfos)
    {
        if (MonitorInfo.bIsPrimary)
        {
            DisplayScaling = MonitorInfo.DisplayScaling;
            break;
        }
    }

    const uint32 MinWidth  = 60;
    const uint32 MinHeight = 20;
    const uint32 MaxWidth  = static_cast<uint32>(PrimaryDisplayWidth / DisplayScaling);
    const uint32 MaxHeight = static_cast<uint32>(PrimaryDisplayHeight / DisplayScaling);

    FGenericWindowInitializer WindowInitializer;
    WindowInitializer.Title    = InWindow->GetTitle();
    WindowInitializer.Position = InWindow->GetCachedPosition();
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
    PlatformApplication->QueryDisplayInfo(DisplayInfo);
    bIsMonitorInfoValid = true;
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

    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputHandlers), KeyEvent,
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

    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputHandlers), KeyEvent,
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

    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputHandlers), AnalogGamepadEvent,
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

    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputHandlers), KeyEvent,
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
    
    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputHandlers), KeyEvent,
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
    
    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputHandlers), KeyEvent,
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

    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputHandlers), CursorEvent,
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

    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputHandlers), CursorEvent,
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
    
    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputHandlers), CursorEvent,
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

    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputHandlers), CursorEvent,
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

    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputHandlers), CursorEvent,
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

bool FApplicationInterface::OnHighPrecisionMouseInput(int32 x, uint32 y)
{
    return false;
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

bool FApplicationInterface::OnWindowResizing(const TSharedRef<FGenericWindow>& Window)
{
    return false;
}

bool FApplicationInterface::OnWindowMoved(const TSharedRef<FGenericWindow>& PlatformWindow, int32 x, int32 y)
{
    bool bResult = false;
    if (TSharedPtr<FWindow> Window = FindWindowFromGenericWindow(PlatformWindow))
    {
        FIntVector2 NewScreenPosition(x, y);
        Window->MoveTo(NewScreenPosition);
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
    // First update the cached information...
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

void FApplicationInterface::GetDisplayInfo(FDisplayInfo& OutDisplayInfo)
{
    if (!bIsMonitorInfoValid)
    {
        UpdateMonitorInfo();
    }

    OutDisplayInfo = DisplayInfo;
}

TSharedPtr<FWindow> FApplicationInterface::GetFocusWindow() const
{
    if (TSharedRef<FGenericWindow> ActiveWindow = PlatformApplication->GetActiveWindow())
    {
        return FindWindowFromGenericWindow(ActiveWindow);
    }

    return nullptr;
}
