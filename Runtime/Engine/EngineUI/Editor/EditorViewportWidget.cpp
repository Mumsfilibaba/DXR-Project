#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Templates/CString.h"
#include "Application/Application.h"
#include "RHI/RHIResources.h" 
#include "Engine/Engine.h" 
#include "Engine/EngineUI/Editor/EditorGuizmo.h" 
#include "Engine/EngineUI/Editor/EditorViewportWidget.h" 
#include "RendererCore/Interfaces/IRendererModule.h" 
#include "ImGuiPlugin/ImGuiCore.h" 
#include "ImGuiPlugin/ImGuiRenderer.h" 

static TAutoConsoleVariable<bool> CVarDrawFps(
    "Engine.DrawFps",
    "Enable FPS counter in the viewport corner",
    false,
    EConsoleVariableFlags::Default);

FEditorViewportWidget::FEditorViewportWidget()
    : CachedViewportSize(0, 0)
    , ViewportImage()
    , ImGuiDelegateHandle()
    , bVisible(true)
    , bViewportInputActive(false)
{
    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDrawDelegate(FImGuiDelegate::CreateRaw(this, &FEditorViewportWidget::Draw));
        CHECK(ImGuiDelegateHandle.IsValid());
    }
}

FEditorViewportWidget::~FEditorViewportWidget()
{
    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().RemoveDrawDelegate(ImGuiDelegateHandle);
    }
}

void FEditorViewportWidget::Draw()
{
    if (!bVisible)
    {
        return;
    }

    if (FApplication::IsInitialized() && ViewportWidget)
    {
        bViewportInputActive = FApplication::Get().GetFocusLeafWidget() == ViewportWidget;
    }
    else
    {
        bViewportInputActive = false;
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    const ImGuiWindowFlags ViewportFlags =
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;

    if (ImGui::Begin("Viewport", &bVisible, ViewportFlags))
    {
        const bool bAnyMouseClick =
            ImGui::IsMouseClicked(ImGuiMouseButton_Left) ||
            ImGui::IsMouseClicked(ImGuiMouseButton_Right) ||
            ImGui::IsMouseClicked(ImGuiMouseButton_Middle);

        const bool bViewportWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
        if (bViewportInputActive && bAnyMouseClick && !bViewportWindowHovered)
        {
            if (FApplication::IsInitialized())
            {
                FApplication::Get().SetFocusWidget(nullptr);
            }

            bViewportInputActive = false;
        }

        // Update the relative viewport position
        const ImVec2 ContentPos = ImGui::GetCursorScreenPos();
        ViewportWidget->SetPosition(FIntVector2(static_cast<int32>(ContentPos.x), static_cast<int32>(ContentPos.y)), EViewportPositionSpace::Screen);

        // Update the viewport image that we will render to
        const ImVec2 ContentSize = ImGui::GetContentRegionAvail();
        CachedViewportSize = FIntVector2(static_cast<int32>(ContentSize.x), static_cast<int32>(ContentSize.y));
        ViewportWidget->SetSize(CachedViewportSize);

        // Draw the viewport texture
        ImGui::Image(&ViewportImage, ContentSize);
        const ImVec2 ImageMin  = ImGui::GetItemRectMin();
        const ImVec2 ImageSize = ImGui::GetItemRectSize();

        // ---------------------------------------------------------------------
        // Viewport activation
        // ---------------------------------------------------------------------

        ImGui::SetCursorScreenPos(ImageMin);

        const ImGuiButtonFlags ButtonFlags =
            ImGuiButtonFlags_MouseButtonLeft |
            ImGuiButtonFlags_MouseButtonRight |
            ImGuiButtonFlags_MouseButtonMiddle;

        ImGui::InvisibleButton("##ViewportInputArea", ImageSize, ButtonFlags);

        const bool bWasViewportInputActive = bViewportInputActive;
        const bool bClickedLeft            = ImGui::IsItemClicked(ImGuiMouseButton_Left);
        const bool bClickedRight           = ImGui::IsItemClicked(ImGuiMouseButton_Right);
        const bool bClickedMiddle          = ImGui::IsItemClicked(ImGuiMouseButton_Middle);
        const bool bAnyItemClick           = bClickedLeft || bClickedRight || bClickedMiddle;

        if (bAnyItemClick)
        {
            bViewportInputActive = true;

            if (ViewportWidget && FApplication::IsInitialized())
            {
                FApplication::Get().SetFocusWidget(ViewportWidget);
            }
        }

        // ---------------------------------------------------------------------
        // Editor picking (ObjectID readback request) 
        // --------------------------------------------------------------------- 
 
        // Only pick if the viewport was already active before this click. The first click just activates input. 
        const bool bBlockPickForGizmo = 
            EditorGuizmo::IsUsingAny() || 
            EditorGuizmo::IsOver() || 
            EditorGuizmo::IsUsingViewManipulate() || 
            EditorGuizmo::IsViewManipulateHovered(); 
 
        if (bWasViewportInputActive && bClickedLeft && !bBlockPickForGizmo) 
        { 
            const ImVec2 MousePos = ImGui::GetMousePos(); 
 
            const float LocalXf = MousePos.x - ImageMin.x; 
            const float LocalYf = MousePos.y - ImageMin.y; 

            if (LocalXf >= 0.0f && LocalYf >= 0.0f && LocalXf < ImageSize.x && LocalYf < ImageSize.y)
            {
                if (FEngine::IsInitialized())
                {
                    if (FWorld* World = FEngine::Get()->GetWorld())
                    {
                        if (IRendererModule* RendererModule = IRendererModule::Get())
                        {
                            const uint32 RenderWidth  = ViewportImage.Texture ? ViewportImage.Texture->GetWidth() : static_cast<uint32>(ContentSize.x);
                            const uint32 RenderHeight = ViewportImage.Texture ? ViewportImage.Texture->GetHeight() : static_cast<uint32>(ContentSize.y);

                            const float SafeW = ImageSize.x > 0.0f ? ImageSize.x : 1.0f;
                            const float SafeH = ImageSize.y > 0.0f ? ImageSize.y : 1.0f;

                            const float U = LocalXf / SafeW;
                            const float V = LocalYf / SafeH;

                            const uint32 PixelX = RenderWidth > 0 ? Math::Min(static_cast<uint32>(U * float(RenderWidth)), RenderWidth - 1) : 0;
                            const uint32 PixelY = RenderHeight > 0 ? Math::Min(static_cast<uint32>(V * float(RenderHeight)), RenderHeight - 1) : 0;
                            RendererModule->RequestEditorObjectPick(World->GetSceneInterface(), PixelX, PixelY);
                        }
                    }
                }
            }
        }

        // ---------------------------------------------------------------------
        // Active border
        // ---------------------------------------------------------------------
        
        if (bViewportInputActive)
        {
            ImDrawList* DrawList = ImGui::GetWindowDrawList();

            const ImU32 BorderColorU32  = IM_COL32(9, 92, 176, 255);
            const float BorderThickness = 2.0f;

            const ImVec2 BorderMin = ImVec2(ContentPos.x + 0.5f, ContentPos.y + 0.5f);
            const ImVec2 BorderMax = ImVec2(ContentPos.x + ContentSize.x - 0.5f, ContentPos.y + ContentSize.y - 0.5f);

            DrawList->AddRect(BorderMin, BorderMax, BorderColorU32, 0.0f, ImDrawListFlags_AntiAliasedLines, BorderThickness);
        }

        // ---------------------------------------------------------------------
        // FPS counter
        // ---------------------------------------------------------------------

        if (CVarDrawFps.GetValue())
        {
            char FpsText[16];
            FCString::Snprintf(FpsText, sizeof(FpsText), "%d", FFrameProfiler::Get().GetFramesPerSecond());

            ImDrawList* DrawList = ImGui::GetWindowDrawList();
            
            const ImFont* Font     = ImGui::GetFont();
            const ImVec2  TextSize = Font->CalcTextSizeA(Font->FontSize, FLT_MAX, 0.0f, FpsText);

            const float Margin  = 6.0f;
            const float Padding = 4.0f;

            const ImVec2 BoxMax  = ImVec2(ContentPos.x + ContentSize.x - Margin, ContentPos.y + Margin + TextSize.y + Padding * 2.0f);
            const ImVec2 BoxMin  = ImVec2(BoxMax.x - TextSize.x - Padding * 2.0f, ContentPos.y + Margin);
            const ImVec2 TextPos = ImVec2(BoxMin.x + Padding, BoxMin.y + Padding);

            DrawList->AddRectFilled(BoxMin, BoxMax, IM_COL32(32, 32, 32, 192), 0.0f);
            DrawList->AddText(TextPos, IM_COL32(0, 255, 50, 255), FpsText);
        }
    }

    ImGui::End();

    ImGui::PopStyleVar(); // WindowPadding
}

void FEditorViewportWidget::SetViewportWidget(const TSharedPtr<FViewportWidget>& InViewportWidget)
{
    ViewportWidget = InViewportWidget;
}

void FEditorViewportWidget::SetViewportImage(FRHITextureRef InViewportImage)
{
    if (InViewportImage)
    {
        ViewportImage.Texture              = InViewportImage;
        ViewportImage.View                 = MakeSharedRef<FRHIShaderResourceView>(InViewportImage->GetShaderResourceView());
        ViewportImage.ResourceState        = EResourceAccess::RenderTarget;
        ViewportImage.bEnableLinearSampler = false;
        ViewportImage.bEnableBlending      = false;
    }
}

FIntVector2 FEditorViewportWidget::GetViewportSize() const
{
    if (CachedViewportSize.X > 0 && CachedViewportSize.Y > 0)
    {
        return CachedViewportSize;
    }

    if (ImGuiWindow* ViewportWindow = ImGui::FindWindowByName("Viewport"))
    {
        const ImVec2 Size = ViewportWindow->ContentRegionRect.GetSize();
        return FIntVector2(static_cast<int32>(Size.x), static_cast<int32>(Size.y));
    }

    return FIntVector2(1920, 1080);
}
