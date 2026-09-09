#pragma once
#include "Core/Misc/IOutputDevice.h"
#include "Engine/EngineUI/EditorUI/EditorPanel.h"
#include "Application/Elements/CompoundElement.h"

class FLogView;
class FMenuAnchor;
class FSearchBox;
class FToolBar;

/** @brief Called with where the log body was right-clicked, in screen coordinates. */
DECLARE_DELEGATE(FOnLogContextMenu, const IntVector2& /*ScreenPosition*/);

class ENGINE_API FEditorLogContextArea final : public FCompoundElement
{
public:

    /**
     * @brief Wraps the log body so a right-click on it reaches the panel, which the view itself does not
     * report.
     *
     * @param InContent       The element to wrap.
     * @param InOnContextMenu Fired with where the wrapper was right-clicked.
     * @return The wrapper, which takes the place of the element it was handed.
     */
    static TSharedPtr<FEditorLogContextArea> Create(const TSharedPtr<FVisualElement>& InContent, const FOnLogContextMenu& InOnContextMenu);

public:
    FEditorLogContextArea();
    virtual ~FEditorLogContextArea();

    // FVisualElement Interface
    virtual FEventResponse OnMouseButtonDown(const FCursorEvent& CursorEvent) override;

private:
    FOnLogContextMenu OnContextMenu;
};

class ENGINE_API FEditorOutputLogPanel final : public FEditorPanel
{
public:
    FEditorOutputLogPanel(FEditorEngine* InEditorEngine);
    virtual ~FEditorOutputLogPanel();

    // FEditorPanel Interface
    virtual bool Initialize() override final;
    virtual void Release() override final;

    /** @return How many lines the filters currently admit, which is what the footer counter shows. */
    NODISCARD int32 GetNumVisibleLines() const;

    /** @return The view the lines are held and drawn by, which the footer command line also echoes into. */
    NODISCARD FORCEINLINE const TSharedPtr<FLogView>& GetLogView() const
    {
        return LogView;
    }

    /** @return The field the filter text is typed into, which the Find shortcut hands the keyboard to. */
    NODISCARD FORCEINLINE const TSharedPtr<FSearchBox>& GetSearchBox() const
    {
        return SearchBox;
    }

private:
    NODISCARD TSharedPtr<FToolBar> BuildToolBar();
    NODISCARD TSharedPtr<FVisualElement> BuildFilterButton();
    NODISCARD TSharedPtr<FVisualElement> BuildFilterMenu();

    void OnSearchTextChanged(const String& SearchText);
    void OnLogContextMenu(const IntVector2& ScreenPosition);

    TSharedPtr<FLogView>              LogView;
    TSharedPtr<FSearchBox>            SearchBox;
    TSharedPtr<FToolBar>              ToolBar;
    TSharedPtr<FEditorLogContextArea> LogArea;
    TSharedPtr<FMenuAnchor>           FilterAnchor;
};
