#include "Engine/EngineUI/EditorUI/EditorPanel.h"

FEditorPanel::FEditorPanel(FEditorEngine* InEditorEngine, const String& InPanelId, const String& InLabel)
    : EditorEngine(InEditorEngine)
    , PanelId(InPanelId)
    , Label(InLabel)
    , Content(nullptr)
    , bIsOpen(true)
    , bIsVisible(true)
{
}

FEditorPanel::~FEditorPanel()
{
}

void FEditorPanel::Release()
{
    Content.Reset();
}

void FEditorPanel::Tick(float /*DeltaTime*/)
{
}

void FEditorPanel::OnActorRemoved(FActor* /*Actor*/)
{
}

void FEditorPanel::OnVisibilityChanged(bool bInIsVisible)
{
    bIsVisible = bInIsVisible;
}
