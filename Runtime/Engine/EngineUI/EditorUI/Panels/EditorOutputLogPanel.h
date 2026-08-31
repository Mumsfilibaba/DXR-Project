#pragma once
#include "Core/Misc/IOutputDevice.h"
#include "Engine/EngineUI/EditorUI/EditorPanel.h"

class FComboBox;
class FLogView;
class FSearchBox;
class FToolBar;

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

    void OnSearchTextChanged(const String& SearchText);
    void OnSeverityChanged(ELogSeverity Severity);

    TSharedPtr<FLogView>   LogView;
    TSharedPtr<FSearchBox> SearchBox;
    TSharedPtr<FToolBar>   ToolBar;
};
