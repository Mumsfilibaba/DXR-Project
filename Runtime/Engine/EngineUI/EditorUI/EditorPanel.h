#pragma once
#include "Core/Containers/SharedPtr.h"
#include "Core/Containers/String.h"
#include "Application/Elements/VisualElement.h"

class FEditorEngine;

class ENGINE_API FEditorPanel
{
public:

    /**
     * @brief Constructs a panel with the identity a saved layout refers to it by.
     *
     * @param InEditorEngine The engine the panel reads its model from.
     * @param InPanelId      The stable id, which must not change once a layout has been written.
     * @param InLabel        The text its tab shows.
     */
    FEditorPanel(FEditorEngine* InEditorEngine, const String& InPanelId, const String& InLabel);
    virtual ~FEditorPanel();

    /**
     * @brief Builds the panel's element tree, which happens once before it is registered with the docking area.
     *
     * @return True when the tree was built.
     */
    virtual bool Initialize() = 0;

    /** @brief Drops the element tree and anything it registered with the engine. */
    virtual void Release();

    /**
     * @brief Refreshes whatever the panel reads from the model, once per frame.
     *
     * @param DeltaTime Seconds since the previous frame.
     */
    virtual void Tick(float DeltaTime);

    /**
     * @brief Drops any reference the panel holds to an actor the world is about to destroy.
     *
     * @param Actor The actor being removed.
     */
    virtual void OnActorRemoved(FActor* Actor);

    /**
     * @brief Starts or stops whatever the panel only needs while it is on screen, which is how the
     * profilers and the render graph capture avoid costing anything while their tab is hidden.
     *
     * @param bInIsVisible True when the panel became the front tab of a docked node.
     */
    virtual void OnVisibilityChanged(bool bInIsVisible);

    /** @return True while the panel is the front tab of its docked node, so its content is on screen. */
    NODISCARD FORCEINLINE bool IsVisible() const
    {
        return bIsVisible;
    }

    /** @return The stable id a saved layout refers to the panel by. */
    NODISCARD FORCEINLINE const String& GetPanelId() const
    {
        return PanelId;
    }

    /** @return The text the panel's tab shows. */
    NODISCARD FORCEINLINE const String& GetLabel() const
    {
        return Label;
    }

    /** @return The element the docking area places, which is null until Initialize has succeeded. */
    NODISCARD FORCEINLINE const TSharedPtr<FVisualElement>& GetContent() const
    {
        return Content;
    }

    /** @return True while the panel sits in the dock tree, which is what the Windows menu shows a check against. */
    NODISCARD FORCEINLINE bool IsOpen() const
    {
        return bIsOpen;
    }

    /**
     * @brief Records whether the panel is in the dock tree, which the docking area drives.
     *
     * @param bInIsOpen True when the tree holds it.
     */
    FORCEINLINE void SetOpen(bool bInIsOpen)
    {
        bIsOpen = bInIsOpen;
    }

protected:
    FEditorEngine*             EditorEngine;
    String                     PanelId;
    String                     Label;
    TSharedPtr<FVisualElement> Content;
    bool                       bIsOpen;
    bool                       bIsVisible;
};
