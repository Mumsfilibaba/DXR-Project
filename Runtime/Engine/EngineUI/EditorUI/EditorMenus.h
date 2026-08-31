#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Math/Vector3.h"

class FActor;
class FEditorEngine;
class FEditorPanelRegistry;
class FMenu;
class FMenuBar;
class FMenuItem;

class ENGINE_API FEditorMenus
{
public:

    /**
     * @brief Constructs the menu builder over the engine it acts on and the registry the Windows menu lists.
     *
     * @param InEditorEngine The engine every command acts on.
     * @param InRegistry     The registry the Windows menu is built from.
     */
    FEditorMenus(FEditorEngine* InEditorEngine, const TSharedPtr<FEditorPanelRegistry>& InRegistry);
    ~FEditorMenus();

    /**
     * @brief Builds the File, Edit, Windows and Help menus into a bar.
     *
     * @return The bar, or null when it could not be built.
     */
    NODISCARD TSharedPtr<FMenuBar> Build();

    /** @brief Re-reads the state every checkable row shows, which the shell does once per frame. */
    void Refresh();

private:
    void BuildFileMenu(const TSharedPtr<FMenuBar>& Bar);
    void BuildEditMenu(const TSharedPtr<FMenuBar>& Bar);
    void BuildWindowsMenu(const TSharedPtr<FMenuBar>& Bar);
    void BuildHelpMenu(const TSharedPtr<FMenuBar>& Bar);

    NODISCARD TSharedPtr<FMenu> BuildPlaceActorMenu();
    NODISCARD Vector3 GetPlaceActorLocation() const;

    void OnActorPlaced(FActor* Actor);

    FEditorEngine*                   EditorEngine;
    TSharedPtr<FEditorPanelRegistry> Registry;
    TSharedPtr<FMenuBar>             MenuBar;
    TArray<TSharedPtr<FMenuItem>>    WindowItems;
    TSharedPtr<FMenuItem>            PlayItem;
};
