#include "Engine/EngineUI/EditorUI/EditorInputHandler.h"
#include "Engine/EngineUI/EditorUI/EditorPanelRegistry.h"
#include "Engine/EngineUI/EditorUI/Panels/EditorOutputLogPanel.h"
#include "Engine/EngineUI/EditorUI/Panels/EditorViewportPanel.h"
#include "Engine/EditorEngine.h"
#include "Application/Application.h"
#include "Application/Elements/EditableText.h"
#include "Application/Elements/SearchBox.h"
#include "Application/Input/Keys.h"

TSharedPtr<FEditorInputHandler> FEditorInputHandler::Register(FEditorEngine* InEditorEngine, const TSharedPtr<FEditorPanelRegistry>& InRegistry)
{
    TSharedPtr<FEditorInputHandler> InputHandler = MakeSharedPtr<FEditorInputHandler>(InEditorEngine, InRegistry);
    if (FApplication::IsInitialized())
    {
        FApplication::Get().RegisterInputHandler(InputHandler);
    }

    return InputHandler;
}

void FEditorInputHandler::Unregister(const TSharedPtr<FEditorInputHandler>& InputHandler)
{
    if (InputHandler && FApplication::IsInitialized())
    {
        FApplication::Get().UnregisterInputHandler(InputHandler);
    }
}

FEditorInputHandler::FEditorInputHandler(FEditorEngine* InEditorEngine, const TSharedPtr<FEditorPanelRegistry>& InRegistry)
    : EditorEngine(InEditorEngine)
    , Registry(InRegistry)
{
}

bool FEditorInputHandler::IsTypingIntoField()
{
    if (!FApplication::IsInitialized())
    {
        return false;
    }

    const TSharedPtr<FVisualElement> FocusElement = FApplication::Get().GetFocusElementLeaf();
    return FocusElement && FocusElement->WantsTextInput();
}

bool FEditorInputHandler::OnKeyDown(const FKeyEvent& KeyEvent)
{
    if (!EditorEngine || KeyEvent.IsRepeat())
    {
        return false;
    }

    const FKey Key         = KeyEvent.GetKey();
    const bool bIsShortcut = KeyEvent.GetModifierKeys().IsShortcutChordDown();

    if (Key == Keys::F5 || (bIsShortcut && Key == Keys::P))
    {
        if (const TSharedPtr<FEditorViewportPanel>& ViewportPanel = Registry ? Registry->GetViewportPanel() : nullptr)
        {
            ViewportPanel->TogglePlay();
        }

        return true;
    }

    if (IsTypingIntoField())
    {
        return false;
    }

    if (bIsShortcut && Key == Keys::F)
    {
        FocusOutputLogSearch();
        return true;
    }

    if (!bIsShortcut && Key == Keys::Delete)
    {
        DeleteSelection();
        return true;
    }

    return false;
}

void FEditorInputHandler::DeleteSelection()
{
    if (!EditorEngine->IsEditing())
    {
        return;
    }

    const TArray<FActor*> SelectedActors = EditorEngine->GetSelectedActors();
    if (SelectedActors.IsEmpty())
    {
        return;
    }

    EditorEngine->RequestDeleteActors(SelectedActors);
}

void FEditorInputHandler::FocusOutputLogSearch()
{
    if (!Registry || !FApplication::IsInitialized())
    {
        return;
    }

    const TSharedPtr<FEditorOutputLogPanel>& OutputLogPanel = Registry->GetOutputLogPanel();
    if (!OutputLogPanel)
    {
        return;
    }

    Registry->ShowPanel(OutputLogPanel->GetPanelId());

    if (const TSharedPtr<FSearchBox>& SearchBox = OutputLogPanel->GetSearchBox())
    {
        FApplication::Get().SetFocusElement(SearchBox->GetEditor());
    }
}
