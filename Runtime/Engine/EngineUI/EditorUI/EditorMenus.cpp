#include "Engine/EngineUI/EditorUI/EditorMenus.h"
#include "Engine/EngineUI/EditorUI/EditorPanel.h"
#include "Engine/EngineUI/EditorUI/EditorPanelRegistry.h"
#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Engine/EngineUI/EditorUI/IEditorViewportHost.h"
#include "Engine/EngineUI/Editor/EditorActorFactory.h"
#include "Engine/EngineUI/EditorUI/EditorPrimitiveShapeDialog.h"
#include "Engine/World/Components/CameraComponent.h"
#include "Engine/EditorEngine.h"
#include "Core/CoreGlobals.h"
#include "Core/Misc/ConsoleManager.h"
#include "Application/Docking/DockingArea.h"
#include "Application/Menus/Menu.h"
#include "Application/Menus/MenuBar.h"
#include "Application/Menus/MenuItem.h"

FEditorMenus::FEditorMenus(FEditorEngine* InEditorEngine, const TSharedPtr<FEditorPanelRegistry>& InRegistry)
    : EditorEngine(InEditorEngine)
    , Registry(InRegistry)
    , MenuBar(nullptr)
    , WindowItems()
    , PlayItem(nullptr)
{
}

FEditorMenus::~FEditorMenus()
{
}

TSharedPtr<FMenuBar> FEditorMenus::Build()
{
    MenuBar = FMenuBar::Create();
    if (!MenuBar)
    {
        return nullptr;
    }

    BuildFileMenu(MenuBar);
    BuildEditMenu(MenuBar);
    BuildWindowsMenu(MenuBar);
    BuildHelpMenu(MenuBar);

    return MenuBar;
}

void FEditorMenus::BuildFileMenu(const TSharedPtr<FMenuBar>& Bar)
{
    const TSharedPtr<IFontFace>& Font = FEditorStyle::GetFonts().Body;

    TSharedPtr<FMenu> Menu = FMenu::Create();

    Menu->AddSection(TEXT("Open"), Font);

    FMenuItem::FDesc NewLevel;
    NewLevel.Label        = TEXT("New Level");
    NewLevel.ShortcutText = TEXT("Ctrl+N");
    NewLevel.Font         = Font;

    Menu->AddItem(FMenuItem::Create(NewLevel));

    FMenuItem::FDesc OpenLevel;
    OpenLevel.Label        = TEXT("Open Level");
    OpenLevel.ShortcutText = TEXT("Ctrl+O");
    OpenLevel.Font         = Font;

    Menu->AddItem(FMenuItem::Create(OpenLevel));

    Menu->AddSection(TEXT("Save"), Font);

    FMenuItem::FDesc SaveAll;
    SaveAll.Label        = TEXT("Save All");
    SaveAll.ShortcutText = TEXT("Ctrl+Shift+S");
    SaveAll.Font         = Font;

    Menu->AddItem(FMenuItem::Create(SaveAll));

    Menu->AddSection(TEXT("Exit"), Font);

    FMenuItem::FDesc Exit;
    Exit.Label        = TEXT("Exit");
    Exit.ShortcutText = TEXT("Cmd+Q");
    Exit.Font         = Font;
    Exit.OnActivated  = FOnMenuItemActivated::CreateLambda([]()
    {
        RequestEngineExit(TEXT("File / Exit"));
    });

    Menu->AddItem(FMenuItem::Create(Exit));

    Bar->AddMenu(TEXT("File"), Font, Menu);
}

void FEditorMenus::BuildEditMenu(const TSharedPtr<FMenuBar>& Bar)
{
    const TSharedPtr<IFontFace>& Font = FEditorStyle::GetFonts().Body;

    TSharedPtr<FMenu> Menu = FMenu::Create();

    Menu->AddSection(TEXT("Play"), Font);

    FMenuItem::FDesc PlayDesc;
    PlayDesc.Label        = TEXT("Play");
    PlayDesc.ShortcutText = TEXT("F5");
    PlayDesc.Font         = Font;
    PlayDesc.bIsCheckable = true;
    PlayDesc.OnActivated  = FOnMenuItemActivated::CreateLambda([this]()
    {
        if (EditorEngine->IsPlaying())
        {
            EditorEngine->StopPlay();
        }
        else
        {
            EditorEngine->StartPlay();
        }
    });

    PlayItem = FMenuItem::Create(PlayDesc);
    Menu->AddItem(PlayItem);

    Menu->AddSection(TEXT("Create"), Font);

    const FGetPlaceActorLocation GetLocation = FGetPlaceActorLocation::CreateLambda([this]()
    {
        return GetPlaceActorLocation();
    });

    if (TSharedPtr<FMenu> PlaceActorMenu = BuildPlaceActorMenu(EditorEngine, GetLocation))
    {
        FMenuItem::FDesc PlaceDesc;
        PlaceDesc.Label   = TEXT("Place Actor");
        PlaceDesc.Font    = Font;
        PlaceDesc.SubMenu = PlaceActorMenu;

        Menu->AddItem(FMenuItem::Create(PlaceDesc));
    }

    Menu->AddSection(TEXT("Selection"), Font);

    FMenuItem::FDesc DeleteDesc;
    DeleteDesc.Label        = TEXT("Delete Selection");
    DeleteDesc.ShortcutText = TEXT("Del");
    DeleteDesc.Font         = Font;
    DeleteDesc.OnActivated  = FOnMenuItemActivated::CreateLambda([this]()
    {
        EditorEngine->RequestDeleteActors(EditorEngine->GetSelectedActors());
    });

    Menu->AddItem(FMenuItem::Create(DeleteDesc));

    FMenuItem::FDesc ClearDesc;
    ClearDesc.Label       = TEXT("Clear Selection");
    ClearDesc.Font        = Font;
    ClearDesc.OnActivated = FOnMenuItemActivated::CreateLambda([this]()
    {
        EditorEngine->ClearSelection();
    });

    Menu->AddItem(FMenuItem::Create(ClearDesc));

    Menu->AddSection(TEXT("Settings"), Font);

    FMenuItem::FDesc ProjectSettings;
    ProjectSettings.Label = TEXT("Project Settings");
    ProjectSettings.Font  = Font;

    Menu->AddItem(FMenuItem::Create(ProjectSettings));

    FMenuItem::FDesc EditorPreferences;
    EditorPreferences.Label = TEXT("Editor Preferences");
    EditorPreferences.Font  = Font;

    Menu->AddItem(FMenuItem::Create(EditorPreferences));

    Bar->AddMenu(TEXT("Edit"), Font, Menu);
}

TSharedPtr<FMenu> FEditorMenus::BuildPlaceActorMenu(FEditorEngine* InEditorEngine, const FGetPlaceActorLocation& GetLocation)
{
    const TSharedPtr<IFontFace>& Font = FEditorStyle::GetFonts().Body;

    TSharedPtr<FMenu> Menu = FMenu::Create();
    if (!Menu)
    {
        return nullptr;
    }

    const auto SelectPlacedActor = [InEditorEngine](FActor* Actor)
    {
        if (Actor)
        {
            InEditorEngine->SetSelectedActor(Actor);
        }
    };

    Menu->AddSection(TEXT("Primitives"), Font);

    for (int32 Index = 0; Index < PrimitiveShapes::NumShapes; ++Index)
    {
        const EPrimitiveShape Type = static_cast<EPrimitiveShape>(Index);

        FMenuItem::FDesc Desc;
        Desc.Label       = EditorActorFactory::GetPrimitiveName(Type);
        Desc.Font        = Font;
        Desc.OnActivated = FOnMenuItemActivated::CreateLambda([InEditorEngine, GetLocation, Type]()
        {
            FEditorPrimitiveShapeDialog::Place(InEditorEngine, Type, GetLocation.Execute());
        });

        Menu->AddItem(FMenuItem::Create(Desc));
    }

    Menu->AddSection(TEXT("Lights"), Font);

    static const EEditorLightType LightTypes[] =
    {
        EEditorLightType::Point,
        EEditorLightType::Spot,
        EEditorLightType::Directional,
        EEditorLightType::SkyLight,
        EEditorLightType::Sky,
    };

    for (const EEditorLightType Type : LightTypes)
    {
        FMenuItem::FDesc Desc;
        Desc.Label       = EditorActorFactory::GetLightName(Type);
        Desc.Font        = Font;
        Desc.OnActivated = FOnMenuItemActivated::CreateLambda([InEditorEngine, GetLocation, SelectPlacedActor, Type]()
        {
            FWorld* World = InEditorEngine->GetWorld();
            if (EditorActorFactory::CanSpawnLight(World, Type))
            {
                SelectPlacedActor(EditorActorFactory::SpawnLight(World, Type, GetLocation.Execute()));
            }
        });

        Menu->AddItem(FMenuItem::Create(Desc));
    }

    Menu->AddSection(TEXT("Camera"), Font);

    FMenuItem::FDesc CameraDesc;
    CameraDesc.Label       = TEXT("Camera");
    CameraDesc.Font        = Font;
    CameraDesc.OnActivated = FOnMenuItemActivated::CreateLambda([InEditorEngine, GetLocation, SelectPlacedActor]()
    {
        SelectPlacedActor(EditorActorFactory::SpawnCamera(InEditorEngine->GetWorld(), GetLocation.Execute()));
    });

    Menu->AddItem(FMenuItem::Create(CameraDesc));

    return Menu;
}

FVector3 FEditorMenus::GetPlaceActorLocation() const
{
    constexpr float DefaultDistance = 10.0f;

    if (IEditorViewportHost* Host = EditorEngine->GetViewportHost())
    {
        if (FCameraComponent* Camera = Host->GetViewCamera())
        {
            return Camera->GetViewLocationAtDistance(DefaultDistance);
        }
    }

    return FVector3(0.0f, 0.0f, 0.0f);
}

void FEditorMenus::BuildWindowsMenu(const TSharedPtr<FMenuBar>& Bar)
{
    const TSharedPtr<IFontFace>& Font = FEditorStyle::GetFonts().Body;

    TSharedPtr<FMenu> Menu = FMenu::Create();

    WindowItems.Clear();

    Menu->AddSection(TEXT("Windows"), Font);

    for (const TSharedPtr<FEditorPanel>& Panel : Registry->GetPanels())
    {
        const FString PanelId = Panel->GetPanelId();

        FMenuItem::FDesc Desc;
        Desc.Label        = Panel->GetLabel();
        Desc.Font         = Font;
        Desc.bIsCheckable = true;
        Desc.CheckState   = Panel->IsOpen() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
        Desc.OnActivated  = FOnMenuItemActivated::CreateLambda([this, PanelId]()
        {
            Registry->TogglePanel(PanelId);
        });

        TSharedPtr<FMenuItem> Item = FMenuItem::Create(Desc);

        Menu->AddItem(Item);
        WindowItems.Add(Item);
    }

    Bar->AddMenu(TEXT("Windows"), Font, Menu);
}

void FEditorMenus::BuildHelpMenu(const TSharedPtr<FMenuBar>& Bar)
{
    const TSharedPtr<IFontFace>& Font = FEditorStyle::GetFonts().Body;

    TSharedPtr<FMenu> Menu = FMenu::Create();

    Menu->AddSection(TEXT("About"), Font);

    FMenuItem::FDesc AboutDesc;
    AboutDesc.Label       = TEXT("About");
    AboutDesc.Font        = Font;
    AboutDesc.OnActivated = FOnMenuItemActivated::CreateLambda([this]()
    {
        Registry->ShowPanel(TEXT("About"));
    });

    Menu->AddItem(FMenuItem::Create(AboutDesc));

    Bar->AddMenu(TEXT("Help"), Font, Menu);
}

void FEditorMenus::Refresh()
{
    if (PlayItem)
    {
        PlayItem->SetCheckState(EditorEngine->IsPlaying() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
    }

    const TArray<TSharedPtr<FEditorPanel>>& Panels = Registry->GetPanels();
    for (int32 Index = 0; Index < WindowItems.Size() && Index < Panels.Size(); ++Index)
    {
        WindowItems[Index]->SetCheckState(Panels[Index]->IsOpen() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
    }
}
