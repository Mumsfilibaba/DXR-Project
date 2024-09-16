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
        FLeafLastPolicy(TArray<TSharedPtr<FWidget>>& InWidgets)
            : Widgets(InWidgets)
            , Index(static_cast<int32>(InWidgets.LastElementIndex()))
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
        TArray<TSharedPtr<FWidget>>& Widgets;
        int32 Index;
    };

    class FLeafFirstPolicy
    {
    public:
        FLeafFirstPolicy(TArray<TSharedPtr<FWidget>>& InWidgets)
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
        TArray<TSharedPtr<FWidget>>& Widgets;
        int32 Index;
    };

    class FDirectPolicy
    {
    public:
        FDirectPolicy(TArray<TSharedPtr<FWidget>>& InWidgets)
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
        TArray<TSharedPtr<FWidget>>& Widgets;
        bool bIsProcessed;
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

TSharedPtr<FApplication>        FApplication::ApplicationInstance  = nullptr;
TSharedPtr<FGenericApplication> FApplication::PlatformApplication = nullptr;

FApplication::FApplication()
    : FocusWindow(nullptr)
    , InputPreProcessors()
    , Windows()
    , DisplayInfo()
    , bIsTrackingMouse(false)
    , PressedKeys()
    , PressedMouseButtons()
{
    // Init monitor information
    UpdateMonitorInfo();
}

FApplication::~FApplication()
{
}

bool FApplication::Create()
{
    // Initialize the Input mappings
    FInputMapper::Get().Initialize();

    PlatformApplication = FPlatformApplication::Create();
    if (!PlatformApplication)
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to create FPlatformApplication");
        return false;
    }

    ApplicationInstance = MakeShared<FApplication>();
    PlatformApplication->SetMessageHandler(ApplicationInstance);
    return true;
}

void FApplication::Destroy()
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

void FApplication::InitializeWindow(const TSharedPtr<FWindow>& InWindow)
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

    const uint32 MinWidth  = 640;
    const uint32 MinHeight = 480;
    const uint32 MaxWidth  = static_cast<uint32>(PrimaryDisplayWidth / DisplayScaling);
    const uint32 MaxHeight = static_cast<uint32>(PrimaryDisplayHeight / DisplayScaling);

    FGenericWindowInitializer WindowInitializer;
    WindowInitializer.Title    = InWindow->GetTitle();
    WindowInitializer.Position = InWindow->GetPosition();
    WindowInitializer.Style    = EWindowStyleFlags::Default;
    WindowInitializer.Width    = FMath::Clamp<int32>(MinWidth, MaxWidth, InWindow->GetWidth());
    WindowInitializer.Height   = FMath::Clamp<int32>(MinHeight, MaxHeight, InWindow->GetHeight());

    if (PlatformWindow->Initialize(WindowInitializer))
    {
        InWindow->SetPlatformWindow(PlatformWindow);
        Windows.Add(InWindow);

        PlatformWindow->Show(true);
    }
}

void FApplication::DestroyWindow(const TSharedPtr<FWindow>& DestroyedWindow)
{
    if (DestroyedWindow)
    {
        DestroyedWindow->NotifyWindowDestroyed();
        Windows.Remove(DestroyedWindow);
    }
}

void FApplication::Tick(float Delta)
{
    // Run the message-loop
    PlatformApplication->Tick(Delta);

    // Update any extra input devices
    UpdateInputDevices();
}

void FApplication::UpdateInputDevices()
{
    PlatformApplication->UpdateInputDevices();
}

void FApplication::UpdateMonitorInfo()
{
    PlatformApplication->GetDisplayInfo(DisplayInfo);
}

void FApplication::RegisterInputHandler(const TSharedPtr<FInputHandler>& NewInputHandler)
{
    if (NewInputHandler)
    {
        InputPreProcessors.AddUnique(NewInputHandler);
    }
}

void FApplication::UnregisterInputHandler(const TSharedPtr<FInputHandler>& InputHandler)
{
    if (InputHandler)
    {
        InputPreProcessors.Remove(InputHandler);
    }
}

bool FApplication::OnAnalogGamepadChange(EAnalogSourceName::Type AnalogSource, uint32 GamepadIndex, float AnalogValue)
{
    const FAnalogGamepadEvent AnalogGamepadEvent(AnalogSource, GamepadIndex, FPlatformApplicationMisc::GetModifierKeyState(), AnalogValue);

    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputPreProcessors), AnalogGamepadEvent,
        [](const TSharedPtr<FInputHandler>& PreProcessor, const FAnalogGamepadEvent& AnalogGamepadEvent)
        {
            return PreProcessor->OnAnalogGamepadChange(AnalogGamepadEvent);
        });
    
    if (Response.IsEventHandled())
    {
        return true;
    }

    // Retrieve all the widgets from the current active window should receive events
    TArray<TSharedPtr<FWidget>> Widgets;
    GetActiveWindowWidgets(Widgets);
    
    Response = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(Widgets), AnalogGamepadEvent,
        [](const TSharedPtr<FWidget>& Widget, const FAnalogGamepadEvent& AnalogGamepadEvent)
        {
            return Widget->OnAnalogGamepadChange(AnalogGamepadEvent);
        });
    
    return Response.IsEventHandled();
}

bool FApplication::OnGamepadButtonUp(EGamepadButtonName::Type Button, uint32 GamepadIndex)
{
    const FKeyEvent KeyEvent(FInputMapper::Get().GetGamepadKey(Button), FPlatformApplicationMisc::GetModifierKeyState(), 0, GamepadIndex, false, false);

    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputPreProcessors), KeyEvent,
        [](const TSharedPtr<FInputHandler>& PreProcessor, const FKeyEvent& KeyEvent)
        {
            return PreProcessor->OnKeyUp(KeyEvent);
        });

    if (Response.IsEventHandled())
    {
        return true;
    }

    // Retrieve all the widgets from the current active window should receive events
    TArray<TSharedPtr<FWidget>> Widgets;
    GetActiveWindowWidgets(Widgets);

    Response = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(Widgets), KeyEvent,
        [](const TSharedPtr<FWidget>& Widget, const FKeyEvent& KeyEvent)
        {
            return Widget->OnKeyUp(KeyEvent);
        });

    return Response.IsEventHandled();
}

bool FApplication::OnGamepadButtonDown(EGamepadButtonName::Type Button, uint32 GamepadIndex, bool bIsRepeat)
{
    const FKeyEvent KeyEvent(FInputMapper::Get().GetGamepadKey(Button), FPlatformApplicationMisc::GetModifierKeyState(), 0, GamepadIndex, bIsRepeat, true);

    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputPreProcessors), KeyEvent,
        [](const TSharedPtr<FInputHandler>& PreProcessor, const FKeyEvent& KeyEvent)
        {
            return PreProcessor->OnKeyDown(KeyEvent);
        });

    if (Response.IsEventHandled())
    {
        return true;
    }

    // Retrieve all the widgets from the current active window should receive events
    TArray<TSharedPtr<FWidget>> Widgets;
    GetActiveWindowWidgets(Widgets);

    Response = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(Widgets), KeyEvent,
        [](const TSharedPtr<FWidget>& Widget, const FKeyEvent& KeyEvent)
        {
            return Widget->OnKeyDown(KeyEvent);
        });

    return Response.IsEventHandled();
}

bool FApplication::OnKeyUp(EKeyboardKeyName::Type KeyCode, FModifierKeyState ModierKeyState)
{
    const FKeyEvent KeyEvent(FInputMapper::Get().GetKeyboardKey(KeyCode), ModierKeyState, false, false);

    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputPreProcessors), KeyEvent,
        [](const TSharedPtr<FInputHandler>& PreProcessor, const FKeyEvent& KeyEvent)
        {
            return PreProcessor->OnKeyUp(KeyEvent);
        });

    // Remove the Key
    PressedKeys.Remove(KeyCode);

    if (Response.IsEventHandled())
    {
        return true;
    }

    // Retrieve all the widgets from the current active window should receive events
    TArray<TSharedPtr<FWidget>> Widgets;
    GetActiveWindowWidgets(Widgets);

    Response = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(Widgets), KeyEvent, 
        [](const TSharedPtr<FWidget>& Widget, const FKeyEvent& KeyEvent)
        {
            return Widget->OnKeyUp(KeyEvent);
        });

    return Response.IsEventHandled();
}

bool FApplication::OnKeyDown(EKeyboardKeyName::Type KeyCode, bool bIsRepeat, FModifierKeyState ModierKeyState)
{
    const FKeyEvent KeyEvent(FInputMapper::Get().GetKeyboardKey(KeyCode), ModierKeyState, bIsRepeat, true);
    
    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputPreProcessors), KeyEvent,
        [](const TSharedPtr<FInputHandler>& PreProcessor, const FKeyEvent& KeyEvent)
        {
            return PreProcessor->OnKeyDown(KeyEvent);
        });

    if (Response.IsEventHandled())
    {
        return true;
    }

    // Add key
    PressedKeys.Add(KeyCode);

    // Retrieve all the widgets from the current active window should receive events
    TArray<TSharedPtr<FWidget>> Widgets;
    GetActiveWindowWidgets(Widgets);

    Response = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(Widgets), KeyEvent, 
        [](const TSharedPtr<FWidget>& Widget, const FKeyEvent& KeyEvent)
        {
            return Widget->OnKeyDown(KeyEvent);
        });

    return Response.IsEventHandled();
}

bool FApplication::OnKeyChar(uint32 Character)
{
    const FKeyEvent KeyEvent(EKeys::Unknown, FPlatformApplicationMisc::GetModifierKeyState(), Character, false, true);
    
    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputPreProcessors), KeyEvent,
        [](const TSharedPtr<FInputHandler>& PreProcessor, const FKeyEvent& KeyEvent)
        {
            return PreProcessor->OnKeyChar(KeyEvent);
        });

    if (Response.IsEventHandled())
    {
        return true;
    }

    // Retrieve all the widgets from the current active window should receive events
    TArray<TSharedPtr<FWidget>> Widgets;
    GetActiveWindowWidgets(Widgets);

    Response = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(Widgets), KeyEvent, 
        [](const TSharedPtr<FWidget>& Widget, const FKeyEvent& KeyEvent)
        {
            return Widget->OnKeyChar(KeyEvent);
        });

    return Response.IsEventHandled();
}

bool FApplication::OnMouseMove(int32 MouseX, int32 MouseY)
{
    const FCursorEvent CursorEvent(FIntVector2(MouseX, MouseY), FPlatformApplicationMisc::GetModifierKeyState());
    
    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputPreProcessors), CursorEvent,
        [](const TSharedPtr<FInputHandler>& PreProcessor, const FCursorEvent& CursorEvent)
        {
            return PreProcessor->OnMouseMove(CursorEvent);
        });

    if (Response.IsEventHandled())
    {
        return true;
    }

    // Retrieve all the widgets under the cursor which should receive events
    TArray<TSharedPtr<FWidget>> Widgets;
    FindWidgetsUnderCursor(CursorEvent.GetCursorPos(), Widgets);

    // Remove the widget from any widget which is not tracked
    const bool bIsDragging = !PressedMouseButtons.IsEmpty();
    for (TArray<TSharedPtr<FWidget>>::IteratorType Iterator = TrackedWidgets.Iterator(); !Iterator.IsEnd(); )
    {
        const TSharedPtr<FWidget>& CurrentWidget = *Iterator;
        if (!Widgets.Contains(CurrentWidget) && !bIsDragging)
        {
            CurrentWidget->OnMouseLeft(CursorEvent);
            TrackedWidgets.RemoveAt(Iterator.GetIndex());
        }
        else
        {
            Iterator++;
        }
    }

    Response = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(Widgets), CursorEvent,
        [this](const TSharedPtr<FWidget>& Widget, const FCursorEvent& CursorEvent)
        {
            if (!TrackedWidgets.Contains(Widget))
            {
                TrackedWidgets.Add(Widget);
                Widget->OnMouseEntered(CursorEvent);
            }

            return FResponse::Unhandled();
        });

    Response = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(Widgets), CursorEvent,
        [](const TSharedPtr<FWidget>& Widget, const FCursorEvent& CursorEvent)
        {
            return Widget->OnMouseMove(CursorEvent);
        });

    return Response.IsEventHandled();
}

bool FApplication::OnMouseButtonUp(EMouseButtonName::Type Button, FModifierKeyState ModiferKeyState, int32 x, int32 y)
{
    // Remove the Key
    PressedMouseButtons.Remove(Button);

    // Remove the mouse capture if there is a capture
    PlatformApplication->SetCapture(nullptr);
    bIsTrackingMouse = false;

    const FCursorEvent CursorEvent(FInputMapper::Get().GetMouseKey(Button), FIntVector2(x, y), ModiferKeyState, false);
    
    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputPreProcessors), CursorEvent,
        [](const TSharedPtr<FInputHandler>& PreProcessor, const FCursorEvent& CursorEvent)
        {
            return PreProcessor->OnMouseButtonUp(CursorEvent);
        });

    if (Response.IsEventHandled())
    {
        return true;
    }

    // Retrieve all the widgets under the cursor which should receive events
    TArray<TSharedPtr<FWidget>> Widgets;
    FindWidgetsUnderCursor(CursorEvent.GetCursorPos(), Widgets);

    // Remove the widget from any widget which is not tracked
    const bool bIsDragging = !PressedMouseButtons.IsEmpty();
    for (TArray<TSharedPtr<FWidget>>::IteratorType Iterator = TrackedWidgets.Iterator(); !Iterator.IsEnd(); )
    {
        const TSharedPtr<FWidget>& CurrentWidget = *Iterator;
        if (!Widgets.Contains(CurrentWidget) && !bIsDragging)
        {
            CurrentWidget->OnMouseLeft(CursorEvent);
            TrackedWidgets.RemoveAt(Iterator.GetIndex());
        }
        else
        {
            Iterator++;
        }
    }

    Response = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(Widgets), CursorEvent,
        [](const TSharedPtr<FWidget>& Widget, const FCursorEvent& CursorEvent)
        {
            return Widget->OnMouseButtonUp(CursorEvent);
        });

    return Response.IsEventHandled();
}

bool FApplication::OnMouseButtonDown(const TSharedRef<FGenericWindow>& PlatformWindow, EMouseButtonName::Type Button, FModifierKeyState ModierKeyState, int32 x, int32 y)
{
    // Set the mouse capture when the mouse is pressed
    PlatformApplication->SetCapture(PlatformWindow);
    bIsTrackingMouse = true;

    const FCursorEvent CursorEvent(FInputMapper::Get().GetMouseKey(Button), FIntVector2(x, y), ModierKeyState, true);

    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputPreProcessors), CursorEvent,
        [](const TSharedPtr<FInputHandler>& PreProcessor, const FCursorEvent& CursorEvent)
        {
            return PreProcessor->OnMouseButtonDown(CursorEvent);
        });

    if (Response.IsEventHandled())
    {
        return true;
    }

    // Add the button to the pressed buttons
    PressedMouseButtons.Remove(Button);

    // Retrieve all the widgets under the cursor which should receive events
    TArray<TSharedPtr<FWidget>> Widgets;
    FindWidgetsUnderCursor(CursorEvent.GetCursorPos(), Widgets);

    Response = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(Widgets), CursorEvent,
        [this](const TSharedPtr<FWidget>& Widget, const FCursorEvent& CursorEvent)
        {
            const FResponse Response = Widget->OnMouseButtonDown(CursorEvent);
            if (Response.IsEventHandled() && !TrackedWidgets.Contains(Widget))
            {
                TrackedWidgets.Add(Widget);
            }

            return Response;
        });

    return Response.IsEventHandled();
}

bool FApplication::OnMouseScrolled(float WheelDelta, bool bVertical, int32 x, int32 y)
{
    const FCursorEvent CursorEvent(FIntVector2(x, y), FPlatformApplicationMisc::GetModifierKeyState(), WheelDelta, bVertical);

    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputPreProcessors), CursorEvent,
        [](const TSharedPtr<FInputHandler>& PreProcessor, const FCursorEvent& CursorEvent)
        {
            return PreProcessor->OnMouseScrolled(CursorEvent);
        });

    if (Response.IsEventHandled())
    {
        return true;
    }

    // Retrieve all the widgets under the cursor which should receive events
    TArray<TSharedPtr<FWidget>> Widgets;
    FindWidgetsUnderCursor(CursorEvent.GetCursorPos(), Widgets);

    Response = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(Widgets), CursorEvent,
        [](const TSharedPtr<FWidget>& Widget, const FCursorEvent& CursorEvent)
        {
            return Widget->OnMouseScroll(CursorEvent);
        });

    return Response.IsEventHandled();
}

bool FApplication::OnWindowResized(const TSharedRef<FGenericWindow>& PlatformWindow, uint32 Width, uint32 Height)
{
    if (TSharedPtr<FWindow> Window = GetWindowFromPlatformWindow(PlatformWindow))
    {
        FIntVector2 NewScreenSize(Width, Height);
        Window->SetScreenSize(NewScreenSize);
        return true;
    }
    else
    {
        return false;
    }
}

bool FApplication::OnWindowMoved(const TSharedRef<FGenericWindow>& PlatformWindow, int32 x, int32 y)
{
    if (TSharedPtr<FWindow> Window = GetWindowFromPlatformWindow(PlatformWindow))
    {
        FIntVector2 NewScreenPosition(x, y);
        Window->SetScreenPosition(NewScreenPosition);
        return true;
    }
    else
    {
        return false;
    }
}

bool FApplication::OnWindowFocusLost(const TSharedRef<FGenericWindow>& PlatformWindow)
{
    if (TSharedPtr<FWindow> Window = GetWindowFromPlatformWindow(PlatformWindow))
    {
        Window->NotifyWindowActivationChanged(false);
        return true;
    }
    else
    {
        return false;
    }
}

bool FApplication::OnWindowFocusGained(const TSharedRef<FGenericWindow>& PlatformWindow)
{
    if (TSharedPtr<FWindow> Window = GetWindowFromPlatformWindow(PlatformWindow))
    {
        Window->NotifyWindowActivationChanged(true);
        return true;
    }
    else
    {
        return false;
    }
}

bool FApplication::OnWindowClosed(const TSharedRef<FGenericWindow>& PlatformWindow)
{
    if (TSharedPtr<FWindow> Window = GetWindowFromPlatformWindow(PlatformWindow))
    {
        DestroyWindow(Window);
        return true;
    }
    else
    {
        return false;
    }
}

bool FApplication::OnMonitorChange()
{
    UpdateMonitorInfo();
    return true;
}

bool FApplication::EnableHighPrecisionMouseForWindow(const TSharedPtr<FWindow>& Window)
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

bool FApplication::SupportsHighPrecisionMouse() const 
{
    return PlatformApplication->SupportsHighPrecisionMouse();
}

void FApplication::SetCursorScreenPosition(const FIntVector2& Position)
{
    if (TSharedPtr<ICursor> Cursor = GetCursor())
    {
        Cursor->SetPosition(Position.x, Position.y);
    }
}

FIntVector2 FApplication::GetCursorScreenPosition() const
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

TSharedPtr<FWindow> FApplication::GetWindowFromPlatformWindow(const TSharedRef<FGenericWindow>& PlatformWindow) const
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

void FApplication::OverridePlatformApplication(const TSharedPtr<FGenericApplication>& InPlatformApplication)
{
    // Set a MessageHandler to avoid any potential nullptr access
    if (PlatformApplication)
    {
        PlatformApplication->SetMessageHandler(MakeShared<FGenericApplicationMessageHandler>());
    }

    if (InPlatformApplication)
    {
        CHECK(PlatformApplication != InPlatformApplication);
        InPlatformApplication->SetMessageHandler(ApplicationInstance);
    }

    PlatformApplication = InPlatformApplication;
}

void FApplication::GetActiveWindowWidgets(TArray<TSharedPtr<FWidget>>& OutWidgets)
{
    // TODO: Retrieve the widgets from the active window
}

TSharedPtr<FWindow> FApplication::FindWindowWidget(const TSharedPtr<FWidget>& InWidget)
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

void FApplication::FindWidgetsUnderCursor(const FIntVector2& CursorPosition, TArray<TSharedPtr<FWidget>>& OutWidgets)
{
    // TODO: Retrieve the widgets from the active window
}

TSharedPtr<FWindow> FApplication::GetFocusWindow() const
{
    if (TSharedRef<FGenericWindow> ActiveWindow = PlatformApplication->GetActiveWindow())
    {
        return GetWindowFromPlatformWindow(ActiveWindow);
    }

    return nullptr;
}