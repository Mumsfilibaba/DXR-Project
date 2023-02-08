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


TSharedPtr<FApplication>        FApplication::CurrentApplication  = nullptr;
TSharedPtr<FGenericApplication> FApplication::PlatformApplication = nullptr;

bool FApplication::Create()
{
    PlatformApplication = MakeSharedPtr(FPlatformApplicationMisc::CreateApplication());
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

FApplication::~FApplication()
{
    if (Context)
    {
        ImGui::DestroyContext(Context);
        Context = nullptr;
    }
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
    ImGuiIO& UIState = ImGui::GetIO();
    UIState.DeltaTime = static_cast<float>(DeltaTime.AsSeconds());
    
    if (UIState.WantSetMousePos)
    {
        SetCursorPos(FIntVector2{ static_cast<int32>(UIState.MousePos.x), static_cast<int32>(UIState.MousePos.y) });
    }

    TWeakPtr<FWindow> Window = MainViewport ? MainViewport->GetParentWindow() : nullptr;
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
}

void FApplication::OnKeyUp(EKey KeyCode, FModifierKeyState ModierKeyState)
{
    FKeyEvent KeyEvent(KeyCode, false, false, ModierKeyState);
    for (int32 Index = 0; Index < InputHandlers.GetSize(); Index++)
    {
        const FInputHandlerPair& Handler = InputHandlers[Index];
        if (Handler.First->HandleKeyEvent(KeyEvent))
        {
            KeyEvent.bIsConsumed = true;
        }
    }

    ImGuiIO& UIState = ImGui::GetIO();
    UIState.KeysDown[KeyEvent.KeyCode] = KeyEvent.bIsDown;

    if (UIState.WantCaptureKeyboard)
    {
        KeyEvent.bIsConsumed = true;
    }

    for (TSharedPtr<FWindow> Window : Windows)
    {
        Window->OnKeyUp(KeyEvent);
    }
}

void FApplication::OnKeyDown(EKey KeyCode, bool bIsRepeat, FModifierKeyState ModierKeyState)
{
    FKeyEvent KeyEvent(KeyCode, true, bIsRepeat, ModierKeyState);
    for (int32 Index = 0; Index < InputHandlers.GetSize(); Index++)
    {
        const FInputHandlerPair& Handler = InputHandlers[Index];
        if (Handler.First->HandleKeyEvent(KeyEvent))
        {
            KeyEvent.bIsConsumed = true;
        }
    }

    ImGuiIO& UIState = ImGui::GetIO();
    UIState.KeysDown[KeyEvent.KeyCode] = KeyEvent.bIsDown;

    if (UIState.WantCaptureKeyboard)
    {
        KeyEvent.bIsConsumed = true;
    }

    for (TSharedPtr<FWindow> Window : Windows)
    {
        Window->OnKeyDown(KeyEvent);
    }
}

void FApplication::OnKeyChar(uint32 Character)
{
    FKeyCharEvent Event(Character);
    for (int32 Index = 0; Index < InputHandlers.GetSize(); Index++)
    {
        const FInputHandlerPair& Handler = InputHandlers[Index];
        if (Handler.First->HandleKeyCharEvent(Event))
        {
            Event.bIsConsumed = true;
        }
    }

    ImGuiIO& UIState = ImGui::GetIO();
    UIState.AddInputCharacter(Event.Character);

    for (TSharedPtr<FWindow> Window : Windows)
    {
        Window->OnKeyChar(Event);
    }
}

void FApplication::OnMouseMove(int32 x, int32 y)
{
    FMouseMovedEvent MouseMovedEvent(x, y);
    for (int32 Index = 0; Index < InputHandlers.GetSize(); Index++)
    {
        const FInputHandlerPair& Handler = InputHandlers[Index];
        if (Handler.First->HandleMouseMove(MouseMovedEvent))
        {
            MouseMovedEvent.bIsConsumed = true;
        }
    }

    for (TSharedPtr<FWindow> Window : Windows)
    {
        Window->OnMouseMove(MouseMovedEvent);
    }
}

void FApplication::OnMouseUp(EMouseButton Button, FModifierKeyState ModierKeyState)
{
    TSharedRef<FGenericWindow> CaptureWindow = PlatformApplication->GetCapture();
    if (CaptureWindow)
    {
        PlatformApplication->SetCapture(nullptr);
    }

    FMouseButtonEvent MouseButtonEvent(Button, false, ModierKeyState);
    for (int32 Index = 0; Index < InputHandlers.GetSize(); Index++)
    {
        const FInputHandlerPair& Handler = InputHandlers[Index];
        if (Handler.First->HandleMouseButtonEvent(MouseButtonEvent))
        {
            MouseButtonEvent.bIsConsumed = true;
        }
    }

    ImGuiIO& UIState = ImGui::GetIO();

    const uint32 ButtonIndex = GetMouseButtonIndex(MouseButtonEvent.Button);
    UIState.MouseDown[ButtonIndex] = MouseButtonEvent.bIsDown;

    if (UIState.WantCaptureMouse)
    {
        MouseButtonEvent.bIsConsumed = true;
    }

    for (TSharedPtr<FWindow> Window : Windows)
    {
        Window->OnMouseUp(MouseButtonEvent);
    }
}

void FApplication::OnMouseDown(EMouseButton Button, FModifierKeyState ModierKeyState)
{
    TSharedRef<FGenericWindow> CaptureWindow = PlatformApplication->GetCapture();
    if (!CaptureWindow)
    {
        TSharedRef<FGenericWindow> ActiveWindow = PlatformApplication->GetActiveWindow();
        PlatformApplication->SetCapture(ActiveWindow);
    }

    FMouseButtonEvent MouseButtonEvent(Button, true, ModierKeyState);
    for (int32 Index = 0; Index < InputHandlers.GetSize(); Index++)
    {
        const FInputHandlerPair& Handler = InputHandlers[Index];
        if (Handler.First->HandleMouseButtonEvent(MouseButtonEvent))
        {
            MouseButtonEvent.bIsConsumed = true;
        }
    }

    ImGuiIO& UIState = ImGui::GetIO();

    const uint32 ButtonIndex = GetMouseButtonIndex(MouseButtonEvent.Button);
    UIState.MouseDown[ButtonIndex] = MouseButtonEvent.bIsDown;

    if (UIState.WantCaptureMouse)
    {
        MouseButtonEvent.bIsConsumed = true;
    }

    for (TSharedPtr<FWindow> Window : Windows)
    {
        Window->OnMouseDown(MouseButtonEvent);
    }
}

void FApplication::OnMouseScrolled(float HorizontalDelta, float VerticalDelta)
{
    FMouseScrolledEvent Event(HorizontalDelta, VerticalDelta);
    for (int32 Index = 0; Index < InputHandlers.GetSize(); Index++)
    {
        const FInputHandlerPair& Handler = InputHandlers[Index];
        if (Handler.First->HandleMouseScrolled(Event))
        {
            Event.bIsConsumed = true;
        }
    }

    ImGuiIO& UIState = ImGui::GetIO();
    UIState.MouseWheel  += Event.VerticalDelta;
    UIState.MouseWheelH += Event.HorizontalDelta;

    if (UIState.WantCaptureMouse)
    {
        Event.bIsConsumed = true;
    }

    for (TSharedPtr<FWindow> Window : Windows)
    {
        Window->OnMouseScroll(Event);
    }
}

void FApplication::OnWindowResized(const TSharedRef<FGenericWindow>& InWindow, uint32 Width, uint32 Height)
{
    TSharedPtr<FWindow> Window = FindWindowFromNativeWindow(InWindow);
    if (Window)
    {
        FWindowResizedEvent WindowResizeEvent(Width, Height);
        if (Window->OnWindowResized(WindowResizeEvent))
        {
            WindowResizeEvent.bIsConsumed = true;
        }
    }
}

void FApplication::OnWindowMoved(const TSharedRef<FGenericWindow>& InWindow, int32 x, int32 y)
{
    TSharedPtr<FWindow> Window = FindWindowFromNativeWindow(InWindow);
    if (Window)
    {
        FWindowMovedEvent WindowsMovedEvent(x, y);
        if (Window->OnWindowMoved(WindowsMovedEvent))
        {
            WindowsMovedEvent.bIsConsumed = true;
        }
    }
}

void FApplication::OnWindowFocusLost(const TSharedRef<FGenericWindow>& InWindow)
{
    TSharedPtr<FWindow> Window = FindWindowFromNativeWindow(InWindow);
    if (Window)
    {
        Window->OnWindowFocusLost();
    }

    ImGuiIO& UIState = ImGui::GetIO();
    FMemory::Memzero(UIState.KeysDown, sizeof(UIState.KeysDown));
}

void FApplication::OnWindowFocusGained(const TSharedRef<FGenericWindow>& InWindow)
{
    TSharedPtr<FWindow> Window = FindWindowFromNativeWindow(InWindow);
    if (Window)
    {
        Window->OnWindowFocusGained();
    }
}

void FApplication::OnWindowMouseLeft(const TSharedRef<FGenericWindow>& InWindow)
{
    TSharedPtr<FWindow> Window = FindWindowFromNativeWindow(InWindow);
    if (Window)
    {
        Window->OnMouseLeft();
    }
}

void FApplication::OnWindowMouseEntered(const TSharedRef<FGenericWindow>& InWindow)
{
    TSharedPtr<FWindow> Window = FindWindowFromNativeWindow(InWindow);
    if (Window)
    {
        Window->OnMouseEntered();
    }
}

void FApplication::OnWindowClosed(const TSharedRef<FGenericWindow>& InWindow)
{
    TSharedPtr<FWindow> Window = FindWindowFromNativeWindow(InWindow);
    if (Window)
    {
        Window->OnWindowClosed();

        TSharedPtr<FViewport> Viewport = Window->GetViewport();
        if (Viewport == MainViewport)
        {
            FPlatformApplicationMisc::RequestExit(0);
        }
    }
}

void FApplication::SetCursor(ECursor InCursor)
{
    TSharedPtr<ICursor> Cursor = GetCursor();
    Cursor->SetCursor(InCursor);
}

void FApplication::SetCursorPos(const FIntVector2& Position)
{
    TSharedPtr<ICursor> Cursor = GetCursor();
    Cursor->SetPosition(Position.x, Position.y);
}

FIntVector2 FApplication::GetCursorPos() const
{
    TSharedPtr<ICursor> Cursor = GetCursor();
    return Cursor->GetPosition();
}

void FApplication::ShowCursor(bool bIsVisible)
{
    TSharedPtr<ICursor> Cursor = GetCursor();
    Cursor->SetVisibility(bIsVisible);
}

bool FApplication::IsCursorVisibile() const
{
    TSharedPtr<ICursor> Cursor = GetCursor();
    return Cursor->IsVisible();
}

bool FApplication::EnableHighPrecisionMouseForWindow(const TSharedPtr<FWindow>& Window) 
{ 
    if (Window)
    {
        TSharedRef<FGenericWindow> NativeWindow = Window->GetNativeWindow();
        return PlatformApplication->EnableHighPrecisionMouseForWindow(NativeWindow);
    }

    return false;
}

void FApplication::SetCapture(const TSharedPtr<FWindow>& CaptureWindow)
{
    if (CaptureWindow)
    {
        TSharedRef<FGenericWindow> NativeWindow = CaptureWindow->GetNativeWindow();
        PlatformApplication->SetCapture(NativeWindow);
    }
}

void FApplication::SetActiveWindow(const TSharedPtr<FWindow>& ActiveWindow)
{
    if (ActiveWindow)
    {
        TSharedRef<FGenericWindow> NativeWindow = ActiveWindow->GetNativeWindow();
        PlatformApplication->SetActiveWindow(NativeWindow);
    }
}

TSharedPtr<FWindow> FApplication::GetActiveWindow() const 
{ 
    TSharedRef<FGenericWindow> NativeWindow = PlatformApplication->GetActiveWindow();
    return FindWindowFromNativeWindow(NativeWindow);
}

TSharedPtr<FWindow> FApplication::GetWindowUnderCursor() const
{ 
    TSharedRef<FGenericWindow> NativeWindow = PlatformApplication->GetActiveWindow();
    return FindWindowFromNativeWindow(NativeWindow);
}

TSharedPtr<FWindow> FApplication::GetCapture() const
{ 
    TSharedRef<FGenericWindow> NativeWindow = PlatformApplication->GetCapture();
    return FindWindowFromNativeWindow(NativeWindow);
}

void FApplication::AddInputHandler(const TSharedPtr<FInputHandler>& NewInputHandler, uint32 NewPriority)
{
    FInputHandlerPair NewPair{ NewInputHandler, NewPriority };
    if (!InputHandlers.Contains(NewPair))
    {
        for (int32 Index = 0; Index < InputHandlers.GetSize(); )
        {
            const FInputHandlerPair Handler = InputHandlers[Index];
            if (NewPriority <= Handler.Second)
            {
                Index++;
                InputHandlers.Insert(Index, NewPair);
                return;
            }
        }

        InputHandlers.Push(NewPair);
    }
}

void FApplication::RemoveInputHandler(const TSharedPtr<FInputHandler>& InputHandler)
{
    for (int32 Index = 0; Index < InputHandlers.GetSize(); Index++)
    {
        const FInputHandlerPair Handler = InputHandlers[Index];
        if (Handler.First == InputHandler)
        {
            InputHandlers.RemoveAt(Index);
            break;
        }
    }
}

void FApplication::RegisterMainViewport(const TSharedPtr<FViewport>& NewMainViewport)
{
    if (MainViewport != NewMainViewport)
    {
        MainViewport = NewMainViewport;

        // TODO: What to do with multiple Viewports
        ImGuiIO& InterfaceState = ImGui::GetIO();
        if (MainViewport)
        {
            TWeakPtr<FWindow> ParentWindow = MainViewport->GetParentWindow();
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

void FApplication::AddWindow(const TSharedPtr<FWindow>& NewWindow)
{
    if (NewWindow && !Windows.Contains(NewWindow))
    {
        NewWindow->Create();
        Windows.Emplace(NewWindow);
    }
}

void FApplication::RemoveWindow(const TSharedPtr<FWindow>& Window)
{
    if (Window)
    {
        Windows.Remove(Window);
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

TSharedPtr<FWindow> FApplication::FindWindowFromNativeWindow(const TSharedRef<FGenericWindow>& NativeWindow) const
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

