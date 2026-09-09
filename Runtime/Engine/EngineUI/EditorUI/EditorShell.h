#pragma once
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/String.h"
#include "Application/Docking/DockNode.h"

class FActor;
class FDockingArea;
class FDockInputHandler;
class FEditorEngine;
class FEditorFooterPanel;
class FEditorInputHandler;
class FEditorPanelRegistry;
class FEditorTitleBar;
class FEditorViewportPanel;
class FMenuInputHandler;
class FVerticalBox;

class ENGINE_API FEditorShell
{
public:

    /**
     * @brief Constructs the shell over the engine whose model its panels show.
     *
     * @param InEditorEngine The engine, which outlives the shell.
     */
    FEditorShell(FEditorEngine* InEditorEngine);
    ~FEditorShell();

    /**
     * @brief Loads the style and the icon atlas, builds the panels and puts the tree in the engine window.
     *
     * @return True when the whole tree was built.
     */
    bool Initialize();

    /**
     * @brief Loads whatever needs the renderer rather than the bare RHI, which is nothing today.
     *
     * @return True, always.
     */
    bool InitPostRenderer();

    /** @brief Writes the layout back, drops the tree and releases the style. */
    void Release();

    /**
     * @brief Refreshes every panel, once per frame.
     *
     * @param DeltaTime Seconds since the previous frame.
     */
    void Tick(float DeltaTime);

    /**
     * @brief Tells every panel an actor is about to be destroyed.
     *
     * @param Actor The actor being removed.
     */
    void OnActorRemoved(FActor* Actor);

    /** @return The viewport panel, which is what the engine drives through IEditorViewportHost. */
    NODISCARD const TSharedPtr<FEditorViewportPanel>& GetViewportPanel() const;

    /** @return The panel registry, which the Windows menu is built from. */
    NODISCARD FORCEINLINE const TSharedPtr<FEditorPanelRegistry>& GetRegistry() const
    {
        return Registry;
    }

    /** @return The docking area holding every panel. */
    NODISCARD FORCEINLINE const TSharedPtr<FDockingArea>& GetDockingArea() const
    {
        return DockingArea;
    }

private:
    static constexpr int32 PanelSetVersion = 2;

    NODISCARD static String GetLayoutFilename();
    NODISCARD static FDockNode BuildDefaultLayout();

    NODISCARD bool RestoreLayout();

    void SaveLayout();
    void OnPanelClosed(const String& PanelId);

    FEditorEngine*                   EditorEngine;
    TSharedPtr<FDockingArea>         DockingArea;
    TSharedPtr<FEditorPanelRegistry> Registry;
    TSharedPtr<FEditorTitleBar>      TitleBar;
    TSharedPtr<FEditorFooterPanel>   Footer;
    TSharedPtr<FVerticalBox>         Root;
    TSharedPtr<FMenuInputHandler>    MenuInputHandler;
    TSharedPtr<FDockInputHandler>    DockInputHandler;
    TSharedPtr<FEditorInputHandler>  EditorInputHandler;
    bool                             bStyleInitialized;
    bool                             bIconsInitialized;
};
