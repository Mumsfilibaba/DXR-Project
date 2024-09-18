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
        FLeafLastPolicy(FPath& InWidgets)
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
        FPath& Widgets;
        int32  Index;
    };

    class FLeafFirstPolicy
    {
    public:
        FLeafFirstPolicy(FPath& InWidgets)
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
        FPath& Widgets;
        int32  Index;
    };

    class FDirectPolicy
    {
    public:
        FDirectPolicy(FPath& InWidgets)
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
        FPath& Widgets;
        bool   bIsProcessed;
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

TSharedPtr<FGenericApplication>  FWindowedApplication::PlatformApplication = nullptr;
TSharedPtr<FWindowedApplication> FWindowedApplication::ApplicationInstance = nullptr;

FWindowedApplication::FWindowedApplication()
    : FocusPath()
    , InputHandlers()
    , Windows()
    , DisplayInfo()
    , bIsTrackingMouse(false)
    , PressedKeys()
    , PressedMouseButtons()
{
    // Init monitor information
    UpdateMonitorInfo();
}

FWindowedApplication::~FWindowedApplication()
{
}

bool FWindowedApplication::Create()
{
    // Initialize the Input mappings
    FInputMapper::Get().Initialize();

    PlatformApplication = FPlatformApplication::Create();
    if (!PlatformApplication)
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to create FPlatformApplication");
        return false;
    }

    ApplicationInstance = MakeShared<FWindowedApplication>();
    PlatformApplication->SetMessageHandler(ApplicationInstance);
    return true;
}

void FWindowedApplication::Destroy()
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

void FWindowedApplication::InitializeWindow(const TSharedPtr<FWindow>& InWindow)
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

void FWindowedApplication::DestroyWindow(const TSharedPtr<FWindow>& DestroyedWindow)
{
    if (DestroyedWindow)
    {
        DestroyedWindow->NotifyWindowDestroyed();
        Windows.Remove(DestroyedWindow);
    }
}

void FWindowedApplication::Tick(float Delta)
{
    // Run the message-loop
    PlatformApplication->Tick(Delta);

    // Update any extra input devices
    UpdateInputDevices();
}

void FWindowedApplication::UpdateInputDevices()
{
    PlatformApplication->UpdateInputDevices();
}

void FWindowedApplication::UpdateMonitorInfo()
{
    PlatformApplication->GetDisplayInfo(DisplayInfo);
}

void FWindowedApplication::RegisterInputHandler(const TSharedPtr<FInputHandler>& NewInputHandler)
{
    if (NewInputHandler)
    {
        InputHandlers.AddUnique(NewInputHandler);
    }
}

void FWindowedApplication::UnregisterInputHandler(const TSharedPtr<FInputHandler>& InputHandler)
{
    if (InputHandler)
    {
        InputHandlers.Remove(InputHandler);
    }
}

bool FWindowedApplication::OnAnalogGamepadChange(EAnalogSourceName::Type AnalogSource, uint32 GamepadIndex, float AnalogValue)
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

bool FWindowedApplication::OnGamepadButtonUp(EGamepadButtonName::Type Button, uint32 GamepadIndex)
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

bool FWindowedApplication::OnGamepadButtonDown(EGamepadButtonName::Type Button, uint32 GamepadIndex, bool bIsRepeat)
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

bool FWindowedApplication::OnKeyUp(EKeyboardKeyName::Type KeyCode, FModifierKeyState ModierKeyState)
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

bool FWindowedApplication::OnKeyDown(EKeyboardKeyName::Type KeyCode, bool bIsRepeat, FModifierKeyState ModierKeyState)
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

bool FWindowedApplication::OnKeyChar(uint32 Character)
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

bool FWindowedApplication::OnMouseMove(int32 MouseX, int32 MouseY)
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
    FPath CursorPath;
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

bool FWindowedApplication::OnMouseButtonUp(EMouseButtonName::Type Button, FModifierKeyState ModiferKeyState, int32 x, int32 y)
{
    PressedMouseButtons.Remove(Button);

    // Remove the mouse capture if there is a capture
    PlatformApplication->SetCapture(nullptr);
    bIsTrackingMouse = false;

    const FCursorEvent CursorEvent(FInputMapper::Get().GetMouseKey(Button), FIntVector2(x, y), ModiferKeyState, false);
    
    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputHandlers), CursorEvent,
        [](const TSharedPtr<FInputHandler>& InputHandler, const FCursorEvent& CursorEvent)
        {
            return InputHandler->OnMouseButtonUp(CursorEvent);
        });

    if (Response.IsEventHandled())
    {
        return true;
    }

    FPath CursorPath;
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

    Response = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(CursorPath), CursorEvent,
        [](const TSharedPtr<FWidget>& Widget, const FCursorEvent& CursorEvent)
        {
            return Widget->OnMouseButtonUp(CursorEvent);
        });

    return Response.IsEventHandled();
}

bool FWindowedApplication::OnMouseButtonDown(const TSharedRef<FGenericWindow>& PlatformWindow, EMouseButtonName::Type Button, FModifierKeyState ModierKeyState, int32 x, int32 y)
{
    // Set the mouse capture when the mouse is pressed
    PlatformApplication->SetCapture(PlatformWindow);
    bIsTrackingMouse = true;

    const FCursorEvent CursorEvent(FInputMapper::Get().GetMouseKey(Button), FIntVector2(x, y), ModierKeyState, true);

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

    FPath CursorPath;
    FindWidgetsUnderCursor(CursorEvent.GetCursorPos(), CursorPath);

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

    FocusPath = CursorPath;
    return Response.IsEventHandled();
}

bool FWindowedApplication::OnMouseScrolled(float WheelDelta, bool bVertical, int32 x, int32 y)
{
    const FCursorEvent CursorEvent(FIntVector2(x, y), FPlatformApplicationMisc::GetModifierKeyState(), WheelDelta, bVertical);

    FResponse Response = FEventDispatcher::PreProcess(FEventDispatcher::FPreProcessPolicy(InputHandlers), CursorEvent,
        [](const TSharedPtr<FInputHandler>& InputHandler, const FCursorEvent& CursorEvent)
        {
            return InputHandler->OnMouseScrolled(CursorEvent);
        });

    if (Response.IsEventHandled())
    {
        return true;
    }

    FPath CursorPath;
    FindWidgetsUnderCursor(CursorEvent.GetCursorPos(), CursorPath);

    Response = FEventDispatcher::Dispatch(FEventDispatcher::FLeafFirstPolicy(CursorPath), CursorEvent,
        [](const TSharedPtr<FWidget>& Widget, const FCursorEvent& CursorEvent)
        {
            return Widget->OnMouseScroll(CursorEvent);
        });

    return Response.IsEventHandled();
}

bool FWindowedApplication::OnWindowResized(const TSharedRef<FGenericWindow>& PlatformWindow, uint32 Width, uint32 Height)
{
    if (TSharedPtr<FWindow> Window = FindWindowFromGenericWindow(PlatformWindow))
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

bool FWindowedApplication::OnWindowMoved(const TSharedRef<FGenericWindow>& PlatformWindow, int32 x, int32 y)
{
    if (TSharedPtr<FWindow> Window = FindWindowFromGenericWindow(PlatformWindow))
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

bool FWindowedApplication::OnWindowFocusLost(const TSharedRef<FGenericWindow>& PlatformWindow)
{
    if (TSharedPtr<FWindow> Window = FindWindowFromGenericWindow(PlatformWindow))
    {
        Window->NotifyWindowActivationChanged(false);
        return true;
    }
    else
    {
        return false;
    }
}

bool FWindowedApplication::OnWindowFocusGained(const TSharedRef<FGenericWindow>& PlatformWindow)
{
    if (TSharedPtr<FWindow> Window = FindWindowFromGenericWindow(PlatformWindow))
    {
        Window->NotifyWindowActivationChanged(true);
        return true;
    }
    else
    {
        return false;
    }
}

bool FWindowedApplication::OnWindowClosed(const TSharedRef<FGenericWindow>& PlatformWindow)
{
    if (TSharedPtr<FWindow> Window = FindWindowFromGenericWindow(PlatformWindow))
    {
        DestroyWindow(Window);
        return true;
    }
    else
    {
        return false;
    }
}

bool FWindowedApplication::OnMonitorChange()
{
    UpdateMonitorInfo();
    return true;
}

bool FWindowedApplication::EnableHighPrecisionMouseForWindow(const TSharedPtr<FWindow>& Window)
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

bool FWindowedApplication::SupportsHighPrecisionMouse() const 
{
    return PlatformApplication->SupportsHighPrecisionMouse();
}

void FWindowedApplication::SetCursorScreenPosition(const FIntVector2& Position)
{
    if (TSharedPtr<ICursor> Cursor = GetCursor())
    {
        Cursor->SetPosition(Position.x, Position.y);
    }
}

FIntVector2 FWindowedApplication::GetCursorScreenPosition() const
{
    if (TSharedPtr<ICursor> Cursor = GetCursor())
    {
        return Cursor->GetPosition();
    }

    return FIntVector2();
}

void FWindowedApplication::SetCursor(ECursor InCursor)
{
    if (TSharedPtr<ICursor> Cursor = GetCursor())
    {
        Cursor->SetCursor(InCursor);
    }
}

void FWindowedApplication::ShowCursor(bool bIsVisible)
{
    if (TSharedPtr<ICursor> Cursor = GetCursor())
    {
        Cursor->SetVisibility(bIsVisible);
    }
}

bool FWindowedApplication::IsCursorVisibile() const
{
    if (TSharedPtr<ICursor> Cursor = GetCursor())
    {
        return Cursor->IsVisible();
    }

    return false;
}

bool FWindowedApplication::IsGamePadConnected() const
{
    if (FInputDevice* InputDevice = GetInputDeviceInterface())
    {
        return InputDevice->IsDeviceConnected();
    }

    return false;
}

TSharedPtr<FWindow> FWindowedApplication::FindWindowFromGenericWindow(const TSharedRef<FGenericWindow>& PlatformWindow) const
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

void FWindowedApplication::OverridePlatformApplication(const TSharedPtr<FGenericApplication>& InPlatformApplication)
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

TSharedPtr<FWindow> FWindowedApplication::FindWindowWidget(const TSharedPtr<FWidget>& InWidget)
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

void FWindowedApplication::FindWidgetsUnderCursor(const FIntVector2& CursorPosition, FPath& OutCursorPath)
{
    TSharedRef<FGenericWindow> PlatformWindow = PlatformApplication->GetWindowUnderCursor();
    if (!PlatformWindow)
    {
        return;
    }

    TSharedPtr<FWindow> CursorWindow = FindWindowFromGenericWindow(PlatformWindow);
    if (CursorWindow)
    {
        OutCursorPath.Add(EVisibility::Visible, CursorWindow);
        OutCursorPath.Add(EVisibility::Visible, CursorWindow->GetContent());
    }
}

TSharedPtr<FWindow> FWindowedApplication::GetFocusWindow() const
{
    if (TSharedRef<FGenericWindow> ActiveWindow = PlatformApplication->GetActiveWindow())
    {
        return FindWindowFromGenericWindow(ActiveWindow);
    }

    return nullptr;
}