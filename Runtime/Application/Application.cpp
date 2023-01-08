#include "Application.h"
#include "CoreApplication/Platform/PlatformApplication.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"

#include <imgui.h>

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


TSharedPtr<FApplication> FApplication::GInstance;

bool FApplication::Create()
{
    TSharedPtr<FGenericApplication> Application = MakeSharedPtr(FPlatformApplicationMisc::CreateApplication());
    if (!Application)
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to create FPlatformApplication");
        return false;
    }

    GInstance = MakeSharedPtr(new FApplication(Application));
    if (!GInstance->Initialize())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to create UI Context");
        return false;
    }

    Application->SetMessageListener(GInstance);
    return true;
}

bool FApplication::Create(const TSharedPtr<FApplication>& NewApplication)
{
    TSharedPtr<FGenericApplication> Application = MakeSharedPtr(FPlatformApplicationMisc::CreateApplication());
    if (!Application)
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to create FPlatformApplication");
        return false;
    }

    GInstance = NewApplication;
    if (!GInstance)
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "FApplication cannot be nullptr");
        return false;
    }

    GInstance->SetPlatformApplication(Application);

    if (!GInstance->Initialize())
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to create UI Context");
        return false;
    }

    Application->SetMessageListener(GInstance);
    return true;
}

void FApplication::Destroy()
{
    if (GInstance)
    {
        GInstance->SetPlatformApplication(nullptr);
        GInstance.Reset();
    }
}

FApplication::FApplication(const TSharedPtr<FGenericApplication>& InPlatformApplication)
    : PlatformApplication(InPlatformApplication)
    , MainViewport(nullptr)
    , Renderer(nullptr)
    , DebugStrings()
    , InterfaceWindows()
    , InputHandlers()
    , bIsRunning(true)
    , Context(nullptr)
{ }

bool FApplication::InitializeRHI()
{
    Renderer = new FViewportRenderer();
    if (!Renderer->Initialize(Context))
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "Failed to init ViewportRenderer ");
    }

    return true; 
}

void FApplication::ReleaseRHI()
{
    if (Renderer)
    {
        delete Renderer;
        Renderer = nullptr;
    }
}

FApplication::~FApplication()
{
    if (Context)
    {
        ImGui::DestroyContext(Context);
    }
}

bool FApplication::Initialize()
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
    Style.FramePadding     = ImVec2(6.0f, 4.0f);
    
    // Use AA for lines etc.
    Style.AntiAliasedLines       = true;
    Style.AntiAliasedFill        = true;
    // Style.AntiAliasedLinesUseTex = true;

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

    return true;
}

FGenericWindowRef FApplication::CreateWindow()
{
    return PlatformApplication->CreateWindow();
}

void FApplication::Tick(FTimespan DeltaTime)
{
    // Update UI
    ImGuiIO& UIState = ImGui::GetIO();

    FGenericWindowRef Window = MainViewport ? MainViewport->GetWindow() : nullptr;
    if (Window)
    {
        if (UIState.WantSetMousePos)
        {
            SetCursorPos(Window, FIntVector2(static_cast<int32>(UIState.MousePos.x), static_cast<int32>(UIState.MousePos.y)));
        }

        FWindowShape CurrentWindowShape;
        Window->GetWindowShape(CurrentWindowShape);

        UIState.DeltaTime   = static_cast<float>(DeltaTime.AsSeconds());
        UIState.DisplaySize = ImVec2{ float(CurrentWindowShape.Width), float(CurrentWindowShape.Height) };

        const FMonitorDesc MonitorDesc = PlatformApplication->GetMonitorDescFromWindow(Window);
        UIState.DisplayFramebufferScale = ImVec2{ MonitorDesc.DisplayScaling, MonitorDesc.DisplayScaling };
        UIState.FontGlobalScale = MonitorDesc.DisplayScaling;

        const FIntVector2 Position = GetCursorPos(Window);
        UIState.MousePos = ImVec2(static_cast<float>(Position.x), static_cast<float>(Position.y));
    }

    FModifierKeyState KeyState = FPlatformApplicationMisc::GetModifierKeyState();
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

        InterfaceWindows.Foreach([](TSharedRef<FWidget>& Window)
        {
            if (Window->IsTickable())
            {
                Window->Tick();
            }
        });

        RenderStrings();

        Renderer->EndFrame();
    }

    // Update platform
    const float Delta = static_cast<float>(DeltaTime.AsMilliseconds());
    PlatformApplication->Tick(Delta);
}

void FApplication::SetCursor(ECursor InCursor)
{
    TSharedPtr<ICursor> Cursor = GetCursor();
    Cursor->SetCursor(InCursor);
}

void FApplication::SetCursorPos(const FIntVector2& Position)
{
    TSharedPtr<ICursor> Cursor = GetCursor();
    Cursor->SetPosition(nullptr, Position.x, Position.y);
}

void FApplication::SetCursorPos(const FGenericWindowRef& RelativeWindow, const FIntVector2& Position)
{
    TSharedPtr<ICursor> Cursor = GetCursor();
    Cursor->SetPosition(RelativeWindow.Get(), Position.x, Position.y);
}

FIntVector2 FApplication::GetCursorPos() const
{
    TSharedPtr<ICursor> Cursor = GetCursor();

    FIntVector2 CursorPosition;
    Cursor->GetPosition(nullptr, CursorPosition.x, CursorPosition.y);

    return CursorPosition;
}

FIntVector2 FApplication::GetCursorPos(const FGenericWindowRef& RelativeWindow) const
{
    TSharedPtr<ICursor> Cursor = GetCursor();

    FIntVector2 CursorPosition;
    Cursor->GetPosition(RelativeWindow.Get(), CursorPosition.x, CursorPosition.y);

    return CursorPosition;
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

void FApplication::SetCapture(const FGenericWindowRef& CaptureWindow)
{
    PlatformApplication->SetCapture(CaptureWindow);
}

void FApplication::SetActiveWindow(const FGenericWindowRef& ActiveWindow)
{
    PlatformApplication->SetActiveWindow(ActiveWindow);
}

void FApplication::AddInputHandler(const TSharedPtr<FInputHandler>& NewInputHandler, uint32 NewPriority)
{
    TPair NewPair(NewInputHandler, NewPriority);
    if (!InputHandlers.Contains(NewPair))
    {
        for (int32 Index = 0; Index < InputHandlers.GetSize(); )
        {
            const auto Handler = InputHandlers[Index];
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
        const auto Handler = InputHandlers[Index];
        if (Handler.First == InputHandler)
        {
            InputHandlers.RemoveAt(Index);
            break;
        }
    }
}

void FApplication::RegisterMainViewport(const TSharedRef<FViewport>& NewMainViewport)
{
    if (MainViewport != NewMainViewport)
    {
        MainViewport = NewMainViewport;

        if (ViewportChangedEvent.IsBound())
        {
            ViewportChangedEvent.Broadcast(NewMainViewport.Get());
        }

        // TODO: What to do with multiple Viewports
        ImGuiIO& InterfaceState = ImGui::GetIO();
        if (MainViewport)
        {
            const FGenericWindowRef Window = NewMainViewport->GetWindow();
            InterfaceState.ImeWindowHandle = Window->GetPlatformHandle();
        }
        else
        {
            InterfaceState.ImeWindowHandle = nullptr;
        }

        RegisterViewport(NewMainViewport);
    }
}

void FApplication::RegisterViewport(const TSharedRef<FViewport>& NewViewport)
{
    if (NewViewport)
    {
        Viewports.PushUnique(NewViewport);
    }
}

void FApplication::UnregisterViewport(const TSharedRef<FViewport>& Viewport)
{
    if (Viewport)
    {
        Viewports.RemoveAll(Viewport);
    }
}

void FApplication::AddWidget(const TSharedRef<FWidget>& NewWindow)
{
    if (NewWindow && !InterfaceWindows.Contains(NewWindow))
    {
        TSharedRef<FWidget>& Window = InterfaceWindows.Emplace(NewWindow);
        Window->InitContext(Context);
    }
}

void FApplication::RemoveWidget(const TSharedRef<FWidget>& Window)
{
    InterfaceWindows.Remove(Window);
}

void FApplication::DrawString(const FString& NewString)
{
    DebugStrings.Emplace(NewString);
}

void FApplication::DrawWindows(FRHICommandList& CommandList)
{
    // NOTE: Renderer is not forced to be valid
    if (Renderer)
    {
        Renderer->Render(CommandList);
    }
}

TSharedRef<FViewport> FApplication::GetViewportFromWindow(const FGenericWindowRef& Window)
{
    if (Window)
    {
        for (const TSharedRef<FViewport>& Viewport : Viewports)
        {
            if (Window == Viewport->GetWindow())
            {
                return Viewport;
            }
        }
    }

    return nullptr;
}

void FApplication::SetPlatformApplication(const TSharedPtr<FGenericApplication>& InPlatformApplication)
{
    if (InPlatformApplication)
    {
        CHECK(this == GInstance);
        InPlatformApplication->SetMessageListener(GInstance);
    }

    PlatformApplication = InPlatformApplication;
}

void FApplication::OnKeyReleased(EKey KeyCode, FModifierKeyState ModierKeyState)
{
    FKeyEvent KeyEvent(KeyCode, false, false, ModierKeyState);
    for (int32 Index = 0; Index < InputHandlers.GetSize(); Index++)
    {
        const TPair<TSharedPtr<FInputHandler>, uint32>& Handler = InputHandlers[Index];
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

    for (TSharedRef<FViewport> Viewport : Viewports)
    {
        Viewport->OnKeyUp(KeyEvent);
    }
}

void FApplication::OnKeyPressed(EKey KeyCode, bool bIsRepeat, FModifierKeyState ModierKeyState)
{
    FKeyEvent KeyEvent(KeyCode, true, bIsRepeat, ModierKeyState);
    for (int32 Index = 0; Index < InputHandlers.GetSize(); Index++)
    {
        const TPair<TSharedPtr<FInputHandler>, uint32>& Handler = InputHandlers[Index];
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

    for (TSharedRef<FViewport> Viewport : Viewports)
    {
        Viewport->OnKeyDown(KeyEvent);
    }
}

void FApplication::OnKeyChar(uint32 Character)
{
    FKeyCharEvent Event(Character);
    for (int32 Index = 0; Index < InputHandlers.GetSize(); Index++)
    {
        const TPair<TSharedPtr<FInputHandler>, uint32>& Handler = InputHandlers[Index];
        if (Handler.First->HandleKeyTyped(Event))
        {
            Event.bIsConsumed = true;
        }
    }

    ImGuiIO& UIState = ImGui::GetIO();
    UIState.AddInputCharacter(Event.Character);

    for (TSharedRef<FViewport> Viewport : Viewports)
    {
        Viewport->OnKeyChar(Event);
    }
}

void FApplication::OnCursorMove(int32 x, int32 y)
{
    FMouseMovedEvent MouseMovedEvent(x, y);
    for (int32 Index = 0; Index < InputHandlers.GetSize(); Index++)
    {
        const TPair<TSharedPtr<FInputHandler>, uint32>& Handler = InputHandlers[Index];
        if (Handler.First->HandleMouseMove(MouseMovedEvent))
        {
            MouseMovedEvent.bIsConsumed = true;
        }
    }

    for (TSharedRef<FViewport> Viewport : Viewports)
    {
        Viewport->OnCursorMove(MouseMovedEvent);
    }
}

void FApplication::OnCursorReleased(EMouseButton Button, FModifierKeyState ModierKeyState)
{
    FGenericWindowRef CaptureWindow = PlatformApplication->GetCapture();
    if (CaptureWindow)
    {
        PlatformApplication->SetCapture(nullptr);
    }

    FMouseButtonEvent MouseButtonEvent(Button, false, ModierKeyState);
    for (int32 Index = 0; Index < InputHandlers.GetSize(); Index++)
    {
        const TPair<TSharedPtr<FInputHandler>, uint32>& Handler = InputHandlers[Index];
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

    for (TSharedRef<FViewport> Viewport : Viewports)
    {
        Viewport->OnCursorButtonUp(MouseButtonEvent);
    }
}

void FApplication::OnCursorPressed(EMouseButton Button, FModifierKeyState ModierKeyState)
{
    FGenericWindowRef CaptureWindow = PlatformApplication->GetCapture();
    if (!CaptureWindow)
    {
        FGenericWindowRef ActiveWindow = PlatformApplication->GetActiveWindow();
        PlatformApplication->SetCapture(ActiveWindow);
    }

    FMouseButtonEvent MouseButtonEvent(Button, true, ModierKeyState);
    for (int32 Index = 0; Index < InputHandlers.GetSize(); Index++)
    {
        const TPair<TSharedPtr<FInputHandler>, uint32>& Handler = InputHandlers[Index];
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

    for (TSharedRef<FViewport> Viewport : Viewports)
    {
        Viewport->OnCursorButtonDown(MouseButtonEvent);
    }
}

void FApplication::OnCursorScrolled(float HorizontalDelta, float VerticalDelta)
{
    FMouseScrolledEvent Event(HorizontalDelta, VerticalDelta);
    for (int32 Index = 0; Index < InputHandlers.GetSize(); Index++)
    {
        const TPair<TSharedPtr<FInputHandler>, uint32>& Handler = InputHandlers[Index];
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

    for (TSharedRef<FViewport> Viewport : Viewports)
    {
        Viewport->OnCursorScroll(Event);
    }
}

void FApplication::OnWindowResized(const FGenericWindowRef& Window, uint32 Width, uint32 Height)
{
    TSharedRef<FViewport> Viewport = GetViewportFromWindow(Window);
    if (Viewport)
    {
        FWindowResizeEvent WindowResizeEvent(Window, Width, Height);
        if (Viewport->OnViewportResized(WindowResizeEvent))
        {
            WindowResizeEvent.bIsConsumed = true;
        }
    }
}

void FApplication::OnWindowMoved(const FGenericWindowRef& Window, int32 x, int32 y)
{
    TSharedRef<FViewport> Viewport = GetViewportFromWindow(Window);
    if (Viewport)
    {
        FWindowMovedEvent WindowsMovedEvent(Window, x, y);
        if (Viewport->OnViewportMoved(WindowsMovedEvent))
        {
            WindowsMovedEvent.bIsConsumed = true;
        }
    }
}

void FApplication::OnWindowFocusLost(const FGenericWindowRef& Window)
{
    TSharedRef<FViewport> Viewport = GetViewportFromWindow(Window);
    if (Viewport)
    {
        Viewport->OnViewportFocusLost();
    }

    ImGuiIO& UIState = ImGui::GetIO();
    FMemory::Memzero(UIState.KeysDown, sizeof(UIState.KeysDown));
}

void FApplication::OnWindowFocusGained(const FGenericWindowRef& Window)
{
    TSharedRef<FViewport> Viewport = GetViewportFromWindow(Window);
    if (Viewport)
    {
        Viewport->OnViewportFocusGained();
    }
}

void FApplication::OnWindowCursorLeft(const FGenericWindowRef& Window)
{
    TSharedRef<FViewport> Viewport = GetViewportFromWindow(Window);
    if (Viewport)
    {
        Viewport->OnViewportCursorLeft();
    }
}

void FApplication::OnWindowCursorEntered(const FGenericWindowRef& Window)
{
    TSharedRef<FViewport> Viewport = GetViewportFromWindow(Window);
    if (Viewport)
    {
        Viewport->OnViewportCursorEntered();
    }
}

void FApplication::RenderStrings()
{
    if (MainViewport && !DebugStrings.IsEmpty())
    {
        const FIntVector2 Size = MainViewport->GetSize();

        constexpr float Width = 400.0f;
        ImGui::SetNextWindowPos(ImVec2(static_cast<float>(Size.x - Width), 18.0f));
        ImGui::SetNextWindowSize(ImVec2(Width, 0.0f));

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.3f, 0.3f, 0.3f, 0.6f));

        const ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings;

        ImGui::Begin("DebugWindow", nullptr, WindowFlags);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

        for (const FString& String : DebugStrings)
        {
            ImGui::Text("%s", String.GetCString());
        }

        DebugStrings.Clear();

        ImGui::PopStyleColor();
        ImGui::PopStyleColor();
        ImGui::End();
    }
}

void FApplication::OnWindowClosed(const FGenericWindowRef& Window)
{
    TSharedRef<FViewport> Viewport = GetViewportFromWindow(Window);
    if (Viewport)
    {
        Viewport->OnViewportClosed();
    }

    if (Viewport == MainViewport)
    {
        FPlatformApplicationMisc::RequestExit(0);
    }
}

void FApplication::OnApplicationExit(int32 ExitCode)
{
    bIsRunning = false;
    ExitEvent.Broadcast(ExitCode);
}
