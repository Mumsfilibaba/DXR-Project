#include "TextureDebugger.h"

#include "Core/Debug/Console/ConsoleManager.h"

#include "Application/ApplicationInterface.h"
#include "Application/WidgetUtilities.h"

#include <imgui.h>

TAutoConsoleVariable<bool> GDrawTextureDebugger("Renderer.Debug.ViewRenderTargets", false);

TSharedRef<FRenderTargetDebugWindow> FRenderTargetDebugWindow::Create()
{
    return dbg_new FRenderTargetDebugWindow();
}

void FRenderTargetDebugWindow::Tick()
{
    if (GDrawTextureDebugger.GetBool())
    {
        FGenericWindowRef MainViewport = FApplicationInterface::Get().GetMainViewport();

        const uint32 WindowWidth  = MainViewport->GetWidth();
        const uint32 WindowHeight = MainViewport->GetHeight();

        const float Width  = float(WindowWidth);
        const float Height = float(WindowHeight);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(Width, Height));

        constexpr ImGuiWindowFlags Flags =
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoSavedSettings;

        constexpr float MinImageSize = 96.0f;

        bool bTempDrawTextureDebugger = GDrawTextureDebugger.GetBool();
        if (ImGui::Begin("RenderTarget Debugger", &bTempDrawTextureDebugger, Flags))
        {
            const int32 ImageIndex = (SelectedTextureIndex < 0) ? 0 : SelectedTextureIndex;

            // Draw Image (Clamped to the window size)
            if (!DebugTextures.IsEmpty())
            {
                if (FDrawableTexture* CurrImage = &DebugTextures[ImageIndex])
                {
                    const float TexWidth       = float(CurrImage->Texture->GetWidth());
                    const float TexHeight      = float(CurrImage->Texture->GetHeight());
                    const float AspectRatio    = TexHeight / TexWidth;
                    const float InvAspectRatio = TexWidth / TexHeight;

                    float ImageWidth  = 0.0f;
                    float ImageHeight = 0.0f;
                    if (TexWidth > TexHeight)
                    {
                        ImageWidth  = NMath::Max(MinImageSize, NMath::Min(TexWidth, float(Width)));
                        ImageHeight = NMath::Max(MinImageSize, ImageWidth * AspectRatio);
                    }
                    else
                    {
                        ImageHeight = NMath::Max(MinImageSize, NMath::Min(TexHeight, float(Height)));
                        ImageWidth  = NMath::Max(MinImageSize, ImageHeight * InvAspectRatio);
                    }

                    {
                        const ImVec2 ImageSize     = ImVec2(ImageWidth, ImageHeight);
                        const ImVec2 ContentRegion = ImGui::GetContentRegionAvail();
                        const ImVec2 NewPosition   = ImVec2(
                            (ContentRegion.x - ImageSize.x) * 0.5f,
                            (ContentRegion.y - ImageSize.y) * 0.5f);
                        ImGui::SetCursorPos(NewPosition);
                    }

                    CurrImage->bSamplerLinear = false;
                    ImGui::Image(CurrImage, ImVec2(ImageWidth, ImageHeight));
                }
            }
            
            // Draw Image menu on top
            ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
            
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.0f, 2.0f));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.05f, 0.05f, 0.95f));

            if (ImGui::BeginChild("##ScrollBox", ImVec2(196.0f, Height), false, ImGuiWindowFlags_None))
            {
                ImGui::NewLine();

                if (ButtonCenteredOnLine("Close"))
                {
                    GDrawTextureDebugger.SetBool(false);
                }

                ImGui::Separator();

                const int32 Count = DebugTextures.GetSize();
                if (SelectedTextureIndex >= Count)
                {
                    SelectedTextureIndex = -1;
                }

                for (int32 Index = 0; Index < Count; ++Index)
                {
                    ImGui::PushID(Index);

                    constexpr float MenuImageSize = 96.0f;
                    constexpr int32 FramePadding  = 0;

                    FDrawableTexture* CurrImage = &DebugTextures[Index];

                    const float ImageRatio = float(CurrImage->Texture->GetWidth()) / float(CurrImage->Texture->GetHeight());
                    ImVec2 Size    = ImVec2(MenuImageSize * ImageRatio, MenuImageSize);
                    ImVec2 Uv0     = ImVec2(0.0f, 0.0f);
                    ImVec2 Uv1     = ImVec2(1.0f, 1.0f);
                    ImVec4 BgCol   = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
                    ImVec4 TintCol = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

                    {
                        const ImVec2 ContentRegion = ImGui::GetContentRegionAvail();
                        ImGui::SetCursorPosX((ContentRegion.x - Size.x) * 0.5f);
                    }

                    if (ImGui::ImageButton(CurrImage, Size, Uv0, Uv1, FramePadding, BgCol, TintCol))
                    {
                        SelectedTextureIndex = Index;
                    }

                    if (ImGui::IsItemHovered())
                    {
                        ImGui::SetTooltip("%s", CurrImage->Texture->GetName().GetCString());
                    }

                    ImGui::Separator();
                    ImGui::PopID();
                }
            }

            ImGui::EndChild();
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }
}

bool FRenderTargetDebugWindow::IsTickable()
{
    return GDrawTextureDebugger.GetBool();
}

void FRenderTargetDebugWindow::AddTextureForDebugging(const FRHIShaderResourceViewRef& ImageView, const FRHITextureRef& Image, EResourceAccess BeforeState, EResourceAccess AfterState)
{
    DebugTextures.Emplace(ImageView, Image, BeforeState, AfterState);
}
