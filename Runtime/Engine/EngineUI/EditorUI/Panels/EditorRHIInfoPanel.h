#pragma once
#include "Core/Containers/Array.h"
#include "Core/Stats/Stats.h"
#include "Engine/EngineUI/EditorUI/EditorPanel.h"

class FExpander;
class FProgressBar;
class FPropertyTable;
class FTextBlock;
class FVerticalBox;

class ENGINE_API FEditorRHIInfoPanel final : public FEditorPanel
{
public:
    FEditorRHIInfoPanel(FEditorEngine* InEditorEngine);
    virtual ~FEditorRHIInfoPanel();

    // FEditorPanel Interface
    virtual bool Initialize() override final;
    virtual void Release() override final;
    virtual void Tick(float DeltaTime) override final;

private:
    struct FStatGroupSection
    {
        const CHAR*                    GroupName;
        TSharedPtr<FExpander>          Section;
        TSharedPtr<FPropertyTable>     Table;
        TArray<TSharedPtr<FTextBlock>> Values;
    };

    NODISCARD bool BuildOverview(const TSharedPtr<FVerticalBox>& InColumn);
    NODISCARD bool BuildCounters(const TSharedPtr<FVerticalBox>& InColumn);

    void RefreshBudgets();
    void RefreshCounters();
    void RefreshDetailGroups();
    void RebuildGroupRows(FStatGroupSection& GroupSection);
    void RebuildDetailSections();
    void CollectDetailGroups();

    NODISCARD static TSharedPtr<FTextBlock> CreateValueText(const String& Text);
    NODISCARD static TSharedPtr<FProgressBar> CreateMemoryBar();

    TSharedPtr<FTextBlock>    AdapterText;
    TSharedPtr<FProgressBar>  LocalMemoryBar;
    TSharedPtr<FProgressBar>  NonLocalMemoryBar;
    TSharedPtr<FTextBlock>    DrawCallsText;
    TSharedPtr<FTextBlock>    DispatchCallsText;
    TSharedPtr<FTextBlock>    CommandsText;
    TSharedPtr<FVerticalBox>  DetailsColumn;
    TArray<FStatGroupSection> DetailSections;
    TArray<FStatData*>        ScratchStats;
    TArray<const CHAR*>       ScratchGroups;
};
