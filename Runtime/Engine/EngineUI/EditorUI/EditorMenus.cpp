#include "Engine/EngineUI/EditorUI/EditorMenus.h"
#include "Engine/EngineUI/EditorUI/EditorPanel.h"
#include "Engine/EngineUI/EditorUI/EditorPanelRegistry.h"
#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Engine/EngineUI/EditorUI/IEditorViewportHost.h"
#include "Engine/EngineUI/Editor/EditorActorFactory.h"
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

    Menu->AddSection("Open", Font);

    FMenuItem::FDesc NewLevel;
    NewLevel.Label        = "New Level";
    NewLevel.ShortcutText = "Ctrl+N";
    NewLevel.Font         = Font;

    Menu->AddItem(FMenuItem::Create(NewLevel));

    FMenuItem::FDesc OpenLevel;
    OpenLevel.Label        = "Open Level";
    OpenLevel.ShortcutText = "Ctrl+O";
    OpenLevel.Font         = Font;

    Menu->AddItem(FMenuItem::Create(OpenLevel));

    Menu->AddSection("Save", Font);

    FMenuItem::FDesc SaveAll;
    SaveAll.Label        = "Save All";
    SaveAll.ShortcutText = "Ctrl+Shift+S";
    SaveAll.Font         = Font;

    Menu->AddItem(FMenuItem::Create(SaveAll));

    Menu->AddSection("Exit", Font);

    FMenuItem::FDesc Exit;
    Exit.Label        = "Exit";
    Exit.ShortcutText = "Cmd+Q";
    Exit.Font         = Font;
    Exit.OnActivated  = FOnMenuItemActivated::CreateLambda([]()
    {
        RequestEngineExit("File / Exit");
    });

    Menu->AddItem(FMenuItem::Create(Exit));

    Bar->AddMenu("File", Font, Menu);
}

void FEditorMenus::BuildEditMenu(const TSharedPtr<FMenuBar>& Bar)
{
    const TSharedPtr<IFontFace>& Font = FEditorStyle::GetFonts().Body;

    TSharedPtr<FMenu> Menu = FMenu::Create();

    Menu->AddSection("Play", Font);

    FMenuItem::FDesc PlayDesc;
    PlayDesc.Label        = "Play";
    PlayDesc.ShortcutText = "F5";
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

    Menu->AddSection("Create", Font);

    if (TSharedPtr<FMenu> PlaceActorMenu = BuildPlaceActorMenu())
    {
        FMenuItem::FDesc PlaceDesc;
        PlaceDesc.Label   = "Place Actor";
        PlaceDesc.Font    = Font;
        PlaceDesc.SubMenu = PlaceActorMenu;

        Menu->AddItem(FMenuItem::Create(PlaceDesc));
    }

    Menu->AddSection("Selection", Font);

    FMenuItem::FDesc DeleteDesc;
    DeleteDesc.Label        = "Delete Selection";
    DeleteDesc.ShortcutText = "Del";
    DeleteDesc.Font         = Font;
    DeleteDesc.OnActivated  = FOnMenuItemActivated::CreateLambda([this]()
    {
        EditorEngine->RequestDeleteActors(EditorEngine->GetSelectedActors());
    });

    Menu->AddItem(FMenuItem::Create(DeleteDesc));

    FMenuItem::FDesc ClearDesc;
    ClearDesc.Label       = "Clear Selection";
    ClearDesc.Font        = Font;
    ClearDesc.OnActivated = FOnMenuItemActivated::CreateLambda([this]()
    {
        EditorEngine->ClearSelection();
    });

    Menu->AddItem(FMenuItem::Create(ClearDesc));

    Menu->AddSection("Settings", Font);

    FMenuItem::FDesc ProjectSettings;
    ProjectSettings.Label = "Project Settings";
    ProjectSettings.Font  = Font;

    Menu->AddItem(FMenuItem::Create(ProjectSettings));

    FMenuItem::FDesc EditorPreferences;
    EditorPreferences.Label = "Editor Preferences";
    EditorPreferences.Font  = Font;

    Menu->AddItem(FMenuItem::Create(EditorPreferences));

    Bar->AddMenu("Edit", Font, Menu);
}

TSharedPtr<FMenu> FEditorMenus::BuildPlaceActorMenu()
{
    const TSharedPtr<IFontFace>& Font = FEditorStyle::GetFonts().Body;

    TSharedPtr<FMenu> Menu = FMenu::Create();
    if (!Menu)
    {
        return nullptr;
    }

    Menu->AddSection("Primitives", Font);

    for (uint8 Index = 0; Index < static_cast<uint8>(EEditorPrimitiveType::Count); ++Index)
    {
        const EEditorPrimitiveType Type = static_cast<EEditorPrimitiveType>(Index);

        FMenuItem::FDesc Desc;
        Desc.Label       = EditorActorFactory::GetPrimitiveName(Type);
        Desc.Font        = Font;
        Desc.OnActivated = FOnMenuItemActivated::CreateLambda([this, Type]()
        {
            OnActorPlaced(EditorActorFactory::SpawnPrimitive(EditorEngine->GetWorld(), Type, GetPlaceActorLocation()));
        });

        Menu->AddItem(FMenuItem::Create(Desc));
    }

    Menu->AddSection("Lights", Font);

    static const EEditorLightType LightTypes[] =
    {
        EEditorLightType::Point,
        EEditorLightType::Directional,
        EEditorLightType::Sky,
    };

    for (const EEditorLightType Type : LightTypes)
    {
        FMenuItem::FDesc Desc;
        Desc.Label       = EditorActorFactory::GetLightName(Type);
        Desc.Font        = Font;
        Desc.OnActivated = FOnMenuItemActivated::CreateLambda([this, Type]()
        {
            FWorld* World = EditorEngine->GetWorld();
            if (EditorActorFactory::CanSpawnLight(World, Type))
            {
                OnActorPlaced(EditorActorFactory::SpawnLight(World, Type, GetPlaceActorLocation()));
            }
        });

        Menu->AddItem(FMenuItem::Create(Desc));
    }

    Menu->AddSection("Camera", Font);

    FMenuItem::FDesc CameraDesc;
    CameraDesc.Label       = "Camera";
    CameraDesc.Font        = Font;
    CameraDesc.OnActivated = FOnMenuItemActivated::CreateLambda([this]()
    {
        OnActorPlaced(EditorActorFactory::SpawnCamera(EditorEngine->GetWorld(), GetPlaceActorLocation()));
    });

    Menu->AddItem(FMenuItem::Create(CameraDesc));

    return Menu;
}

Vector3 FEditorMenus::GetPlaceActorLocation() const
{
    constexpr float DefaultDistance = 10.0f;

    if (IEditorViewportHost* Host = EditorEngine->GetViewportHost())
    {
        if (FCameraComponent* Camera = Host->GetViewCamera())
        {
            return Camera->GetViewLocationAtDistance(DefaultDistance);
        }
    }

    return Vector3(0.0f, 0.0f, 0.0f);
}

void FEditorMenus::OnActorPlaced(FActor* Actor)
{
    if (Actor)
    {
        EditorEngine->SetSelectedActor(Actor);
    }
}

void FEditorMenus::BuildWindowsMenu(const TSharedPtr<FMenuBar>& Bar)
{
    const TSharedPtr<IFontFace>& Font = FEditorStyle::GetFonts().Body;

    TSharedPtr<FMenu> Menu = FMenu::Create();

    WindowItems.Clear();

    Menu->AddSection("Windows", Font);

    for (const TSharedPtr<FEditorPanel>& Panel : Registry->GetPanels())
    {
        const String PanelId = Panel->GetPanelId();

        FMenuItem::FDesc Desc;
        Desc.Label        = Panel->GetLabel();
        Desc.Font         = Font;
        Desc.bIsCheckable = true;
        Desc.CheckState   = Panel->IsOpen() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
        Desc.OnActivated  = FOnMenuItemActivated::CreateLambda([this, PanelId]()
        {
            Registry->ShowPanel(PanelId);
        });

        TSharedPtr<FMenuItem> Item = FMenuItem::Create(Desc);

        Menu->AddItem(Item);
        WindowItems.Add(Item);
    }

    Bar->AddMenu("Windows", Font, Menu);
}

void FEditorMenus::BuildHelpMenu(const TSharedPtr<FMenuBar>& Bar)
{
    const TSharedPtr<IFontFace>& Font = FEditorStyle::GetFonts().Body;

    TSharedPtr<FMenu> Menu = FMenu::Create();

    Menu->AddSection("About", Font);

    FMenuItem::FDesc AboutDesc;
    AboutDesc.Label       = "About";
    AboutDesc.Font        = Font;
    AboutDesc.OnActivated = FOnMenuItemActivated::CreateLambda([this]()
    {
        Registry->ShowPanel("About");
    });

    Menu->AddItem(FMenuItem::Create(AboutDesc));

    Bar->AddMenu("Help", Font, Menu);
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
