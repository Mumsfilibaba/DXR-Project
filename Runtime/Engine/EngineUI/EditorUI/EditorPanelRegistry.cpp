#include "Engine/EngineUI/EditorUI/EditorPanelRegistry.h"
#include "Engine/EngineUI/EditorUI/EditorPanel.h"
#include "Engine/EngineUI/EditorUI/Panels/EditorAboutPanel.h"
#include "Engine/EngineUI/EditorUI/Panels/EditorContentBrowserPanel.h"
#include "Engine/EngineUI/EditorUI/Panels/EditorFrameProfilerPanel.h"
#include "Engine/EngineUI/EditorUI/Panels/EditorGPUProfilerPanel.h"
#include "Engine/EngineUI/EditorUI/Panels/EditorOutputLogPanel.h"
#include "Engine/EngineUI/EditorUI/Panels/EditorPropertiesPanel.h"
#include "Engine/EngineUI/EditorUI/Panels/EditorRHIInfoPanel.h"
#include "Engine/EngineUI/EditorUI/Panels/EditorRenderGraphPanel.h"
#include "Engine/EngineUI/EditorUI/Panels/EditorRendererSettingsPanel.h"
#include "Engine/EngineUI/EditorUI/Panels/EditorSceneHierarchyPanel.h"
#include "Engine/EngineUI/EditorUI/Panels/EditorStatsPanel.h"
#include "Engine/EngineUI/EditorUI/Panels/EditorViewportPanel.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Application/Docking/DockWindowManager.h"
#include "Application/Docking/DockingArea.h"
#include "Application/Elements/Border.h"

FEditorPanelRegistry::FEditorPanelRegistry(FEditorEngine* InEditorEngine, const TSharedPtr<FDockingArea>& InDockingArea)
    : EditorEngine(InEditorEngine)
    , DockingArea(InDockingArea)
    , Panels()
    , ViewportPanel(nullptr)
    , OutputLogPanel(nullptr)
{
}

FEditorPanelRegistry::~FEditorPanelRegistry()
{
    Release();
}

bool FEditorPanelRegistry::Add(const TSharedPtr<FEditorPanel>& Panel)
{
    if (!Panel || !Panel->Initialize())
    {
        LOG_ERROR("[FEditorPanelRegistry]: Failed to initialize panel '%s'", Panel ? *Panel->GetPanelId() : "<null>");
        return false;
    }

    const FMargin ContentPadding = Panel->GetContentPadding();

    TSharedPtr<FVisualElement> Content = Panel->GetContent();
    if (Content && (ContentPadding.GetTotalHorizontal() > 0 || ContentPadding.GetTotalVertical() > 0))
    {
        FBorder::FDesc FrameDesc;
        FrameDesc.Padding = ContentPadding;
        FrameDesc.Content = Content;

        Content = FBorder::Create(FrameDesc);
    }

    DockingArea->RegisterPanel(Panel->GetPanelId(), Panel->GetLabel(), Content);
    Panels.Add(Panel);
    return true;
}

bool FEditorPanelRegistry::RegisterAll()
{
    ViewportPanel = MakeSharedPtr<FEditorViewportPanel>(EditorEngine);
    if (!Add(ViewportPanel))
    {
        return false;
    }

    OutputLogPanel = MakeSharedPtr<FEditorOutputLogPanel>(EditorEngine);
    if (!Add(OutputLogPanel))
    {
        return false;
    }

    if (!Add(MakeSharedPtr<FEditorSceneHierarchyPanel>(EditorEngine)))
    {
        return false;
    }

    if (!Add(MakeSharedPtr<FEditorPropertiesPanel>(EditorEngine)))
    {
        return false;
    }

    if (!Add(MakeSharedPtr<FEditorRenderGraphPanel>(EditorEngine)))
    {
        return false;
    }

    if (!Add(MakeSharedPtr<FEditorContentBrowserPanel>(EditorEngine)))
    {
        return false;
    }

    if (!Add(MakeSharedPtr<FEditorRendererSettingsPanel>(EditorEngine)))
    {
        return false;
    }

    if (!Add(MakeSharedPtr<FEditorGPUProfilerPanel>(EditorEngine)))
    {
        return false;
    }

    if (!Add(MakeSharedPtr<FEditorFrameProfilerPanel>(EditorEngine)))
    {
        return false;
    }

    if (!Add(MakeSharedPtr<FEditorRHIInfoPanel>(EditorEngine)))
    {
        return false;
    }

    if (!Add(MakeSharedPtr<FEditorStatsPanel>(EditorEngine)))
    {
        return false;
    }

    if (!Add(MakeSharedPtr<FEditorAboutPanel>(EditorEngine)))
    {
        return false;
    }

    return true;
}

void FEditorPanelRegistry::Release()
{
    for (const TSharedPtr<FEditorPanel>& Panel : Panels)
    {
        if (DockingArea)
        {
            DockingArea->UnregisterPanel(Panel->GetPanelId());
        }

        Panel->Release();
    }

    Panels.Clear();

    ViewportPanel.Reset();
    OutputLogPanel.Reset();
    DockingArea.Reset();
}

bool FEditorPanelRegistry::IsPanelDocked(const String& PanelId) const
{
    if (DockingArea && DockingArea->IsPanelDocked(PanelId))
    {
        return true;
    }

    return FDockWindowManager::IsInitialized() && FDockWindowManager::Get().IsPanelDockedAnywhere(PanelId);
}

bool FEditorPanelRegistry::IsPanelVisible(const String& PanelId) const
{
    if (DockingArea && DockingArea->IsPanelVisible(PanelId))
    {
        return true;
    }

    return FDockWindowManager::IsInitialized() && FDockWindowManager::Get().IsPanelVisibleAnywhere(PanelId);
}

void FEditorPanelRegistry::Tick(float DeltaTime)
{
    for (const TSharedPtr<FEditorPanel>& Panel : Panels)
    {
        const bool bIsVisible = IsPanelVisible(Panel->GetPanelId());
        if (bIsVisible != Panel->IsVisible())
        {
            Panel->OnVisibilityChanged(bIsVisible);
        }

        Panel->SetOpen(IsPanelDocked(Panel->GetPanelId()));
        Panel->Tick(DeltaTime);
    }
}

void FEditorPanelRegistry::OnActorRemoved(FActor* Actor)
{
    for (const TSharedPtr<FEditorPanel>& Panel : Panels)
    {
        Panel->OnActorRemoved(Actor);
    }
}

void FEditorPanelRegistry::ShowPanel(const String& PanelId)
{
    TSharedPtr<FEditorPanel> Panel = FindPanel(PanelId);
    if (!Panel || !DockingArea)
    {
        return;
    }

    if (FDockWindowManager::IsInitialized() && FDockWindowManager::Get().FocusPanelInHost(PanelId))
    {
        Panel->SetOpen(true);
        return;
    }

    if (!DockingArea->IsPanelDocked(PanelId))
    {
        DockingArea->DockPanel(PanelId, String(), EDockDirection::Center);
    }

    DockingArea->SetActivePanel(PanelId);
    Panel->SetOpen(true);
}

void FEditorPanelRegistry::HidePanel(const String& PanelId)
{
    TSharedPtr<FEditorPanel> Panel = FindPanel(PanelId);
    if (!Panel || !DockingArea)
    {
        return;
    }

    if (DockingArea->IsPanelDocked(PanelId))
    {
        DockingArea->UndockPanel(PanelId);
    }

    Panel->SetOpen(false);
}

void FEditorPanelRegistry::TogglePanel(const String& PanelId)
{
    if (IsPanelDocked(PanelId))
    {
        HidePanel(PanelId);
        return;
    }

    ShowPanel(PanelId);
}

void FEditorPanelRegistry::OnPanelClosed(const String& PanelId)
{
    if (TSharedPtr<FEditorPanel> Panel = FindPanel(PanelId))
    {
        Panel->SetOpen(false);
    }
}

TSharedPtr<FEditorPanel> FEditorPanelRegistry::FindPanel(const String& PanelId) const
{
    for (const TSharedPtr<FEditorPanel>& Panel : Panels)
    {
        if (Panel->GetPanelId() == PanelId)
        {
            return Panel;
        }
    }

    return nullptr;
}
