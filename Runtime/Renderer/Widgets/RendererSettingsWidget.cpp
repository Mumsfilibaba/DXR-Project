#include "Core/Misc/ConsoleManager.h"
#include "Engine/Engine.h"
#include "Renderer/Widgets/RendererSettingsWidget.h"
#include "ImGuiPlugin/Interface/ImGuiPlugin.h"
#include "ImGuiPlugin/ImGuiExtensions.h"

static TAutoConsoleVariable<bool> CVarDrawSettingsWindow(
    "Renderer.DrawSettingsWindow",
    "Enables the Debug RenderTarget-viewer",
    false);

FRendererSettingsWidget::FRendererSettingsWidget()
    : ImGuiDelegateHandle()
{
    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDelegate(FImGuiDelegate::CreateRaw(this, &FRendererSettingsWidget::Draw));
        CHECK(ImGuiDelegateHandle.IsValid());
    }
}

FRendererSettingsWidget::~FRendererSettingsWidget()
{
    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().RemoveDelegate(ImGuiDelegateHandle);
    }
}

void FRendererSettingsWidget::Draw()
{
    bool bDrawSettingsWindow = CVarDrawSettingsWindow.GetValue();
    if (bDrawSettingsWindow)
    {
        const uint32 WindowWidth  = GEngine->GetEngineWindow()->GetWidth();
        const uint32 WindowHeight = GEngine->GetEngineWindow()->GetHeight();

        const float Width  = FMath::Max(WindowWidth * 0.3f, 400.0f);
        const float Height = WindowHeight * 0.7f;

        // Same column size for all different types
        const float ColumnWidth = 200.0f;

        ImGui::SetNextWindowPos(ImVec2(float(WindowWidth) * 0.5f, float(WindowHeight) * 0.175f), ImGuiCond_Appearing, ImVec2(0.5f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(Width, Height), ImGuiCond_Appearing);

        const ImGuiWindowFlags Flags = ImGuiWindowFlags_NoSavedSettings;
        if (ImGui::Begin("Renderer Settings", &bDrawSettingsWindow, Flags))
        {
            // Screen-Space Occlusion Settings
            if (ImGui::CollapsingHeader("SSAO", ImGuiTreeNodeFlags_None))
            {
                // Setup the columns
                ImGui::Columns(2, nullptr, false);
                ImGui::SetColumnWidth(0, ColumnWidth);

                // KernelSize
                if (IConsoleVariable* CVarKernelSize = FConsoleManager::Get().FindConsoleVariable("Renderer.SSAO.KernelSize"))
                {
                    ImGui::Text("KernelSize");
                    ImGui::NextColumn();

                    int32 KernelSize = CVarKernelSize->GetInt();
                    if (ImGui::SliderInt("##KernelSize", &KernelSize, 1, 128, "%d"))
                    {
                        CVarKernelSize->SetAsInt(KernelSize, EConsoleVariableFlags::SetByCode);
                    }
                    
                    ImGui::NextColumn();
                }
                
                // Bias
                if (IConsoleVariable* CVarBias = FConsoleManager::Get().FindConsoleVariable("Renderer.SSAO.Bias"))
                {
                    ImGui::Text("Bias");
                    ImGui::NextColumn();

                    float Bias = CVarBias->GetFloat();
                    if (ImGui::SliderFloat("##Bias", &Bias, 0.01f, 1.0f, "%.2f"))
                    {
                        CVarBias->SetAsFloat(Bias, EConsoleVariableFlags::SetByCode);
                    }
                    
                    ImGui::NextColumn();
                }

                // Reset the columns
                ImGui::Columns(1);
            }
            
            // Cascaded Shadow Maps
            if (ImGui::CollapsingHeader("Cascaded Shadow Maps", ImGuiTreeNodeFlags_None))
            {
            }
        }

        ImGui::End();

        CVarDrawSettingsWindow->SetAsBool(bDrawSettingsWindow, EConsoleVariableFlags::SetByCode);
    }
}
