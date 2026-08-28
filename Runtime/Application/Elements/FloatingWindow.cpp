#include "Application/Application.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/FloatingWindow.h"

TSharedPtr<FFloatingWindow> FFloatingWindow::Create(const FDesc& Desc)
{
    if (!FApplication::IsInitialized())
    {
        return nullptr;
    }

    TSharedPtr<FFloatingWindow> NewWindow = MakeSharedPtr<FFloatingWindow>();
    NewWindow->Initialize(Desc);

    if (!NewWindow->Window)
    {
        return nullptr;
    }

    return NewWindow;
}

FFloatingWindow::FFloatingWindow()
    : Window(nullptr)
    , TitleBar(nullptr)
    , Panel(nullptr)
    , Content(nullptr)
{
}

FFloatingWindow::~FFloatingWindow() = default;

void FFloatingWindow::Initialize(const FDesc& Desc)
{
    FWindow::FDesc WindowDesc;
    WindowDesc.Title        = Desc.Title;
    WindowDesc.ParentWindow = Desc.ParentWindow;
    WindowDesc.Position     = Desc.Position;
    WindowDesc.Size         = Desc.Size;
    WindowDesc.StyleFlags   = EWindowStyleFlags::Default | EWindowStyleFlags::CustomTitleBar;

    Window = FWindow::Create(WindowDesc);
    if (!Window)
    {
        return;
    }

    FTitleBar::FDesc TitleBarDesc;
    TitleBarDesc.Title               = Desc.Title;
    TitleBarDesc.Font                = Desc.Font;
    TitleBarDesc.Icon                = Desc.Icon;
    TitleBarDesc.Content             = Desc.TitleBarContent;
    TitleBarDesc.bShowCaptionButtons = true;

    TitleBar = FTitleBar::Create(TitleBarDesc);
    Content  = Desc.Content;

    Panel = MakeSharedPtr<FVerticalBox>();

    Panel->AddSlot(TitleBar);
    Panel->AddSlot(Content).SetFillCoefficient(1.0f);

    Window->SetContent(Panel);

    FApplication::Get().CreateWindow(Window);

    if (Desc.bShowOnCreate)
    {
        Show();
    }
}

void FFloatingWindow::SetContent(const TSharedPtr<FVisualElement>& InContent)
{
    if (!Panel)
    {
        return;
    }

    Content = InContent;

    Panel->ClearSlots();
    Panel->AddSlot(TitleBar);
    Panel->AddSlot(Content).SetFillCoefficient(1.0f);
}

void FFloatingWindow::Show()
{
    if (!Window)
    {
        return;
    }

    Window->Show();
    FApplication::LayoutWindow(Window);
}

void FFloatingWindow::Close()
{
    if (Window && FApplication::IsInitialized())
    {
        FApplication::Get().DestroyWindow(Window);
    }

    Window   = nullptr;
    TitleBar = nullptr;
    Panel    = nullptr;
    Content  = nullptr;
}

bool FFloatingWindow::IsOpen() const
{
    return Window != nullptr;
}
