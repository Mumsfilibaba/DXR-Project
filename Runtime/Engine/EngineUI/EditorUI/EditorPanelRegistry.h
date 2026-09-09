#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/String.h"

class FActor;
class FDockingArea;
class FEditorEngine;
class FEditorPanel;
class FEditorOutputLogPanel;
class FEditorViewportPanel;

class ENGINE_API FEditorPanelRegistry
{
public:

    /**
     * @brief Constructs a registry over a docking area, which every panel it builds is registered with.
     *
     * @param InEditorEngine The engine the panels read their model from.
     * @param InDockingArea  The area the panels are placed in.
     */
    FEditorPanelRegistry(FEditorEngine* InEditorEngine, const TSharedPtr<FDockingArea>& InDockingArea);
    ~FEditorPanelRegistry();

    /**
     * @brief Builds every panel and registers it with the docking area.
     *
     * @return True when every panel initialized.
     */
    bool RegisterAll();

    /** @brief Releases every panel and unregisters it. */
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

    /**
     * @brief Puts a panel back in the tree and brings its tab to the front, which is what the Windows menu does.
     *
     * @param PanelId The panel to show.
     */
    void ShowPanel(const String& PanelId);

    /**
     * @brief Records that a panel left the tree, which is what a closed tab reports.
     *
     * @param PanelId The panel that was closed.
     */
    void OnPanelClosed(const String& PanelId);

    /**
     * @brief Finds a panel by the id a saved layout refers to it by.
     *
     * @param PanelId The id to look for.
     * @return The panel, or null when no panel carries that id.
     */
    NODISCARD TSharedPtr<FEditorPanel> FindPanel(const String& PanelId) const;

    /** @return Every panel, in the order they were registered, which is the order the Windows menu lists them. */
    NODISCARD FORCEINLINE const TArray<TSharedPtr<FEditorPanel>>& GetPanels() const
    {
        return Panels;
    }

    /** @return The viewport panel, which the engine also talks to through IEditorViewportHost. */
    NODISCARD FORCEINLINE const TSharedPtr<FEditorViewportPanel>& GetViewportPanel() const
    {
        return ViewportPanel;
    }

    /** @return The output log panel, which the footer command line also writes into. */
    NODISCARD FORCEINLINE const TSharedPtr<FEditorOutputLogPanel>& GetOutputLogPanel() const
    {
        return OutputLogPanel;
    }

private:
    bool Add(const TSharedPtr<FEditorPanel>& Panel);

    NODISCARD bool IsPanelDocked(const String& PanelId) const;
    NODISCARD bool IsPanelVisible(const String& PanelId) const;

    FEditorEngine*                    EditorEngine;
    TSharedPtr<FDockingArea>          DockingArea;
    TArray<TSharedPtr<FEditorPanel>>  Panels;
    TSharedPtr<FEditorViewportPanel>  ViewportPanel;
    TSharedPtr<FEditorOutputLogPanel> OutputLogPanel;
};
