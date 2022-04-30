#include "TextureDebugger.h"

#include "Core/Debug/Console/ConsoleManager.h"

#include "Application/ApplicationInstance.h"

#include <imgui.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Console-variable

TAutoConsoleVariable<bool> GDrawTextureDebugger("renderer.DrawTextureDebugger", false);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// TextureDebugWindow

TSharedRef<CTextureDebugWindow> CTextureDebugWindow::Make()
{
    return dbg_new CTextureDebugWindow();
}

void CTextureDebugWindow::Tick()
{
    if (GDrawTextureDebugger.GetBool())
    {
        // NOTE: This may need to be dynamic
        constexpr float InvAspectRatio = 16.0f / 9.0f;
        constexpr float AspectRatio = 9.0f / 16.0f;

        TSharedRef<CGenericWindow> MainViewport = CApplicationInstance::Get().GetMainViewport();

        const uint32 WindowWidth = MainViewport->GetWidth();
        const uint32 WindowHeight = MainViewport->GetHeight();

        const float Width = NMath::Max(WindowWidth * 0.6f, 400.0f);
        const float Height = WindowHeight * 0.75f;

        ImGui::SetNextWindowPos(ImVec2(float(WindowWidth) * 0.5f, float(WindowHeight) * 0.175f), ImGuiCond_Appearing, ImVec2(0.5f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(Width, Height), ImGuiCond_Appearing);

        const ImGuiWindowFlags Flags =
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoSavedSettings;

        bool TempDrawTextureDebugger = GDrawTextureDebugger.GetBool();
        if (ImGui::Begin("FrameBuffer Debugger", &TempDrawTextureDebugger, Flags))
        {
            ImGui::BeginChild("##ScrollBox", ImVec2(Width * 0.985f, Height * 0.125f), true, ImGuiWindowFlags_HorizontalScrollbar);

            const int32 Count = DebugTextures.Size();
            if (SelectedTextureIndex >= Count)
            {
                SelectedTextureIndex = -1;
            }

            for (int32 i = 0; i < Count; i++)
            {
                ImGui::PushID(i);

                constexpr float MenuImageSize = 96.0f;

                int32 FramePadding = 2;

                ImVec2 Size = ImVec2(MenuImageSize * InvAspectRatio, MenuImageSize);
                ImVec2 Uv0 = ImVec2(0.0f, 0.0f);
                ImVec2 Uv1 = ImVec2(1.0f, 1.0f);
                ImVec4 BgCol = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
                ImVec4 TintCol = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

                SInterfaceImage* CurrImage = &DebugTextures[i];
                if (ImGui::ImageButton(CurrImage, Size, Uv0, Uv1, FramePadding, BgCol, TintCol))
                {
                    SelectedTextureIndex = i;
                }

                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("%s", CurrImage->Image->GetName().CStr());
                }

                ImGui::PopID();

                if (i != Count - 1)
                {
                    ImGui::SameLine();
                }
            }

            ImGui::EndChild();

            const float ImageWidth = Width * 0.985f;
            const float ImageHeight = ImageWidth * AspectRatio;
            const int32 ImageIndex = (SelectedTextureIndex < 0) ? 0 : SelectedTextureIndex;

            SInterfaceImage* CurrImage = &DebugTextures[ImageIndex];
            ImGui::Image(CurrImage, ImVec2(ImageWidth, ImageHeight));
        }

        ImGui::End();

        GDrawTextureDebugger.SetBool(TempDrawTextureDebugger);
    }
}

bool CTextureDebugWindow::IsTickable()
{
    return GDrawTextureDebugger.GetBool();
}

void CTextureDebugWindow::AddTextureForDebugging(const TSharedRef<CRHIShaderResourceView>& ImageView, const TSharedRef<CRHITexture>& Image, ERHIResourceState BeforeState, ERHIResourceState AfterState)
{
    DebugTextures.Emplace(ImageView, Image, BeforeState, AfterState);
}
