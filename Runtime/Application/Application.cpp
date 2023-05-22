#include "Application.h"
#include "Core/Modules/ModuleManager.h"
#include "CoreApplication/Platform/PlatformApplication.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"

#include <imgui.h>

IMPLEMENT_ENGINE_MODULE(FModuleInterface, Application);

static uint32 GetMouseButtonIndex(EMouseButton Button)
{
    switch (Button)
    {
        case MouseButton_Left:    return 0;
        case MouseButton_Right:   return 1;
        case MouseButton_Middle:  return 2;
        case MouseButton_Back:    return 3;
        case MouseButton_Forward: return 4;
        default:                  return static_cast<uint32>(-1);
    }
}


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
        int32              Index;
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
        int32              Index;
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
    static FResponse Dispatch(FWindowedApplication* Application, PolicyType Policy, const EventType& Event, PedicateType&& Predicate)
    {
        FResponse Response = FResponse::Unhandled();
        for (; !Response.IsEventHandled() && Policy.ShouldProcess(); Policy.Next())
        {
            Response = Predicate(Application, Policy.GetWidget(), Event);
        }
        return Response;
    }
};


TSharedPtr<FWindowedApplication> FWindowedApplication::CurrentApplication  = nullptr;
TSharedPtr<FGenericApplication>  FWindowedApplication::PlatformApplication = nullptr;

bool FWindowedApplication::Create()
{
    PlatformApplication = TSharedPtr(FPlatformApplicationMisc::CreateApplication());
    if (!PlatformApplication)
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to create FPlatformApplication");
        return false;
    }

    CurrentApplication = MakeShared<FWindowedApplication>();
    PlatformApplication->SetMessageHandler(CurrentApplication);
    
    FInputDevice* InputDevice = PlatformApplication->GetInputDeviceInterface();
    InputDevice->SetMessageHandler(CurrentApplication);
    return true;
}

void FWindowedApplication::Destroy()
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

FWindowedApplication::FWindowedApplication()
    : MainViewport(nullptr)
    , Renderer(nullptr)
    , Windows()
    , InputHandlers()
    , Context(nullptr)
{
    IMGUI_CHECKVERSION();
    Context = ImGui::CreateContext();
    CHECK(Context != nullptr);

    ImGuiIO& UIState = ImGui::GetIO();
    UIState.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    UIState.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
    UIState.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    // TODO: Not name it windows? 
    UIState.BackendPlatformName = "Windows";

    // Keyboard mapping. ImGui will use those indices to peek into the IO.KeysDown[] array that we will update during the application lifetime.
    UIState.KeyMap[ImGuiKey_Tab]         = EKey::Key_Tab;
    UIState.KeyMap[ImGuiKey_LeftArrow]   = EKey::Key_Left;
    UIState.KeyMap[ImGuiKey_RightArrow]  = EKey::Key_Right;
    UIState.KeyMap[ImGuiKey_UpArrow]     = EKey::Key_Up;
    UIState.KeyMap[ImGuiKey_DownArrow]   = EKey::Key_Down;
    UIState.KeyMap[ImGuiKey_PageUp]      = EKey::Key_PageUp;
    UIState.KeyMap[ImGuiKey_PageDown]    = EKey::Key_PageDown;
    UIState.KeyMap[ImGuiKey_Home]        = EKey::Key_Home;
    UIState.KeyMap[ImGuiKey_End]         = EKey::Key_End;
    UIState.KeyMap[ImGuiKey_Insert]      = EKey::Key_Insert;
    UIState.KeyMap[ImGuiKey_Delete]      = EKey::Key_Delete;
    UIState.KeyMap[ImGuiKey_Backspace]   = EKey::Key_Backspace;
    UIState.KeyMap[ImGuiKey_Space]       = EKey::Key_Space;
    UIState.KeyMap[ImGuiKey_Enter]       = EKey::Key_Enter;
    UIState.KeyMap[ImGuiKey_Escape]      = EKey::Key_Escape;
    UIState.KeyMap[ImGuiKey_KeyPadEnter] = EKey::Key_KeypadEnter;
    UIState.KeyMap[ImGuiKey_A]           = EKey::Key_A;
    UIState.KeyMap[ImGuiKey_C]           = EKey::Key_C;
    UIState.KeyMap[ImGuiKey_V]           = EKey::Key_V;
    UIState.KeyMap[ImGuiKey_X]           = EKey::Key_X;
    UIState.KeyMap[ImGuiKey_Y]           = EKey::Key_Y;
    UIState.KeyMap[ImGuiKey_Z]           = EKey::Key_Z;

    ImGuiStyle& Style = ImGui::GetStyle();
    ImGui::StyleColorsDark();

    // Padding
    Style.FramePadding = ImVec2(6.0f, 4.0f);

    // Use AA for lines etc.
    Style.AntiAliasedLines = true;
    Style.AntiAliasedFill  = true;

    // Size
    Style.WindowBorderSize = 0.0f;
    Style.FrameBorderSize  = 1.0f;
    Style.ChildBorderSize  = 1.0f;
    Style.PopupBorderSize  = 1.0f;
    Style.ScrollbarSize    = 10.0f;
    Style.GrabMinSize      = 20.0f;

    // Rounding
    Style.WindowRounding    = 4.0f;
    Style.FrameRounding     = 4.0f;
    Style.PopupRounding     = 4.0f;
    Style.GrabRounding      = 4.0f;
    Style.TabRounding       = 4.0f;
    Style.ScrollbarRounding = 6.0f;

    Style.Colors[ImGuiCol_WindowBg].x = 0.075f;
    Style.Colors[ImGuiCol_WindowBg].y = 0.075f;
    Style.Colors[ImGuiCol_WindowBg].z = 0.075f;
    Style.Colors[ImGuiCol_WindowBg].w = 0.925f;

    Style.Colors[ImGuiCol_Text].x = 0.95f;
    Style.Colors[ImGuiCol_Text].y = 0.95f;
    Style.Colors[ImGuiCol_Text].z = 0.95f;
    Style.Colors[ImGuiCol_Text].w = 1.0f;

    Style.Colors[ImGuiCol_PlotHistogram].x = 0.9f;
    Style.Colors[ImGuiCol_PlotHistogram].y = 0.9f;
    Style.Colors[ImGuiCol_PlotHistogram].z = 0.9f;
    Style.Colors[ImGuiCol_PlotHistogram].w = 1.0f;

    Style.Colors[ImGuiCol_PlotHistogramHovered].x = 0.75f;
    Style.Colors[ImGuiCol_PlotHistogramHovered].y = 0.75f;
    Style.Colors[ImGuiCol_PlotHistogramHovered].z = 0.75f;
    Style.Colors[ImGuiCol_PlotHistogramHovered].w = 1.0f;

    Style.Colors[ImGuiCol_TitleBg].x = 0.025f;
    Style.Colors[ImGuiCol_TitleBg].y = 0.025f;
    Style.Colors[ImGuiCol_TitleBg].z = 0.025f;
    Style.Colors[ImGuiCol_TitleBg].w = 1.0f;

    Style.Colors[ImGuiCol_TitleBgActive].x = 0.15f;
    Style.Colors[ImGuiCol_TitleBgActive].y = 0.15f;
    Style.Colors[ImGuiCol_TitleBgActive].z = 0.15f;
    Style.Colors[ImGuiCol_TitleBgActive].w = 1.0f;

    Style.Colors[ImGuiCol_FrameBg].x = 0.1f;
    Style.Colors[ImGuiCol_FrameBg].y = 0.1f;
    Style.Colors[ImGuiCol_FrameBg].z = 0.1f;
    Style.Colors[ImGuiCol_FrameBg].w = 1.0f;

    Style.Colors[ImGuiCol_FrameBgHovered].x = 0.2f;
    Style.Colors[ImGuiCol_FrameBgHovered].y = 0.2f;
    Style.Colors[ImGuiCol_FrameBgHovered].z = 0.2f;
    Style.Colors[ImGuiCol_FrameBgHovered].w = 1.0f;

    Style.Colors[ImGuiCol_FrameBgActive].x = 0.15f;
    Style.Colors[ImGuiCol_FrameBgActive].y = 0.15f;
    Style.Colors[ImGuiCol_FrameBgActive].z = 0.15f;
    Style.Colors[ImGuiCol_FrameBgActive].w = 1.0f;

    Style.Colors[ImGuiCol_Button].x = 0.4f;
    Style.Colors[ImGuiCol_Button].y = 0.4f;
    Style.Colors[ImGuiCol_Button].z = 0.4f;
    Style.Colors[ImGuiCol_Button].w = 1.0f;

    Style.Colors[ImGuiCol_ButtonHovered].x = 0.3f;
    Style.Colors[ImGuiCol_ButtonHovered].y = 0.3f;
    Style.Colors[ImGuiCol_ButtonHovered].z = 0.3f;
    Style.Colors[ImGuiCol_ButtonHovered].w = 1.0f;

    Style.Colors[ImGuiCol_ButtonActive].x = 0.25f;
    Style.Colors[ImGuiCol_ButtonActive].y = 0.25f;
    Style.Colors[ImGuiCol_ButtonActive].z = 0.25f;
    Style.Colors[ImGuiCol_ButtonActive].w = 1.0f;

    Style.Colors[ImGuiCol_CheckMark].x = 0.15f;
    Style.Colors[ImGuiCol_CheckMark].y = 0.15f;
    Style.Colors[ImGuiCol_CheckMark].z = 0.15f;
    Style.Colors[ImGuiCol_CheckMark].w = 1.0f;

    Style.Colors[ImGuiCol_SliderGrab].x = 0.15f;
    Style.Colors[ImGuiCol_SliderGrab].y = 0.15f;
    Style.Colors[ImGuiCol_SliderGrab].z = 0.15f;
    Style.Colors[ImGuiCol_SliderGrab].w = 1.0f;

    Style.Colors[ImGuiCol_SliderGrabActive].x = 0.16f;
    Style.Colors[ImGuiCol_SliderGrabActive].y = 0.16f;
    Style.Colors[ImGuiCol_SliderGrabActive].z = 0.16f;
    Style.Colors[ImGuiCol_SliderGrabActive].w = 1.0f;

    Style.Colors[ImGuiCol_ResizeGrip].x = 0.25f;
    Style.Colors[ImGuiCol_ResizeGrip].y = 0.25f;
    Style.Colors[ImGuiCol_ResizeGrip].z = 0.25f;
    Style.Colors[ImGuiCol_ResizeGrip].w = 1.0f;

    Style.Colors[ImGuiCol_ResizeGripHovered].x = 0.35f;
    Style.Colors[ImGuiCol_ResizeGripHovered].y = 0.35f;
    Style.Colors[ImGuiCol_ResizeGripHovered].z = 0.35f;
    Style.Colors[ImGuiCol_ResizeGripHovered].w = 1.0f;

    Style.Colors[ImGuiCol_ResizeGripActive].x = 0.5f;
    Style.Colors[ImGuiCol_ResizeGripActive].y = 0.5f;
    Style.Colors[ImGuiCol_ResizeGripActive].z = 0.5f;
    Style.Colors[ImGuiCol_ResizeGripActive].w = 1.0f;

    Style.Colors[ImGuiCol_Tab].x = 0.55f;
    Style.Colors[ImGuiCol_Tab].y = 0.55f;
    Style.Colors[ImGuiCol_Tab].z = 0.55f;
    Style.Colors[ImGuiCol_Tab].w = 1.0f;

    Style.Colors[ImGuiCol_TabHovered].x = 0.4f;
    Style.Colors[ImGuiCol_TabHovered].y = 0.4f;
    Style.Colors[ImGuiCol_TabHovered].z = 0.4f;
    Style.Colors[ImGuiCol_TabHovered].w = 1.0f;

    Style.Colors[ImGuiCol_TabActive].x = 0.25f;
    Style.Colors[ImGuiCol_TabActive].y = 0.25f;
    Style.Colors[ImGuiCol_TabActive].z = 0.25f;
    Style.Colors[ImGuiCol_TabActive].w = 1.0f;
}

FWindowedApplication::~FWindowedApplication()
{
    if (Context)
    {
        ImGui::DestroyContext(Context);
        Context = nullptr;
    }
}

bool FWindowedApplication::InitializeRenderer()
{
    Renderer = MakeUnique<FViewportRenderer>();
    if (!Renderer->Initialize())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to init ViewportRenderer ");
        return false;
    }

    return true; 
}

void FWindowedApplication::ReleaseRenderer()
{
    Renderer.Reset();
}

void FWindowedApplication::Tick(FTimespan DeltaTime)
{
    ImGuiIO& UIState = ImGui::GetIO();
    UIState.DeltaTime = static_cast<float>(DeltaTime.AsSeconds());
    
    if (UIState.WantSetMousePos)
    {
        SetCursorPos(FIntVector2{ static_cast<int32>(UIState.MousePos.x), static_cast<int32>(UIState.MousePos.y) });
    }

    TWeakPtr<FWindow> Window;// = MainViewport ? MainViewport->GetParentWindow() : nullptr;
    if (Window)
    {
        UIState.DisplaySize = ImVec2{ float(Window->GetWidth()), float(Window->GetHeight()) };

        TSharedRef<FGenericWindow> NativeWindow = Window->GetNativeWindow();
        if (NativeWindow)
        {
            const FMonitorDesc MonitorDesc  = PlatformApplication->GetMonitorDescFromWindow(NativeWindow);
            UIState.DisplayFramebufferScale = ImVec2{ MonitorDesc.DisplayScaling, MonitorDesc.DisplayScaling };
            UIState.FontGlobalScale         = MonitorDesc.DisplayScaling;
        }
    }

    const FIntVector2 Position = GetCursorPos();
    UIState.MousePos = ImVec2(static_cast<float>(Position.x), static_cast<float>(Position.y));

    const FModifierKeyState KeyState = FPlatformApplicationMisc::GetModifierKeyState();
    UIState.KeyCtrl  = KeyState.bIsCtrlDown;
    UIState.KeyShift = KeyState.bIsShiftDown;
    UIState.KeyAlt   = KeyState.bIsAltDown;
    UIState.KeySuper = KeyState.bIsSuperKeyDown;

    if (!(UIState.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange))
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

    // Update all the UI windows
    if (Renderer)
    {
        Renderer->BeginFrame();

        //Windows.Foreach([](TSharedRef<FWidget>& Window)
        //{
        //    Window->Tick();
        //});

        Renderer->EndFrame();
    }

    // Update platform
    const float Delta = static_cast<float>(DeltaTime.AsMilliseconds());
    PlatformApplication->Tick(Delta);

    // Poll input devices
    PollInputDevices();
}

void FWindowedApplication::PollInputDevices()
{
    PlatformApplication->PollInputDevices();
}

bool FWindowedApplication::OnControllerButtonUp(EControllerButton Button, uint32 ControllerIndex)
{
    // Create the event
    const FControllerEvent ControllerEvent(Button, ControllerIndex);

    // Let the InputHandlers handle the event first
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const FPriorityInputHandler& Handler = InputHandlers[Index];
        if (Handler.InputHandler->OnControllerButtonUpEvent(ControllerEvent))
        {
            return true;
        }
    }

    DISABLE_UNREFERENCED_VARIABLE_WARNING

    // Dispatch the events to the widgets in-focus
    FResponse Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(FocusPath), ControllerEvent,
        [](FWindowedApplication* Application, const TSharedPtr<FWidget>& Widget, const FControllerEvent& ControllerEvent)
        {
            return Widget->OnControllerButtonUp(ControllerEvent);
        });

    ENABLE_UNREFERENCED_VARIABLE_WARNING
    return Response.IsEventHandled();
}

bool FWindowedApplication::OnControllerButtonDown(EControllerButton Button, uint32 ControllerIndex)
{
    // Create the event
    const FControllerEvent ControllerEvent(Button, ControllerIndex);

    // Let the InputHandlers handle the event first
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const FPriorityInputHandler& Handler = InputHandlers[Index];
        if (Handler.InputHandler->OnControllerButtonDownEvent(ControllerEvent))
        {
            return true;
        }
    }

    DISABLE_UNREFERENCED_VARIABLE_WARNING

    // Dispatch the events to the widgets in-focus
    FResponse Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(FocusPath), ControllerEvent,
        [](FWindowedApplication* Application, const TSharedPtr<FWidget>& Widget, const FControllerEvent& ControllerEvent)
        {
            return Widget->OnControllerButtonDown(ControllerEvent);
        });

    ENABLE_UNREFERENCED_VARIABLE_WARNING
    return Response.IsEventHandled();
}

bool FWindowedApplication::OnControllerAnalog(EControllerAnalog AnalogSource, uint32 ControllerIndex, float AnalogValue)
{
    // Create the event
    const FControllerEvent ControllerEvent(AnalogSource, AnalogValue, ControllerIndex);

    // Let the InputHandlers handle the event first
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const FPriorityInputHandler& Handler = InputHandlers[Index];
        if (Handler.InputHandler->OnControllerAnalogEvent(ControllerEvent))
        {
            return true;
        }
    }

    DISABLE_UNREFERENCED_VARIABLE_WARNING

    // Dispatch the events to the widgets in-focus
    FResponse Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(FocusPath), ControllerEvent,
        [](FWindowedApplication* Application, const TSharedPtr<FWidget>& Widget, const FControllerEvent& ControllerEvent)
        {
            return Widget->OnControllerButtonAnalog(ControllerEvent);
        });

    ENABLE_UNREFERENCED_VARIABLE_WARNING
    return Response.IsEventHandled();
}

bool FWindowedApplication::OnKeyUp(EKey KeyCode, FModifierKeyState ModierKeyState)
{
    FResponse Response = FResponse::Unhandled();

    // Create the event
    const FKeyEvent KeyEvent(ModierKeyState, KeyCode, false, false);

    // Let the InputHandlers handle the event first
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

    // Update the UI-State
    ImGuiIO& UIState = ImGui::GetIO();
    UIState.KeysDown[KeyEvent.GetKey()] = KeyEvent.IsDown();

    if (UIState.WantCaptureKeyboard)
    {
        Response = FResponse::Handled();
    }

    // If the event is handled, abort the process
    if (Response.IsEventHandled())
    {
        return true;
    }

    DISABLE_UNREFERENCED_VARIABLE_WARNING

    // Dispatch the events to the widgets in-focus
    Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(FocusPath), KeyEvent, 
        [](FWindowedApplication* Application, const TSharedPtr<FWidget>& Widget, const FKeyEvent& KeyEvent)
        {
            return Widget->OnKeyUp(KeyEvent);
        });

    ENABLE_UNREFERENCED_VARIABLE_WARNING
    return Response.IsEventHandled();
}

bool FWindowedApplication::OnKeyDown(EKey KeyCode, bool bIsRepeat, FModifierKeyState ModierKeyState)
{
    FResponse Response = FResponse::Unhandled();

    // Create the event
    const FKeyEvent KeyEvent(ModierKeyState, KeyCode, bIsRepeat, true);
    
    // Let the InputHandlers handle the event first
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

    // Update the UI-State
    ImGuiIO& UIState = ImGui::GetIO();
    UIState.KeysDown[KeyEvent.GetKey()] = KeyEvent.IsDown();

    if (UIState.WantCaptureKeyboard)
    {
        Response = FResponse::Handled();
    }

    // If the event is handled, abort the process
    if (Response.IsEventHandled())
    {
        return true;
    }

    DISABLE_UNREFERENCED_VARIABLE_WARNING

    // Dispatch the events to the widgets in-focus
    Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(FocusPath), KeyEvent, 
        [](FWindowedApplication* Application, const TSharedPtr<FWidget>& Widget, const FKeyEvent& KeyEvent)
        {
            return Widget->OnKeyDown(KeyEvent);
        });

    ENABLE_UNREFERENCED_VARIABLE_WARNING
    return Response.IsEventHandled();
}

bool FWindowedApplication::OnKeyChar(uint32 Character)
{
    FResponse Response = FResponse::Unhandled();

    // Create the event
    const FKeyEvent KeyEvent(FPlatformApplicationMisc::GetModifierKeyState(), Key_Unknown, Character, false, true);
    
    // Let the InputHandlers handle the event first
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const FPriorityInputHandler& Handler = InputHandlers[Index];
        if (Handler.InputHandler->OnKeyCharEvent(KeyEvent))
        {
            Response = FResponse::Handled();
            break;
        }
    }

    // Update the UI-State
    ImGuiIO& UIState = ImGui::GetIO();
    UIState.AddInputCharacter(KeyEvent.GetAnsiChar());

    // If the event is handled, abort the process
    if (Response.IsEventHandled())
    {
        return true;
    }

    DISABLE_UNREFERENCED_VARIABLE_WARNING

    // Dispatch the events to the widgets in-focus
    Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(FocusPath), KeyEvent, 
        [](FWindowedApplication* Application, const TSharedPtr<FWidget>& Widget, const FKeyEvent& KeyEvent)
        {
            return Widget->OnKeyChar(KeyEvent);
        });

    ENABLE_UNREFERENCED_VARIABLE_WARNING
    return Response.IsEventHandled();
}

bool FWindowedApplication::OnMouseMove(int32 x, int32 y)
{
    // Create the event
    const FMouseEvent MouseEvent(FIntVector2{ x, y }, FPlatformApplicationMisc::GetModifierKeyState());
    
    // Let the InputHandlers handle the event first
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const FPriorityInputHandler& Handler = InputHandlers[Index];
        if (Handler.InputHandler->OnMouseMove(MouseEvent))
        {
            return true;
        }
    }

    // Find the Widgets under the cursor
    FFilteredWidgets Children;
    FindWidgetsUnderCursor(MouseEvent.GetCursorPos(), Children);

    // Assume that we are dragging since a MouseButton is pressed
    const bool bIsNotDragging = PressedMouseButtons.empty();

    // If the Widgets is not in the newly pressed widgets, but is tracked then remove it
    for (int32 Index = 0; Index < TrackedWidgets.Size();)
    {
        const TSharedPtr<FWidget>& Widget = TrackedWidgets[Index];
        if (!Children.Contains(Widget) && bIsNotDragging)
        {
            Widget->OnMouseLeft(MouseEvent);
            TrackedWidgets.RemoveAt(Index);
        }
        else
        {
            ++Index;
        }
    }

    // If the tracked widgets contain the widget, send the events that the mouse entered the widgets
    FResponse Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(Children), MouseEvent,
        [](FWindowedApplication* Application, const TSharedPtr<FWidget>& Widget, const FMouseEvent& MouseEvent)
        {
            if (!Application->TrackedWidgets.Contains(Widget))
            {
                Application->TrackedWidgets.Add(Widget);
                Widget->OnMouseEntered(MouseEvent);
            }

            return FResponse::Unhandled();
        });

    DISABLE_UNREFERENCED_VARIABLE_WARNING

    // Dispatch the MouseEvent to the widgets under the cursor
    Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(Children), MouseEvent,
        [](FWindowedApplication* Application, const TSharedPtr<FWidget>& Widget, const FMouseEvent& KeyEvent)
        {
            return Widget->OnMouseMove(KeyEvent);
        });

    ENABLE_UNREFERENCED_VARIABLE_WARNING
    return Response.IsEventHandled();
}

bool FWindowedApplication::OnMouseButtonUp(EMouseButton Button, FModifierKeyState ModiferKeyState, int32 x, int32 y)
{
    // Remove the mouse capture if there is a capture
    PlatformApplication->SetCapture(nullptr);

    // Create the event
    const FMouseEvent MouseEvent(FIntVector2{ x, y }, ModiferKeyState, Button);
    
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

    // Update the UI-State
    ImGuiIO& UIState = ImGui::GetIO();

    const uint32 ButtonIndex = GetMouseButtonIndex(MouseEvent.GetButton());
    UIState.MouseDown[ButtonIndex] = MouseEvent.IsDown();

    if (UIState.WantCaptureMouse)
    {
        Response = FResponse::Handled();
    }

    // Remove the Key
    PressedMouseButtons.erase(Button);

    // If the event is handled, abort the process
    if (Response.IsEventHandled())
    {
        return true;
    }

    // Find the Widgets under the cursor
    FFilteredWidgets Children;
    FindWidgetsUnderCursor(MouseEvent.GetCursorPos(), Children);

    // If the Widgets is not in the newly pressed widgets, but is tracked then remove it
    for (int32 Index = 0; Index < TrackedWidgets.Size();)
    {
        const TSharedPtr<FWidget>& Widget = TrackedWidgets[Index];
        if (!Children.Contains(Widget))
        {
            Widget->OnMouseLeft(MouseEvent);
            TrackedWidgets.RemoveAt(Index);
        }
        else
        {
            ++Index;
        }
    }

    DISABLE_UNREFERENCED_VARIABLE_WARNING

    // Dispatch the MouseEvent to the widgets under the cursor
    Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(Children), MouseEvent,
        [](FWindowedApplication* Application, const TSharedPtr<FWidget>& Widget, const FMouseEvent& MouseEvent)
        {
            return Widget->OnMouseButtonUp(MouseEvent);
        });

    ENABLE_UNREFERENCED_VARIABLE_WARNING
    return Response.IsEventHandled();
}

bool FWindowedApplication::OnMouseButtonDown(const TSharedRef<FGenericWindow>& Window, EMouseButton Button, FModifierKeyState ModierKeyState, int32 x, int32 y)
{
    // Set the mouse capture when the mouse is pressed
    PlatformApplication->SetCapture(Window);

    // Create the event
    const FMouseEvent MouseEvent(FIntVector2{ x, y }, ModierKeyState, Button);

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

    // Update the UI-State
    ImGuiIO& UIState = ImGui::GetIO();

    const uint32 ButtonIndex = GetMouseButtonIndex(MouseEvent.GetButton());
    UIState.MouseDown[ButtonIndex] = MouseEvent.IsDown();

    if (UIState.WantCaptureMouse)
    {
        Response = FResponse::Handled();
    }

    // Remove the Key
    PressedMouseButtons.insert(Button);

    // If the event is handled, abort the process
    if (Response.IsEventHandled())
    {
        return true;
    }

    // Find the Widgets under the cursor
    FFilteredWidgets Children;
    FindWidgetsUnderCursor(MouseEvent.GetCursorPos(), Children);

    // Dispatch the MouseEvent to the widgets under the cursor
    Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(Children), MouseEvent,
        [](FWindowedApplication* Application, const TSharedPtr<FWidget>& Widget, const FMouseEvent& MouseEvent)
        {
            const FResponse Response = Widget->OnMouseButtonDown(MouseEvent);
            if (Response.IsEventHandled())
            {
                if (!Application->TrackedWidgets.Contains(Widget))
                {
                    Application->TrackedWidgets.Add(Widget);
                }
            }

            return Response;
        });

    FocusPath = Children;
    return Response.IsEventHandled();
}

bool FWindowedApplication::OnMouseScrolled(float WheelDelta, bool bVertical, int32 x, int32 y)
{
    // Create the event
    const FMouseEvent MouseEvent(FIntVector2{ x, y }, FPlatformApplicationMisc::GetModifierKeyState(), WheelDelta, bVertical);

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

    // Update the UI-State
    ImGuiIO& UIState = ImGui::GetIO();
    if (MouseEvent.IsVerticalScrollDelta())
    {
        UIState.MouseWheel += MouseEvent.GetScrollDelta();
    }
    else
    {
        UIState.MouseWheelH += MouseEvent.GetScrollDelta();
    }

    if (Response.IsEventHandled())
    {
        return true;
    }

    // Find the Widgets under the cursor
    FFilteredWidgets Children;
    FindWidgetsUnderCursor(MouseEvent.GetCursorPos(), Children);

    DISABLE_UNREFERENCED_VARIABLE_WARNING

    // Dispatch the MouseEvent to the widgets under the cursor
    Response = FEventDispatcher::Dispatch(this, FEventDispatcher::FLeafFirstPolicy(Children), MouseEvent,
        [](FWindowedApplication* Application, const TSharedPtr<FWidget>& Widget, const FMouseEvent& MouseEvent)
        {
            // TODO: return Widget->OnMouseScrolled(MouseEvent);
            return FResponse::Unhandled();
        });

    ENABLE_UNREFERENCED_VARIABLE_WARNING
    return Response.IsEventHandled();
}

bool FWindowedApplication::OnWindowResized(const TSharedRef<FGenericWindow>& InWindow, uint32 Width, uint32 Height)
{
    TSharedPtr<FWindow> Window = FindWindowFromNativeWindow(InWindow);
    if (Window)
    {
        const FWindowResizedEvent WindowResizeEvent(Width, Height);
        const FResponse Response = Window->OnWindowResized(WindowResizeEvent);
        if (Response.IsEventHandled())
        {
            return true;
        }
    }

    return false;
}

bool FWindowedApplication::OnWindowMoved(const TSharedRef<FGenericWindow>& InWindow, int32 x, int32 y)
{
    TSharedPtr<FWindow> Window = FindWindowFromNativeWindow(InWindow);
    if (Window)
    {
        const FWindowMovedEvent WindowsMovedEvent(FIntVector2{ x, y });
        const FResponse Response = Window->OnWindowMoved(WindowsMovedEvent);
        if (Response.IsEventHandled())
        {
            return true;
        }
    }

    return false;
}

DISABLE_UNREFERENCED_VARIABLE_WARNING

bool FWindowedApplication::OnWindowFocusLost(const TSharedRef<FGenericWindow>& InWindow)
{
    // The state needs to be reset when the window loses focus
    ImGuiIO& UIState = ImGui::GetIO();
    FMemory::Memzero(UIState.KeysDown, sizeof(UIState.KeysDown));
    return true;
}

bool FWindowedApplication::OnWindowFocusGained(const TSharedRef<FGenericWindow>& InWindow)
{
    return false;
}

bool FWindowedApplication::OnWindowMouseLeft(const TSharedRef<FGenericWindow>& InWindow)
{
    //TSharedPtr<FWindow> Window = FindWindowFromNativeWindow(InWindow);
    //if (Window)
    //{
    //    Window->OnMouseLeft();
    //}
    
    return false;
}

bool FWindowedApplication::OnWindowMouseEntered(const TSharedRef<FGenericWindow>& InWindow)
{
    //TSharedPtr<FWindow> Window = FindWindowFromNativeWindow(InWindow);
    //if (Window)
    //{
    //    Window->OnMouseEntered();
    //}

    return false;
}

bool FWindowedApplication::OnWindowClosed(const TSharedRef<FGenericWindow>& InWindow)
{
    TSharedPtr<FWindow> Window = FindWindowFromNativeWindow(InWindow);
    if (Window)
    {
        Window->OnWindowClosed();

        TSharedPtr<FViewportWidget> Viewport;// = Window->GetViewport();
        if (Viewport == MainViewport)
        {
            FPlatformApplicationMisc::RequestExit(0);
        }
    }

    return true;
}

ENABLE_UNREFERENCED_VARIABLE_WARNING

void FWindowedApplication::SetCursor(ECursor InCursor)
{
    TSharedPtr<ICursor> Cursor = GetCursor();
    Cursor->SetCursor(InCursor);
}

void FWindowedApplication::SetCursorPos(const FIntVector2& Position)
{
    TSharedPtr<ICursor> Cursor = GetCursor();
    Cursor->SetPosition(Position.x, Position.y);
}

FIntVector2 FWindowedApplication::GetCursorPos() const
{
    TSharedPtr<ICursor> Cursor = GetCursor();
    return Cursor->GetPosition();
}

void FWindowedApplication::ShowCursor(bool bIsVisible)
{
    TSharedPtr<ICursor> Cursor = GetCursor();
    Cursor->SetVisibility(bIsVisible);
}

bool FWindowedApplication::IsCursorVisibile() const
{
    TSharedPtr<ICursor> Cursor = GetCursor();
    return Cursor->IsVisible();
}

bool FWindowedApplication::EnableHighPrecisionMouseForWindow(const TSharedPtr<FWindow>& Window) 
{ 
    if (Window)
    {
        TSharedRef<FGenericWindow> NativeWindow = Window->GetNativeWindow();
        return PlatformApplication->EnableHighPrecisionMouseForWindow(NativeWindow);
    }

    return false;
}

void FWindowedApplication::SetCapture(const TSharedPtr<FWindow>& CaptureWindow)
{
    if (CaptureWindow)
    {
        TSharedRef<FGenericWindow> NativeWindow = CaptureWindow->GetNativeWindow();
        PlatformApplication->SetCapture(NativeWindow);
    }
}

void FWindowedApplication::SetActiveWindow(const TSharedPtr<FWindow>& ActiveWindow)
{
    if (ActiveWindow)
    {
        TSharedRef<FGenericWindow> NativeWindow = ActiveWindow->GetNativeWindow();
        PlatformApplication->SetActiveWindow(NativeWindow);
    }
}

TSharedPtr<FWindow> FWindowedApplication::GetActiveWindow() const 
{ 
    TSharedRef<FGenericWindow> NativeWindow = PlatformApplication->GetActiveWindow();
    return FindWindowFromNativeWindow(NativeWindow);
}

TSharedPtr<FWindow> FWindowedApplication::GetWindowUnderCursor() const
{ 
    TSharedRef<FGenericWindow> NativeWindow = PlatformApplication->GetActiveWindow();
    return FindWindowFromNativeWindow(NativeWindow);
}

TSharedPtr<FWindow> FWindowedApplication::GetCapture() const
{ 
    TSharedRef<FGenericWindow> NativeWindow = PlatformApplication->GetCapture();
    return FindWindowFromNativeWindow(NativeWindow);
}

void FWindowedApplication::AddInputHandler(const TSharedPtr<FInputHandler>& NewInputHandler, uint32 NewPriority)
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

void FWindowedApplication::RemoveInputHandler(const TSharedPtr<FInputHandler>& InputHandler)
{
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const FPriorityInputHandler Handler = InputHandlers[Index];
        if (Handler.InputHandler == InputHandler)
        {
            InputHandlers.RemoveAt(Index);
            break;
        }
    }
}

void FWindowedApplication::RegisterMainViewport(const TSharedPtr<FViewportWidget>& NewMainViewport)
{
    if (MainViewport != NewMainViewport)
    {
        MainViewport = NewMainViewport;

        // TODO: What to do with multiple Viewports
        ImGuiIO& InterfaceState = ImGui::GetIO();
        if (MainViewport)
        {
            TWeakPtr<FWindow> ParentWindow;// = MainViewport->GetParentWindow();
            if (ParentWindow)
            {
                TSharedRef<FGenericWindow> NativeWindow = ParentWindow->GetNativeWindow();
                InterfaceState.ImeWindowHandle = NativeWindow->GetPlatformHandle();
            }
        }
        else
        {
            InterfaceState.ImeWindowHandle = nullptr;
        }
    }
}

void FWindowedApplication::AddWindow(const TSharedPtr<FWindow>& NewWindow)
{
    if (NewWindow && !Windows.Contains(NewWindow))
    {
        Windows.Emplace(NewWindow);
    }
}

void FWindowedApplication::RemoveWindow(const TSharedPtr<FWindow>& Window)
{
    if (Window)
    {
        Windows.Remove(Window);
    }
}

void FWindowedApplication::DrawWindows(FRHICommandList& CommandList)
{
    // NOTE: Renderer is not forced to be valid
    if (Renderer)
    {
        Renderer->Render(CommandList);
    }
}

TSharedPtr<FWindow> FWindowedApplication::FindWindowFromNativeWindow(const TSharedRef<FGenericWindow>& NativeWindow) const
{
    if (NativeWindow)
    {
        for (const TSharedPtr<FWindow>& Window : Windows)
        {
            if (NativeWindow == Window->GetNativeWindow())
            {
                return Window;
            }
        }
    }

    return nullptr;
}

TSharedPtr<FWindow> FWindowedApplication::FindWindowUnderCursor()
{
    return FindWindowFromNativeWindow(PlatformApplication->GetWindowUnderCursor());
}

void FWindowedApplication::FindWidgetsUnderCursor(FIntVector2 CursorPos, FFilteredWidgets& OutWidgets)
{
    if (TSharedPtr<FWindow> Window = FindWindowUnderCursor())
    {
        Window->FindWidgetsUnderCursor(CursorPos, OutWidgets);
    }
}

void FWindowedApplication::OverridePlatformApplication(const TSharedPtr<FGenericApplication>& InPlatformApplication)
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
