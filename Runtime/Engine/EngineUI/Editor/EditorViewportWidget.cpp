#include "Core/Containers/StaticArray.h"
#include "Core/Math/Math.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Templates/CString.h"
#include "Application/Application.h"
#include "RHI/RHIResources.h" 
#include "Engine/Engine.h" 
#include "Engine/EngineUI/Editor/EditorGuizmo.h" 
#include "Engine/EngineUI/Editor/EditorViewportWidget.h" 
#include "Engine/EngineUI/Editor/EditorHelpers.h"
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
    , DebugView(FSceneRenderView::EDebugView::None)
    , SecondaryDebugView(FSceneRenderView::EDebugView::None)
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

            if (ImGui::BeginChild("##ViewportToolbar", ImVec2(0.0f, ToolbarHeight), false, ToolbarFlags))
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

                const auto DrawSubmenuRow = [&](const CHAR* Label, PopupAnchor& OutAnchor, bool& bOutHovered, bool bForceActive) -> bool
                {
                    const float PaddingY    = 4.0f;
                    const float RowHeight   = ImGui::GetFontSize() + PaddingY * 2.0f;
                    const float RowWidth    = ImGui::GetContentRegionAvail().x;
                    const float MenuIndentX = 20.0f;

                    ImGui::PushID(Label);

                    const ImGuiSelectableFlags Flags =
                        ImGuiSelectableFlags_SpanAvailWidth |
                        ImGuiSelectableFlags_NoPadWithHalfSpacing;

                    const bool bPressed = ImGui::Selectable("##row", false, Flags, ImVec2(RowWidth, RowHeight));
                    const bool bHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
                    const bool bActive  = ImGui::IsItemActive();

                    const ImVec2 RectMin = ImGui::GetItemRectMin();
                    const ImVec2 RectMax = ImGui::GetItemRectMax();

                    ImDrawList* DrawList = ImGui::GetWindowDrawList();

                    ImU32 Background = ImGui::GetColorU32(ImGuiCol_Header);
                    if (bActive || bForceActive)
                    {
                        Background = ImGui::GetColorU32(ImGuiCol_HeaderActive);
                    }
                    else if (bHovered)
                    {
                        Background = ImGui::GetColorU32(ImGuiCol_HeaderHovered);
                    }

                    DrawList->AddRectFilled(RectMin, RectMax, Background, 0.0f);

                    const ImVec2 LabelSize = ImGui::CalcTextSize(Label);
                    const float  CircleX   = RectMin.x + MenuIndentX + 6.0f;
                    const float  LabelX    = CircleX + LabelGapFromCircle;
                    const float  LabelY    = RectMin.y + (RowHeight - LabelSize.y) * 0.5f;

                    DrawList->AddText(ImVec2(LabelX, LabelY), ImGui::GetColorU32(ImGuiCol_Text), Label);

                    const float  ArrowSize = 12.0f;
                    const float  ArrowY    = RectMin.y + (RowHeight - ArrowSize) * 0.5f;
                    const float  ArrowX    = RectMax.x - MenuIndentX - ArrowSize;
                    const ImVec2 ArrowMin  = ImVec2(ArrowX, ArrowY);
                    const ImVec2 ArrowMax  = ImVec2(ArrowX + ArrowSize, ArrowY + ArrowSize);

                    if (EditorIcons::RightArrowIcon)
                    {
                        DrawList->AddImage(EditorIcons::RightArrowIcon, ArrowMin, ArrowMax, ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImGuiCol_Text));
                    }
                    else
                    {
                        DrawList->AddText(ImVec2(ArrowX, LabelY), ImGui::GetColorU32(ImGuiCol_Text), ">");
                    }

                    OutAnchor.Min              = RectMin;
                    OutAnchor.Max              = RectMax;
                    OutAnchor.bRequestPosition = false;

                    bOutHovered = bHovered;

                    ImGui::PopID();
                    return bPressed;
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
                        FCString::Snprintf(OutBuffer.Data(), static_cast<int32>(OutBuffer.Size()), "%s", InLabel);
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
                    FCString::Strcat(OutBuffer.Data(), Ellipsis);

                    return OutBuffer.Data();
                };

                const CHAR* MenuLabelText = BuildClampedLabel(CurrentLabel, MaxTextWidth, MenuLabel);

                const ImVec2 LabelSize     = ImGui::CalcTextSize(MenuLabelText);
                const ImVec2 ChildPos      = ImGui::GetWindowPos();
                const ImVec2 ChildSize     = ImGui::GetWindowSize();
                const ImVec2 ContentMin    = ImGui::GetWindowContentRegionMin();
                const ImVec2 ContentMax    = ImGui::GetWindowContentRegionMax();
                const float  ContentWidth  = ContentMax.x - ContentMin.x;
                const float  CursorX       = ContentMin.x + Math::Max(0.0f, ContentWidth - ButtonWidth - RightPadding);
                const float  CursorY       = ContentMin.y + 6.0f;

                ImGui::SetCursorScreenPos(ImVec2(ChildPos.x + CursorX, ChildPos.y + CursorY));

                const CHAR* ViewMenuPopupId      = "##ViewportViewModeMenu";
                const CHAR* ShadowMenuPopupId    = "##ViewportShadowMenu";
                const CHAR* SecondaryMenuPopupId = "##ViewportSecondaryMenu";
                const float SubmenuOverlap       = 0.0f;

                const ImGuiPopupFlags PopupQueryFlags = ImGuiPopupFlags_AnyPopupLevel;

                const bool bViewPopupOpen      = ImGui::IsPopupOpen(ViewMenuPopupId, PopupQueryFlags);
                const bool bShadowPopupOpen    = ImGui::IsPopupOpen(ShadowMenuPopupId, PopupQueryFlags);
                const bool bSecondaryPopupOpen = ImGui::IsPopupOpen(SecondaryMenuPopupId, PopupQueryFlags);
                const bool bAnyPopupOpen       = bViewPopupOpen || bShadowPopupOpen || bSecondaryPopupOpen;

                PopupAnchor ViewMenuAnchor;
                const auto DrawViewModeButton = [&](const CHAR* Label, PopupAnchor& OutAnchor, bool& bOutHovered) -> bool
                {
                    const float ButtonHeightLocal = ButtonHeight;

                    const ImVec2 ButtonSize = ImVec2(ButtonWidth, ButtonHeight);

                    const bool bPressed = ImGui::InvisibleButton("##ViewportViewModeButton", ButtonSize);
                    const bool bHovered = ImGui::IsItemHovered();
                    const bool bHeld    = ImGui::IsItemActive();

                    const ImVec2 Min = ImGui::GetItemRectMin();
                    const ImVec2 Max = ImGui::GetItemRectMax();

                    const ImU32 BgIdle  = IM_COL32(56, 56, 56, 255);
                    const ImU32 BgHover = IM_COL32(87, 87, 87, 255);

                    ImU32 Bg = BgIdle;
                    if (bViewPopupOpen || bHeld || bHovered)
                    {
                        Bg = BgHover;
                    }

                    ImDrawList* DrawList = ImGui::GetWindowDrawList();
                    DrawList->AddRectFilled(Min, Max, Bg, 6.0f);

                    const float TextY = Min.y + (ButtonHeightLocal - LabelSize.y) * 0.5f;
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

                bool bViewHovered = false;

                const bool bViewPressed = DrawViewModeButton(MenuLabelText, ViewMenuAnchor, bViewHovered);
                if (bViewPressed || (bAnyPopupOpen && bViewHovered))
                {
                    ImGui::OpenPopup(ViewMenuPopupId);
                    ViewMenuAnchor.bRequestPosition = true;
                }

                if (EditorWidgets::BeginMenuPopup(ViewMenuPopupId, ViewMenuAnchor, 240.0f))
                {
                    EditorWidgets::MenuLabeledSeparator("VIEW MODE");

                    bool bRequestClosePopup = false;
                    bool bAnyMainRowHovered = false;

                    for (const FDebugItem& Item : BaseViewItems)
                    {
                        bool bRowHovered = false;
                        if (DrawRadioMenuItem(Item.Label, DebugView == Item.View, true, false, &bRowHovered))
                        {
                            DebugView          = Item.View;
                            bRequestClosePopup = true;
                        }

                        bAnyMainRowHovered |= bRowHovered;
                    }

                    const auto DrawSubmenuOverlay = [&](const CHAR* Label, const PopupAnchor& Anchor)
                    {
                        if (Anchor.Max.x <= Anchor.Min.x || Anchor.Max.y <= Anchor.Min.y)
                        {
                            return;
                        }

                        ImDrawList* DrawList = ImGui::GetWindowDrawList();
                        DrawList->AddRectFilled(Anchor.Min, Anchor.Max, ImGui::GetColorU32(ImGuiCol_HeaderActive), 0.0f);

                        const float  RowHeight   = Anchor.Max.y - Anchor.Min.y;
                        const float  MenuIndentX = 20.0f;
                        const ImVec2 LblSize     = ImGui::CalcTextSize(Label);
                        const float  CircleX     = Anchor.Min.x + MenuIndentX + 6.0f;
                        const float  LblX        = CircleX + LabelGapFromCircle;
                        const float  LblY        = Anchor.Min.y + (RowHeight - LblSize.y) * 0.5f;

                        DrawList->AddText(ImVec2(LblX, LblY), ImGui::GetColorU32(ImGuiCol_Text), Label);

                        const float  ArrowSize = 12.0f;
                        const float  ArwY      = Anchor.Min.y + (RowHeight - ArrowSize) * 0.5f;
                        const float  ArwX      = Anchor.Max.x - MenuIndentX - ArrowSize;
                        const ImVec2 ArwMin    = ImVec2(ArwX, ArwY);
                        const ImVec2 ArwMax    = ImVec2(ArwX + ArrowSize, ArwY + ArrowSize);

                        if (EditorIcons::RightArrowIcon)
                        {
                            DrawList->AddImage(EditorIcons::RightArrowIcon, ArwMin, ArwMax, ImVec2(0, 0), ImVec2(1, 1), ImGui::GetColorU32(ImGuiCol_Text));
                        }
                        else
                        {
                            DrawList->AddText(ImVec2(ArwX, LblY), ImGui::GetColorU32(ImGuiCol_Text), ">");
                        }
                    };

                    {
                        PopupAnchor ShadowAnchor;
                        bool bShadowHovered = false;

                        const bool bShadowPressed = DrawSubmenuRow("Shadow Debug", ShadowAnchor, bShadowHovered, bShadowPopupOpen);
                        bAnyMainRowHovered |= bShadowHovered;

                        if (bShadowPressed || (bAnyPopupOpen && bShadowHovered))
                        {
                            ImGui::OpenPopup(ShadowMenuPopupId);
                            ShadowAnchor.bRequestPosition = true;
                        }

                        PopupAnchor ShadowPopupAnchor = ShadowAnchor;
                        ShadowPopupAnchor.Min              = ImVec2(ShadowAnchor.Max.x - SubmenuOverlap, ShadowAnchor.Min.y);
                        ShadowPopupAnchor.Max              = ImVec2(ShadowAnchor.Max.x - SubmenuOverlap, ShadowAnchor.Min.y);
                        ShadowPopupAnchor.bRequestPosition = ShadowAnchor.bRequestPosition;

                        if (EditorWidgets::BeginMenuPopup(ShadowMenuPopupId, ShadowPopupAnchor, 240.0f))
                        {
                            const bool bShadowPopupHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
                            for (const FDebugItem& Item : ShadowDebugItems)
                            {
                                if (DrawRadioMenuItem(Item.Label, DebugView == Item.View, true, true, nullptr))
                                {
                                    DebugView          = Item.View;
                                    bRequestClosePopup = true;
                                }
                            }

                            const float BridgeMaxX = Math::Max(ShadowPopupAnchor.Min.x + 4.0f, ShadowAnchor.Max.x);

                            const bool bShadowBridgeHovered = ImGui::IsMouseHoveringRect(ShadowAnchor.Min, ImVec2(BridgeMaxX, ShadowAnchor.Max.y), false);
                            const bool bShadowKeepOpen      = bShadowPopupHovered || bShadowHovered || bShadowBridgeHovered;

                            if (!bShadowKeepOpen)
                            {
                                ImGui::CloseCurrentPopup();
                            }

                            EditorWidgets::EndMenuPopup();
                        }

                        if (ImGui::IsPopupOpen(ShadowMenuPopupId, PopupQueryFlags))
                        {
                            DrawSubmenuOverlay("Shadow Debug", ShadowAnchor);
                        }
                    }

                    {
                        PopupAnchor SecondaryAnchor;

                        bool bSecondaryHovered = false;

                        const bool bSecondaryPressed = DrawSubmenuRow("Secondary View", SecondaryAnchor, bSecondaryHovered, bSecondaryPopupOpen);
                        bAnyMainRowHovered |= bSecondaryHovered;

                        if (bSecondaryPressed || (bAnyPopupOpen && bSecondaryHovered))
                        {
                            ImGui::OpenPopup(SecondaryMenuPopupId);
                            SecondaryAnchor.bRequestPosition = true;
                        }

                        PopupAnchor SecondaryPopupAnchor      = SecondaryAnchor;
                        SecondaryPopupAnchor.Min              = ImVec2(SecondaryAnchor.Max.x - SubmenuOverlap, SecondaryAnchor.Min.y);
                        SecondaryPopupAnchor.Max              = ImVec2(SecondaryAnchor.Max.x - SubmenuOverlap, SecondaryAnchor.Min.y);
                        SecondaryPopupAnchor.bRequestPosition = SecondaryAnchor.bRequestPosition;

                        if (EditorWidgets::BeginMenuPopup(SecondaryMenuPopupId, SecondaryPopupAnchor, 240.0f))
                        {
                            const bool bSecondaryPopupHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
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

                            const float BridgeMaxX = Math::Max(SecondaryPopupAnchor.Min.x + 4.0f, SecondaryAnchor.Max.x);

                            const bool bSecondaryBridgeHovered = ImGui::IsMouseHoveringRect(SecondaryAnchor.Min, ImVec2(BridgeMaxX, SecondaryAnchor.Max.y), false);
                            const bool bSecondaryKeepOpen      = bSecondaryPopupHovered || bSecondaryHovered || bSecondaryBridgeHovered;

                            if (!bSecondaryKeepOpen)
                            {
                                ImGui::CloseCurrentPopup();
                            }

                            EditorWidgets::EndMenuPopup();
                        }

                        if (ImGui::IsPopupOpen(SecondaryMenuPopupId, PopupQueryFlags))
                        {
                            DrawSubmenuOverlay("Secondary View", SecondaryAnchor);
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
 
        const bool bBlockPickForGizmo = 
            EditorGuizmo::IsUsingAny() || 
            EditorGuizmo::IsOver() || 
            EditorGuizmo::IsUsingViewManipulate() || 
            EditorGuizmo::IsViewManipulateHovered(); 
 
        if (bWasViewportInputActive && bClickedLeft && !bBlockPickForGizmo && DebugView == FSceneRenderView::EDebugView::None) 
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
                            FRHITexture* ViewportTexture = ViewportImage.GetTexture();

                            const uint32 RenderWidth  = ViewportTexture ? ViewportTexture->GetDesc().Extent.X : static_cast<uint32>(ContentSize.x);
                            const uint32 RenderHeight = ViewportTexture ? ViewportTexture->GetDesc().Extent.Y : static_cast<uint32>(ContentSize.y);

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
        ViewportImage = FImGuiTexture(InViewportImage);
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

FSceneRenderView::EDebugView FEditorViewportWidget::GetDebugView() const
{
    return DebugView;
}

FSceneRenderView::EDebugView FEditorViewportWidget::GetSecondaryDebugView() const
{
    return SecondaryDebugView;
}
