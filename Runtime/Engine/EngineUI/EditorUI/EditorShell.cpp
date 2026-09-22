#include "Engine/EngineUI/EditorUI/EditorShell.h"
#include "Engine/EngineUI/EditorUI/EditorIcons.h"
#include "Engine/EngineUI/EditorUI/EditorFooterPanel.h"
#include "Engine/EngineUI/EditorUI/EditorInputHandler.h"
#include "Engine/EngineUI/EditorUI/EditorPanelRegistry.h"
#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Engine/EngineUI/EditorUI/EditorTitleBar.h"
#include "Engine/EngineUI/EditorUI/Panels/EditorViewportPanel.h"
#include "Engine/EditorEngine.h"
#include "Core/Misc/IniFile.h"
#include "Core/Misc/OutputDeviceManager.h"
#include "Core/Misc/Paths.h"
#include "Core/Platform/PlatformFile.h"
#include "Application/Docking/DockInputHandler.h"
#include "Application/Docking/DockLayoutFile.h"
#include "Application/Docking/DockWindowManager.h"
#include "Application/Docking/DockingArea.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/Separator.h"
#include "Application/Elements/Window.h"
#include "Application/Menus/MenuInputHandler.h"

FEditorShell::FEditorShell(FEditorEngine* InEditorEngine)
    : EditorEngine(InEditorEngine)
    , DockingArea(nullptr)
    , Registry(nullptr)
    , TitleBar(nullptr)
    , Footer(nullptr)
    , Root(nullptr)
    , MenuInputHandler(nullptr)
    , EditorInputHandler(nullptr)
    , bStyleInitialized(false)
    , bIconsInitialized(false)
{
}

FEditorShell::~FEditorShell()
{
    Release();
}

String FEditorShell::GetLayoutFilename()
{
    return Paths::GetProjectDir() + "/EditorLayout.ini";
}

bool FEditorShell::Initialize()
{
    if (!FEditorStyle::Initialize())
    {
        LOG_ERROR("[FEditorShell]: Failed to load the editor style");
        return false;
    }

    bStyleInitialized = true;

    bIconsInitialized = FEditorIcons::Initialize();
    if (!bIconsInitialized)
    {
        LOG_WARNING("[FEditorShell]: The icon atlas could not be built, so the editor runs without icons");
    }

    FDockingArea::FDesc DockDesc;
    DockDesc.Font          = FEditorStyle::GetFonts().Body;
    DockDesc.TabCloseIcon  = FEditorIcons::Close;
    DockDesc.bAllowTearOut = true;
    DockDesc.OnPanelClosed = FOnPanelClosed::CreateRaw(this, &FEditorShell::OnPanelClosed);

    DockingArea = FDockingArea::Create(DockDesc);
    if (!DockingArea)
    {
        return false;
    }

    FDockWindowManager::FDesc HostDesc;
    HostDesc.MainArea = DockingArea;
    HostDesc.AreaDesc = DockDesc;

    FDockWindowManager::Get().Initialize(HostDesc);

    Registry = MakeSharedPtr<FEditorPanelRegistry>(EditorEngine, DockingArea);
    if (!Registry->RegisterAll())
    {
        return false;
    }

    TitleBar = MakeSharedPtr<FEditorTitleBar>(EditorEngine, Registry);
    if (!TitleBar->Initialize())
    {
        return false;
    }

    Footer = MakeSharedPtr<FEditorFooterPanel>();
    if (!Footer->Initialize())
    {
        return false;
    }

    Root = FVerticalBox::Create();
    if (!Root)
    {
        return false;
    }

    Root->AddSlot(TitleBar->GetElement());
    Root->AddSlot(DockingArea).SetFillCoefficient(1.0f);
    Root->AddSlot(FSeparator::CreateHorizontal());
    Root->AddSlot(Footer->GetElement());

    if (!RestoreLayout())
    {
        DockingArea->RestoreLayout(BuildDefaultLayout());
    }

    TSharedPtr<FWindow> EngineWindow = EditorEngine->GetEngineWindow();
    if (!EngineWindow)
    {
        LOG_ERROR("[FEditorShell]: The engine has no window to put the editor in");
        return false;
    }

    EngineWindow->SetContent(Root);

    MenuInputHandler   = FMenuInputHandler::Register();
    DockInputHandler   = FDockInputHandler::Register();
    EditorInputHandler = FEditorInputHandler::Register(EditorEngine, Registry);
    return true;
}

bool FEditorShell::InitPostRenderer()
{
    return true;
}

bool FEditorShell::RestoreLayout()
{
    const String LayoutFilename = GetLayoutFilename();
    if (!FPlatformFile::IsFile(*LayoutFilename))
    {
        return false;
    }

    FIniFile LayoutFile;
    if (LayoutFile.LoadFromFile(LayoutFilename))
    {
        int32 SavedVersion = 0;
        if (!LayoutFile.GetInt("Editor", "PanelSetVersion", SavedVersion) || SavedVersion != PanelSetVersion)
        {
            LOG_INFO("[FEditorShell]: The saved layout predates the current panel set, so the default is used");
            return false;
        }
    }

    TArray<FDockWindowLayout> Windows;
    if (!FDockLayoutFile::Load(LayoutFilename, Windows))
    {
        return false;
    }

    DockingArea->RestoreLayout(Windows[0].Root);

    TArray<FDockWindowLayout> Hosts;
    for (int32 Index = 1; Index < Windows.Size(); ++Index)
    {
        Hosts.Add(Windows[Index]);
    }

    FDockWindowManager::Get().RestoreHostLayouts(Hosts);
    return true;
}

void FEditorShell::SaveLayout()
{
    FDockWindowLayout MainLayout;
    MainLayout.Root = DockingArea->SaveLayout();

    if (TSharedPtr<FWindow> EngineWindow = EditorEngine ? EditorEngine->GetEngineWindow() : nullptr)
    {
        MainLayout.Title    = EngineWindow->GetTitle();
        MainLayout.Position = EngineWindow->GetPosition();
        MainLayout.Size     = EngineWindow->GetSize();
    }

    TArray<FDockWindowLayout> Windows;
    Windows.Add(MainLayout);

    if (FDockWindowManager::IsInitialized())
    {
        Windows.Append(FDockWindowManager::Get().SaveHostLayouts());
    }

    const String LayoutFilename = GetLayoutFilename();
    if (!FDockLayoutFile::Save(LayoutFilename, Windows))
    {
        return;
    }

    FIniFile LayoutFile;
    if (LayoutFile.LoadFromFile(LayoutFilename))
    {
        LayoutFile.SetOrAddInt("Editor", "PanelSetVersion", PanelSetVersion);
        LayoutFile.WriteToFile();
    }
}

void FEditorShell::Release()
{
    if (EditorInputHandler)
    {
        FEditorInputHandler::Unregister(EditorInputHandler);
        EditorInputHandler.Reset();
    }

    if (DockInputHandler)
    {
        FDockInputHandler::Unregister(DockInputHandler);
        DockInputHandler.Reset();
    }

    if (MenuInputHandler)
    {
        FMenuInputHandler::Unregister(MenuInputHandler);
        MenuInputHandler.Reset();
    }

    if (DockingArea)
    {
        SaveLayout();
    }

    FDockWindowManager::Shutdown();

    if (TSharedPtr<FWindow> EngineWindow = EditorEngine ? EditorEngine->GetEngineWindow() : nullptr)
    {
        EngineWindow->SetContent(nullptr);
    }

    if (Registry)
    {
        Registry->Release();
        Registry.Reset();
    }

    Root.Reset();
    Footer.Reset();
    TitleBar.Reset();
    DockingArea.Reset();

    if (bIconsInitialized)
    {
        FEditorIcons::Release();
        bIconsInitialized = false;
    }

    if (bStyleInitialized)
    {
        FEditorStyle::Release();
        bStyleInitialized = false;
    }
}

void FEditorShell::Tick(float DeltaTime)
{
    FMenuInputHandler::Tick(DeltaTime);

    if (FDockWindowManager::IsInitialized())
    {
        FDockWindowManager::Get().Tick();
    }

    if (Registry)
    {
        Registry->Tick(DeltaTime);
    }

    if (TitleBar)
    {
        TitleBar->Refresh();
    }

    if (Footer)
    {
        Footer->Refresh();
    }
}

void FEditorShell::OnActorRemoved(FActor* Actor)
{
    if (Registry)
    {
        Registry->OnActorRemoved(Actor);
    }
}

const TSharedPtr<FEditorViewportPanel>& FEditorShell::GetViewportPanel() const
{
    return Registry->GetViewportPanel();
}

void FEditorShell::OnPanelClosed(const String& PanelId)
{
    if (Registry)
    {
        Registry->OnPanelClosed(PanelId);
    }
}

FDockNode FEditorShell::BuildDefaultLayout()
{
    FDockNode Right = FDockNode::CreateSplit(EDockSplitOrientation::Vertical,
        FDockNode::CreateTabs({ "SceneHierarchy" }), FDockNode::CreateTabs({ "Properties" }));
    Right.ChildFractions = { 0.55f, 0.45f };

    FDockNode Centre = FDockNode::CreateSplit(EDockSplitOrientation::Horizontal,
        FDockNode::CreateTabs({ "RendererSettings", "Stats", "RHIInfo", "About" }), FDockNode::CreateTabs({ "Viewport" }));
    Centre.ChildFractions = { 0.24f, 0.76f };

    FDockNode Upper = FDockNode::CreateSplit(EDockSplitOrientation::Horizontal, Centre, Right);
    Upper.ChildFractions = { 0.78f, 0.22f };

    FDockNode Root = FDockNode::CreateSplit(EDockSplitOrientation::Vertical,
        Upper, FDockNode::CreateTabs({ "OutputLog", "ContentBrowser", "GPUProfiler", "FrameProfiler", "RenderGraph" }));
    Root.ChildFractions = { 0.72f, 0.28f };

    return Root;
}
