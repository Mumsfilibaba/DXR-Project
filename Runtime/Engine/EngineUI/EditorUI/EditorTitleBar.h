#pragma once
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/UniquePtr.h"

class FEditorEngine;
class FEditorMenus;
class FEditorPanelRegistry;
class FTitleBar;
class FVisualElement;

class ENGINE_API FEditorTitleBar
{
public:

    /**
     * @brief Constructs the caption over the engine it names and the registry its Windows menu lists.
     *
     * @param InEditorEngine The engine, which the caption reads the world name from.
     * @param InRegistry     The registry the Windows menu is built from.
     */
    FEditorTitleBar(FEditorEngine* InEditorEngine, const TSharedPtr<FEditorPanelRegistry>& InRegistry);
    ~FEditorTitleBar();

    /**
     * @brief Builds the caption and the menu bar inside it.
     *
     * @return True when the caption was built.
     */
    bool Initialize();

    /** @brief Re-reads the caption text and the state every checkable menu row shows. */
    void Refresh();

    /** @return The element the shell places at the top of the window, null until Initialize has succeeded. */
    NODISCARD const TSharedPtr<FVisualElement>& GetElement() const;

private:
    FEditorEngine*             EditorEngine;
    TUniquePtr<FEditorMenus>   Menus;
    TSharedPtr<FTitleBar>      Bar;
    TSharedPtr<FVisualElement> Element;
    bool                       bWasPlaying;
};
