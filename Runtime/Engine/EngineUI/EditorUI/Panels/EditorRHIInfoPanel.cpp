#include "Engine/EngineUI/EditorUI/Panels/EditorRHIInfoPanel.h"
#include "Engine/EngineUI/EditorUI/EditorStyle.h"
#include "Application/Elements/Box.h"
#include "Application/Elements/Expander.h"
#include "Application/Elements/ProgressBar.h"
#include "Application/Elements/PropertyTable.h"
#include "Application/Elements/ScrollBox.h"
#include "Application/Elements/TextBlock.h"
#include "RHI/RHI.h"
#include "RHI/RHIDevice.h"
#include "RHI/RHIStats.h"

static const CHAR* const GRHIDetailGroups[] =
{
    "RHI",
    "D3D12 Allocators",
    "Vulkan Allocators",
    "D3D12 PSO",
    "Vulkan PSO",
    "D3D12 Commands",
    "Vulkan Commands",
    "D3D12 Queries",
    "Vulkan Queries",
    "D3D12 Residency",
};

constexpr double BYTES_PER_MEGABYTE = 1024.0 * 1024.0;

static void QueryStatsByGroup(const CHAR* GroupName, TArray<FStatData*>& OutStats)
{
    OutStats.Clear();
    FStatRegistry::Get().GetStatsByGroup(GroupName, OutStats);
}

static String FormatStatValue(const FStatData& Stat)
{
    const int64 Value = Stat.Value.Load();
    if (Stat.Type == EStatType::Memory)
    {
        return String::Printf("%.2f MB", static_cast<double>(Value) / BYTES_PER_MEGABYTE);
    }

    return String::Printf("%lld", static_cast<long long>(Value));
}

static void UpdateMemoryBar(const TSharedPtr<FProgressBar>& Bar, int64 Usage, int64 Budget)
{
    const float Ratio = Budget > 0 ? static_cast<float>(static_cast<double>(Usage) / static_cast<double>(Budget)) : 0.0f;

    Bar->SetPercent(Ratio);
    Bar->SetOverlayText(String::Printf("%lld / %lld MB",
        static_cast<long long>(static_cast<double>(Usage) / BYTES_PER_MEGABYTE),
        static_cast<long long>(static_cast<double>(Budget) / BYTES_PER_MEGABYTE)));

    Bar->SetFillColor(Ratio > 0.9f
        ? FFloatColor(0.85f, 0.35f, 0.25f, 1.0f)
        : FUIStyle::GetDefault().Colors.Accent);
}

FEditorRHIInfoPanel::FEditorRHIInfoPanel(FEditorEngine* InEditorEngine)
    : FEditorPanel(InEditorEngine, "RHIInfo", "RHI Info")
    , AdapterText(nullptr)
    , LocalMemoryBar(nullptr)
    , NonLocalMemoryBar(nullptr)
    , DrawCallsText(nullptr)
    , DispatchCallsText(nullptr)
    , CommandsText(nullptr)
    , DetailsColumn(nullptr)
    , DetailSections()
    , ScratchStats()
    , ScratchGroups()
{
}

FEditorRHIInfoPanel::~FEditorRHIInfoPanel()
{
}

bool FEditorRHIInfoPanel::Initialize()
{
    TSharedPtr<FVerticalBox> Column = FVerticalBox::Create();

    if (!BuildOverview(Column))
    {
        return false;
    }

    if (!BuildCounters(Column))
    {
        return false;
    }

    DetailsColumn = FVerticalBox::Create();
    Column->AddSlot(DetailsColumn);

    TSharedPtr<FScrollBox> ScrollBox = FScrollBox::Create();
    if (!ScrollBox)
    {
        return false;
    }

    ScrollBox->SetContent(Column);

    Content = FEditorStyle::MakeInnerFrame(ScrollBox);

    RebuildDetailSections();
    return true;
}

bool FEditorRHIInfoPanel::BuildOverview(const TSharedPtr<FVerticalBox>& InColumn)
{
    TSharedPtr<FPropertyTable> Table = FPropertyTable::Create(FEditorStyle::MakeDataTableDesc());
    if (!Table)
    {
        return false;
    }

    const String AdapterName = RHI::Device ? RHI::Device->GetAdapterName() : String();
    AdapterText = CreateValueText(AdapterName.IsEmpty() ? String("Unknown") : AdapterName);

    LocalMemoryBar    = CreateMemoryBar();
    NonLocalMemoryBar = CreateMemoryBar();

    Table->AddRow("Adapter", AdapterText);
    Table->AddRow("Local Memory", LocalMemoryBar).ToolTipText = "Memory on the adapter itself, against the budget the driver grants this process";
    Table->AddRow("Non-Local Memory", NonLocalMemoryBar).ToolTipText = "System memory the adapter reads across the bus, against the budget the driver grants this process";

    InColumn->AddSlot(Table).SetPadding(FEditorStyle::GetSectionSpacing());
    return true;
}

bool FEditorRHIInfoPanel::BuildCounters(const TSharedPtr<FVerticalBox>& InColumn)
{
    TSharedPtr<FPropertyTable> Table = FPropertyTable::Create(FEditorStyle::MakeDataTableDesc());
    if (!Table)
    {
        return false;
    }

    DrawCallsText     = CreateValueText("0");
    DispatchCallsText = CreateValueText("0");
    CommandsText      = CreateValueText("0");

    Table->AddHeaderRow("Command Submission");
    Table->AddRow("Draw Calls", DrawCallsText);
    Table->AddRow("Dispatch Calls", DispatchCallsText);
    Table->AddRow("Commands", CommandsText);

    InColumn->AddSlot(Table).SetPadding(FEditorStyle::GetSectionSpacing());
    return true;
}

void FEditorRHIInfoPanel::Release()
{
    AdapterText.Reset();
    LocalMemoryBar.Reset();
    NonLocalMemoryBar.Reset();
    DrawCallsText.Reset();
    DispatchCallsText.Reset();
    CommandsText.Reset();
    DetailsColumn.Reset();
    DetailSections.Clear();
    ScratchStats.Clear();
    ScratchGroups.Clear();

    FEditorPanel::Release();
}

void FEditorRHIInfoPanel::Tick(float /*DeltaTime*/)
{
    if (!IsVisible())
    {
        return;
    }

    RefreshBudgets();
    RefreshCounters();
    RefreshDetailGroups();
}

void FEditorRHIInfoPanel::RefreshBudgets()
{
    UpdateMemoryBar(LocalMemoryBar, STAT_GET(STAT_RHI_LocalMemoryUsage), STAT_GET(STAT_RHI_LocalMemoryBudget));
    UpdateMemoryBar(NonLocalMemoryBar, STAT_GET(STAT_RHI_NonLocalMemoryUsage), STAT_GET(STAT_RHI_NonLocalMemoryBudget));
}

void FEditorRHIInfoPanel::RefreshCounters()
{
    DrawCallsText->SetText(String::Printf("%lld", static_cast<long long>(STAT_GET(STAT_RHI_DrawCalls))));
    DispatchCallsText->SetText(String::Printf("%lld", static_cast<long long>(STAT_GET(STAT_RHI_DispatchCalls))));
    CommandsText->SetText(String::Printf("%lld", static_cast<long long>(STAT_GET(STAT_RHI_Commands))));
}

void FEditorRHIInfoPanel::RefreshDetailGroups()
{
    CollectDetailGroups();

    bool bMatchesSections = ScratchGroups.Size() == DetailSections.Size();
    for (int32 Index = 0; bMatchesSections && Index < ScratchGroups.Size(); ++Index)
    {
        bMatchesSections = ScratchGroups[Index] == DetailSections[Index].GroupName;
    }

    if (!bMatchesSections)
    {
        RebuildDetailSections();
    }

    for (FStatGroupSection& GroupSection : DetailSections)
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

void FEditorRHIInfoPanel::CollectDetailGroups()
{
    ScratchGroups.Clear();

    for (const CHAR* GroupName : GRHIDetailGroups)
    {
        QueryStatsByGroup(GroupName, ScratchStats);

        if (ScratchStats.Size() > 0)
        {
            ScratchGroups.Emplace(GroupName);
        }
    }
}

void FEditorRHIInfoPanel::RebuildDetailSections()
{
    CollectDetailGroups();

    DetailsColumn->ClearSlots();
    DetailSections.Clear();
    DetailSections.Reserve(ScratchGroups.Size());

    for (const CHAR* GroupName : ScratchGroups)
    {
        FStatGroupSection GroupSection;
        GroupSection.GroupName = GroupName;
        GroupSection.Table     = FPropertyTable::Create(FEditorStyle::MakeDataTableDesc());
        GroupSection.Section   = FExpander::Create(FEditorStyle::MakeExpanderDesc(GroupName, GroupSection.Table, false));

        DetailsColumn->AddSlot(GroupSection.Section).SetPadding(FEditorStyle::GetSectionSpacing());
        DetailSections.Emplace(GroupSection);
    }

    for (FStatGroupSection& GroupSection : DetailSections)
    {
        QueryStatsByGroup(GroupSection.GroupName, ScratchStats);
        RebuildGroupRows(GroupSection);
    }
}

void FEditorRHIInfoPanel::RebuildGroupRows(FStatGroupSection& GroupSection)
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

TSharedPtr<FTextBlock> FEditorRHIInfoPanel::CreateValueText(const String& Text)
{
    FTextBlock::FDesc Desc;
    Desc.Text = Text;
    Desc.Font = FEditorStyle::GetFonts().Monospace;

    return FTextBlock::Create(Desc);
}

TSharedPtr<FProgressBar> FEditorRHIInfoPanel::CreateMemoryBar()
{
    FProgressBar::FDesc Desc;
    Desc.Font        = FEditorStyle::GetFonts().Body;
    Desc.OverlayText = "0 / 0 MB";

    return FProgressBar::Create(Desc);
}
