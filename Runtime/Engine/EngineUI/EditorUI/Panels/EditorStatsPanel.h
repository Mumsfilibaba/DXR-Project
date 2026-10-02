#pragma once
#include "Core/Containers/Array.h"
#include "Core/Stats/Stats.h"
#include "Engine/EngineUI/EditorUI/EditorPanel.h"

class FExpander;
class FPropertyTable;
class FTextBlock;
class FVerticalBox;

class ENGINE_API FEditorStatsPanel final : public FEditorPanel
{
public:
    FEditorStatsPanel(FEditorEngine* InEditorEngine);
    virtual ~FEditorStatsPanel();

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

    NODISCARD static TSharedPtr<FTextBlock> CreateValueText(const String& Text);

    void RebuildSections();
    void RebuildGroupRows(FStatGroupSection& GroupSection);
    void CollectGroups();

    TSharedPtr<FVerticalBox>  Column;
    TSharedPtr<FTextBlock>    SummaryText;
    TArray<FStatGroupSection> Sections;
    TArray<FStatData*>        ScratchStats;
    TArray<const CHAR*>       ScratchGroups;
};
