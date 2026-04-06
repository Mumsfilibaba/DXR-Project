#include "Core/Stats/Stats.h"
#include "Core/Templates/CString.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "ImGuiPlugin/ImGuiExtensions.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "Engine/EngineUI/Editor/EditorStatsWidget.h"

static bool IsRHIGroup(const CHAR* GroupName)
{
    return FCString::Strcmp(GroupName, "RHI") == 0
        || FCString::Strcmp(GroupName, "RHI Budget") == 0
        || FCString::Strcmp(GroupName, "D3D12 Allocators") == 0
        || FCString::Strcmp(GroupName, "Vulkan Allocators") == 0
        || FCString::Strcmp(GroupName, "D3D12 PSO") == 0
        || FCString::Strcmp(GroupName, "Vulkan PSO") == 0;
}

FEditorStatsWidget::FEditorStatsWidget()
    : ImGuiDelegateHandle()
    , bVisible(false)
{
    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDrawDelegate(FImGuiDelegate::CreateRaw(this, &FEditorStatsWidget::Draw));
        CHECK(ImGuiDelegateHandle.IsValid());
    }
}

FEditorStatsWidget::~FEditorStatsWidget()
{
    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().RemoveDrawDelegate(ImGuiDelegateHandle);
    }
}

void FEditorStatsWidget::Draw()
{
    if (!bVisible)
    {
        return;
    }

    const ImVec2 FrameBufferScale = ImGuiExtensions::GetDisplayFramebufferScale();
    const float  Scale            = FrameBufferScale.x;

    ImGui::SetNextWindowSize(ImVec2(380.0f * Scale, 450.0f * Scale), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Engine Stats", &bVisible))
    {
        const FStatRegistry& Registry = FStatRegistry::Get();
        TArray<const CHAR*> Groups;
        Registry.GetGroups(Groups);

        const ImGuiTableFlags TableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;

        for (const CHAR* GroupName : Groups)
        {
            if (IsRHIGroup(GroupName))
            {
                continue;
            }

            if (!ImGui::CollapsingHeader(GroupName, ImGuiTreeNodeFlags_DefaultOpen))
            {
                continue;
            }

            TArray<FStatData*> GroupStats;
            Registry.GetStatsByGroup(GroupName, GroupStats);

            if (ImGui::BeginTable(GroupName, 2, TableFlags))
            {
                ImGui::TableSetupColumn("Stat",  ImGuiTableColumnFlags_WidthStretch, 0.6f);
                ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 0.4f);

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

    ImGui::End();
}
