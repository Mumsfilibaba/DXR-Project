#include "Core/Containers/StaticString.h"
#include "Core/Templates/CString.h"
#include "Core/Stats/Stats.h"
#include "RHI/RHI.h"
#include "RHI/RHIStats.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "ImGuiPlugin/ImGuiExtensions.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "Engine/EngineUI/Editor/EditorRHIInfoWidget.h"

FEditorRHIInfoWidget::FEditorRHIInfoWidget()
    : ImGuiDelegateHandle()
    , bVisible(false)
{
    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDrawDelegate(FImGuiDelegate::CreateRaw(this, &FEditorRHIInfoWidget::Draw));
        CHECK(ImGuiDelegateHandle.IsValid());
    }
}

FEditorRHIInfoWidget::~FEditorRHIInfoWidget()
{
    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().RemoveDrawDelegate(ImGuiDelegateHandle);
    }
}

void FEditorRHIInfoWidget::Draw()
{
    if (!bVisible)
    {
        return;
    }

    const ImVec2 FrameBufferScale = ImGuiExtensions::GetDisplayFramebufferScale();
    const float  Scale            = FrameBufferScale.x;

    ImGui::SetNextWindowSize(ImVec2(420.0f * Scale, 500.0f * Scale), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("RHI Info", &bVisible))
    {
        DrawAdapterInfo();
        DrawBudgetSection();
        DrawCommandSubmission();
        DrawResourceMemory();
        DrawAllocatorDetails();
    }

    ImGui::End();
}

void FEditorRHIInfoWidget::DrawAdapterInfo()
{
    ImGui::SeparatorText("Adapter");

    if (FRHIDevice* Device = RHI::Device)
    {
        const String AdapterName = Device->GetAdapterName();
        ImGui::Text("Adapter: %s", *AdapterName);
    }
}

void FEditorRHIInfoWidget::DrawBudgetSection()
{
    ImGui::SeparatorText("Memory Budget");

#if !STATS_ENABLED
    ImGui::TextUnformatted("Memory statistics are compiled out in this build configuration.");
#else
    const int64 LocalBudget = STAT_GET(STAT_RHI_LocalMemoryBudget);
    const int64 LocalUsage  = STAT_GET(STAT_RHI_LocalMemoryUsage);

    if (LocalBudget > 0)
    {
        char BudgetText[64];
        snprintf(BudgetText, sizeof(BudgetText), "%.2f MB / %.2f MB",
            static_cast<double>(LocalUsage) / (1024.0 * 1024.0),
            static_cast<double>(LocalBudget) / (1024.0 * 1024.0));

        const float Ratio = static_cast<float>(static_cast<double>(LocalUsage) / static_cast<double>(LocalBudget));
        ImGui::Text("Local Memory:");
        ImGui::ProgressBar(Ratio, ImVec2(-1.0f, 0.0f), BudgetText);
    }

    const int64 NonLocalBudget = STAT_GET(STAT_RHI_NonLocalMemoryBudget);
    const int64 NonLocalUsage  = STAT_GET(STAT_RHI_NonLocalMemoryUsage);

    char NonLocalText[64];
    snprintf(NonLocalText, sizeof(NonLocalText), "%.2f MB / %.2f MB",
        static_cast<double>(NonLocalUsage) / (1024.0 * 1024.0),
        static_cast<double>(NonLocalBudget) / (1024.0 * 1024.0));

    ImGui::Text("Non-Local Memory:");
    if (NonLocalBudget > 0)
    {
        const float Ratio = static_cast<float>(static_cast<double>(NonLocalUsage) / static_cast<double>(NonLocalBudget));
        ImGui::ProgressBar(Ratio, ImVec2(-1.0f, 0.0f), NonLocalText);
    }
    else
    {
        ImGui::Text("  %s", NonLocalText);
    }
#endif
}

void FEditorRHIInfoWidget::DrawCommandSubmission()
{
    ImGui::SeparatorText("Command Submission");

    const ImGuiTableFlags TableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;
    if (ImGui::BeginTable("CommandStats", 2, TableFlags))
    {
        ImGui::TableSetupColumn("Stat", ImGuiTableColumnFlags_WidthStretch, 0.6f);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.4f);

        ImGui::TableNextRow();
        ImGui::TableNextColumn(); ImGui::Text("Draw Calls");
        ImGui::TableNextColumn(); ImGui::Text("%lld", static_cast<long long>(STAT_GET(STAT_RHI_DrawCalls)));

        ImGui::TableNextRow();
        ImGui::TableNextColumn(); ImGui::Text("Dispatch Calls");
        ImGui::TableNextColumn(); ImGui::Text("%lld", static_cast<long long>(STAT_GET(STAT_RHI_DispatchCalls)));

        ImGui::TableNextRow();
        ImGui::TableNextColumn(); ImGui::Text("Commands");
        ImGui::TableNextColumn(); ImGui::Text("%lld", static_cast<long long>(STAT_GET(STAT_RHI_Commands)));

        ImGui::EndTable();
    }
}

void FEditorRHIInfoWidget::DrawResourceMemory()
{
    ImGui::SeparatorText("Resource Memory");

    const ImGuiTableFlags TableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;
    if (ImGui::BeginTable("ResourceMemory", 2, TableFlags))
    {
        ImGui::TableSetupColumn("Resource Type", ImGuiTableColumnFlags_WidthStretch, 0.6f);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthStretch, 0.4f);

        const FStatRegistry& Registry = FStatRegistry::Get();
        TArray<FStatData*> RHIStats;
        Registry.GetStatsByGroup("RHI", RHIStats);

        for (const FStatData* Stat : RHIStats)
        {
            if (Stat->Type != EStatType::Memory)
            {
                continue;
            }

            char SizeText[32];
            EditorHelpers::FormatBytes(Stat->Value.Load(), SizeText, sizeof(SizeText));

            ImGui::TableNextRow();
            ImGui::TableNextColumn(); ImGui::Text("%s", Stat->StatName);
            ImGui::TableNextColumn(); ImGui::Text("%s", SizeText);
        }

        ImGui::EndTable();
    }
}

void FEditorRHIInfoWidget::DrawAllocatorDetails()
{
    const FStatRegistry& Registry = FStatRegistry::Get();
    TArray<const CHAR*> Groups;
    Registry.GetGroups(Groups);

    for (const CHAR* GroupName : Groups)
    {
        const bool bIsRHIDetailGroup =
            (CString::Strcmp(GroupName, "D3D12 Allocators") == 0) ||
            (CString::Strcmp(GroupName, "Vulkan Allocators") == 0) ||
            (CString::Strcmp(GroupName, "D3D12 PSO") == 0) ||
            (CString::Strcmp(GroupName, "Vulkan PSO") == 0) ||
            (CString::Strcmp(GroupName, "D3D12 Commands") == 0) ||
            (CString::Strcmp(GroupName, "Vulkan Commands") == 0) ||
            (CString::Strcmp(GroupName, "D3D12 Queries") == 0) ||
            (CString::Strcmp(GroupName, "Vulkan Queries") == 0) ||
            (CString::Strcmp(GroupName, "D3D12 Residency") == 0);

        if (!bIsRHIDetailGroup)
        {
            continue;
        }

        ImGui::SeparatorText(GroupName);

        const ImGuiTableFlags TableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;
        if (ImGui::BeginTable(GroupName, 2, TableFlags))
        {
            ImGui::TableSetupColumn("Pool", ImGuiTableColumnFlags_WidthStretch, 0.6f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.4f);

            TArray<FStatData*> GroupStats;
            Registry.GetStatsByGroup(GroupName, GroupStats);
            for (const FStatData* Stat : GroupStats)
            {
                char ValueText[32];
                if (Stat->Type == EStatType::Memory)
                {
                    EditorHelpers::FormatBytes(Stat->Value.Load(), ValueText, sizeof(ValueText));
                }
                else
                {
                    snprintf(ValueText, sizeof(ValueText), "%lld", static_cast<long long>(Stat->Value.Load()));
                }

                ImGui::TableNextRow();
                ImGui::TableNextColumn(); ImGui::Text("%s", Stat->StatName);
                ImGui::TableNextColumn(); ImGui::Text("%s", ValueText);
            }

            ImGui::EndTable();
        }
    }
}
