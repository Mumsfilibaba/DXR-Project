#include "TextureDebugger.h"
#include "Core/Misc/ConsoleManager.h"
#include "Application/Application.h"
#include "Application/ImGuiModule.h"

static TAutoConsoleVariable<bool> CVarDrawTextureDebugger(
    "Renderer.Debug.ViewRenderTargets",
    "Enables the Debug RenderTarget-viewer",
    false);

void FRenderTargetDebugWindow::Paint()
{
    if (CVarDrawTextureDebugger.GetValue())
    {
        const ImVec2 MainViewportPos = FImGui::GetMainViewportPos();
        const ImVec2 DisplaySize     = FImGui::GetMainViewportSize();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

        ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);
        ImGui::SetNextWindowPos(MainViewportPos);
        ImGui::SetNextWindowSize(DisplaySize);

        constexpr ImGuiWindowFlags Flags = 
            ImGuiWindowFlags_NoDecoration | 
            ImGuiWindowFlags_AlwaysAutoResize | 
            ImGuiWindowFlags_NoFocusOnAppearing | 
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoDocking;

        constexpr float MinImageSize = 96.0f;
        const float Width  = DisplaySize.x;
		const float Height = DisplaySize.y;

        bool bTempDrawTextureDebugger = CVarDrawTextureDebugger.GetValue();
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
                        ImageWidth  = FMath::Max(MinImageSize, FMath::Min(TexWidth, float(Width)));
                        ImageHeight = FMath::Max(MinImageSize, ImageWidth * AspectRatio);
                    }
                    else
                    {
                        ImageHeight = FMath::Max(MinImageSize, FMath::Min(TexHeight, float(Height)));
                        ImageWidth  = FMath::Max(MinImageSize, ImageHeight * InvAspectRatio);
                    }

                    {
                        const ImVec2 ImageSize     = ImVec2(ImageWidth, ImageHeight);
                        const ImVec2 ContentRegion = ImGui::GetContentRegionAvail();
                        const ImVec2 NewPosition   = ImVec2((ContentRegion.x - ImageSize.x) * 0.5f, (ContentRegion.y - ImageSize.y) * 0.5f);
                        ImGui::SetCursorPos(NewPosition);
                    }

                    CurrImage->bSamplerLinear = false;
                    ImGui::Image(CurrImage, ImVec2(ImageWidth, ImageHeight));
                }
            }
            
            // Draw Image menu on top
            ImGui::SetNextWindowPos(MainViewportPos);
            
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.0f, 2.0f));
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.05f, 0.05f, 0.05f, 0.95f));

            if (ImGui::BeginChild("##ScrollBox", ImVec2(196.0f, Height), false, ImGuiWindowFlags_None))
            {
                ImGui::NewLine();

                if (FImGui::ButtonCenteredOnLine("Close"))
                {
                    CVarDrawTextureDebugger->SetAsBool(false, EConsoleVariableFlags::SetByCode);
                }

                ImGui::Separator();

                const int32 Count = DebugTextures.Size();
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
                        ImGui::SetTooltip("%s", CurrImage->Texture->GetDebugName().GetCString());
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

void FRenderTargetDebugWindow::AddTextureForDebugging(const FRHIShaderResourceViewRef& ImageView, const FRHITextureRef& Image, EResourceAccess BeforeState, EResourceAccess AfterState)
{
    DebugTextures.Emplace(ImageView, Image, BeforeState, AfterState);
}
