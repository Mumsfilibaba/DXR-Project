#include "Core/Containers/StaticArray.h"
#include "Core/Math/Math.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Templates/CString.h"
#include "Application/Application.h"
#include "RHI/RHIResources.h" 
#include "Engine/Engine.h" 
#include "Engine/EditorEngine.h"
#include "Engine/EngineUI/Editor/EditorGuizmo.h" 
#include "Engine/EngineUI/Editor/EditorCameraController.h"
#include "Engine/EngineUI/Editor/EditorViewportWidget.h" 
#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "Engine/EngineUI/Editor/EditorActorFactory.h"
#include "Engine/World/Components/CameraComponent.h"
#include "Engine/World/SceneViewport.h"
#include "Engine/World/World.h"
#include "RendererCore/Interfaces/IRendererModule.h" 
#include "ImGuiPlugin/ImGuiCore.h" 
#include "ImGuiPlugin/ImGuiRenderer.h" 

static TAutoConsoleVariable<bool> CVarDrawFps(
    "Engine.DrawFps",
    "Enable FPS counter in the viewport corner",
    false,
    EConsoleVariableFlags::Default);

constexpr float SPEED_OVERLAY_HOLD_TIME = 0.9f;
constexpr float SPEED_OVERLAY_FADE_TIME = 0.35f;

constexpr const CHAR* VIEWPORT_CONTEXT_MENU_ID = "ViewportContextMenu";

FEditorViewportWidget::FEditorViewportWidget(FEditorEngine* InEditorEngine)
    : EditorEngine(InEditorEngine)
    , CameraController(MakeUniquePtr<FEditorCameraController>())
    , CachedViewportSize(0, 0)
    , ViewportImage()
    , ImGuiDelegateHandle()
    , bVisible(true)
    , bViewportInputActive(false)
    , bMouseLookActive(false)
    , bRawLookActive(false)
    , bCursorWasVisible(true)
    , MouseLookRestorePosition()
    , PendingCameraInput()
    , SpeedOverlayTimer(0.0f)
    , bRightMousePressedOnImage(false)
    , RightMouseDragDistance(0.0f)
    , ContextMenuPickRequestId(0)
    , ContextMenuNdc(0.0f, 0.0f)
    , ContextMenuViewProjectionInverse()
    , ContextMenuLocation(0.0f, 0.0f, 0.0f)
    , ContextMenuActor(nullptr)
    , DebugView(FSceneRenderView::EDebugView::None)
    , SecondaryDebugView(FSceneRenderView::EDebugView::None)
    , DebugViewChannelMask(FSceneRenderView::EDebugViewChannel::All)
    , GizmoPlacement(EGizmoPlacement::Center)
    , GizmoOrientation(EditorGuizmo::EMode::World)
    , GizmoOperation(EditorGuizmo::EOperation::Translate)
{
    CHECK(EditorEngine != nullptr);

    if (const TSharedPtr<FSceneViewport> SceneViewport = EditorEngine->GetSceneViewport())
    {
        SceneViewport->SetPlayerInputEnabled(false);
    }

    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDrawDelegate(FImGuiDelegate::CreateRaw(this, &FEditorViewportWidget::Draw));
        CHECK(ImGuiDelegateHandle.IsValid());
    }
}

FEditorViewportWidget::~FEditorViewportWidget()
{
    EndMouseLook();

    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().RemoveDrawDelegate(ImGuiDelegateHandle);
    }
}

void FEditorViewportWidget::Draw()
{
    if (!bVisible)
    {
        EndMouseLook();
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

        // ---------------------------------------------------------------------
        // Viewport toolbar
        // ---------------------------------------------------------------------

        {
            const ImGuiWindowFlags ToolbarFlags =
                ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse;

            const float ButtonHeight  = ImGui::GetFontSize() + 8.0f;
            const float ToolbarHeight = ButtonHeight + 12.0f;

            const ImVec4 ToolbarBg = ImVec4(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f);
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ToolbarBg);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

            if (ImGui::BeginChild("##ViewportToolbar", ImVec2(0.0f, ToolbarHeight), ImGuiChildFlags_None, ToolbarFlags))
            {
                struct FDebugItem
                {
                    const CHAR* Label;
                    FSceneRenderView::EDebugView View;
                };

                static const FDebugItem BaseViewItems[] =
                {
                    { "Lit",                FSceneRenderView::EDebugView::None },
                    { "Shadow Mask",        FSceneRenderView::EDebugView::ShadowMask },
                    { "GBuffer: Albedo",    FSceneRenderView::EDebugView::GBufferAlbedo },
                    { "GBuffer: Normal",    FSceneRenderView::EDebugView::GBufferNormal },
                    { "GBuffer: Material",  FSceneRenderView::EDebugView::GBufferMaterial },
                    { "GBuffer: Velocity",  FSceneRenderView::EDebugView::GBufferVelocity },
                    { "SSAO",               FSceneRenderView::EDebugView::SSAO },
                    { "Depth",              FSceneRenderView::EDebugView::Depth },
                };

                static const FDebugItem ShadowDebugItems[] =
                {
                    { "CSM Cascades (2x2)",   FSceneRenderView::EDebugView::ShadowCascades },
                    { "CSM Cascade Index",    FSceneRenderView::EDebugView::ShadowCascadeIndex },
                    { "CSM Cascade Overlay",  FSceneRenderView::EDebugView::ShadowCascadeOverlay },
                };

                static const FDebugItem RayTracingDebugItems[] =
                {
                    { "Reflections Radiance",        FSceneRenderView::EDebugView::RayTracingReflectionsRaw },
                    { "Reflections Temporal Filter", FSceneRenderView::EDebugView::RayTracingReflectionsTemporal },
                    { "Reflections Spatial Filter",  FSceneRenderView::EDebugView::RayTracingReflectionsSpatial },
                    { "Reflections Variance",        FSceneRenderView::EDebugView::RayTracingReflectionsVariance },
                    { "Reflections History Length",  FSceneRenderView::EDebugView::RayTracingReflectionsHistory },
                    { "Geometry Debug",              FSceneRenderView::EDebugView::RayTracingPrimaryID },
                };

                const auto FindDebugLabel = [&](FSceneRenderView::EDebugView InView) -> const CHAR*
                {
                    for (const FDebugItem& Item : BaseViewItems)
                    {
                        if (Item.View == InView)
                        {
                            return Item.Label;
                        }
                    }

                    for (const FDebugItem& Item : ShadowDebugItems)
                    {
                        if (Item.View == InView)
                        {
                            return Item.Label;
                        }
                    }

                    for (const FDebugItem& Item : RayTracingDebugItems)
                    {
                        if (Item.View == InView)
                        {
                            return Item.Label;
                        }
                    }

                    return "Lit";
                };

                const float LabelGapFromCircle = 16.0f;

                const auto DrawRadioMenuItem = [&](const CHAR* Label, bool bSelected, bool bEnabled, bool bCloseOnSelect, bool* OutHovered) -> bool
                {
                    const float PaddingY    = 4.0f;
                    const float RowHeight   = ImGui::GetFontSize() + PaddingY * 2.0f;
                    const float RowWidth    = ImGui::GetContentRegionAvail().x;
                    const float MenuIndentX = 20.0f;

                    ImGui::PushID(Label);

                    if (!bEnabled)
                    {
                        ImGui::BeginDisabled();
                    }

                    const ImGuiSelectableFlags Flags =
                        ImGuiSelectableFlags_SpanAvailWidth |
                        ImGuiSelectableFlags_NoPadWithHalfSpacing;

                    const bool bPressed = ImGui::Selectable("##row", false, Flags, ImVec2(RowWidth, RowHeight));
                    const bool bHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
                    const bool bActive  = ImGui::IsItemActive();

                    if (OutHovered)
                    {
                        *OutHovered = bHovered;
                    }

                    const ImVec2 RectMin = ImGui::GetItemRectMin();
                    const ImVec2 RectMax = ImGui::GetItemRectMax();

                    ImDrawList* DrawList = ImGui::GetWindowDrawList();

                    ImU32 Background = ImGui::GetColorU32(ImGuiCol_Header);
                    if (bActive)
                    {
                        Background = ImGui::GetColorU32(ImGuiCol_HeaderActive);
                    }
                    else if (bHovered)
                    {
                        Background = ImGui::GetColorU32(ImGuiCol_HeaderHovered);
                    }

                    DrawList->AddRectFilled(RectMin, RectMax, Background, 0.0f);

                    const float CircleX = RectMin.x + MenuIndentX + 6.0f;
                    const float CircleY = RectMin.y + RowHeight * 0.5f;

                    const ImU32 SelectedColor   = IM_COL32(255, 255, 255, 255);
                    const ImU32 UnselectedColor = IM_COL32(15, 15, 15, 255);

                    if (bSelected)
                    {
                        DrawList->AddCircleFilled(ImVec2(CircleX, CircleY), 4.5f, SelectedColor);
                        DrawList->AddCircle(ImVec2(CircleX, CircleY), 5.5f, SelectedColor, 0, 1.0f);
                    }
                    else
                    {
                        DrawList->AddCircleFilled(ImVec2(CircleX, CircleY), 4.5f, UnselectedColor);
                    }

                    const ImVec2 LabelSize = ImGui::CalcTextSize(Label);

                    const float LabelX = CircleX + LabelGapFromCircle;
                    const float LabelY = RectMin.y + (RowHeight - LabelSize.y) * 0.5f;

                    DrawList->AddText(ImVec2(LabelX, LabelY), ImGui::GetColorU32(ImGuiCol_Text), Label);

                    if (!bEnabled)
                    {
                        ImGui::EndDisabled();
                    }

                    ImGui::PopID();

                    if (bPressed && bEnabled && bCloseOnSelect)
                    {
                        ImGui::CloseCurrentPopup();
                    }

                    return bPressed && bEnabled;
                };

                const CHAR* CurrentLabel = FindDebugLabel(DebugView);
                TStaticArray<CHAR, 128> MenuLabel{};

                const float ArrowIconSize  = 14.0f;
                const float TextArrowGap   = 6.0f;
                const float MaxButtonWidth = 128.0f;
                const float RightPadding   = 12.0f;

                float MaxLabelWidth = 0.0f;
                for (const FDebugItem& Item : BaseViewItems)
                {
                    MaxLabelWidth = Math::Max(MaxLabelWidth, ImGui::CalcTextSize(Item.Label).x);
                }
                for (const FDebugItem& Item : ShadowDebugItems)
                {
                    MaxLabelWidth = Math::Max(MaxLabelWidth, ImGui::CalcTextSize(Item.Label).x);
                }
                for (const FDebugItem& Item : RayTracingDebugItems)
                {
                    MaxLabelWidth = Math::Max(MaxLabelWidth, ImGui::CalcTextSize(Item.Label).x);
                }

                const float PaddingX           = ImGui::GetStyle().FramePadding.x;
                const float DesiredButtonWidth = MaxLabelWidth + TextArrowGap + ArrowIconSize + PaddingX * 2.0f;
                const float ButtonWidth        = Math::Min(DesiredButtonWidth, MaxButtonWidth);
                const float MaxTextWidth       = Math::Max(0.0f, ButtonWidth - (PaddingX * 2.0f + TextArrowGap + ArrowIconSize));

                const auto BuildClampedLabel = [&](const CHAR* InLabel, float MaxWidth, TStaticArray<CHAR, 128>& OutBuffer) -> const CHAR*
                {
                    if (!InLabel)
                    {
                        OutBuffer[0] = '\0';
                        return OutBuffer.Data();
                    }

                    if (ImGui::CalcTextSize(InLabel).x <= MaxWidth)
                    {
                        CString::Snprintf(OutBuffer.Data(), static_cast<int32>(OutBuffer.Size()), "%s", InLabel);
                        return OutBuffer.Data();
                    }

                    const CHAR* Ellipsis = "...";

                    const float EllipsisW = ImGui::CalcTextSize(Ellipsis).x;
                    const float Budget    = Math::Max(0.0f, MaxWidth - EllipsisW);

                    int32 Count = 0;
                    float Width = 0.0f;

                    while (InLabel[Count] != '\0')
                    {
                        CHAR Ch[2] = { InLabel[Count], '\0' };

                        const float ChW = ImGui::CalcTextSize(Ch).x;
                        if (Width + ChW > Budget)
                        {
                            break;
                        }

                        Width += ChW;
                        ++Count;
                    }

                    if (Count <= 0)
                    {
                        OutBuffer[0] = '\0';
                        return OutBuffer.Data();
                    }

                    const int32 MaxCopy = Math::Min<int32>(Count, static_cast<int32>(OutBuffer.Size()) - 4);
                    for (int32 i = 0; i < MaxCopy; ++i)
                    {
                        OutBuffer[i] = InLabel[i];
                    }

                    OutBuffer[MaxCopy] = '\0';
                    CString::Strcat(OutBuffer.Data(), Ellipsis);

                    return OutBuffer.Data();
                };

                const CHAR* MenuLabelText = BuildClampedLabel(CurrentLabel, MaxTextWidth, MenuLabel);

                const ImVec2 ChildPos               = ImGui::GetWindowPos();
                const ImVec2 ContentMin             = ImGui::GetWindowContentRegionMin();
                const ImVec2 ContentMax             = ImGui::GetWindowContentRegionMax();
                const float  ContentWidth           = ContentMax.x - ContentMin.x;
                const float  TranslationButtonWidth = 76.0f;
                const float  RotateButtonWidth      = 56.0f;
                const float  ScaleButtonWidth       = 52.0f;
                const float  PlacementButtonWidth   = 56.0f;
                const float  OrientationButtonWidth = 52.0f;
                const float  CameraButtonWidth      = 118.0f;
                const float  ToolbarControlGap      = 8.0f;
                const float  LeftPadding            = 12.0f;
                const float  ViewModeCursorX        = ContentMin.x + Math::Max(0.0f, ContentWidth - ButtonWidth - RightPadding);
                const float  CameraCursorX          = ViewModeCursorX - ToolbarControlGap - CameraButtonWidth;
                const float  CursorY                = ContentMin.y + 6.0f;

                ImGui::SetCursorScreenPos(ImVec2(ChildPos.x + ContentMin.x + LeftPadding, ChildPos.y + CursorY));

                const CHAR* CameraMenuPopupId     = "##ViewportCameraMenu";
                const CHAR* ViewMenuPopupId       = "##ViewportViewModeMenu";
                const CHAR* ShadowMenuPopupId     = "##ViewportShadowMenu";
                const CHAR* RayTracingMenuPopupId = "##ViewportRayTracingMenu";
                const CHAR* SecondaryMenuPopupId  = "##ViewportSecondaryMenu";

                const ImGuiPopupFlags PopupQueryFlags = ImGuiPopupFlags_AnyPopupLevel;

                const bool bCameraPopupOpen     = ImGui::IsPopupOpen(ImGui::GetID(CameraMenuPopupId), PopupQueryFlags);
                const bool bViewPopupOpen       = ImGui::IsPopupOpen(ImGui::GetID(ViewMenuPopupId), PopupQueryFlags);
                const bool bShadowPopupOpen     = ImGui::IsPopupOpen(ImGui::GetID(ShadowMenuPopupId), PopupQueryFlags);
                const bool bRayTracingPopupOpen = ImGui::IsPopupOpen(ImGui::GetID(RayTracingMenuPopupId), PopupQueryFlags);
                const bool bSecondaryPopupOpen  = ImGui::IsPopupOpen(ImGui::GetID(SecondaryMenuPopupId), PopupQueryFlags);
                const bool bAnyPopupOpen        = bCameraPopupOpen || bViewPopupOpen || bShadowPopupOpen || bRayTracingPopupOpen || bSecondaryPopupOpen;

                PopupAnchor CameraMenuAnchor;
                PopupAnchor ViewMenuAnchor;
                const auto DrawToolbarMenuButton = [&](const CHAR* Id, const CHAR* Label, float Width, bool bPopupOpen, PopupAnchor& OutAnchor, bool& bOutHovered) -> bool
                {
                    const float ButtonHeightLocal = ButtonHeight;

                    const ImVec2 ButtonSize = ImVec2(Width, ButtonHeight);

                    const bool bPressed = ImGui::InvisibleButton(Id, ButtonSize);
                    const bool bHovered = ImGui::IsItemHovered();
                    const bool bHeld    = ImGui::IsItemActive();

                    const ImVec2 Min = ImGui::GetItemRectMin();
                    const ImVec2 Max = ImGui::GetItemRectMax();

                    const ImU32 BgIdle  = IM_COL32(56, 56, 56, 255);
                    const ImU32 BgHover = IM_COL32(87, 87, 87, 255);

                    ImU32 Bg = BgIdle;
                    if (bPopupOpen || bHeld || bHovered)
                    {
                        Bg = BgHover;
                    }

                    ImDrawList* DrawList = ImGui::GetWindowDrawList();
                    DrawList->AddRectFilled(Min, Max, Bg, 6.0f);

                    const ImVec2 ButtonLabelSize = ImGui::CalcTextSize(Label);
                    const float TextY = Min.y + (ButtonHeightLocal - ButtonLabelSize.y) * 0.5f;
                    const float TextX = Min.x + ImGui::GetStyle().FramePadding.x;

                    DrawList->AddText(ImVec2(TextX, TextY), ImGui::GetColorU32(ImGuiCol_Text), Label);

                    const float  ArrowX   = Max.x - ImGui::GetStyle().FramePadding.x - ArrowIconSize;
                    const float  ArrowY   = Min.y + (ButtonHeightLocal - ArrowIconSize) * 0.5f;
                    const ImVec2 ArrowMin = ImVec2(ArrowX, ArrowY);
                    const ImVec2 ArrowMax = ImVec2(ArrowX + ArrowIconSize, ArrowY + ArrowIconSize);

                    if (EditorIcons::DownArrowIcon)
                    {
                        DrawList->AddImage(EditorIcons::DownArrowIcon, ArrowMin, ArrowMax, ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImGuiCol_Text));
                    }

                    OutAnchor.Min              = Min;
                    OutAnchor.Max              = Max;
                    OutAnchor.bRequestPosition = false;
                    bOutHovered                = bHovered;

                    return bPressed;
                };

                const auto DrawGizmoToggle = [&](const CHAR* Id, const CHAR* Label, bool bSelected, float Width, ImDrawFlags Corners) -> bool
                {
                    const bool bPressed = ImGui::InvisibleButton(Id, ImVec2(Width, ButtonHeight));
                    const bool bHovered = ImGui::IsItemHovered();
                    const bool bHeld    = ImGui::IsItemActive();

                    const ImVec2 Min = ImGui::GetItemRectMin();
                    const ImVec2 Max = ImGui::GetItemRectMax();

                    const ImU32 BgIdle          = IM_COL32(56, 56, 56, 255);
                    const ImU32 BgHover         = IM_COL32(87, 87, 87, 255);
                    const ImU32 BgSelected      = IM_COL32(9, 92, 176, 255);
                    const ImU32 BgSelectedHover = IM_COL32(15, 110, 205, 255);

                    ImU32 Bg = bSelected ? BgSelected : BgIdle;
                    if (bHovered || bHeld)
                    {
                        Bg = bSelected ? BgSelectedHover : BgHover;
                    }

                    ImDrawList* DrawList = ImGui::GetWindowDrawList();
                    DrawList->AddRectFilled(Min, Max, Bg, 6.0f, Corners);

                    const ImVec2 TextSize = ImGui::CalcTextSize(Label);
                    const ImVec2 TextPos  = ImVec2(Min.x + (Width - TextSize.x) * 0.5f, Min.y + (ButtonHeight - TextSize.y) * 0.5f);

                    DrawList->AddText(TextPos, ImGui::GetColorU32(ImGuiCol_Text), Label);

                    return bPressed;
                };

                if (DrawGizmoToggle("##GizmoOperationTranslation", "Translation", GizmoOperation == EditorGuizmo::EOperation::Translate, TranslationButtonWidth, ImDrawFlags_RoundCornersLeft))
                {
                    GizmoOperation = EditorGuizmo::EOperation::Translate;
                }

                ImGui::SameLine(0.0f, 0.0f);
                if (DrawGizmoToggle("##GizmoOperationRotate", "Rotate", GizmoOperation == EditorGuizmo::EOperation::Rotate, RotateButtonWidth, ImDrawFlags_RoundCornersNone))
                {
                    GizmoOperation = EditorGuizmo::EOperation::Rotate;
                }

                ImGui::SameLine(0.0f, 0.0f);
                if (DrawGizmoToggle("##GizmoOperationScale", "Scale", GizmoOperation == EditorGuizmo::EOperation::Scale, ScaleButtonWidth, ImDrawFlags_RoundCornersRight))
                {
                    GizmoOperation = EditorGuizmo::EOperation::Scale;
                }

                ImGui::SameLine(0.0f, ToolbarControlGap);
                if (DrawGizmoToggle("##GizmoPlacementCenter", "Center", GizmoPlacement == EGizmoPlacement::Center, PlacementButtonWidth, ImDrawFlags_RoundCornersLeft))
                {
                    GizmoPlacement = EGizmoPlacement::Center;
                }

                ImGui::SameLine(0.0f, 0.0f);
                if (DrawGizmoToggle("##GizmoPlacementPivot", "Pivot", GizmoPlacement == EGizmoPlacement::Pivot, PlacementButtonWidth, ImDrawFlags_RoundCornersRight))
                {
                    GizmoPlacement = EGizmoPlacement::Pivot;
                }

                ImGui::SameLine(0.0f, ToolbarControlGap);
                if (DrawGizmoToggle("##GizmoOrientationLocal", "Local", GizmoOrientation == EditorGuizmo::EMode::Local, OrientationButtonWidth, ImDrawFlags_RoundCornersLeft))
                {
                    GizmoOrientation = EditorGuizmo::EMode::Local;
                }

                ImGui::SameLine(0.0f, 0.0f);
                if (DrawGizmoToggle("##GizmoOrientationWorld", "World", GizmoOrientation == EditorGuizmo::EMode::World, OrientationButtonWidth, ImDrawFlags_RoundCornersRight))
                {
                    GizmoOrientation = EditorGuizmo::EMode::World;
                }

                ImGui::SetCursorScreenPos(ImVec2(ChildPos.x + CameraCursorX, ChildPos.y + CursorY));

                bool bCameraHovered = false;
                const bool bCameraPressed = DrawToolbarMenuButton(
                    "##ViewportCameraButton",
                    "Camera",
                    CameraButtonWidth,
                    bCameraPopupOpen,
                    CameraMenuAnchor,
                    bCameraHovered);

                if (bCameraPressed || (bAnyPopupOpen && bCameraHovered))
                {
                    ImGui::OpenPopup(CameraMenuPopupId);
                    CameraMenuAnchor.bRequestPosition = true;
                }

                if (EditorWidgets::BeginMenuPopup(CameraMenuPopupId, CameraMenuAnchor, 280.0f))
                {
                    FEditorCameraController* Controller = CameraController.Get();

                    if (Controller)
                    {
                        EditorWidgets::MenuLabeledSeparator("NAVIGATION");

                        float MoveSpeed = Controller->GetMoveSpeed();
                        if (EditorWidgets::MenuSliderFloat("Move Speed", MoveSpeed, FEditorCameraController::MinMoveSpeed, FEditorCameraController::MaxMoveSpeed, "%.1f"))
                        {
                            Controller->SetMoveSpeed(MoveSpeed);
                        }

                        float RotationSpeed = Controller->GetRotationSpeed();
                        if (EditorWidgets::MenuSliderFloat("Rotation Speed", RotationSpeed, 1.0f, 360.0f, "%.0f"))
                        {
                            Controller->SetRotationSpeed(RotationSpeed);
                        }

                        float MouseSensitivity = Controller->GetMouseSensitivity();
                        if (EditorWidgets::MenuSliderFloat("Mouse Sensitivity", MouseSensitivity, 0.01f, 2.0f, "%.2f"))
                        {
                            Controller->SetMouseSensitivity(MouseSensitivity);
                        }

                        float PanSpeed = Controller->GetPanSpeed();
                        if (EditorWidgets::MenuSliderFloat("Pan Speed", PanSpeed, 0.0002f, 0.02f, "%.4f"))
                        {
                            Controller->SetPanSpeed(PanSpeed);
                        }

                        EditorWidgets::MenuLabeledSeparator("LENS");

                        float FieldOfView = Controller->GetFieldOfView();
                        if (EditorWidgets::MenuSliderFloat("Field of View", FieldOfView, 30.0f, 120.0f, "%.0f deg"))
                        {
                            Controller->SetFieldOfView(FieldOfView);
                        }

                        float NearPlane = Controller->GetNearPlane();
                        if (EditorWidgets::MenuDragFloat("Near Plane", NearPlane, 0.001f, 0.001f, 10.0f, "%.3f"))
                        {
                            Controller->SetNearPlane(NearPlane);
                        }

                        float FarPlane = Controller->GetFarPlane();
                        if (EditorWidgets::MenuDragFloat("Far Plane", FarPlane, 1.0f, 1.0f, 100000.0f, "%.1f"))
                        {
                            Controller->SetFarPlane(FarPlane);
                        }

                        EditorWidgets::MenuLabeledSeparator("ACTIONS");

                        if (EditorWidgets::MenuItem("Reset Camera", "R"))
                        {
                            Controller->Reset();
                        }

                        FActor* SelectedActor = EditorEngine ? EditorEngine->GetSelectedActor() : nullptr;
                        if (EditorWidgets::MenuItem("Focus Selected", "F", false, SelectedActor != nullptr))
                        {
                            Controller->FocusOn(SelectedActor);
                        }

                        if (EditorWidgets::MenuItem("Attach To Selected", nullptr, false, SelectedActor != nullptr))
                        {
                            Controller->AttachTo(SelectedActor);
                        }

                        if (EditorWidgets::MenuItem("Detach", nullptr, false, Controller->IsAttached()))
                        {
                            Controller->Detach();
                        }

                        EditorWidgets::MenuLabeledSeparator("CONTROLS");

                        EditorWidgets::MenuItem("Look", "RMB Drag", false, false);
                        EditorWidgets::MenuItem("Fly", "RMB + WASDQE", false, false);
                        EditorWidgets::MenuItem("Boost", "Shift", false, false);
                        EditorWidgets::MenuItem("Zoom", "Wheel", false, false);
                        EditorWidgets::MenuItem("Fly Speed", "Alt + Wheel", false, false);
                        EditorWidgets::MenuItem("Orbit", "Alt + LMB", false, false);
                        EditorWidgets::MenuItem("Dolly", "Alt + RMB", false, false);
                    #if PLATFORM_MACOS
                        EditorWidgets::MenuItem("Pan", "Alt + MMB / Alt + Cmd + LMB", false, false);
                    #else
                        EditorWidgets::MenuItem("Pan", "Alt + MMB", false, false);
                    #endif
                    }

                    EditorWidgets::EndMenuPopup();
                }

                ImGui::SetCursorScreenPos(ImVec2(ChildPos.x + ViewModeCursorX, ChildPos.y + CursorY));

                bool bViewHovered = false;

                const bool bViewPressed = DrawToolbarMenuButton(
                    "##ViewportViewModeButton",
                    MenuLabelText,
                    ButtonWidth,
                    bViewPopupOpen,
                    ViewMenuAnchor,
                    bViewHovered);
                if (bViewPressed || (bAnyPopupOpen && bViewHovered))
                {
                    ImGui::OpenPopup(ViewMenuPopupId);
                    ViewMenuAnchor.bRequestPosition = true;
                }

                if (EditorWidgets::BeginMenuPopup(ViewMenuPopupId, ViewMenuAnchor, 240.0f))
                {
                    EditorWidgets::MenuLabeledSeparator("VIEW MODE");

                    bool bRequestClosePopup = false;

                    for (const FDebugItem& Item : BaseViewItems)
                    {
                        if (DrawRadioMenuItem(Item.Label, DebugView == Item.View, true, false, nullptr))
                        {
                            DebugView          = Item.View;
                            bRequestClosePopup = true;
                        }
                    }

                    {
                        FSubMenuState ShadowSubMenu;
                        ShadowSubMenu.LabelIndentX = MenuRadioLabelIndentX;
                        ShadowSubMenu.MinWidth     = 240.0f;

                        if (EditorWidgets::BeginSubMenu(ShadowSubMenu, ShadowMenuPopupId, "Shadow Debug"))
                        {
                            for (const FDebugItem& Item : ShadowDebugItems)
                            {
                                if (DrawRadioMenuItem(Item.Label, DebugView == Item.View, true, true, nullptr))
                                {
                                    DebugView          = Item.View;
                                    bRequestClosePopup = true;
                                }
                            }

                            EditorWidgets::EndSubMenu(ShadowSubMenu);
                        }
                    }

                    {
                        FSubMenuState RayTracingSubMenu;
                        RayTracingSubMenu.LabelIndentX = MenuRadioLabelIndentX;
                        RayTracingSubMenu.MinWidth     = 240.0f;

                        if (EditorWidgets::BeginSubMenu(RayTracingSubMenu, RayTracingMenuPopupId, "Ray Tracing"))
                        {
                            for (const FDebugItem& Item : RayTracingDebugItems)
                            {
                                if (DrawRadioMenuItem(Item.Label, DebugView == Item.View, true, true, nullptr))
                                {
                                    DebugView          = Item.View;
                                    bRequestClosePopup = true;
                                }
                            }

                            EditorWidgets::EndSubMenu(RayTracingSubMenu);
                        }
                    }

                    {
                        FSubMenuState SecondarySubMenu;
                        SecondarySubMenu.LabelIndentX = MenuRadioLabelIndentX;
                        SecondarySubMenu.MinWidth     = 240.0f;

                        if (EditorWidgets::BeginSubMenu(SecondarySubMenu, SecondaryMenuPopupId, "Secondary View"))
                        {
                            if (DrawRadioMenuItem("None", SecondaryDebugView == FSceneRenderView::EDebugView::None, true, true, nullptr))
                            {
                                SecondaryDebugView = FSceneRenderView::EDebugView::None;
                                bRequestClosePopup = true;
                            }

                            EditorWidgets::MenuSeparator();

                            for (const FDebugItem& Item : BaseViewItems)
                            {
                                const bool bIsLitEntry = (Item.View == FSceneRenderView::EDebugView::None);
                                const FSceneRenderView::EDebugView TargetView = bIsLitEntry ? FSceneRenderView::EDebugView::Lit : Item.View;
                                if (DrawRadioMenuItem(Item.Label, SecondaryDebugView == TargetView, true, true, nullptr))
                                {
                                    SecondaryDebugView = TargetView;
                                    bRequestClosePopup = true;
                                }
                            }

                            for (const FDebugItem& Item : ShadowDebugItems)
                            {
                                if (DrawRadioMenuItem(Item.Label, SecondaryDebugView == Item.View, true, true, nullptr))
                                {
                                    SecondaryDebugView = Item.View;
                                    bRequestClosePopup = true;
                                }
                            }

                            for (const FDebugItem& Item : RayTracingDebugItems)
                            {
                                if (DrawRadioMenuItem(Item.Label, SecondaryDebugView == Item.View, true, true, nullptr))
                                {
                                    SecondaryDebugView = Item.View;
                                    bRequestClosePopup = true;
                                }
                            }

                            EditorWidgets::EndSubMenu(SecondarySubMenu);
                        }
                    }

                    {
                        EditorWidgets::MenuLabeledSeparator("CHANNELS");

                        // Isolating a component only means anything once a debug view is being drawn
                        const bool bChannelsEnabled = DebugView != FSceneRenderView::EDebugView::None;
                        if (!bChannelsEnabled)
                        {
                            ImGui::BeginDisabled();
                        }

                        const float ChannelIndentX     = 20.0f;
                        const float ChannelRowWidth    = ImGui::GetContentRegionAvail().x - (ChannelIndentX * 2.0f);
                        const float ChannelButtonWidth = Math::Max(Math::Floor(ChannelRowWidth * 0.25f), 1.0f);
                        const float ChannelHeight      = ImGui::GetFrameHeight();

                        const auto DrawChannelToggle = [&](const CHAR* Id, const CHAR* Label, FSceneRenderView::EDebugViewChannel Channel, ImDrawFlags Corners)
                        {
                            const bool bSelected = IsEnumFlagSet(DebugViewChannelMask, Channel);

                            const bool bPressed = ImGui::InvisibleButton(Id, ImVec2(ChannelButtonWidth, ChannelHeight));
                            const bool bHovered = ImGui::IsItemHovered();
                            const bool bHeld    = ImGui::IsItemActive();

                            const ImVec2 Min = ImGui::GetItemRectMin();
                            const ImVec2 Max = ImGui::GetItemRectMax();

                            const ImU32 BgIdle          = IM_COL32(56, 56, 56, 255);
                            const ImU32 BgHover         = IM_COL32(87, 87, 87, 255);
                            const ImU32 BgSelected      = IM_COL32(9, 92, 176, 255);
                            const ImU32 BgSelectedHover = IM_COL32(15, 110, 205, 255);

                            ImU32 Bg = bSelected ? BgSelected : BgIdle;
                            if (bHovered || bHeld)
                            {
                                Bg = bSelected ? BgSelectedHover : BgHover;
                            }

                            ImDrawList* DrawList = ImGui::GetWindowDrawList();

                            // Routed through GetColorU32 so the row fades with the rest of the menu while it is disabled
                            DrawList->AddRectFilled(Min, Max, ImGui::GetColorU32(Bg), 6.0f, Corners);

                            const ImVec2 TextSize = ImGui::CalcTextSize(Label);
                            const ImVec2 TextPos  = ImVec2(Min.x + (ChannelButtonWidth - TextSize.x) * 0.5f, Min.y + (ChannelHeight - TextSize.y) * 0.5f);

                            DrawList->AddText(TextPos, ImGui::GetColorU32(ImGuiCol_Text), Label);

                            if (bPressed)
                            {
                                DebugViewChannelMask ^= Channel;
                            }
                        };

                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ChannelIndentX);

                        DrawChannelToggle("##DebugViewChannelR", "R", FSceneRenderView::EDebugViewChannel::Red, ImDrawFlags_RoundCornersLeft);

                        ImGui::SameLine(0.0f, 0.0f);
                        DrawChannelToggle("##DebugViewChannelG", "G", FSceneRenderView::EDebugViewChannel::Green, ImDrawFlags_RoundCornersNone);

                        ImGui::SameLine(0.0f, 0.0f);
                        DrawChannelToggle("##DebugViewChannelB", "B", FSceneRenderView::EDebugViewChannel::Blue, ImDrawFlags_RoundCornersNone);

                        ImGui::SameLine(0.0f, 0.0f);
                        DrawChannelToggle("##DebugViewChannelA", "A", FSceneRenderView::EDebugViewChannel::Alpha, ImDrawFlags_RoundCornersRight);

                        if (!bChannelsEnabled)
                        {
                            ImGui::EndDisabled();
                        }
                    }

                    if (bRequestClosePopup)
                    {
                        ImGui::CloseCurrentPopup();
                    }

                    EditorWidgets::EndMenuPopup();
                }
            }

            ImGui::EndChild();

            const float ItemSpacingY = ImGui::GetStyle().ItemSpacing.y;
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - ItemSpacingY);
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
        }

        // Update the relative viewport position
        const ImVec2 ContentPos = ImGui::GetCursorScreenPos();
        ViewportWidget->SetPosition(IntVector2(static_cast<int32>(ContentPos.x), static_cast<int32>(ContentPos.y)), EViewportPositionSpace::Screen);

        // Update the viewport image that we will render to
        const ImVec2 ContentSize = ImGui::GetContentRegionAvail();
        CachedViewportSize = IntVector2(static_cast<int32>(ContentSize.x), static_cast<int32>(ContentSize.y));
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
        const bool bViewportImageHovered   = ImGui::IsItemHovered();
        const bool bViewportImageActive    = ImGui::IsItemActive();

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
 
        const bool bBlockPickForGizmo = 
            EditorGuizmo::IsUsingAny() || 
            EditorGuizmo::IsOver() || 
            EditorGuizmo::IsUsingViewManipulate() || 
            EditorGuizmo::IsViewManipulateHovered(); 

        PendingCameraInput = FEditorCameraInputState();

        IntVector2 RawMouseDelta;
        if (const TSharedPtr<FSceneViewport> SceneViewport = EditorEngine->GetSceneViewport())
        {
            RawMouseDelta = SceneViewport->ConsumeHighPrecisionMouseDelta();
        }

        const bool bContextMenuOpen = ImGui::IsPopupOpen(VIEWPORT_CONTEXT_MENU_ID);
        if (bClickedRight)
        {
            bRightMousePressedOnImage = true;
            RightMouseDragDistance    = 0.0f;
        }

        if (bRightMousePressedOnImage && ImGui::IsMouseDown(ImGuiMouseButton_Right))
        {
            const ImVec2 MouseDelta = ImGui::GetIO().MouseDelta;
            RightMouseDragDistance += Math::Abs(MouseDelta.x) + Math::Abs(MouseDelta.y);
            RightMouseDragDistance += float(Math::Abs(RawMouseDelta.X) + Math::Abs(RawMouseDelta.Y));
        }

        const bool bCanControlEditorCamera =
            bViewportInputActive &&
            !bBlockPickForGizmo &&
            !bContextMenuOpen &&
            (bViewportImageHovered || bViewportImageActive || bMouseLookActive);

        if (bCanControlEditorCamera)
        {
            const ImGuiIO& IO = ImGui::GetIO();

            PendingCameraInput.LookDelta        = Vector2(IO.MouseDelta.x, IO.MouseDelta.y);
            PendingCameraInput.PanDelta         = PendingCameraInput.LookDelta;
            PendingCameraInput.WheelDelta       = IO.MouseWheel;
            PendingCameraInput.bLeftMouseDown   = ImGui::IsMouseDown(ImGuiMouseButton_Left);
            PendingCameraInput.bRightMouseDown  = ImGui::IsMouseDown(ImGuiMouseButton_Right);
            PendingCameraInput.bMiddleMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Middle);
            PendingCameraInput.bAltDown         = IO.KeyAlt;
        #if PLATFORM_MACOS
            PendingCameraInput.bCmdDown         = FApplication::Get().GetModifierKeyState().IsSuperDown();
        #endif
            PendingCameraInput.bBoost           = IO.KeyShift || ImGui::IsKeyDown(ImGuiKey_GamepadL3);
            PendingCameraInput.bFocusPressed    = ImGui::IsKeyPressed(ImGuiKey_F, false);
            PendingCameraInput.bResetPressed    = ImGui::IsKeyPressed(ImGuiKey_R, false);

            const float GamepadMoveRight =
                ImGui::GetKeyData(ImGuiKey_GamepadLStickLeft)->AnalogValue -
                ImGui::GetKeyData(ImGuiKey_GamepadLStickRight)->AnalogValue;
            const float GamepadMoveForward =
                ImGui::GetKeyData(ImGuiKey_GamepadLStickUp)->AnalogValue -
                ImGui::GetKeyData(ImGuiKey_GamepadLStickDown)->AnalogValue;
            const bool bKeyboardFlyActive =
                PendingCameraInput.bRightMouseDown &&
                !PendingCameraInput.bAltDown;

            const float MoveRight =
                (bKeyboardFlyActive && ImGui::IsKeyDown(ImGuiKey_A) ? 1.0f : 0.0f) -
                (bKeyboardFlyActive && ImGui::IsKeyDown(ImGuiKey_D) ? 1.0f : 0.0f) +
                GamepadMoveRight;
            const float MoveUp =
                (bKeyboardFlyActive && ImGui::IsKeyDown(ImGuiKey_Q) ? 1.0f : 0.0f) -
                (bKeyboardFlyActive && ImGui::IsKeyDown(ImGuiKey_E) ? 1.0f : 0.0f);
            const float MoveForward =
                (bKeyboardFlyActive && ImGui::IsKeyDown(ImGuiKey_W) ? 1.0f : 0.0f) -
                (bKeyboardFlyActive && ImGui::IsKeyDown(ImGuiKey_S) ? 1.0f : 0.0f) +
                GamepadMoveForward;

            PendingCameraInput.MoveAxis = Vector3(
                Math::Clamp(MoveRight, -1.0f, 1.0f),
                Math::Clamp(MoveUp, -1.0f, 1.0f),
                Math::Clamp(MoveForward, -1.0f, 1.0f));
            PendingCameraInput.bFlyActive =
                bKeyboardFlyActive ||
                Math::Abs(GamepadMoveRight) > 0.01f ||
                Math::Abs(GamepadMoveForward) > 0.01f;

            const float RotateRight =
                (ImGui::IsKeyDown(ImGuiKey_RightArrow) ? 1.0f : 0.0f) -
                (ImGui::IsKeyDown(ImGuiKey_LeftArrow) ? 1.0f : 0.0f) +
                ImGui::GetKeyData(ImGuiKey_GamepadRStickRight)->AnalogValue -
                ImGui::GetKeyData(ImGuiKey_GamepadRStickLeft)->AnalogValue;
            const float RotateDown =
                (ImGui::IsKeyDown(ImGuiKey_DownArrow) ? 1.0f : 0.0f) -
                (ImGui::IsKeyDown(ImGuiKey_UpArrow) ? 1.0f : 0.0f) +
                ImGui::GetKeyData(ImGuiKey_GamepadRStickDown)->AnalogValue -
                ImGui::GetKeyData(ImGuiKey_GamepadRStickUp)->AnalogValue;

            PendingCameraInput.RotationAxis = Vector2(
                Math::Clamp(RotateRight, -1.0f, 1.0f),
                Math::Clamp(RotateDown, -1.0f, 1.0f));

            const bool bWantsMouseLook =
                PendingCameraInput.bRightMouseDown &&
                !PendingCameraInput.bAltDown;

            if (bWantsMouseLook && !bMouseLookActive && bViewportImageHovered)
            {
                bMouseLookActive         = true;
                bCursorWasVisible        = FApplication::Get().IsCursorVisible();
                MouseLookRestorePosition = FApplication::Get().GetCursorPosition();
                bRawLookActive           = FApplication::Get().SetHighPrecisionMouseMode(FApplication::Get().GetFocusWindow(), EHighPrecisionMouseMode::Enabled);

                if (bRawLookActive)
                {
                    FApplication::Get().ShowCursor(false);
                }
            }

            if (bMouseLookActive)
            {
                if (bWantsMouseLook)
                {
                    if (bRawLookActive)
                    {
                        PendingCameraInput.LookDelta = Vector2(float(RawMouseDelta.X), float(RawMouseDelta.Y));
                    }
                }
                else
                {
                    EndMouseLook();
                }
            }
        }
        else
        {
            if (bViewportImageHovered && !bBlockPickForGizmo && !bContextMenuOpen)
            {
                const ImGuiIO& IO = ImGui::GetIO();
                PendingCameraInput.WheelDelta = IO.MouseWheel;
                PendingCameraInput.bAltDown   = IO.KeyAlt;
            }

            EndMouseLook();
        }

        const bool bBlockPickForCamera =
            bMouseLookActive ||
             bContextMenuOpen ||
             PendingCameraInput.bAltDown ||
             PendingCameraInput.bCmdDown ||
             PendingCameraInput.bMiddleMouseDown;
 
        if (bWasViewportInputActive && bClickedLeft && !bBlockPickForGizmo && !bBlockPickForCamera && DebugView == FSceneRenderView::EDebugView::None)
        { 
            uint32 PixelX = 0;
            uint32 PixelY = 0;

            if (ComputeViewportPixel(ImageMin, ImageSize, PixelX, PixelY))
            {
                EditorEngine->RequestPick(PixelX, PixelY, EEditorPickPurpose::Selection);
            }
        }

        // ---------------------------------------------------------------------
        // Context menu
        // ---------------------------------------------------------------------

        constexpr float ContextMenuDragThreshold = 4.0f;

        const bool bWantsContextMenu =
            bRightMousePressedOnImage &&
            bViewportImageHovered &&
            ImGui::IsMouseReleased(ImGuiMouseButton_Right) &&
            RightMouseDragDistance <= ContextMenuDragThreshold &&
            !bBlockPickForGizmo &&
            !PendingCameraInput.bAltDown &&
            !PendingCameraInput.bCmdDown &&
            !PendingCameraInput.bMiddleMouseDown;

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
        {
            bRightMousePressedOnImage = false;
        }

        if (bWantsContextMenu)
        {
            const ImVec2 MousePos = ImGui::GetMousePos();
            const float  SafeW    = ImageSize.x > 0.0f ? ImageSize.x : 1.0f;
            const float  SafeH    = ImageSize.y > 0.0f ? ImageSize.y : 1.0f;

            ContextMenuNdc = Vector2(
                ((MousePos.x - ImageMin.x) / SafeW) * 2.0f - 1.0f,
                1.0f - ((MousePos.y - ImageMin.y) / SafeH) * 2.0f);

            if (FCameraComponent* Camera = GetViewCamera())
            {
                ContextMenuViewProjectionInverse = Camera->GetViewProjectionInverseMatrix();
            }

            ContextMenuActor         = nullptr;
            ContextMenuPickRequestId = 0;
            ComputeFallbackPlacement(ContextMenuNdc, ContextMenuLocation);

            uint32 PixelX = 0;
            uint32 PixelY = 0;

            if (ComputeViewportPixel(ImageMin, ImageSize, PixelX, PixelY))
            {
                ContextMenuPickRequestId = EditorEngine->RequestPick(PixelX, PixelY, EEditorPickPurpose::ContextMenu);
            }

            ImGui::OpenPopup(VIEWPORT_CONTEXT_MENU_ID);
        }

        DrawContextMenu();

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

            DrawList->AddRect(BorderMin, BorderMax, BorderColorU32, 0.0f, ImDrawFlags_None, BorderThickness);
        }

        // ---------------------------------------------------------------------
        // Corner overlays
        // ---------------------------------------------------------------------

        const float OverlayMargin  = 6.0f;
        const float OverlayPadding = 4.0f;
        const float OverlayRight   = ContentPos.x + ContentSize.x - OverlayMargin;

        float OverlayTop = ContentPos.y + OverlayMargin;

        if (CVarDrawFps.GetValue())
        {
            char FpsText[16];
            CString::Snprintf(FpsText, sizeof(FpsText), "%d", FFrameProfiler::Get().GetFramesPerSecond());

            ImDrawList* DrawList = ImGui::GetWindowDrawList();
            
            const ImFont* Font     = ImGui::GetFont();
            const ImVec2  TextSize = Font->CalcTextSizeA(Font->FontSize, FLT_MAX, 0.0f, FpsText);

            const ImVec2 BoxMin  = ImVec2(OverlayRight - TextSize.x - OverlayPadding * 2.0f, OverlayTop);
            const ImVec2 BoxMax  = ImVec2(OverlayRight, OverlayTop + TextSize.y + OverlayPadding * 2.0f);
            const ImVec2 TextPos = ImVec2(BoxMin.x + OverlayPadding, BoxMin.y + OverlayPadding);

            DrawList->AddRectFilled(BoxMin, BoxMax, IM_COL32(32, 32, 32, 192), 0.0f);
            DrawList->AddText(TextPos, IM_COL32(0, 255, 50, 255), FpsText);

            OverlayTop = BoxMax.y + OverlayMargin;
        }

        if (SpeedOverlayTimer > 0.0f && CameraController)
        {
            const float Alpha        = Math::Clamp(SpeedOverlayTimer / SPEED_OVERLAY_FADE_TIME, 0.0f, 1.0f);
            const float MinSpeed     = FEditorCameraController::MinMoveSpeed;
            const float MaxSpeed     = FEditorCameraController::MaxMoveSpeed;
            const float Speed        = Math::Clamp(CameraController->GetMoveSpeed(), MinSpeed, MaxSpeed);
            const float Fraction     = Math::Log2(Speed / MinSpeed) / Math::Log2(MaxSpeed / MinSpeed);
            const float TrackWidth   = 120.0f;
            const float TrackHeight  = 4.0f;
            const float HandleRadius = 5.0f;

            const ImVec2 BoxMin = ImVec2(OverlayRight - (TrackWidth + (OverlayPadding + HandleRadius) * 2.0f), OverlayTop);
            const ImVec2 BoxMax = ImVec2(OverlayRight, OverlayTop + (HandleRadius + OverlayPadding) * 2.0f);

            const float TrackLeft = BoxMin.x + OverlayPadding + HandleRadius;
            const float TrackY    = (BoxMin.y + BoxMax.y) * 0.5f;
            const float HandleX   = TrackLeft + TrackWidth * Fraction;

            const ImU32 BackgroundColor = IM_COL32(32, 32, 32, static_cast<int32>(192.0f * Alpha));
            const ImU32 TrackColor      = IM_COL32(51, 51, 51, static_cast<int32>(255.0f * Alpha));
            const ImU32 FillColor       = IM_COL32(9, 92, 176, static_cast<int32>(255.0f * Alpha));
            const ImU32 HandleColor     = IM_COL32(220, 220, 220, static_cast<int32>(255.0f * Alpha));

            ImDrawList* DrawList = ImGui::GetWindowDrawList();
            DrawList->AddRectFilled(BoxMin, BoxMax, BackgroundColor, 3.0f);
            DrawList->AddRectFilled(
                ImVec2(TrackLeft, TrackY - TrackHeight * 0.5f),
                ImVec2(TrackLeft + TrackWidth, TrackY + TrackHeight * 0.5f),
                TrackColor,
                TrackHeight * 0.5f);
            DrawList->AddRectFilled(
                ImVec2(TrackLeft, TrackY - TrackHeight * 0.5f),
                ImVec2(HandleX, TrackY + TrackHeight * 0.5f),
                FillColor,
                TrackHeight * 0.5f);
            DrawList->AddCircleFilled(ImVec2(HandleX, TrackY), HandleRadius, HandleColor);
        }
    }

    ImGui::End();

    ImGui::PopStyleVar(); // WindowPadding
}

void FEditorViewportWidget::Tick(float DeltaTime)
{
    if (CameraController)
    {
        CameraController->UpdateProjection(CachedViewportSize);
        CameraController->Tick(DeltaTime, PendingCameraInput, EditorEngine ? EditorEngine->GetSelectedActor() : nullptr);

        if (CameraController->ConsumeMoveSpeedChanged())
        {
            SpeedOverlayTimer = SPEED_OVERLAY_HOLD_TIME + SPEED_OVERLAY_FADE_TIME;
        }
        else
        {
            SpeedOverlayTimer = Math::Max(SpeedOverlayTimer - DeltaTime, 0.0f);
        }
    }

    PendingCameraInput = FEditorCameraInputState();
}

FCameraComponent* FEditorViewportWidget::GetViewCamera() const
{
    return CameraController ? CameraController->GetCamera() : nullptr;
}

void FEditorViewportWidget::OnActorRemoved(FActor* Actor)
{
    if (CameraController)
    {
        CameraController->OnActorRemoved(Actor);
    }

    if (ContextMenuActor == Actor)
    {
        ContextMenuActor = nullptr;
    }
}

bool FEditorViewportWidget::ComputeFallbackPlacement(const Vector2& Ndc, Vector3& OutLocation) const
{
    FCameraComponent* Camera = GetViewCamera();
    if (!Camera)
    {
        return false;
    }

    const Matrix4& ViewProjectionInverse = Camera->GetViewProjectionInverseMatrix();

    const Vector4 NearWorld = ViewProjectionInverse.Transform(Vector4(Ndc.X, Ndc.Y, 0.0f, 1.0f));
    const Vector4 FarWorld  = ViewProjectionInverse.Transform(Vector4(Ndc.X, Ndc.Y, 1.0f, 1.0f));

    if (Math::Abs(NearWorld.W) < 1e-6f || Math::Abs(FarWorld.W) < 1e-6f)
    {
        return false;
    }

    const Vector3 RayStart     = Vector3(NearWorld.X, NearWorld.Y, NearWorld.Z) / NearWorld.W;
    const Vector3 RayEnd       = Vector3(FarWorld.X, FarWorld.Y, FarWorld.Z) / FarWorld.W;
    const Vector3 RayDirection = (RayEnd - RayStart).GetNormalized();

    constexpr float MaxGroundDistance = 1000.0f;

    if (Math::Abs(RayDirection.Y) > 1e-4f)
    {
        const float Distance = -RayStart.Y / RayDirection.Y;
        if (Distance > 0.0f && Distance < MaxGroundDistance)
        {
            OutLocation = RayStart + (RayDirection * Distance);
            return true;
        }
    }

    constexpr float DefaultDistance = 10.0f;

    OutLocation = Camera->GetPosition() + (Camera->GetForwardVector() * DefaultDistance);
    return true;
}

void FEditorViewportWidget::DrawContextMenu()
{
    if (!EditorWidgets::BeginPopupContext(VIEWPORT_CONTEXT_MENU_ID))
    {
        return;
    }

    FWorld* World = FEngine::IsInitialized() ? FEngine::Get()->GetWorld() : nullptr;

    EditorWidgets::MenuLabeledSeparator("Place Actor");

    bool bRequestClosePopup = false;

    if (FActor* NewActor = EditorActorFactory::DrawPlaceActorMenu(World, ContextMenuLocation))
    {
        EditorEngine->SetSelectedActor(NewActor);
        bRequestClosePopup = true;
    }

    if (ContextMenuActor)
    {
        EditorWidgets::MenuLabeledSeparator("Actor");

        if (EditorWidgets::MenuItem("Focus", "F"))
        {
            if (CameraController)
            {
                CameraController->FocusOn(ContextMenuActor);
            }
        }

        if (EditorWidgets::MenuItem("Delete", "Delete"))
        {
            EditorEngine->RequestDeleteActor(ContextMenuActor);
            ContextMenuActor = nullptr;
        }
    }

    if (bRequestClosePopup)
    {
        ImGui::CloseCurrentPopup();
    }

    EditorWidgets::EndPopupContext();
}

void FEditorViewportWidget::OnContextMenuPickResult(const FEditorPickResult& Result, FActor* PickedActor)
{
    if (Result.RequestId != ContextMenuPickRequestId)
    {
        return;
    }

    ContextMenuActor = PickedActor;

    if (Result.bHasDepth)
    {
        const Vector4 Clip  = Vector4(ContextMenuNdc.X, ContextMenuNdc.Y, Result.DeviceDepth, 1.0f);
        const Vector4 World = ContextMenuViewProjectionInverse.Transform(Clip);

        if (Math::Abs(World.W) > 1e-6f)
        {
            ContextMenuLocation = Vector3(World.X, World.Y, World.Z) / World.W;
        }
    }
}

bool FEditorViewportWidget::ConsumeCameraCut()
{
    return CameraController && CameraController->ConsumeCameraCut();
}

void FEditorViewportWidget::EndMouseLook()
{
    if (!bMouseLookActive || !FApplication::IsInitialized())
    {
        bMouseLookActive = false;
        bRawLookActive   = false;
        return;
    }

    if (bRawLookActive)
    {
        FApplication::Get().SetHighPrecisionMouseMode(FApplication::Get().GetFocusWindow(), EHighPrecisionMouseMode::Disabled);
        FApplication::Get().SetCursorPosition(MouseLookRestorePosition);
        FApplication::Get().ShowCursor(bCursorWasVisible);
    }

    bMouseLookActive = false;
    bRawLookActive   = false;
}

bool FEditorViewportWidget::ComputeViewportPixel(const ImVec2& ImageMin, const ImVec2& ImageSize, uint32& OutPixelX, uint32& OutPixelY) const
{
    const ImVec2 MousePos = ImGui::GetMousePos();

    const float LocalXf = MousePos.x - ImageMin.x;
    const float LocalYf = MousePos.y - ImageMin.y;

    if (LocalXf < 0.0f || LocalYf < 0.0f || LocalXf >= ImageSize.x || LocalYf >= ImageSize.y)
    {
        return false;
    }

    FRHITexture* ViewportTexture = ViewportImage.GetTexture();

    const uint32 RenderWidth  = ViewportTexture ? ViewportTexture->GetDesc().Extent.X : static_cast<uint32>(ImageSize.x);
    const uint32 RenderHeight = ViewportTexture ? ViewportTexture->GetDesc().Extent.Y : static_cast<uint32>(ImageSize.y);

    const float SafeW = ImageSize.x > 0.0f ? ImageSize.x : 1.0f;
    const float SafeH = ImageSize.y > 0.0f ? ImageSize.y : 1.0f;

    const float U = LocalXf / SafeW;
    const float V = LocalYf / SafeH;

    OutPixelX = RenderWidth  > 0 ? Math::Min(static_cast<uint32>(U * float(RenderWidth)),  RenderWidth  - 1) : 0;
    OutPixelY = RenderHeight > 0 ? Math::Min(static_cast<uint32>(V * float(RenderHeight)), RenderHeight - 1) : 0;
    return true;
}

void FEditorViewportWidget::SetViewportWidget(const TSharedPtr<FViewportWidget>& InViewportWidget)
{
    ViewportWidget = InViewportWidget;
}

void FEditorViewportWidget::SetViewportImage(FRHITextureRef InViewportImage)
{
    if (InViewportImage)
    {
        ViewportImage = FImGuiTexture(InViewportImage);
        ViewportImage.bEnableLinearSampler = false;
        ViewportImage.bEnableBlending      = false;
    }
}

IntVector2 FEditorViewportWidget::GetViewportSize() const
{
    if (CachedViewportSize.X > 0 && CachedViewportSize.Y > 0)
    {
        return CachedViewportSize;
    }

    if (ImGuiWindow* ViewportWindow = ImGui::FindWindowByName("Viewport"))
    {
        const ImVec2 Size = ViewportWindow->ContentRegionRect.GetSize();
        return IntVector2(static_cast<int32>(Size.x), static_cast<int32>(Size.y));
    }

    return IntVector2(1920, 1080);
}

FSceneRenderView::EDebugView FEditorViewportWidget::GetDebugView() const
{
    return DebugView;
}

FSceneRenderView::EDebugView FEditorViewportWidget::GetSecondaryDebugView() const
{
    return SecondaryDebugView;
}

FSceneRenderView::EDebugViewChannel FEditorViewportWidget::GetDebugViewChannelMask() const
{
    return DebugViewChannelMask;
}
