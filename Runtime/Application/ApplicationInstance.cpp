#include "ApplicationInstance.h"

#include "CoreApplication/Platform/PlatformApplication.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"

#include "Core/Input/InputStates.h"

#include <imgui.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CApplicationInstance

TSharedPtr<CApplicationInstance> CApplicationInstance::Instance;

bool CApplicationInstance::CreateApplication()
{
    TSharedPtr<CGenericApplication> Application = MakeSharedPtr(PlatformApplicationMisc::CreateApplication());
    if (Application && !Application->Initialize())
    {
        PlatformApplicationMisc::MessageBox("ERROR", "Failed to create PlatformApplication");
        return false;
    }

    Instance = MakeSharedPtr(dbg_new CApplicationInstance(Application));
    if (!Instance->CreateContext())
    {
        PlatformApplicationMisc::MessageBox("ERROR", "Failed to create UI Context");
        return false;
    }

    Application->SetMessageListener(Instance);

    return true;
}

bool CApplicationInstance::CreateApplication(const TSharedPtr<CApplicationInstance>& InApplication)
{
    Instance = InApplication;
    return (Instance != nullptr);
}

void CApplicationInstance::Release()
{
    if (Instance)
    {
        Instance->SetPlatformApplication(nullptr);
        Instance.Reset();
    }
}

CApplicationInstance::CApplicationInstance(const TSharedPtr<CGenericApplication>& InPlatformApplication)
    : CGenericApplicationMessageHandler()
    , PlatformApplication(InPlatformApplication)
    , Renderer()
    , MainViewport()
    , InterfaceWindows()
    , DebugStrings()
    , InputHandlers()
    , WindowMessageHandlers()
    , RegisteredUsers()
    , bIsRunning(true)
    , Context(nullptr)
{ }

bool CApplicationInstance::CreateContext()
{
    IMGUI_CHECKVERSION();

    Context = ImGui::CreateContext();
    if (!Context)
    {
        return false;
    }

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

    ImGui::StyleColorsDark();

    ImGuiStyle& Style = ImGui::GetStyle();

    // Padding
    Style.FramePadding = ImVec2(6.0f, 4.0f);

    // Size
    Style.WindowBorderSize = 0.0f;
    Style.FrameBorderSize  = 1.0f;
    Style.ChildBorderSize  = 1.0f;
    Style.PopupBorderSize  = 1.0f;
    Style.ScrollbarSize    = 14.0f;
    Style.GrabMinSize      = 20.0f;

    // Rounding
    Style.WindowRounding    = 4.0f;
    Style.FrameRounding     = 4.0f;
    Style.PopupRounding     = 4.0f;
    Style.GrabRounding      = 4.0f;
    Style.TabRounding       = 4.0f;
    Style.ScrollbarRounding = 6.0f;

    Style.Colors[ImGuiCol_WindowBg].x = 0.2f;
    Style.Colors[ImGuiCol_WindowBg].y = 0.2f;
    Style.Colors[ImGuiCol_WindowBg].z = 0.2f;
    Style.Colors[ImGuiCol_WindowBg].w = 0.9f;

    Style.Colors[ImGuiCol_Text].x = 1.0f;
    Style.Colors[ImGuiCol_Text].y = 1.0f;
    Style.Colors[ImGuiCol_Text].z = 1.0f;
    Style.Colors[ImGuiCol_Text].w = 1.0f;

    Style.Colors[ImGuiCol_PlotHistogram].x = 0.9f;
    Style.Colors[ImGuiCol_PlotHistogram].y = 0.9f;
    Style.Colors[ImGuiCol_PlotHistogram].z = 0.9f;
    Style.Colors[ImGuiCol_PlotHistogram].w = 1.0f;

    Style.Colors[ImGuiCol_PlotHistogramHovered].x = 0.75f;
    Style.Colors[ImGuiCol_PlotHistogramHovered].y = 0.75f;
    Style.Colors[ImGuiCol_PlotHistogramHovered].z = 0.75f;
    Style.Colors[ImGuiCol_PlotHistogramHovered].w = 1.0f;

    Style.Colors[ImGuiCol_TitleBg].x = 0.3f;
    Style.Colors[ImGuiCol_TitleBg].y = 0.3f;
    Style.Colors[ImGuiCol_TitleBg].z = 0.3f;
    Style.Colors[ImGuiCol_TitleBg].w = 1.0f;

    Style.Colors[ImGuiCol_TitleBgActive].x = 0.15f;
    Style.Colors[ImGuiCol_TitleBgActive].y = 0.15f;
    Style.Colors[ImGuiCol_TitleBgActive].z = 0.15f;
    Style.Colors[ImGuiCol_TitleBgActive].w = 1.0f;

    Style.Colors[ImGuiCol_FrameBg].x = 0.4f;
    Style.Colors[ImGuiCol_FrameBg].y = 0.4f;
    Style.Colors[ImGuiCol_FrameBg].z = 0.4f;
    Style.Colors[ImGuiCol_FrameBg].w = 1.0f;

    Style.Colors[ImGuiCol_FrameBgHovered].x = 0.3f;
    Style.Colors[ImGuiCol_FrameBgHovered].y = 0.3f;
    Style.Colors[ImGuiCol_FrameBgHovered].z = 0.3f;
    Style.Colors[ImGuiCol_FrameBgHovered].w = 1.0f;

    Style.Colors[ImGuiCol_FrameBgActive].x = 0.24f;
    Style.Colors[ImGuiCol_FrameBgActive].y = 0.24f;
    Style.Colors[ImGuiCol_FrameBgActive].z = 0.24f;
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

    return true;
}

CApplicationInstance::~CApplicationInstance()
{
    if (Context)
    {
        ImGui::DestroyContext(Context);
    }
}

TSharedRef<CGenericWindow> CApplicationInstance::MakeWindow()
{
    return PlatformApplication->MakeWindow();
}

void CApplicationInstance::Tick(CTimestamp DeltaTime)
{
    // Update UI
    ImGuiIO& UIState = ImGui::GetIO();

    TSharedRef<CGenericWindow> Window = MainViewport;
    if (UIState.WantSetMousePos)
    {
        SetCursorPos(Window, CIntVector2(static_cast<int32>(UIState.MousePos.x), static_cast<int32>(UIState.MousePos.y)));
    }

    SWindowShape CurrentWindowShape;
    Window->GetWindowShape(CurrentWindowShape);

    UIState.DeltaTime               = static_cast<float>(DeltaTime.AsSeconds());
    UIState.DisplaySize             = ImVec2(float(CurrentWindowShape.Width), float(CurrentWindowShape.Height));
    UIState.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    CIntVector2 Position = CApplicationInstance::Get().GetCursorPos(Window);
    UIState.MousePos = ImVec2(static_cast<float>(Position.x), static_cast<float>(Position.y));

    SModifierKeyState KeyState = PlatformApplicationMisc::GetModifierKeyState();
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
        Renderer->BeginTick();

        InterfaceWindows.Foreach([](TSharedRef<IWindow>& Window)
        {
            if (Window->IsTickable())
            {
                Window->Tick();
            }
        });

        RenderStrings();

        Renderer->EndTick();
    }

    // Update platform
    const float Delta = static_cast<float>(DeltaTime.AsMilliSeconds());
    PlatformApplication->Tick(Delta);

    if (!RegisteredUsers.IsEmpty())
    {
        for (const TSharedPtr<CApplicationUser>& User : RegisteredUsers)
        {
            User->Tick(DeltaTime);
        }
    }
}

void CApplicationInstance::SetCursor(ECursor InCursor)
{
    TSharedPtr<ICursor> Cursor = GetCursor();
    Cursor->SetCursor(InCursor);
}

void CApplicationInstance::SetCursorPos(const CIntVector2& Position)
{
    TSharedPtr<ICursor> Cursor = GetCursor();
    Cursor->SetPosition(nullptr, Position.x, Position.y);
}

void CApplicationInstance::SetCursorPos(const TSharedRef<CGenericWindow>& RelativeWindow, const CIntVector2& Position)
{
    TSharedPtr<ICursor> Cursor = GetCursor();
    Cursor->SetPosition(RelativeWindow.Get(), Position.x, Position.y);
}

CIntVector2 CApplicationInstance::GetCursorPos() const
{
    TSharedPtr<ICursor> Cursor = GetCursor();

    CIntVector2 CursorPosition;
    Cursor->GetPosition(nullptr, CursorPosition.x, CursorPosition.y);

    return CursorPosition;
}

CIntVector2 CApplicationInstance::GetCursorPos(const TSharedRef<CGenericWindow>& RelativeWindow) const
{
    TSharedPtr<ICursor> Cursor = GetCursor();

    CIntVector2 CursorPosition;
    Cursor->GetPosition(RelativeWindow.Get(), CursorPosition.x, CursorPosition.y);

    return CursorPosition;
}

void CApplicationInstance::ShowCursor(bool bIsVisible)
{
    TSharedPtr<ICursor> Cursor = GetCursor();
    Cursor->SetVisibility(bIsVisible);
}

bool CApplicationInstance::IsCursorVisibile() const
{
    TSharedPtr<ICursor> Cursor = GetCursor();
    return Cursor->IsVisible();
}

void CApplicationInstance::SetCapture(const TSharedRef<CGenericWindow>& CaptureWindow)
{
    PlatformApplication->SetCapture(CaptureWindow);
}

void CApplicationInstance::SetActiveWindow(const TSharedRef<CGenericWindow>& ActiveWindow)
{
    PlatformApplication->SetActiveWindow(ActiveWindow);
}

template<typename MessageHandlerType>
void CApplicationInstance::InsertMessageHandler(TArray<TPair<TSharedPtr<MessageHandlerType>, uint32>>& OutMessageHandlerArray
                                               ,const TSharedPtr<MessageHandlerType>& NewMessageHandler
                                               ,uint32 NewPriority)
{
    TPair NewPair(NewMessageHandler, NewPriority);
    if (!OutMessageHandlerArray.Contains(NewPair))
    {
        for (int32 Index = 0; Index < OutMessageHandlerArray.Size(); )
        {
            const TPair<TSharedPtr<MessageHandlerType>, uint32> Handler = OutMessageHandlerArray[Index];
            if (NewPriority <= Handler.Second)
            {
                Index++;
            }
            else
            {
                OutMessageHandlerArray.Insert(Index, NewPair);
                return;
            }
        }

        OutMessageHandlerArray.Push(NewPair);
    }
}

void CApplicationInstance::AddInputHandler(const TSharedPtr<CInputHandler>& NewInputHandler, uint32 Priority)
{
    InsertMessageHandler(InputHandlers, NewInputHandler, Priority);
}

void CApplicationInstance::RemoveInputHandler(const TSharedPtr<CInputHandler>& InputHandler)
{
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const TPair<TSharedPtr<CInputHandler>, uint32> Handler = InputHandlers[Index];
        if (Handler.First == InputHandler)
        {
            InputHandlers.RemoveAt(Index);
            break;
        }
    }
}

void CApplicationInstance::RegisterMainViewport(const TSharedRef<CGenericWindow>& NewMainViewport)
{
    MainViewport = NewMainViewport;
    if (MainViewportChange.IsBound())
    {
        MainViewportChange.Broadcast(MainViewport);
    }

    ImGuiIO& InterfaceState = ImGui::GetIO();
    if (MainViewport)
    {
        InterfaceState.ImeWindowHandle = MainViewport->GetPlatformHandle();
    }
    else
    {
        InterfaceState.ImeWindowHandle = nullptr;
    }
}

void CApplicationInstance::SetRenderer(const TSharedRef<IInterfaceRenderer>& NewRenderer)
{
    Renderer = NewRenderer;
    if (Renderer)
    {
        if (!Renderer->InitContext(Context))
        {
            PlatformApplicationMisc::MessageBox("ERROR", "Failed to init InterfaceRenderer ");
        }
    }
}

void CApplicationInstance::AddWindow(const TSharedRef<IWindow>& NewWindow)
{
    if (NewWindow && !InterfaceWindows.Contains(NewWindow))
    {
        TSharedRef<IWindow>& Window = InterfaceWindows.Emplace(NewWindow);
        Window->InitContext(Context);
    }
}

void CApplicationInstance::RemoveWindow(const TSharedRef<IWindow>& Window)
{
    InterfaceWindows.Remove(Window);
}

void CApplicationInstance::DrawString(const String& NewString)
{
    DebugStrings.Emplace(NewString);
}

void CApplicationInstance::DrawWindows(CRHICommandList& CommandList)
{
    // NOTE: Renderer is not forced to be valid 
    if (Renderer)
    {
        Renderer->Render(CommandList);
    }
}

void CApplicationInstance::AddWindowMessageHandler(const TSharedPtr<CWindowMessageHandler>& NewWindowMessageHandler, uint32 Priority)
{
    InsertMessageHandler(WindowMessageHandlers, NewWindowMessageHandler, Priority);
}

void CApplicationInstance::RemoveWindowMessageHandler(const TSharedPtr<CWindowMessageHandler>& WindowMessageHandler)
{
    for (int32 Index = 0; Index < WindowMessageHandlers.Size(); Index++)
    {
        const TPair<TSharedPtr<CWindowMessageHandler>, uint32> Handler = WindowMessageHandlers[Index];
        if (Handler.First == WindowMessageHandler)
        {
            WindowMessageHandlers.RemoveAt(Index);
            break;
        }
    }
}

void CApplicationInstance::SetPlatformApplication(const TSharedPtr<CGenericApplication>& InPlatformApplication)
{
    if (InPlatformApplication)
    {
        Check(this == Instance);
        InPlatformApplication->SetMessageListener(Instance);
    }

    PlatformApplication = InPlatformApplication;
}

void CApplicationInstance::HandleKeyReleased(EKey KeyCode, SModifierKeyState ModierKeyState)
{
    SKeyEvent KeyEvent(KeyCode, false, false, ModierKeyState);
    HandleKeyEvent(KeyEvent);
}

void CApplicationInstance::HandleKeyPressed(EKey KeyCode, bool bIsRepeat, SModifierKeyState ModierKeyState)
{
    SKeyEvent KeyEvent(KeyCode, true, bIsRepeat, ModierKeyState);
    HandleKeyEvent(KeyEvent);
}

void CApplicationInstance::HandleKeyEvent(const SKeyEvent& KeyEvent)
{
    SKeyEvent Event = KeyEvent;
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const TPair<TSharedPtr<CInputHandler>, uint32>& Handler = InputHandlers[Index];
        if (Handler.First->HandleKeyEvent(Event))
        {
            Event.bIsConsumed = true;
        }
    }

    ImGuiIO& UIState = ImGui::GetIO();
    UIState.KeysDown[Event.KeyCode] = Event.bIsDown;

    if (UIState.WantCaptureKeyboard)
    {
        Event.bIsConsumed = true;
    }

    if (!Event.bIsConsumed && !RegisteredUsers.IsEmpty())
    {
        for (const TSharedPtr<CApplicationUser>& User : RegisteredUsers)
        {
            User->HandleKeyEvent(Event);
        }
    }

    // TODO: Update viewport
}

void CApplicationInstance::HandleKeyChar(uint32 Character)
{
    SKeyCharEvent Event(Character);
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const TPair<TSharedPtr<CInputHandler>, uint32>& Handler = InputHandlers[Index];
        if (Handler.First->HandleKeyTyped(Event))
        {
            Event.bIsConsumed = true;
        }
    }

    ImGuiIO& UIState = ImGui::GetIO();
    UIState.AddInputCharacter(Event.Character);
}

void CApplicationInstance::HandleMouseMove(int32 x, int32 y)
{
    SMouseMovedEvent MouseMovedEvent(x, y);
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const TPair<TSharedPtr<CInputHandler>, uint32>& Handler = InputHandlers[Index];
        if (Handler.First->HandleMouseMove(MouseMovedEvent))
        {
            MouseMovedEvent.bIsConsumed = true;
        }
    }

    if (!MouseMovedEvent.bIsConsumed && !RegisteredUsers.IsEmpty())
    {
        for (const TSharedPtr<CApplicationUser>& User : RegisteredUsers)
        {
            User->HandleMouseMovedEvent(MouseMovedEvent);
        }
    }
}

void CApplicationInstance::HandleMouseReleased(EMouseButton Button, SModifierKeyState ModierKeyState)
{
    TSharedRef<CGenericWindow> CaptureWindow = PlatformApplication->GetCapture();
    if (CaptureWindow)
    {
        PlatformApplication->SetCapture(nullptr);
    }

    SMouseButtonEvent MouseButtonEvent(Button, false, ModierKeyState);
    HandleMouseButtonEvent(MouseButtonEvent);
}

void CApplicationInstance::HandleMousePressed(EMouseButton Button, SModifierKeyState ModierKeyState)
{
    TSharedRef<CGenericWindow> CaptureWindow = PlatformApplication->GetCapture();
    if (!CaptureWindow)
    {
        TSharedRef<CGenericWindow> ActiveWindow = PlatformApplication->GetActiveWindow();
        PlatformApplication->SetCapture(ActiveWindow);
    }

    SMouseButtonEvent MouseButtonEvent(Button, true, ModierKeyState);
    HandleMouseButtonEvent(MouseButtonEvent);
}

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

void CApplicationInstance::HandleMouseButtonEvent(const SMouseButtonEvent& MouseButtonEvent)
{
    SMouseButtonEvent Event = MouseButtonEvent;
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const TPair<TSharedPtr<CInputHandler>, uint32>& Handler = InputHandlers[Index];
        if (Handler.First->HandleMouseButtonEvent(Event))
        {
            Event.bIsConsumed = true;
        }
    }

    ImGuiIO& UIState = ImGui::GetIO();

    const uint32 ButtonIndex = GetMouseButtonIndex(Event.Button);
    UIState.MouseDown[ButtonIndex] = Event.bIsDown;

    if (UIState.WantCaptureMouse)
    {
        Event.bIsConsumed = true;
    }

    if (!Event.bIsConsumed && !RegisteredUsers.IsEmpty())
    {
        for (const TSharedPtr<CApplicationUser>& User : RegisteredUsers)
        {
            User->HandleMouseButtonEvent(Event);
        }
    }
}

void CApplicationInstance::HandleMouseScrolled(float HorizontalDelta, float VerticalDelta)
{
    SMouseScrolledEvent Event(HorizontalDelta, VerticalDelta);
    for (int32 Index = 0; Index < InputHandlers.Size(); Index++)
    {
        const TPair<TSharedPtr<CInputHandler>, uint32>& Handler = InputHandlers[Index];
        if (Handler.First->HandleMouseScrolled(Event))
        {
            Event.bIsConsumed = true;
        }
    }

    ImGuiIO& UIState = ImGui::GetIO();
    UIState.MouseWheel += Event.VerticalDelta;
    UIState.MouseWheelH += Event.HorizontalDelta;

    if (UIState.WantCaptureMouse)
    {
        Event.bIsConsumed = true;
    }

    if (!Event.bIsConsumed && !RegisteredUsers.IsEmpty())
    {
        for (const TSharedPtr<CApplicationUser>& User : RegisteredUsers)
        {
            User->HandleMouseScrolledEvent(Event);
        }
    }
}

void CApplicationInstance::HandleWindowResized(const TSharedRef<CGenericWindow>& Window, uint32 Width, uint32 Height)
{
    SWindowResizeEvent WindowResizeEvent(Window, Width, Height);
    for (int32 Index = 0; Index < WindowMessageHandlers.Size(); Index++)
    {
        const TPair<TSharedPtr<CWindowMessageHandler>, uint32>& Handler = WindowMessageHandlers[Index];
        if (Handler.First->OnWindowResized(WindowResizeEvent))
        {
            WindowResizeEvent.bIsConsumed = true;
        }
    }
}

void CApplicationInstance::HandleWindowMoved(const TSharedRef<CGenericWindow>& Window, int32 x, int32 y)
{
    SWindowMovedEvent WindowsMovedEvent(Window, x, y);
    for (int32 Index = 0; Index < WindowMessageHandlers.Size(); Index++)
    {
        const TPair<TSharedPtr<CWindowMessageHandler>, uint32>& Handler = WindowMessageHandlers[Index];
        if (Handler.First->OnWindowMoved(WindowsMovedEvent))
        {
            WindowsMovedEvent.bIsConsumed = true;
        }
    }
}

void CApplicationInstance::HandleWindowFocusChanged(const TSharedRef<CGenericWindow>& Window, bool bHasFocus)
{
    SWindowFocusChangedEvent WindowFocusChangedEvent(Window, bHasFocus);
    for (int32 Index = 0; Index < WindowMessageHandlers.Size(); Index++)
    {
        const TPair<TSharedPtr<CWindowMessageHandler>, uint32>& Handler = WindowMessageHandlers[Index];
        if (Handler.First->OnWindowFocusChanged(WindowFocusChangedEvent))
        {
            WindowFocusChangedEvent.bIsConsumed = true;
        }
    }
}

void CApplicationInstance::HandleWindowMouseLeft(const TSharedRef<CGenericWindow>& Window)
{
    SWindowFrameMouseEvent WindowFrameMouseEvent(Window, false);
    HandleWindowFrameMouseEvent(WindowFrameMouseEvent);
}

void CApplicationInstance::HandleWindowMouseEntered(const TSharedRef<CGenericWindow>& Window)
{
    SWindowFrameMouseEvent WindowFrameMouseEvent(Window, true);
    HandleWindowFrameMouseEvent(WindowFrameMouseEvent);
}

void CApplicationInstance::HandleWindowFrameMouseEvent(const SWindowFrameMouseEvent& WindowFrameMouseEvent)
{
    SWindowFrameMouseEvent Event = WindowFrameMouseEvent;
    for (int32 Index = 0; Index < WindowMessageHandlers.Size(); Index++)
    {
        const TPair<TSharedPtr<CWindowMessageHandler>, uint32>& Handler = WindowMessageHandlers[Index];
        if (Handler.First->OnWindowFrameMouseEvent(Event))
        {
            Event.bIsConsumed = true;
        }
    }
}

void CApplicationInstance::RenderStrings()
{
    if (MainViewport && !DebugStrings.IsEmpty())
    {
        SWindowShape CurrentWindowShape;
        MainViewport->GetWindowShape(CurrentWindowShape);

        constexpr float Width = 400.0f;
        ImGui::SetNextWindowPos(ImVec2(static_cast<float>(CurrentWindowShape.Width - Width), 18.0f));
        ImGui::SetNextWindowSize(ImVec2(Width, 0.0f));

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.3f, 0.3f, 0.3f, 0.6f));

        const ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings;

        ImGui::Begin("DebugWindow", nullptr, WindowFlags);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

        for (const String& String : DebugStrings)
        {
            ImGui::Text("%s", String.CStr());
        }

        DebugStrings.Clear();

        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::End();
    }
}

void CApplicationInstance::HandleWindowClosed(const TSharedRef<CGenericWindow>& Window)
{
    SWindowClosedEvent WindowClosedEvent(Window);
    for (int32 Index = 0; Index < WindowMessageHandlers.Size(); Index++)
    {
        const TPair<TSharedPtr<CWindowMessageHandler>, uint32>& Handler = WindowMessageHandlers[Index];
        if (Handler.First->OnWindowClosed(WindowClosedEvent))
        {
            WindowClosedEvent.bIsConsumed = true;
        }
    }

    // TODO: Register a main viewport and when that closes, request exit for now just exit
    PlatformApplicationMisc::RequestExit(0);
}

void CApplicationInstance::HandleApplicationExit(int32 ExitCode)
{
    bIsRunning = false;

    ExitEvent.Broadcast(ExitCode);
}
