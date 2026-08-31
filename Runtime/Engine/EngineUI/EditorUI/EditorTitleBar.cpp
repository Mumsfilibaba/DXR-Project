#include "Engine/EngineUI/EditorUI/EditorTitleBar.h"
#include "Engine/EngineUI/EditorUI/EditorMenus.h"
#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Engine/EditorEngine.h"
#include "Application/Elements/TitleBar.h"
#include "Application/Menus/MenuBar.h"

FEditorTitleBar::FEditorTitleBar(FEditorEngine* InEditorEngine, const TSharedPtr<FEditorPanelRegistry>& InRegistry)
    : EditorEngine(InEditorEngine)
    , Menus(MakeUniquePtr<FEditorMenus>(InEditorEngine, InRegistry))
    , Bar(nullptr)
    , Element(nullptr)
    , bWasPlaying(false)
{
}

FEditorTitleBar::~FEditorTitleBar()
{
}

bool FEditorTitleBar::Initialize()
{
    TSharedPtr<FMenuBar> MenuBar = Menus->Build();
    if (!MenuBar)
    {
        return false;
    }

    FTitleBar::FDesc Desc;
    Desc.Title   = "DXR Engine";
    Desc.Font    = FEditorStyle::GetFonts().Title;
    Desc.Content = MenuBar;

    // On macOS the OS draws the traffic lights, so drawing our own would double them up.
#if PLATFORM_WINDOWS
    Desc.bShowCaptionButtons = true;
#endif

    Bar = FTitleBar::Create(Desc);
    if (!Bar)
    {
        return false;
    }

    Element = Bar;
    return true;
}

void FEditorTitleBar::Refresh()
{
    Menus->Refresh();

    const bool bIsPlaying = EditorEngine->IsPlaying();
    if (bIsPlaying != bWasPlaying)
    {
        Bar->SetTitle(bIsPlaying ? "DXR Engine (Playing)" : "DXR Engine");
        bWasPlaying = bIsPlaying;
    }
}

const TSharedPtr<FVisualElement>& FEditorTitleBar::GetElement() const
{
    return Element;
}
