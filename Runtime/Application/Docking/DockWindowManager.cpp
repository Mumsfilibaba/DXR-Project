#include "Application/Application.h"
#include "Application/Docking/DockDragState.h"
#include "Application/Docking/DockWindowManager.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/Image.h"
#include "Application/Elements/TitleBar.h"
#include "Application/Elements/Window.h"
#include "Application/IApplicationRenderer.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Misc/ConsoleManager.h"

static TAutoConsoleVariable<bool> CVarDecoratorSnapshot(
    "Docking.DecoratorSnapshot",
    "Drags a picture of the torn-off panel taken once at tear-out, instead of a live docking area",
    false,
    EConsoleVariableFlags::Default);

TUniquePtr<FDockWindowManager> FDockWindowManager::DockWindowManager = nullptr;

FDockWindowManager& FDockWindowManager::Get()
{
    if (!DockWindowManager)
    {
        DockWindowManager = MakeUniquePtr<FDockWindowManager>();
    }

    return *DockWindowManager;
}

bool FDockWindowManager::IsInitialized()
{
    return DockWindowManager.IsValid();
}

void FDockWindowManager::Shutdown()
{
    if (DockWindowManager)
    {
        DockWindowManager->CloseAllHosts();
        DockWindowManager.Reset();
    }
}

FDockWindowManager::FDockWindowManager()
    : Desc()
    , Hosts()
    , DecoratorWindow(nullptr)
    , DecoratorArea(nullptr)
    , DecoratorGrabOffset()
    , DropPreviews()
{
}

FDockWindowManager::~FDockWindowManager()
{
    if (FDockDragState::IsInitialized())
    {
        FDockDragState::Get().SetOnDragBegan(FOnDockDragBegan());
        FDockDragState::Get().SetOnDropOutside(FOnDockDropOutside());
        FDockDragState::Get().SetOnDragEnded(FOnDockDragEnded());
    }
}

void FDockWindowManager::Initialize(const FDesc& InDesc)
{
    Desc = InDesc;

    FDockDragState::Get().SetOnDragBegan(FOnDockDragBegan::CreateRaw(this, &FDockWindowManager::OnDragBegan));
    FDockDragState::Get().SetOnDropOutside(FOnDockDropOutside::CreateRaw(this, &FDockWindowManager::OnDropOutside));
    FDockDragState::Get().SetOnDragEnded(FOnDockDragEnded::CreateRaw(this, &FDockWindowManager::DestroyDecorator));
}

void FDockWindowManager::Tick()
{
    for (int32 Index = Hosts.Size() - 1; Index >= 0; --Index)
    {
        const TSharedPtr<FWindow>      Window = Hosts[Index].Window;
        const TSharedPtr<FDockingArea> Area   = Hosts[Index].Area;

        if (!Window || !Area || Area->GetDockedPanelIds().IsEmpty())
        {
            CloseHost(Index);
            continue;
        }

        const String Title = ResolveHostTitle(Area);
        if (!Title.IsEmpty() && Title != Window->GetTitle())
        {
            if (const TSharedPtr<FTitleBar>& TitleBar = Hosts[Index].TitleBar)
            {
                TitleBar->SetTitle(Title);
            }

            Window->SetTitle(Title);
        }
    }
}

TSharedPtr<FDockingArea> FDockWindowManager::SpawnHost(const String& PanelId, const String& Label, const TSharedPtr<FVisualElement>& Content, const IntVector2& ScreenPosition, const IntVector2& Size)
{
    if (PanelId.IsEmpty() || !FApplication::IsInitialized())
    {
        return nullptr;
    }

    FDockingArea::FDesc AreaDesc = Desc.AreaDesc;
    AreaDesc.bAllowTearOut       = true;

    TSharedPtr<FDockingArea> Area = FDockingArea::Create(AreaDesc);
    if (!Area)
    {
        return nullptr;
    }

    FWindow::FDesc WindowDesc;
    WindowDesc.Title         = Label.IsEmpty() ? PanelId : Label;
    WindowDesc.Position      = ScreenPosition;
    WindowDesc.Size          = Size;
    WindowDesc.StyleFlags    = EWindowStyleFlags::Default | EWindowStyleFlags::CustomTitleBar;
    WindowDesc.bShowOnCreate = false;

    TSharedPtr<FWindow> Window = FWindow::Create(WindowDesc);
    if (!Window)
    {
        return nullptr;
    }

    Area->RegisterPanel(PanelId, Label, Content);
    Area->DockPanel(PanelId, String(), EDockDirection::Center);

    FTitleBar::FDesc TitleBarDesc;
    TitleBarDesc.Title   = WindowDesc.Title;
    TitleBarDesc.Font    = AreaDesc.Font;
    TitleBarDesc.Content = Desc.TitleBarContent;

    // On macOS the OS draws the traffic lights over ours, so drawing our own would double them up.
#if PLATFORM_WINDOWS
    TitleBarDesc.bShowCaptionButtons = true;
#endif

    TSharedPtr<FTitleBar> TitleBar = FTitleBar::Create(TitleBarDesc);
    if (!TitleBar)
    {
        return nullptr;
    }

    TSharedPtr<FVerticalBox> Root = FVerticalBox::Create();
    Root->AddSlot(TitleBar);
    Root->AddSlot(Area).SetFillCoefficient(1.0f);

    Window->SetContent(Root);

    FHost Host;
    Host.Window   = Window;
    Host.Area     = Area;
    Host.TitleBar = TitleBar;

    Hosts.Add(Host);

    FWindow* const WindowPtr = Window.Get();
    Window->SetOnWindowClosed(FOnWindowClosed::CreateLambda([this, WindowPtr]()
    {
        OnHostWindowClosed(WindowPtr);
    }));

    FApplication::Get().CreateWindow(Window);

    FApplication::LayoutWindow(Window);
    return Area;
}

void FDockWindowManager::CloseAllHosts()
{
    DestroyDecorator();

    for (int32 Index = Hosts.Size() - 1; Index >= 0; --Index)
    {
        CloseHost(Index);
    }
}

void FDockWindowManager::MoveDecorator(const IntVector2& ScreenPosition)
{
    if (!DecoratorWindow)
    {
        return;
    }

    DecoratorWindow->MoveTo(ScreenPosition - DecoratorGrabOffset);
}

void FDockWindowManager::UpdateDropPreview()
{
    const FDockDragState& DragState = FDockDragState::Get();
    FDockingArea* const   Target    = DragState.GetTargetArea();

    if (!Target || !DecoratorArea || !DecoratorWindow || !FApplication::IsInitialized())
    {
        return;
    }

    TSharedPtr<IApplicationRenderer> Renderer = FApplication::Get().GetRenderer();
    if (!Renderer)
    {
        return;
    }

    FRectangle PreviewBounds;
    if (!Target->GetDropPreviewBounds(DragState.GetTargetPanelId(), DragState.GetTargetDirection(), PreviewBounds))
    {
        return;
    }

    const IntVector2 PreviewSize(PreviewBounds.Width, PreviewBounds.Height);
    if (FindDropPreview(Target, DragState.GetTargetPanelId(), DragState.GetTargetDirection(), PreviewSize) >= 0)
    {
        return;
    }

    DecoratorArea->PrepareDesiredSize();
    DecoratorArea->Tick(FRectangle(IntVector2(0, 0), PreviewSize.X, PreviewSize.Y));

    FRHITextureRef Texture = Renderer->RenderElementToTexture(DecoratorArea, PreviewSize, 1.0f);

    FApplication::LayoutWindow(DecoratorWindow);

    if (!Texture)
    {
        return;
    }

    if (DropPreviews.Size() >= MaxDropPreviews)
    {
        ReleaseTextureDeferred(DropPreviews[0].Texture);
        DropPreviews.RemoveAt(0);
    }

    DropPreviews.Add(FDropPreview{ Texture, Target, DragState.GetTargetPanelId(), DragState.GetTargetDirection(), PreviewSize });
}

FRHITexture* FDockWindowManager::GetDropPreviewTexture() const
{
    const FDockDragState& DragState = FDockDragState::Get();
    FDockingArea* const   Target    = DragState.GetTargetArea();
    if (!Target)
    {
        return nullptr;
    }

    FRectangle PreviewBounds;
    if (!Target->GetDropPreviewBounds(DragState.GetTargetPanelId(), DragState.GetTargetDirection(), PreviewBounds))
    {
        return nullptr;
    }

    const int32 Index = FindDropPreview(Target, DragState.GetTargetPanelId(), DragState.GetTargetDirection(),
        IntVector2(PreviewBounds.Width, PreviewBounds.Height));

    return Index >= 0 ? DropPreviews[Index].Texture.Get() : nullptr;
}

int32 FDockWindowManager::FindDropPreview(const FDockingArea* Area, const String& PanelId, EDockDirection Direction, const IntVector2& Size) const
{
    for (int32 Index = 0; Index < DropPreviews.Size(); ++Index)
    {
        const FDropPreview& Preview = DropPreviews[Index];
        if (Preview.Area == Area && Preview.Direction == Direction && Preview.Size == Size && Preview.PanelId == PanelId)
        {
            return Index;
        }
    }

    return -1;
}

void FDockWindowManager::ReleaseTextureDeferred(FRHITextureRef& Texture)
{
    if (Texture && FApplication::IsInitialized())
    {
        if (TSharedPtr<IApplicationRenderer> Renderer = FApplication::Get().GetRenderer())
        {
            Renderer->RetireTexture(Texture);
        }
    }

    Texture = nullptr;
}

void FDockWindowManager::ClearDropPreview()
{
    for (FDropPreview& Preview : DropPreviews)
    {
        ReleaseTextureDeferred(Preview.Texture);
    }

    DropPreviews.Clear();
}

void FDockWindowManager::DestroyDecorator()
{
    ClearDropPreview();
    ReleaseTextureDeferred(DecoratorSnapshot);

    if (!DecoratorWindow)
    {
        DecoratorArea = nullptr;
        return;
    }

    const TSharedPtr<FWindow>      Window = DecoratorWindow;
    const TSharedPtr<FDockingArea> Area   = DecoratorArea;

    DecoratorWindow = nullptr;
    DecoratorArea   = nullptr;

    if (Area)
    {
        for (const String& PanelId : Area->GetRegisteredPanelIds())
        {
            Area->UnregisterPanel(PanelId);
        }
    }

    Window->SetContent(nullptr);

    if (FApplication::IsInitialized())
    {
        FApplication::Get().DestroyWindow(Window);
    }
}

TArray<FDockWindowLayout> FDockWindowManager::SaveHostLayouts() const
{
    TArray<FDockWindowLayout> Layouts;
    for (const FHost& Host : Hosts)
    {
        if (!Host.Window || !Host.Area)
        {
            continue;
        }

        FDockWindowLayout Layout;
        Layout.Title    = Host.Window->GetTitle();
        Layout.Position = Host.Window->GetPosition();
        Layout.Size     = Host.Window->GetSize();
        Layout.Root     = Host.Area->SaveLayout();

        Layouts.Add(Layout);
    }

    return Layouts;
}

void FDockWindowManager::RestoreHostLayouts(const TArray<FDockWindowLayout>& Layouts)
{
    const TSharedPtr<FDockingArea> Main = Desc.MainArea;
    if (!Main)
    {
        return;
    }

    for (const FDockWindowLayout& Layout : Layouts)
    {
        TArray<String> PanelIds;
        Layout.Root.GatherPanelIds(PanelIds);

        TSharedPtr<FDockingArea> Area = nullptr;
        for (const String& PanelId : PanelIds)
        {
            String                     Label;
            TSharedPtr<FVisualElement> Panel;

            if (!Main->GetPanelRegistration(PanelId, Label, Panel))
            {
                continue;
            }

            if (Area)
            {
                Area->RegisterPanel(PanelId, Label, Panel);
            }
            else
            {
                const bool       bHasSize = Layout.Size.X > 0 && Layout.Size.Y > 0;
                const IntVector2 Size     = bHasSize ? Layout.Size : Desc.DefaultSize;

                Area = SpawnHost(PanelId, Label, Panel, Layout.Position, Size);
                if (!Area)
                {
                    break;
                }
            }

            Main->UnregisterPanel(PanelId);
        }

        if (Area)
        {
            Area->RestoreLayout(Layout.Root);
        }
    }
}

TSharedPtr<FDockingArea> FDockWindowManager::FindAreaForPanel(const String& PanelId) const
{
    if (Desc.MainArea && Desc.MainArea->IsPanelDocked(PanelId))
    {
        return Desc.MainArea;
    }

    for (const FHost& Host : Hosts)
    {
        if (Host.Area && Host.Area->IsPanelDocked(PanelId))
        {
            return Host.Area;
        }
    }

    return nullptr;
}

bool FDockWindowManager::IsPanelDockedAnywhere(const String& PanelId) const
{
    return FindAreaForPanel(PanelId) != nullptr;
}

bool FDockWindowManager::IsPanelVisibleAnywhere(const String& PanelId) const
{
    const TSharedPtr<FDockingArea> Area = FindAreaForPanel(PanelId);
    return Area && Area->IsPanelVisible(PanelId);
}

bool FDockWindowManager::FocusPanelInHost(const String& PanelId)
{
    for (const FHost& Host : Hosts)
    {
        if (!Host.Area || !Host.Area->IsPanelDocked(PanelId))
        {
            continue;
        }

        Host.Area->SetActivePanel(PanelId);

        if (Host.Window)
        {
            Host.Window->Show();
            Host.Window->SetFocus();
        }

        return true;
    }

    return false;
}

TSharedPtr<FWindow> FDockWindowManager::GetHostWindow(int32 HostIndex) const
{
    return Hosts.IsValidIndex(HostIndex) ? Hosts[HostIndex].Window : nullptr;
}

TSharedPtr<FDockingArea> FDockWindowManager::GetHostArea(int32 HostIndex) const
{
    return Hosts.IsValidIndex(HostIndex) ? Hosts[HostIndex].Area : nullptr;
}

IntVector2 FDockWindowManager::ResolveHostPosition(const IntVector2& ScreenPosition)
{
    return ScreenPosition - IntVector2(SpawnCursorInset, SpawnCursorInset);
}

String FDockWindowManager::ResolveHostTitle(const TSharedPtr<FDockingArea>& Area)
{
    if (!Area)
    {
        return String();
    }

    for (const String& PanelId : Area->GetDockedPanelIds())
    {
        if (!Area->IsPanelVisible(PanelId))
        {
            continue;
        }

        String                     Label;
        TSharedPtr<FVisualElement> Panel;

        if (Area->GetPanelRegistration(PanelId, Label, Panel))
        {
            return Label;
        }
    }

    return String();
}

int32 FDockWindowManager::FindHostByArea(const FDockingArea* Area) const
{
    for (int32 Index = 0; Index < Hosts.Size(); ++Index)
    {
        if (Hosts[Index].Area.Get() == Area)
        {
            return Index;
        }
    }

    return -1;
}

int32 FDockWindowManager::FindHostByWindow(const FWindow* Window) const
{
    for (int32 Index = 0; Index < Hosts.Size(); ++Index)
    {
        if (Hosts[Index].Window.Get() == Window)
        {
            return Index;
        }
    }

    return -1;
}

void FDockWindowManager::OnDragBegan(const FDockDragPanel& Panel, const IntVector2& ScreenPosition)
{
    DestroyDecorator();

    if (Panel.PanelId.IsEmpty() || !FApplication::IsInitialized())
    {
        return;
    }

    FDockingArea::FDesc AreaDesc = Desc.AreaDesc;
    AreaDesc.bAllowTearOut = false;
    AreaDesc.bIsDropTarget = false;
    AreaDesc.OnPanelTornOut = FOnPanelTornOut();
    AreaDesc.OnPanelClosed  = FOnPanelClosed();

    TSharedPtr<FDockingArea> Area = FDockingArea::Create(AreaDesc);
    if (!Area)
    {
        return;
    }

    DecoratorGrabOffset = IntVector2(SpawnCursorInset, SpawnCursorInset);

    FWindow::FDesc WindowDesc;
    WindowDesc.Title           = Panel.Label.IsEmpty() ? Panel.PanelId : Panel.Label;
    WindowDesc.Position        = ScreenPosition - DecoratorGrabOffset;
    WindowDesc.Size            = IntVector2(DecoratorWidth, DecoratorHeight);
    WindowDesc.StyleFlags      = EWindowStyleFlags::TopMost | EWindowStyleFlags::NoTaskBarIcon | EWindowStyleFlags::Opaque;
    WindowDesc.bAcceptsInput   = false;
    WindowDesc.bActivateOnShow = false;
    WindowDesc.bShowOnCreate   = false;

    TSharedPtr<FWindow> Window = FWindow::Create(WindowDesc);
    if (!Window)
    {
        return;
    }

    Area->RegisterPanel(Panel.PanelId, Panel.Label, Panel.Content);
    Area->DockPanel(Panel.PanelId, String(), EDockDirection::Center);

    Window->SetContent(Area);
    Window->SetOpacity(DecoratorOpacity);

    DecoratorWindow = Window;
    DecoratorArea   = Area;

    FApplication::Get().CreateWindow(Window);

    FApplication::LayoutWindow(Window);

    if (CVarDecoratorSnapshot.GetValue())
    {
        SnapshotDecorator();
    }
}

void FDockWindowManager::SnapshotDecorator()
{
    if (!DecoratorWindow || !DecoratorArea)
    {
        return;
    }

    TSharedPtr<IApplicationRenderer> Renderer = FApplication::Get().GetRenderer();
    if (!Renderer)
    {
        return;
    }

    const IntVector2 Size = DecoratorWindow->GetSize();
    FRHITextureRef Snapshot = Renderer->RenderElementToTexture(DecoratorArea, Size, DecoratorWindow->GetWindowDPIScale());
    if (!Snapshot)
    {
        return;
    }

    FImage::FDesc ImageDesc;
    ImageDesc.Brush = FUIBrush(Snapshot.Get());

    TSharedPtr<FImage> Image = FImage::Create(ImageDesc);
    if (!Image)
    {
        return;
    }

    DecoratorSnapshot = Snapshot;
    DecoratorWindow->SetContent(Image);

    FApplication::LayoutWindow(DecoratorWindow);
}

void FDockWindowManager::OnDropOutside(const FDockDragPanel& Panel, const IntVector2& ScreenPosition)
{
    if (Panel.PanelId.IsEmpty())
    {
        return;
    }

    const IntVector2 Position = DecoratorWindow ? DecoratorWindow->GetPosition() : ResolveHostPosition(ScreenPosition);
    const IntVector2 Size     = Desc.DefaultSize;

    const int32 SourceHostIndex = FindHostByArea(Panel.SourceArea);
    if (SourceHostIndex >= 0 && Panel.SourceArea->GetDockedPanelIds().IsEmpty())
    {
        Panel.SourceArea->RegisterPanel(Panel.PanelId, Panel.Label, Panel.Content);
        Panel.SourceArea->DockPanel(Panel.PanelId, String(), EDockDirection::Center);

        if (const TSharedPtr<FWindow>& Window = Hosts[SourceHostIndex].Window)
        {
            Window->MoveTo(Position);
        }

        return;
    }

    SpawnHost(Panel.PanelId, Panel.Label, Panel.Content, Position, Size);
}

void FDockWindowManager::OnHostWindowClosed(FWindow* Window)
{
    const int32 HostIndex = FindHostByWindow(Window);
    if (HostIndex < 0)
    {
        return;
    }

    const TSharedPtr<FDockingArea> Area = Hosts[HostIndex].Area;
    Hosts.RemoveAt(HostIndex);

    ReturnPanelRegistrationsToMainArea(Area);
}

void FDockWindowManager::CloseHost(int32 HostIndex)
{
    if (!Hosts.IsValidIndex(HostIndex))
    {
        return;
    }

    const FHost Host = Hosts[HostIndex];
    Hosts.RemoveAt(HostIndex);

    ReturnPanelRegistrationsToMainArea(Host.Area);

    if (Host.Window)
    {
        Host.Window->SetContent(nullptr);

        if (FApplication::IsInitialized())
        {
            FApplication::Get().DestroyWindow(Host.Window);
        }
    }
}

void FDockWindowManager::ReturnPanelRegistrationsToMainArea(const TSharedPtr<FDockingArea>& Area)
{
    const TSharedPtr<FDockingArea> Main = Desc.MainArea;
    if (!Area || !Main || Main == Area)
    {
        return;
    }

    for (const String& PanelId : Area->GetRegisteredPanelIds())
    {
        String                     Label;
        TSharedPtr<FVisualElement> Panel;

        if (Area->GetPanelRegistration(PanelId, Label, Panel))
        {
            Main->RegisterPanel(PanelId, Label, Panel);
        }

        Area->UnregisterPanel(PanelId);
    }
}
