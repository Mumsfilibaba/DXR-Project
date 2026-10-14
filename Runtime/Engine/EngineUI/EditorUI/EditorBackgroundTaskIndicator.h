#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/SharedPtr.h"
#include "Engine/EngineModule.h"

class FBackgroundTask;
class FHorizontalBox;
class FMenuAnchor;
class FProgressBar;
class FTextBlock;
class FVerticalBox;
class FVisualElement;

class FEditorBackgroundTaskIndicator
{
public:
    FEditorBackgroundTaskIndicator();
    ~FEditorBackgroundTaskIndicator();

    bool Initialize();

    /** @brief Show the background tasks running now, which the footer calls every frame */
    void Refresh();

    FORCEINLINE const TSharedPtr<FVisualElement>& GetElement() const
    {
        return Element;
    }

private:
    struct FTaskRow
    {
        TSharedPtr<FBackgroundTask> Task;
        TSharedPtr<FProgressBar>    Bar;
    };

    static void UpdateBar(const TSharedPtr<FProgressBar>& Bar, const TSharedPtr<FBackgroundTask>& Task);

    void RebuildTaskRows();

    TArray<TSharedPtr<FBackgroundTask>> Tasks;
    TArray<FTaskRow>                    TaskRows;
    TSharedPtr<FTextBlock>              Label;
    TSharedPtr<FProgressBar>            Bar;
    TSharedPtr<FVerticalBox>            TaskList;
    TSharedPtr<FMenuAnchor>             Anchor;
    TSharedPtr<FVisualElement>          Element;
};
