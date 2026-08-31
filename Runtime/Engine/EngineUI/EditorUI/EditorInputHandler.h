#pragma once
#include "Core/Containers/SharedPtr.h"
#include "Application/InputHandler.h"

class FEditorEngine;
class FEditorPanelRegistry;

class ENGINE_API FEditorInputHandler final : public FInputHandler
{
public:

    /**
     * @brief Creates the handler and registers it with the application.
     *
     * @param InEditorEngine The engine the shortcuts act on, which outlives the handler.
     * @param InRegistry     The panels the shortcuts reach, which outlives the handler.
     * @return The handler, which is only registered when an application exists to register it with.
     */
    static TSharedPtr<FEditorInputHandler> Register(FEditorEngine* InEditorEngine, const TSharedPtr<FEditorPanelRegistry>& InRegistry);

    /**
     * @brief Unregisters a handler, after which none of the chords do anything.
     *
     * @param InputHandler The handler to remove.
     */
    static void Unregister(const TSharedPtr<FEditorInputHandler>& InputHandler);

public:
    FEditorInputHandler(FEditorEngine* InEditorEngine, const TSharedPtr<FEditorPanelRegistry>& InRegistry);
    virtual ~FEditorInputHandler() = default;

    // FInputHandler Interface
    virtual bool OnKeyDown(const FKeyEvent& KeyEvent) override final;

private:
    NODISCARD static bool IsTypingIntoField();

    void DeleteSelection();
    void FocusOutputLogSearch();

    FEditorEngine*                   EditorEngine;
    TSharedPtr<FEditorPanelRegistry> Registry;
};
