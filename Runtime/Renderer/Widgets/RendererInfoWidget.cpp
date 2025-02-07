#include "Core/Containers/StaticString.h"
#include "Core/Misc/ConsoleManager.h"
#include "RHI/RHI.h"
#include "Application/ApplicationInterface.h"
#include "Renderer/SceneRenderer.h"
#include "Renderer/Widgets/RendererInfoWidget.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "ImGuiPlugin/ImGuiExtensions.h"

static TAutoConsoleVariable<bool> CVarDrawRendererInfo(
    "Renderer.DrawRendererInfo",
    "Enables the drawing of the Renderer Info Window", 
    true);

FRendererInfoWidget::FRendererInfoWidget(FSceneRenderer* InRenderer)
    : Renderer(InRenderer)
    , ImGuiDelegateHandle()
{
    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDelegate(FImGuiDelegate::CreateRaw(this, &FRendererInfoWidget::Draw));
        CHECK(ImGuiDelegateHandle.IsValid());
    }
}

FRendererInfoWidget::~FRendererInfoWidget()
{
    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().RemoveDelegate(ImGuiDelegateHandle);
    }
}

void FRendererInfoWidget::Draw()
{
    bool bDrawRendererInfo = CVarDrawRendererInfo.GetValue();
    if (bDrawRendererInfo)
    {
        const FString AdapterName = RHIGetAdapterName();

        const ImVec2 MainViewportPos  = ImGuiExtensions::GetMainViewportPos();
        const ImVec2 DisplaySize      = ImGuiExtensions::GetDisplaySize();
        const ImVec2 TextSize         = ImGui::CalcTextSize(*AdapterName);
        const ImVec2 FrameBufferScale = ImGuiExtensions::GetDisplayFramebufferScale();

        const float WindowWidth  = DisplaySize.x;
        const float WindowHeight = DisplaySize.y;
        const float Scale        = FrameBufferScale.x;
        const float ColumnWidth  = 160.0f * Scale;
	    const float Width        = FMath::Max(TextSize.x + ColumnWidth + 15.0f * Scale, 300.0f * Scale);
	    const float Height       = WindowHeight * 0.25f;

        ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
        ImGui::SetNextWindowPos(ImVec2(MainViewportPos.x + WindowWidth, MainViewportPos.y + 10.0f * Scale), ImGuiCond_Once, ImVec2(1.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(Width, Height), ImGuiCond_Appearing);

        const ImGuiWindowFlags Flags = ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking;
        if (ImGui::Begin("Renderer Info", &bDrawRendererInfo, Flags))
        {
            // Viewport Info
            ImGui::SeparatorText("Viewport Info");

            ImGui::Columns(2, nullptr, false);
            ImGui::SetColumnWidth(0, ColumnWidth);

            ImGui::Text("Render Resolution:");
            ImGui::NextColumn();

            ImGui::Text("%u x %d", Renderer->GetRenderWidth(), Renderer->GetRenderHeight());

            ImGui::Columns(1);

            // Adapter Info
            ImGui::SeparatorText("Adapter Info");

            ImGui::Columns(2, nullptr, false);
            ImGui::SetColumnWidth(0, ColumnWidth);

            ImGui::Text("Adapter: ");
            ImGui::NextColumn();

            ImGui::Text("%s", *AdapterName);

            FRHIVideoMemoryInfo LocalMemoryInfo;
            if (GRHI->RHIQueryVideoMemoryInfo(EVideoMemoryType::Local, LocalMemoryInfo))
            {
                ImGui::NextColumn();

                ImGui::Text("Local Memory:");
                ImGui::NextColumn();

                const FStaticString<64> MemoryUsageText = FStaticString<64>::CreateFormatted("%.2f MB / %.2f MB",
                    LocalMemoryInfo.MemoryUsage / (1024.0f * 1024.0f),
                    LocalMemoryInfo.MemoryBudget / (1024.0f * 1024.0f));

                const float MemoryUsageRatio = (LocalMemoryInfo.MemoryBudget > 0.0f) ? static_cast<float>(LocalMemoryInfo.MemoryUsage) / static_cast<float>(LocalMemoryInfo.MemoryBudget) : 0.0f;
                ImGui::ProgressBar(MemoryUsageRatio, ImVec2(-1.0f, 0.0f), *MemoryUsageText);

            }

            FRHIVideoMemoryInfo NonLocalMemoryInfo;
            if (GRHI->RHIQueryVideoMemoryInfo(EVideoMemoryType::NonLocal, NonLocalMemoryInfo))
            {
                ImGui::NextColumn();

                ImGui::Text("Non-Local Memory:");
                ImGui::NextColumn();

                const FStaticString<64> MemoryUsageText = FStaticString<64>::CreateFormatted("%.2f MB / %.2f MB",
                    NonLocalMemoryInfo.MemoryUsage / (1024.0f * 1024.0f),
                    NonLocalMemoryInfo.MemoryBudget / (1024.0f * 1024.0f));

                const float MemoryUsageRatio = (NonLocalMemoryInfo.MemoryBudget > 0.0f) ? static_cast<float>(NonLocalMemoryInfo.MemoryUsage) / static_cast<float>(NonLocalMemoryInfo.MemoryBudget) : 0.0f;
                ImGui::ProgressBar(MemoryUsageRatio, ImVec2(-1.0f, 0.0f), *MemoryUsageText);
            }

            ImGui::Columns(1);

            // RHI Stats
            ImGui::SeparatorText("RHI Stats");

            ImGui::Columns(2, nullptr, false);
            ImGui::SetColumnWidth(0, ColumnWidth);

            ImGui::Text("DrawCalls: ");
            ImGui::NextColumn();

            ImGui::Text("%d", GRHINumDrawCalls.Load());
            ImGui::NextColumn();

            ImGui::Text("DispatchCalls: ");
            ImGui::NextColumn();

            ImGui::Text("%d", GRHINumDispatchCalls.Load());
            ImGui::NextColumn();

            ImGui::Text("Command Count: ");
            ImGui::NextColumn();

            ImGui::Text("%d", GRHINumCommands.Load());

            ImGui::Columns(1);

            ImGui::End();

            CVarDrawRendererInfo->SetAsBool(bDrawRendererInfo, EConsoleVariableFlags::SetByCode);
        }
    }
}
