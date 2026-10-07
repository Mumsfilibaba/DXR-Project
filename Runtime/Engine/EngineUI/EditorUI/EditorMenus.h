#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Math/Vector3.h"
#include "Core/Delegates/Delegate.h"

class FEditorEngine;
class FEditorPanelRegistry;
class FMenu;
class FMenuBar;
class FMenuItem;

DECLARE_RETURN_DELEGATE(FGetPlaceActorLocation, Vector3);

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

    /**
     * @brief Builds the Place Actor menu, shared by the Edit menu and the viewport context menu so they always list the same actors.
     *
     * @param InEditorEngine The engine the actor is spawned into and selected in.
     * @param GetLocation    Resolves where the actor is placed, evaluated when a row is activated.
     * @return The menu, or null when it could not be built.
     */
    NODISCARD static TSharedPtr<FMenu> BuildPlaceActorMenu(FEditorEngine* InEditorEngine, const FGetPlaceActorLocation& GetLocation);

private:
    void BuildFileMenu(const TSharedPtr<FMenuBar>& Bar);
    void BuildEditMenu(const TSharedPtr<FMenuBar>& Bar);
    void BuildWindowsMenu(const TSharedPtr<FMenuBar>& Bar);
    void BuildHelpMenu(const TSharedPtr<FMenuBar>& Bar);

    NODISCARD Vector3 GetPlaceActorLocation() const;

    FEditorEngine*                   EditorEngine;
    TSharedPtr<FEditorPanelRegistry> Registry;
    TSharedPtr<FMenuBar>             MenuBar;
    TArray<TSharedPtr<FMenuItem>>    WindowItems;
    TSharedPtr<FMenuItem>            PlayItem;
};
