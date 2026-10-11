#include "Engine/EngineUI/EditorUI/Panels/EditorStatsPanel.h"
#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/Expander.h"
#include "Application/Elements/PropertyTable.h"
#include "Application/Elements/ScrollBox.h"
#include "Application/Elements/TextBlock.h"
#include "Core/Templates/CString.h"

static const CHAR_T* const GRHIOwnedGroups[] =
{
    TEXT("RHI"),
    TEXT("RHI Budget"),
    TEXT("D3D11 Resources"),
    TEXT("D3D12 Allocators"),
    TEXT("Vulkan Allocators"),
    TEXT("Metal Allocators"),
    TEXT("Metal Heaps"),
    TEXT("Metal Standalone"),
    TEXT("Metal Defrag"),
    TEXT("D3D11 PSO"),
    TEXT("D3D12 PSO"),
    TEXT("Vulkan PSO"),
    TEXT("Metal PSO"),
    TEXT("Metal Commands"),
    TEXT("Metal Queries"),
    TEXT("Metal GPU"),
    TEXT("Metal Residency"),
    TEXT("Metal Bindless"),
};

constexpr double BYTES_PER_MEGABYTE = 1024.0 * 1024.0;

static void QueryStatsByGroup(const CHAR_T* GroupName, TArray<FStatData*>& OutStats)
{
    OutStats.Clear();
    FStatRegistry::Get().GetStatsByGroup(GroupName, OutStats);
}

static bool IsRHIOwnedGroup(const CHAR_T* GroupName)
{
    for (const CHAR_T* OwnedGroup : GRHIOwnedGroups)
    {
        if (CString::Strcmp(GroupName, OwnedGroup) == 0)
        {
            return true;
        }
    }

    return false;
}

static FString FormatStatValue(const FStatData& Stat)
{
    const int64 Value = Stat.Value.Load();
    if (Stat.Type == EStatType::Memory)
    {
        return FString::Printf(TEXT("%.2f MB"), static_cast<double>(Value) / BYTES_PER_MEGABYTE);
    }

    return FString::Printf(TEXT("%lld"), static_cast<long long>(Value));
}

FEditorStatsPanel::FEditorStatsPanel(FEditorEngine* InEditorEngine)
    : FEditorPanel(InEditorEngine, TEXT("Stats"), TEXT("Engine Stats"))
    , Column(nullptr)
    , SummaryText(nullptr)
    , Sections()
    , ScratchStats()
    , ScratchGroups()
{
}

FEditorStatsPanel::~FEditorStatsPanel()
{
}

bool FEditorStatsPanel::Initialize()
{
    Column = FVerticalBox::Create();

    TSharedPtr<FScrollBox> ScrollBox = FScrollBox::Create();
    if (!ScrollBox)
    {
        return false;
    }

    ScrollBox->SetContent(Column);

    TSharedPtr<FVisualElement> HeaderCard = FEditorStyle::MakeHeaderCard(TEXT("Engine Stats"), FString(), &SummaryText);

    TSharedPtr<FVerticalBox> Layout = FVerticalBox::Create();
    Layout->AddSlot(HeaderCard).SetPadding(FMargin(0, 0, 0, FEditorStyle::ItemSpacing));
    Layout->AddSlot(ScrollBox).SetFillCoefficient(1.0f);

    Content = Layout;

    RebuildSections();
    return true;
}

void FEditorStatsPanel::Release()
{
    Column.Reset();
    SummaryText.Reset();
    Sections.Clear();
    ScratchStats.Clear();
    ScratchGroups.Clear();

    FEditorPanel::Release();
}

void FEditorStatsPanel::Tick(float /*DeltaTime*/)
{
    if (!IsVisible())
    {
        return;
    }

    CollectGroups();

    bool bMatchesSections = ScratchGroups.Size() == Sections.Size();
    for (int32 Index = 0; bMatchesSections && Index < ScratchGroups.Size(); ++Index)
    {
        bMatchesSections = CString::Strcmp(ScratchGroups[Index], Sections[Index].GroupName) == 0;
    }

    if (!bMatchesSections)
    {
        RebuildSections();
    }

    for (FStatGroupSection& GroupSection : Sections)
    {
        QueryStatsByGroup(GroupSection.GroupName, ScratchStats);

        if (ScratchStats.Size() != GroupSection.Values.Size())
        {
            RebuildGroupRows(GroupSection);
        }

        for (int32 Index = 0; Index < GroupSection.Values.Size(); ++Index)
        {
            GroupSection.Values[Index]->SetText(FormatStatValue(*ScratchStats[Index]));
        }
    }
}

void FEditorStatsPanel::CollectGroups()
{
    ScratchGroups.Clear();
    FStatRegistry::Get().GetGroups(ScratchGroups);

    for (int32 Index = ScratchGroups.Size() - 1; Index >= 0; --Index)
    {
        if (IsRHIOwnedGroup(ScratchGroups[Index]))
        {
            ScratchGroups.RemoveAt(Index);
        }
    }
}

void FEditorStatsPanel::RebuildSections()
{
    CollectGroups();

    Column->ClearSlots();
    Sections.Clear();
    Sections.Reserve(ScratchGroups.Size());

    for (const CHAR_T* GroupName : ScratchGroups)
    {
        FStatGroupSection GroupSection;
        GroupSection.GroupName = GroupName;
        GroupSection.Table     = FPropertyTable::Create(FEditorStyle::MakeInfoTableDesc());
        GroupSection.Section   = FExpander::Create(FEditorStyle::MakeExpanderDesc(GroupName, GroupSection.Table, true));

        Column->AddSlot(GroupSection.Section).SetPadding(FEditorStyle::GetSectionStackSpacing());
        Sections.Emplace(GroupSection);
    }

    int32 NumCounters = 0;
    for (FStatGroupSection& GroupSection : Sections)
    {
        QueryStatsByGroup(GroupSection.GroupName, ScratchStats);
        RebuildGroupRows(GroupSection);

        NumCounters += GroupSection.Values.Size();
    }

    SummaryText->SetText(FString::Printf(TEXT("%d groups  |  %d counters"), Sections.Size(), NumCounters));
}

void FEditorStatsPanel::RebuildGroupRows(FStatGroupSection& GroupSection)
{
    GroupSection.Table->ClearRows();
    GroupSection.Values.Clear();
    GroupSection.Values.Reserve(ScratchStats.Size());

    for (const FStatData* Stat : ScratchStats)
    {
        TSharedPtr<FTextBlock> Value = CreateValueText(FormatStatValue(*Stat));

        GroupSection.Table->AddRow(Stat->StatName, Value);
        GroupSection.Values.Emplace(Value);
    }
}

TSharedPtr<FTextBlock> FEditorStatsPanel::CreateValueText(const FString& Text)
{
    FTextBlock::FDesc Desc;
    Desc.Text = Text;
    Desc.Font = FEditorStyle::GetFonts().Monospace;

    return FTextBlock::Create(Desc);
}
