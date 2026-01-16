#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "Core/Misc/Paths.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Engine/Assets/AssetManager.h"
#include "ImGuiPlugin/ImGuiRenderer.h"
#include <imgui_internal.h>

float EditorStyleVars::MainMenuBarHeight = 28.0f;

ImVec2 EditorStyleVars::InputFieldFramePadding    = ImVec2(12.0f, 6.0f);
float  EditorStyleVars::InputFieldBorderThickness = 2.0f;
float  EditorStyleVars::InputFieldBorderRounding  = 16.0f;
ImU32  EditorStyleVars::InputFieldBorderColor     = IM_COL32(100, 136, 234, 255);

ImVec2 EditorStyleVars::SceneHierarchyItemSpacing    = ImVec2(8.0f, 8.0f);
ImVec2 EditorStyleVars::SceneHierarchyWindowPadding  = ImVec2(8.0f, 8.0f);
float  EditorStyleVars::SceneHierarchyTableRowHeight = 32.0f;

ImVec2 EditorStyleVars::PropertiesItemSpacing                 = ImVec2(8.0f, 8.0f);
ImVec2 EditorStyleVars::PropertiesWindowPadding               = ImVec2(8.0f, 8.0f);
ImVec2 EditorStyleVars::PropertiesCollapsingHeaderItemSpacing = ImVec2(8.0f, 2.0f);
ImVec2 EditorStyleVars::PropertiesCollapsingFramePadding      = ImVec2(10.0f, 8.0f);
float  EditorStyleVars::PropertiesCollapsingFrameRounding     = 2.0f;

static void ApplyHoveredRowBg(bool bRowHovered)
{
    if (!bRowHovered)
    {
        return;
    }

    ImGuiTable* Table = ImGui::GetCurrentTable();
    if (!Table)
    {
        return;
    }

    const ImU32 HoverBg = IM_COL32(47, 47, 47, 255);

    const int32 ColumnCount = ImGui::TableGetColumnCount();
    for (int32 ColumnIndex = 0; ColumnIndex < ColumnCount; ++ColumnIndex)
    {
        ImGui::TableSetColumnIndex(ColumnIndex);
        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, HoverBg);
    }
}

static bool BeginFullRowHoverCatcher(float RowHeight)
{
    ImGuiTable* Table = ImGui::GetCurrentTable();
    if (!Table)
    {
        return false;
    }

    ImGui::TableSetColumnIndex(0);

    const ImVec2 Cursor = ImGui::GetCursorScreenPos();

    const ImGuiSelectableFlags HoverFlags =
        ImGuiSelectableFlags_SpanAllColumns |
        ImGuiSelectableFlags_AllowOverlap |
        ImGuiSelectableFlags_Disabled;

    ImGui::Selectable("##RowHover", false, HoverFlags, ImVec2(0.0f, RowHeight));
    const bool bHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

    ImGui::SetCursorScreenPos(Cursor);
    return bHovered;
}

static void DrawInputBorderLastItem(float Rounding = -1.0f, float Thickness = 2.0f)
{
    ImGuiWindow* Window = ImGui::GetCurrentWindow();
    if (!Window || Window->SkipItems)
    {
        return;
    }

    if (Rounding < 0.0f)
    {
        ImGuiStyle& Style = ImGui::GetStyle();
        Rounding = Style.FrameRounding;
    }

    const bool bActive  = ImGui::IsItemActive();
    const bool bHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

    const ImU32 ColorIdle   = IM_COL32(54, 54, 54, 255);
    const ImU32 ColorHover  = IM_COL32(84, 84, 84, 255);
    const ImU32 ColorActive = IM_COL32(0, 112, 224, 255);
    const ImU32 Color       = bActive ? ColorActive : (bHovered ? ColorHover : ColorIdle);

    ImVec2 Min = ImGui::GetItemRectMin();
    ImVec2 Max = ImGui::GetItemRectMax();
    Window->DrawList->AddRect(Min, Max, Color, Rounding, 0, Thickness);
}

static void DrawAxisLineForLastItem(ImU32 InColor)
{
    ImGuiWindow* Window = ImGui::GetCurrentWindow();
    if (!Window || Window->SkipItems)
    {
        return;
    }

    const ImVec2 ItemMin = ImGui::GetItemRectMin();
    const ImVec2 ItemMax = ImGui::GetItemRectMax();

    const float PadX      = 6.0f;
    const float PadY      = 5.0f;
    const float Thickness = 2.0f;
    const float X         = ItemMin.x + PadX;

    const ImVec2 LineMin = ImVec2(X - Thickness * 0.5f, ItemMin.y + PadY);
    const ImVec2 LineMax = ImVec2(X + Thickness * 0.5f, ItemMax.y - PadY);

    Window->DrawList->AddRectFilled(LineMin, LineMax, InColor, 1.0f);
}

static bool ResetIconButton(float InSizePx = 0.0f)
{
    float ButtonSizePx = InSizePx;
    if (ButtonSizePx <= 0.0f)
    {
        const float AvailableWidthPx = ImGui::GetContentRegionAvail().x;
        ButtonSizePx = Math::Min(ImGui::GetFrameHeight(), AvailableWidthPx);
    }

    ButtonSizePx = Math::Max(1.0f, ButtonSizePx);
    const ImVec2 ButtonSize = ImVec2(ButtonSizePx, ButtonSizePx);

    ImGuiID UniqueSeedId = ImGui::GetItemID();
    if (UniqueSeedId == 0)
    {
        UniqueSeedId = ImGui::GetID("##ResetIconButtonSeed");
    }

    ImGui::PushID((int32)UniqueSeedId);

    const bool bWasPressed = ImGui::InvisibleButton("##Revert", ButtonSize);
    const bool bIsHovered  = ImGui::IsItemHovered();
    const bool bIsHeld     = ImGui::IsItemActive();

    const ImVec2 ButtonRectMin = ImGui::GetItemRectMin();
    const ImVec2 ButtonRectMax = ImGui::GetItemRectMax();

    const ImGuiStyle& ImGuiStyle = ImGui::GetStyle();

    ImDrawList* WindowDrawList = ImGui::GetWindowDrawList();
    if (EditorIcons::UndoIcon)
    {
        const ImVec2 IconRectMin = ButtonRectMin;
        const ImVec2 IconRectMax = ButtonRectMax;

        const int32 IconTintIdle   = 220;
        const int32 IconTintHover  = 160;
        const int32 IconTintActive = 130;
        const int32 IconTintValue  = bIsHeld ? IconTintActive : (bIsHovered ? IconTintHover : IconTintIdle);

        const float Alpha01  = Math::Clamp(ImGuiStyle.Alpha, 0.0f, 1.0f);
        const int32 Alpha255 = (int32)(Alpha01 * 255.0f);

        const ImU32 IconTintColor = IM_COL32(IconTintValue, IconTintValue, IconTintValue, Alpha255);
        WindowDrawList->AddImage(EditorIcons::UndoIcon, IconRectMin, IconRectMax, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), IconTintColor);
    }
    else
    {
        const char* FallbackText = "R";

        const ImVec2 FallbackTextSize     = ImGui::CalcTextSize(FallbackText);
        const ImVec2 FallbackTextPosition = ImVec2((ButtonRectMin.x + ButtonRectMax.x) * 0.5f - FallbackTextSize.x * 0.5f, (ButtonRectMin.y + ButtonRectMax.y) * 0.5f - FallbackTextSize.y * 0.5f);

        const ImU32 TextColor = ImGui::GetColorU32(ImGuiCol_Text);
        WindowDrawList->AddText(FallbackTextPosition, TextColor, FallbackText);
    }

    ImGui::PopID();
    return bWasPressed;
}

static void DrawSuffixForLastItem(const char* InSuffix, ImU32 InColor, float InRightPadding = 6.0f)
{
    ImGuiWindow* Window = ImGui::GetCurrentWindow();
    if (!Window || Window->SkipItems || !InSuffix)
    {
        return;
    }

    const ImVec2 ItemMin  = ImGui::GetItemRectMin();
    const ImVec2 ItemMax  = ImGui::GetItemRectMax();
    const ImVec2 TextSize = ImGui::CalcTextSize(InSuffix);

    const float X = ItemMax.x - InRightPadding - TextSize.x;
    const float Y = ItemMin.y + (ItemMax.y - ItemMin.y - TextSize.y) * 0.5f;

    Window->DrawList->AddText(ImVec2(X, Y), InColor, InSuffix);
}

static void DrawSuffixAfterTempInputText(ImGuiID InItemId, const char* InSuffix, ImU32 InColor, float InGapPx = 1.0f)
{
    ImGuiWindow* Window = ImGui::GetCurrentWindow();
    if (!Window || Window->SkipItems || !InSuffix)
    {
        return;
    }

    ImGuiContext& Context = *GImGui;
    if (!ImGui::TempInputIsActive(InItemId))
    {
        return;
    }

    if (Context.InputTextState.ID != InItemId)
    {
        return;
    }

    const char* EditText = Context.InputTextState.TextA.Data;
    if (!EditText)
    {
        return;
    }

    const ImVec2 ItemMin    = ImGui::GetItemRectMin();
    const ImVec2 ItemMax    = ImGui::GetItemRectMax();
    const ImVec2 TextSize   = ImGui::CalcTextSize(EditText);
    const ImVec2 SuffixSize = ImGui::CalcTextSize(InSuffix);

    const float TextStartX = ItemMin.x + ImGui::GetStyle().FramePadding.x;
    const float RightLimit = ItemMax.x - ImGui::GetStyle().FramePadding.x;
    const float DesiredX   = TextStartX + TextSize.x + InGapPx;

    if (DesiredX + SuffixSize.x > RightLimit)
    {
        return;
    }

    const float Y = ItemMin.y + (ItemMax.y - ItemMin.y - SuffixSize.y) * 0.5f;
    Window->DrawList->AddText(ImVec2(DesiredX, Y), InColor, InSuffix);
}

bool EditorWidgets::ButtonCenteredOnLine(const CHAR* Label, float Alignment)
{
    ImGuiStyle& Style = ImGui::GetStyle();

    const float Size   = ImGui::CalcTextSize(Label).x + Style.FramePadding.x * 2.0f;
    const float Offset = (ImGui::GetContentRegionAvail().x - Size) * Alignment;

    if (Offset > 0.0f)
    {
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + Offset);
    }

    return ImGui::Button(Label);
}

bool EditorWidgets::DrawFloat3Control(const CHAR* Label, FVector3& OutValue, float Speed, const FVector3* InRevertValue, EVector3ControlType InType)
{
    ImGuiTable* CurrentTable = ImGui::GetCurrentTable();
    if (!CurrentTable)
    {
        return false;
    }

    ImGuiStyle& Style = ImGui::GetStyle();

    const float AxisGapPx     = 2.0f;
    const float FrameHeightPx = ImGui::GetFontSize() + Style.FramePadding.y * 2.0f;
    const float RowHeightPx   = FrameHeightPx;

    ImGui::TableNextRow(0, RowHeightPx);
    
    bool bAnyValueChanged = false;
    bool bRowHovered      = BeginFullRowHoverCatcher(RowHeightPx);

    const bool bIsRotationDegrees = (InType == EVector3ControlType::RotationDegrees);
    const bool bIsScaleControl    = (InType == EVector3ControlType::Scale);

    const auto GetAxisDragSpeed = [&]() -> float
    {
        if (bIsScaleControl)
        {
            return 0.0025f;
        }

        if (InType == EVector3ControlType::Position || bIsRotationDegrees)
        {
            return 1.0f;
        }

        return Speed;
    };

    const auto GetDynamicFormatString = [](float Value, char(&OutFormat)[8]) -> const char*
    {
        static const float Pow10Table[4] = { 10.0f, 100.0f, 1000.0f, 10000.0f };

        const float AbsoluteValue = Math::Abs(Value);

        int32 DecimalsToShow = 1;

        for (int32 DecimalIndex = 1; DecimalIndex <= 4; ++DecimalIndex)
        {
            const float ScaleFactor       = Pow10Table[DecimalIndex - 1];
            const float RoundedToDecimals = Math::Round(AbsoluteValue * ScaleFactor) / ScaleFactor;

            // Tolerance to avoid float noise.
            const float Tolerance = 1e-5f * (AbsoluteValue + 1.0f);

            if (Math::Abs(AbsoluteValue - RoundedToDecimals) <= Tolerance)
            {
                DecimalsToShow = DecimalIndex;
                break;
            }

            DecimalsToShow = 4;
        }

        FCString::Snprintf(OutFormat, sizeof(OutFormat), "%%.%df", DecimalsToShow);
        return OutFormat;
    };

    ImGui::TableSetColumnIndex(0);

    const float LabelIndentPx = 24.0f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + LabelIndentPx);

    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", Label);

    bool bUniformScaleEnabled = false;
    if (bIsScaleControl)
    {
        ImGui::SameLine(0.0f, 12.0f);

        ImGui::PushID(Label);

        const ImGuiID UniformScaleKey = ImGui::GetID("UniformScale");

        ImGuiStorage* StateStorage = ImGui::GetStateStorage();
        bUniformScaleEnabled = StateStorage->GetBool(UniformScaleKey, false);
        const bool bUniformScalePrev = bUniformScaleEnabled;

        const float  IconButtonSizePx = 16.0f;
        const ImVec2 IconButtonSize   = ImVec2(IconButtonSizePx, IconButtonSizePx);

        {
            const ImVec2 CursorScreen = ImGui::GetCursorScreenPos();
            const float  CenteredY    = CursorScreen.y + (RowHeightPx - IconButtonSizePx) * 0.5f;
            ImGui::SetCursorScreenPos(ImVec2(CursorScreen.x, CenteredY));
        }

        const bool bPressed = ImGui::InvisibleButton("##UniformScale", IconButtonSize);

        {
            ImDrawList* DrawList = ImGui::GetWindowDrawList();

            const ImVec2 RectMin = ImGui::GetItemRectMin();
            const ImVec2 RectMax = ImGui::GetItemRectMax();

            const float Pad = 2.0f;
            ImVec2 IconMin = ImVec2(RectMin.x + Pad, RectMin.y + Pad);
            ImVec2 IconMax = ImVec2(RectMax.x - Pad, RectMax.y - Pad);

            const float Alpha01  = Math::Clamp(Style.Alpha, 0.0f, 1.0f);
            const int32 Alpha255 = (int32)(Alpha01 * 255.0f);
            const ImU32 Tint     = IM_COL32(255, 255, 255, Alpha255);

            ImTextureID Icon = bUniformScaleEnabled ? EditorIcons::LockedIcon : EditorIcons::UnlockedIcon;
            if (Icon)
            {
                DrawList->AddImage(Icon, IconMin, IconMax, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), Tint);
            }
            else
            {
                const char* FallbackText = bUniformScaleEnabled ? "L" : "U";

                const ImVec2 TextSize = ImGui::CalcTextSize(FallbackText);
                const ImVec2 TextPos  = ImVec2((RectMin.x + RectMax.x) * 0.5f - TextSize.x * 0.5f, (RectMin.y + RectMax.y) * 0.5f - TextSize.y * 0.5f);
                
                DrawList->AddText(TextPos, ImGui::GetColorU32(ImGuiCol_Text), FallbackText);
            }
        }

        if (bPressed)
        {
            bUniformScaleEnabled = !bUniformScaleEnabled;
        }

        if (bUniformScaleEnabled != bUniformScalePrev)
        {
            StateStorage->SetBool(UniformScaleKey, bUniformScaleEnabled);
        }

        // Keep row hover highlighting behavior (but icon itself does not change appearance)
        bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        bRowHovered |= ImGui::IsItemActive();

        ImGui::PopID();
    }

    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(Label);

    const float AvailableWidthPx = ImGui::GetContentRegionAvail().x;
    const float TotalAxisGapsPx  = 2.0f * AxisGapPx;

    float AxisFieldWidthPx = (AvailableWidthPx - TotalAxisGapsPx) / 3.0f;
    AxisFieldWidthPx = Math::Max(AxisFieldWidthPx, 1.0f);

    bool bXChanged = false;
    bool bYChanged = false;
    bool bZChanged = false;

    const auto DrawAxisField = [&](const char* DragWidgetId, float& InOutAxisValue, ImU32 AxisIndicatorColor, float FieldWidthPx, bool bPlaceOnSameLine) -> bool
    {
        if (bPlaceOnSameLine)
        {
            ImGui::SameLine(0.0f, AxisGapPx);
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(AxisGapPx, 0.0f));
        ImGui::SetNextItemWidth(FieldWidthPx);

        bool bAxisValueChanged = false;

        // Dynamic decimals based on current value (1..4)
        char FormatBuffer[8];
        const char* FormatString = GetDynamicFormatString(InOutAxisValue, FormatBuffer);

        const float AxisDragSpeed = GetAxisDragSpeed();
        bAxisValueChanged |= ImGui::DragFloat(DragWidgetId, &InOutAxisValue, AxisDragSpeed, 0.0f, 0.0f, FormatString, ImGuiSliderFlags_NoRoundToFormat);

        if (bIsRotationDegrees && ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f))
        {
            const float RoundedDegrees = Math::Round(InOutAxisValue);
            if (RoundedDegrees != InOutAxisValue)
            {
                InOutAxisValue    = RoundedDegrees;
                bAxisValueChanged = true;
            }
        }

        const bool bIsItemActive  = ImGui::IsItemActive();
        const bool bIsItemHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

        const ImGuiID LastItemId = ImGui::GetItemID();

        const bool bIsTempInputActive = ImGui::TempInputIsActive(LastItemId);
        if (!bIsTempInputActive)
        {
            // Recompute format after potential snapping.
            char FormatBufferAfter[8];
            const char* FormatStringAfter = GetDynamicFormatString(InOutAxisValue, FormatBufferAfter);

            char ValueTextBuffer[64];
            FCString::Snprintf(ValueTextBuffer, sizeof(ValueTextBuffer), FormatStringAfter, InOutAxisValue);

            ImGuiWindow* Window = ImGui::GetCurrentWindow();
            if (Window && !Window->SkipItems)
            {
                const ImVec2 ItemMin         = ImGui::GetItemRectMin();
                const ImVec2 ItemMax         = ImGui::GetItemRectMax();
                const ImU32  BackgroundColor = ImGui::GetColorU32(bIsItemActive ? ImGuiCol_FrameBgActive : (bIsItemHovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg));

                Window->DrawList->AddRectFilled(ItemMin, ItemMax, BackgroundColor, ImGui::GetStyle().FrameRounding);

                const float  TextStartX    = ItemMin.x + ImGui::GetStyle().FramePadding.x;
                const ImVec2 ValueTextSize = ImGui::CalcTextSize(ValueTextBuffer);
                const float  TextPosY      = ItemMin.y + (ItemMax.y - ItemMin.y - ValueTextSize.y) * 0.5f;

                Window->DrawList->AddText(ImVec2(TextStartX, TextPosY), ImGui::GetColorU32(ImGuiCol_Text), ValueTextBuffer);

                if (bIsRotationDegrees)
                {
                    const float DegreeGapPx = 1.0f;
                    const float DegreePosX  = TextStartX + ValueTextSize.x + DegreeGapPx;

                    Window->DrawList->AddText(ImVec2(DegreePosX, TextPosY), ImGui::GetColorU32(ImGuiCol_Text), "\xC2\xB0");
                }
            }
        }

        DrawInputBorderLastItem(ImGui::GetStyle().FrameRounding);
        DrawAxisLineForLastItem(AxisIndicatorColor);

        if (bIsRotationDegrees && bIsTempInputActive)
        {
            DrawSuffixAfterTempInputText(LastItemId, "\xC2\xB0", ImGui::GetColorU32(ImGuiCol_Text));
        }

        bRowHovered |= bIsItemHovered;
        bRowHovered |= bIsItemActive;

        ImGui::PopStyleVar();

        bAnyValueChanged |= bAxisValueChanged;
        return bAxisValueChanged;
    };

    const ImU32 XAxisColor = IM_COL32(204, 26, 38, 255);
    const ImU32 YAxisColor = IM_COL32(51, 179, 51, 255);
    const ImU32 ZAxisColor = IM_COL32(26, 64, 204, 255);

    bXChanged = DrawAxisField("##X", OutValue.X, XAxisColor, AxisFieldWidthPx, false);
    bYChanged = DrawAxisField("##Y", OutValue.Y, YAxisColor, AxisFieldWidthPx, true);
    bZChanged = DrawAxisField("##Z", OutValue.Z, ZAxisColor, AxisFieldWidthPx, true);

    if (bIsScaleControl && bUniformScaleEnabled)
    {
        if (bXChanged || bYChanged || bZChanged)
        {
            const float NewUniformScale = bXChanged ? OutValue.X : (bYChanged ? OutValue.Y : OutValue.Z);

            bool bAppliedUniformScale = false;

            if (OutValue.X != NewUniformScale)
            {
                OutValue.X = NewUniformScale;
                bAppliedUniformScale = true;
            }

            if (OutValue.Y != NewUniformScale)
            {
                OutValue.Y = NewUniformScale;
                bAppliedUniformScale = true;
            }

            if (OutValue.Z != NewUniformScale)
            {
                OutValue.Z = NewUniformScale;
                bAppliedUniformScale = true;
            }

            if (bAppliedUniformScale)
            {
                bAnyValueChanged = true;
            }
        }
    }

    ImGui::PopID();

    ImGui::TableSetColumnIndex(2);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 1.0f);

    const bool bHasRevertValue         = InRevertValue != nullptr;
    const bool bShouldShowRevertButton = bHasRevertValue && (OutValue != *InRevertValue);

    const float RevertSlotSizePx = FrameHeightPx;
    if (bShouldShowRevertButton)
    {
        if (ResetIconButton())
        {
            OutValue = *InRevertValue;
            bAnyValueChanged = true;
        }

        bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    }
    else
    {
        ImGui::Dummy(ImVec2(RevertSlotSizePx, RevertSlotSizePx));
    }

    ApplyHoveredRowBg(bRowHovered);
    return bAnyValueChanged;
}

bool EditorWidgets::DrawFloatProperty(const char* Label, float& InOutValue, float Speed, float MinValue, float MaxValue, const char* Format, bool bUseSlider, const float* InRevertValue, bool bEnabled)
{
    ImGuiTable* Table = ImGui::GetCurrentTable();
    if (!Table)
    {
        return false;
    }

    const float RowHeight = ImGui::GetFrameHeight();
    ImGui::TableNextRow();

    bool bResult = false;
    bool bRowHovered = BeginFullRowHoverCatcher(RowHeight);

    // Label
    ImGui::TableSetColumnIndex(0);
    
    const float LabelIndentPx = 24.0f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + LabelIndentPx);

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(Label);

    // Value
    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(Label);

    if (!bEnabled)
    {
        ImGui::BeginDisabled();
    }

    ImGui::SetNextItemWidth(-FLT_MIN);
    if (bUseSlider)
    {
        bResult = ImGui::SliderFloat("##Value", &InOutValue, MinValue, MaxValue, Format);
    }
    else
    {
        bResult = ImGui::DragFloat("##Value", &InOutValue, Speed, MinValue, MaxValue, Format);
    }

    ImGuiStyle& Style = ImGui::GetStyle();
    DrawInputBorderLastItem(Style.FrameRounding);

    bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    bRowHovered |= ImGui::IsItemActive();

    if (!bEnabled)
    {
        ImGui::EndDisabled();
    }

    ImGui::PopID();

    // Revert
    ImGui::TableSetColumnIndex(2);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 1.0f);

    const bool bCanRevert = (InRevertValue != nullptr);
    if (!bCanRevert || !bEnabled)
    {
        ImGui::BeginDisabled();
    }

    if (ResetIconButton())
    {
        InOutValue = *InRevertValue;
        bResult    = true;
    }

    bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

    if (!bCanRevert || !bEnabled)
    {
        ImGui::EndDisabled();
    }

    ApplyHoveredRowBg(bRowHovered);
    return bEnabled && bResult;
}

bool EditorWidgets::DrawCheckboxProperty(const char* Label, bool& InOutValue, const bool* InRevertValue, bool bEnabled)
{
    ImGuiTable* Table = ImGui::GetCurrentTable();
    if (!Table)
    {
        return false;
    }

    const float RowHeight = ImGui::GetFrameHeight();

    ImGui::TableNextRow();

    bool bResult     = false;
    bool bRowHovered = BeginFullRowHoverCatcher(RowHeight);

    ImGui::TableSetColumnIndex(0);

    const float LabelIndentPx = 24.0f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + LabelIndentPx);

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(Label);

    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(Label);

    if (!bEnabled)
    {
        ImGui::BeginDisabled();
    }

    bResult = ImGui::Checkbox("##Value", &InOutValue);
    
    ImGuiStyle& Style = ImGui::GetStyle();
    DrawInputBorderLastItem(Style.FrameRounding);

    bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    bRowHovered |= ImGui::IsItemActive();

    if (!bEnabled)
    {
        ImGui::EndDisabled();
    }

    ImGui::PopID();

    ImGui::TableSetColumnIndex(2);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 1.0f);

    const bool bCanRevert = (InRevertValue != nullptr);
    if (!bCanRevert || !bEnabled)
    {
        ImGui::BeginDisabled();
    }

    if (ResetIconButton() && bCanRevert)
    {
        InOutValue = *InRevertValue;
        bResult    = true;
    }

    bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

    if (!bCanRevert || !bEnabled)
    {
        ImGui::EndDisabled();
    }

    ApplyHoveredRowBg(bRowHovered);
    return bEnabled && bResult;
}

bool EditorWidgets::DrawColor3Property(const char* Label, float* InOutColor, const float* InRevertColor, bool bEnabled, ImGuiColorEditFlags Flags)
{
    ImGuiTable* Table = ImGui::GetCurrentTable();
    if (!Table)
    {
        return false;
    }

    const float RowHeight = ImGui::GetFrameHeight();
    ImGui::TableNextRow(0, RowHeight);

    bool bResult     = false;
    bool bRowHovered = BeginFullRowHoverCatcher(RowHeight);

    ImGui::TableSetColumnIndex(0);
    {
        // Optional indent (keep/remove depending on your current style)
        const float LabelIndentPx = 24.0f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + LabelIndentPx);

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(Label);
    }

    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(Label);

    if (!bEnabled)
    {
        ImGui::BeginDisabled();
    }

    ImGuiStyle& Style = ImGui::GetStyle();
    const float Gap         = 4.0f;
    const float FrameHeight = ImGui::GetFrameHeight();

    const float Avail       = ImGui::GetContentRegionAvail().x;
    const float ButtonWidth = FrameHeight;
    const float TotalGaps   = 3.0f * Gap;

    float FieldWidth = (Avail - ButtonWidth - TotalGaps) / 3.0f;
    FieldWidth = ImMax(FieldWidth, 1.0f);

    {
        const ImVec4 Color = ImVec4(InOutColor[0], InOutColor[1], InOutColor[2], 1.0f);

        const ImGuiColorEditFlags ButtonFlags = Flags | ImGuiColorEditFlags_NoTooltip;
        if (ImGui::ColorButton("##ColorBtn", Color, ButtonFlags, ImVec2(ButtonWidth, FrameHeight)))
        {
            ImGui::OpenPopup("##ColorPicker");
        }

        DrawInputBorderLastItem(Style.FrameRounding);

        bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        bRowHovered |= ImGui::IsItemActive();

        if (ImGui::BeginPopup("##ColorPicker"))
        {
            bResult |= ImGui::ColorPicker3("##Picker", InOutColor, Flags);
            ImGui::EndPopup();
        }
    }

    ImGui::SameLine(0.0f, Gap);

    // R / G / B drag floats (0..1)
    const auto DrawComponentDragFloat = [&](const char* Id, float& OutValue, bool bSameLine)
    {
        if (bSameLine)
        {
            ImGui::SameLine(0.0f, Gap);
        }

        ImGui::SetNextItemWidth(FieldWidth);
        bResult |= ImGui::DragFloat(Id, &OutValue, 0.01f, 0.0f, 1.0f, "%.3f");

        ImGuiStyle& Style = ImGui::GetStyle();
        DrawInputBorderLastItem(Style.FrameRounding);

        bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
        bRowHovered |= ImGui::IsItemActive();
    };

    DrawComponentDragFloat("##R", InOutColor[0], false);
    DrawComponentDragFloat("##G", InOutColor[1], true);
    DrawComponentDragFloat("##B", InOutColor[2], true);

    if (!bEnabled)
    {
        ImGui::EndDisabled();
    }

    ImGui::PopID();

    ImGui::TableSetColumnIndex(2);
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 1.0f);

    const bool bCanRevert = (InRevertColor != nullptr);
    if (!bCanRevert || !bEnabled)
    {
        ImGui::BeginDisabled();
    }

    if (ResetIconButton() && bCanRevert)
    {
        static constexpr uint64 SizeInBytes = sizeof(float[3]);
        FMemory::Memcpy(InOutColor, InRevertColor, SizeInBytes);
        bResult = true;
    }

    bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

    if (!bCanRevert || !bEnabled)
    {
        ImGui::EndDisabled();
    }

    ApplyHoveredRowBg(bRowHovered);
    return bEnabled && bResult;
}

void EditorWidgets::DrawTextProperty(const char* Label, const char* ValueText)
{
    ImGuiTable* Table = ImGui::GetCurrentTable();
    if (!Table)
    {
        ImGui::Text("%s: %s", Label, ValueText ? ValueText : "");
        return;
    }

    const float RowHeight = ImGui::GetFrameHeight();
    ImGui::TableNextRow();

    bool bRowHovered = BeginFullRowHoverCatcher(RowHeight);

    ImGui::TableSetColumnIndex(0);

    {
        const float LabelIndentPx = 24.0f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + LabelIndentPx);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(Label);
    }

    ImGui::TableSetColumnIndex(1);
    
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(ValueText ? ValueText : "");
    bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

    ImGui::TableSetColumnIndex(2);

    ApplyHoveredRowBg(bRowHovered);
}

void EditorWidgets::DrawReadOnlyFloat3Property(const char* Label, const FVector3& Value)
{
    ImGuiTable* Table = ImGui::GetCurrentTable();
    if (!Table)
    {
        ImGui::Text("%s: %.3f %.3f %.3f", Label, Value.X, Value.Y, Value.Z);
        return;
    }

    const float RowHeight = ImGui::GetFrameHeight();
    ImGui::TableNextRow();

    bool bRowHovered = BeginFullRowHoverCatcher(RowHeight);

    ImGui::TableSetColumnIndex(0);
    {
        const float LabelIndentPx = 24.0f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + LabelIndentPx);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(Label);
    }

    ImGui::TableSetColumnIndex(1);

    float Temp[3] = { Value.X, Value.Y, Value.Z };
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputFloat3("##Value", Temp, "%.3f", ImGuiInputTextFlags_ReadOnly);

    ImGuiStyle& Style = ImGui::GetStyle();
    DrawInputBorderLastItem(Style.FrameRounding);

    bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

    ImGui::TableSetColumnIndex(2);

    ApplyHoveredRowBg(bRowHovered);
}

void EditorWidgets::EditorDrawCheckMark(ImDrawList* DrawList, ImVec2 Position, ImU32 Color, float CheckMarkSize)
{
    const float Thickness = ImMax(1.0f, CheckMarkSize / 6.0f);

    const ImVec2 PointA = ImVec2(Position.x + CheckMarkSize * 0.15f, Position.y + CheckMarkSize * 0.55f);
    const ImVec2 PointB = ImVec2(Position.x + CheckMarkSize * 0.40f, Position.y + CheckMarkSize * 0.80f);
    const ImVec2 PointC = ImVec2(Position.x + CheckMarkSize * 0.85f, Position.y + CheckMarkSize * 0.20f);

    DrawList->AddLine(PointA, PointB, Color, Thickness);
    DrawList->AddLine(PointB, PointC, Color, Thickness);
}

void EditorWidgets::EditorMenuSeparator(float Thickness, float PaddingY)
{
    ImGuiWindow* Window = ImGui::GetCurrentWindow();
    if (!Window || Window->SkipItems)
    {
        return;
    }

    if (PaddingY > 0.0f)
    {
        ImGui::Dummy(ImVec2(0.0f, PaddingY));
    }

    const ImVec2 CursorMin = ImGui::GetCursorScreenPos();
    const float  Width     = ImGui::GetContentRegionAvail().x;

    constexpr float InsetX = 20.0f;

    if (Width > (InsetX * 2.0f + 1.0f))
    {
        const ImU32 Color = IM_COL32(106, 106, 106, 255);

        const float Y  = CursorMin.y + ((Thickness <= 1.0f) ? 0.5f : 0.0f);
        const ImVec2 A = ImVec2(CursorMin.x + InsetX, Y);
        const ImVec2 B = ImVec2(CursorMin.x + Width - InsetX, Y);

        ImGui::GetWindowDrawList()->AddLine(A, B, Color, Thickness);
    }

    ImGui::Dummy(ImVec2(0.0f, Thickness));

    if (PaddingY > 0.0f)
    {
        ImGui::Dummy(ImVec2(0.0f, PaddingY));
    }
}

void EditorWidgets::EditorMenuLabeledSeparator(const char* Label, float Thickness, float PaddingY)
{
    ImGuiWindow* Window = ImGui::GetCurrentWindow();
    if (!Window || Window->SkipItems)
    {
        return;
    }

    if (PaddingY > 0.0f)
    {
        ImGui::Dummy(ImVec2(0.0f, PaddingY));
    }

    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    const ImVec2 CursorMin = ImGui::GetCursorScreenPos();
    const float  Width     = ImGui::GetContentRegionAvail().x;

    constexpr float InsetX = 20.0f;

    const ImU32 LineColor  = IM_COL32(106, 106, 106, 255);
    const ImU32 LabelColor = IM_COL32(160, 160, 160, 255);

    const char* LabelToDraw = "";
    char UpperLabel[128] = {};

    if (Label && Label[0] != '\0')
    {
        int32 i = 0;
        for (; Label[i] != '\0' && i < (int32)(sizeof(UpperLabel) - 1); ++i)
        {
            const CHAR Ch = (CHAR)Label[i];
            UpperLabel[i] = (char)FCharTraits::ToUpper(Ch);
        }

        UpperLabel[i] = '\0';
        LabelToDraw   = UpperLabel;
    }

    const ImVec2 LabelSize = ImGui::CalcTextSize(LabelToDraw);
    const float  RowHeight = Math::Max(LabelSize.y, Thickness);
    const float  YLine     = CursorMin.y + (RowHeight * 0.5f) + ((Thickness <= 1.0f) ? 0.5f : 0.0f);
    const float  LabelX    = CursorMin.x + InsetX;
    const float  LabelY    = CursorMin.y + (RowHeight - LabelSize.y) * 0.5f;

    if (LabelSize.x > 0.0f)
    {
        DrawList->AddText(ImVec2(LabelX, LabelY), LabelColor, LabelToDraw);
    }

    const float GapAfterLabel = 12.0f;
    const float LineStartX    = LabelX + LabelSize.x + GapAfterLabel;
    const float LineEndX      = CursorMin.x + Width - InsetX;

    if (LineEndX > LineStartX + 1.0f)
    {
        DrawList->AddLine(ImVec2(LineStartX, YLine), ImVec2(LineEndX, YLine), LineColor, Thickness);
    }

    ImGui::Dummy(ImVec2(0.0f, RowHeight));

    if (PaddingY > 0.0f)
    {
        ImGui::Dummy(ImVec2(0.0f, PaddingY));
    }
}

bool EditorWidgets::EditorMenuItem(const char* Label, const char* Shortcut, bool bSelected, bool bEnabled, bool bDrawBorder)
{
    const float PaddingX    = 8.0f;
    const float PaddingY    = 4.0f;
    const float RowHeight   = ImGui::GetFontSize() + PaddingY * 2.0f;
    const float RowWidth    = ImGui::GetContentRegionAvail().x;
    const float CheckSize   = ImGui::GetFontSize() * 0.85f;
    const float GapRight    = 8.0f;
    const float ClipGap     = 4.0f;
    const float MenuIndentX = 20.0f;
    const float IconGutterX = 0.0f;

    ImGui::PushID(Label);

    if (!bEnabled)
    {
        ImGui::BeginDisabled();
    }

    const ImGuiSelectableFlags Flags =
        ImGuiSelectableFlags_SpanAvailWidth |
        ImGuiSelectableFlags_NoPadWithHalfSpacing;

    const bool bPressed = ImGui::Selectable("##row", false, Flags, ImVec2(RowWidth, RowHeight));
    const bool bHovered = ImGui::IsItemHovered();
    const bool bActive  = ImGui::IsItemActive();

    const ImVec2 RectMin = ImGui::GetItemRectMin();
    const ImVec2 RectMax = ImGui::GetItemRectMax();

    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    // -----------------------------------------------------------------------------------------
    // Background
    // -----------------------------------------------------------------------------------------
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

    if (bDrawBorder && bHovered)
    {
        const ImVec4 HoveredColor = ImVec4(0.0f / 255.0f, 112.0f / 255.0f, 224.0f / 255.0f, 1.0f);
        const ImU32  BorderColor = EditorHelpers::MakeBrighterColorU32(HoveredColor, 0.20f);
        DrawList->AddRect(RectMin, RectMax, BorderColor, 0.0f, 0, 1.0f);
    }

    // -----------------------------------------------------------------------------------------
    // Layout
    // -----------------------------------------------------------------------------------------
    const float LeftInnerX  = RectMin.x + MenuIndentX;
    const float RightInnerX = RectMax.x - MenuIndentX;

    const bool bHasShortcut = (Shortcut && Shortcut[0] != '\0');

    // -----------------------------------------------------------------------------------------
    // Upper-case shortcut
    // -----------------------------------------------------------------------------------------
    const char* ShortcutToDraw = Shortcut;

    char UpperShortcut[128] = {};
    if (bHasShortcut)
    {
        int32 i = 0;
        for (; Shortcut[i] != '\0' && i < (int32)(sizeof(UpperShortcut) - 1); ++i)
        {
            const CHAR Ch    = (CHAR)Shortcut[i];
            UpperShortcut[i] = (char)FCharTraits::ToUpper(Ch);
        }
        UpperShortcut[i] = '\0';
        ShortcutToDraw   = UpperShortcut;
    }

    const ImVec2 ShortcutSize = bHasShortcut ? ImGui::CalcTextSize(ShortcutToDraw) : ImVec2(0.0f, 0.0f);
    const ImVec2 LabelSize    = ImGui::CalcTextSize(Label);

    const float OpticalBiasY = 0.5f;
    const float LabelY       = RectMin.y + (RowHeight - LabelSize.y) * 0.5f + OpticalBiasY;

    const bool bDrawCheckMark = bSelected;
    const bool bDrawShortcut  = bHasShortcut;

    ImVec2 CheckPos         = ImVec2(0.0f, 0.0f);
    float  ShortcutX        = 0.0f;
    float  RightContentMinX = RightInnerX;

    if (bDrawCheckMark)
    {
        const float CheckY    = RectMin.y + (RowHeight - CheckSize) * 0.5f + OpticalBiasY;
        const float CheckMinX = RightInnerX - CheckSize;

        CheckPos         = ImVec2(CheckMinX, CheckY);
        RightContentMinX = CheckMinX;

        if (bDrawShortcut)
        {
            ShortcutX        = CheckMinX - GapRight - ShortcutSize.x;
            RightContentMinX = ShortcutX;
        }
    }
    else if (bDrawShortcut)
    {
        ShortcutX        = RightInnerX - ShortcutSize.x;
        RightContentMinX = ShortcutX;
    }

    // -----------------------------------------------------------------------------------------
    // Draw shortcut text
    // -----------------------------------------------------------------------------------------
    if (bDrawShortcut)
    {
        const float ShortcutY = RectMin.y + (RowHeight - ShortcutSize.y) * 0.5f + OpticalBiasY;
        DrawList->AddText(ImVec2(ShortcutX, ShortcutY), ImGui::GetColorU32(ImGuiCol_TextDisabled), ShortcutToDraw);
    }

    // -----------------------------------------------------------------------------------------
    // Draw checkmark
    // -----------------------------------------------------------------------------------------
    if (bDrawCheckMark)
    {
        if (EditorIcons::Checkmark)
        {
            const ImVec2 IconMin = CheckPos;
            const ImVec2 IconMax = ImVec2(CheckPos.x + CheckSize, CheckPos.y + CheckSize);

            // Tint to match menu text color (works well for monochrome icons)
            const ImU32 Tint = ImGui::GetColorU32(ImGuiCol_Text);

            DrawList->AddImage(EditorIcons::Checkmark, IconMin, IconMax, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), Tint);
        }
        else
        {
            // Fallback if icon not loaded
            EditorDrawCheckMark(DrawList, CheckPos, ImGui::GetColorU32(ImGuiCol_Text), CheckSize);
        }
    }

    // -----------------------------------------------------------------------------------------
    // Draw label
    // -----------------------------------------------------------------------------------------
    const float LabelX = LeftInnerX + PaddingX + IconGutterX;

    float ClipMaxX = RightInnerX - PaddingX;
    if (bDrawShortcut || bDrawCheckMark)
    {
        ClipMaxX = RightContentMinX - ClipGap;
    }

    ClipMaxX = Math::Max(ClipMaxX, LabelX + 1.0f);

    DrawList->PushClipRect(ImVec2(LabelX, RectMin.y), ImVec2(ClipMaxX, RectMax.y), true);
    DrawList->AddText(ImVec2(LabelX, LabelY), ImGui::GetColorU32(ImGuiCol_Text), Label);
    DrawList->PopClipRect();

    if (!bEnabled)
    {
        ImGui::EndDisabled();
    }

    ImGui::PopID();
    return bEnabled && bPressed;
}

void EditorWidgets::EditorDrawMenuButton(const char* Label, const char* PopupId, bool bAnyPopupOpen, const ImVec4& BrightPopupBg, float ButtonHeight, PopupAnchor& OutAnchor, bool bDrawBorder)
{
    const bool bThisPopupOpen = ImGui::IsPopupOpen(PopupId, ImGuiPopupFlags_None);
    OutAnchor.bRequestPosition = false;

    if (bThisPopupOpen)
    {
        const ImVec4 PressedBlue = ImVec4(0.0f / 255.0f, 112.0f / 255.0f, 224.0f / 255.0f, 1.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, PressedBlue);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, PressedBlue);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, PressedBlue);
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, 0.0f));
    const bool bPressed = ImGui::Button(Label, ImVec2(0.0f, ButtonHeight));
    ImGui::PopStyleVar();

    if (bPressed)
    {
        ImGui::OpenPopup(PopupId);
        OutAnchor.bRequestPosition = true;
    }

    if (bAnyPopupOpen && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup) && !bThisPopupOpen)
    {
        ImGui::OpenPopup(PopupId);
        OutAnchor.bRequestPosition = true;
    }

    const bool bHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);

    OutAnchor.Min = ImGui::GetItemRectMin();
    OutAnchor.Max = ImGui::GetItemRectMax();

    if (bDrawBorder && bHovered)
    {
        const ImU32 BorderColor = ImGui::GetColorU32(ImGuiCol_Border);

        ImDrawList* DrawList = ImGui::GetWindowDrawList();
        DrawList->AddRect(OutAnchor.Min, OutAnchor.Max, BorderColor, 0.0f, 0, 1.0f);
    }

    if (bThisPopupOpen)
    {
        ImGui::PopStyleColor(3);
    }
}

bool EditorWidgets::EditorBeginMenuPopup(const char* PopupId, const PopupAnchor& Anchor, float MinWidth)
{
    if (Anchor.bRequestPosition || ImGui::IsPopupOpen(PopupId, ImGuiPopupFlags_None))
    {
        ImGui::SetNextWindowPos(ImVec2(Anchor.Min.x, Anchor.Max.y), ImGuiCond_Always);
    }

    // -----------------------------------------------------------------------------------------
    // Popup styling
    // -----------------------------------------------------------------------------------------
    const ImVec4 PopupBg       = ImVec4(56.0f / 255.0f, 56.0f / 255.0f, 56.0f / 255.0f, 1.0f);
    const ImVec4 PopupBorder   = ImVec4(63.0f / 255.0f, 63.0f / 255.0f, 63.0f / 255.0f, 1.0f);
    const ImVec4 TextColor     = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    const ImVec4 ShortcutColor = ImVec4(175.0f / 255.0f, 175.0f / 255.0f, 175.0f / 255.0f, 1.0f);
    const ImVec4 HoverBlue     = ImVec4(0.0f / 255.0f, 112.0f / 255.0f, 224.0f / 255.0f, 1.0f);

    constexpr float PopupPadY      = 10.0f;
    constexpr float ContentIndentX = 20.0f;

    const float FinalMinWidth = MinWidth + (ContentIndentX * 2.0f);

    // -----------------------------------------------------------------------------------------
    // Style vars
    // -----------------------------------------------------------------------------------------
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.0f);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, PopupPadY));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));

    // -----------------------------------------------------------------------------------------
    // Style colors
    // -----------------------------------------------------------------------------------------
    ImGui::PushStyleColor(ImGuiCol_PopupBg, PopupBg);
    ImGui::PushStyleColor(ImGuiCol_Border, PopupBorder);

    ImGui::PushStyleColor(ImGuiCol_Text, TextColor);
    ImGui::PushStyleColor(ImGuiCol_TextDisabled, ShortcutColor);

    ImGui::PushStyleColor(ImGuiCol_Header, PopupBg);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, HoverBlue);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, HoverBlue);

    ImGui::SetNextWindowSizeConstraints(ImVec2(FinalMinWidth, 0.0f), ImVec2(FLT_MAX, FLT_MAX));

    const bool bOpen = ImGui::BeginPopup(PopupId);
    if (bOpen)
    {
        ImDrawList* DrawList = ImGui::GetWindowDrawList();

        const ImVec2 WinPos  = ImGui::GetWindowPos();
        const ImVec2 WinSize = ImGui::GetWindowSize();
        const ImVec2 Min     = ImVec2(WinPos.x + 1.0f, WinPos.y + 1.0f);
        const ImVec2 Max     = ImVec2(WinPos.x + WinSize.x - 1.0f, WinPos.y + WinSize.y - 1.0f);

        DrawList->AddRect(Min, Max, IM_COL32(50, 50, 50, 255), 0.0f, 0, 1.0f);
    }

    return bOpen;
}

void EditorWidgets::EditorResetMenuPopup()
{
    ImGui::PopStyleColor(7); // PopupBg, Border, Text, TextDisabled, Header, HeaderHovered, HeaderActive
    ImGui::PopStyleVar(5);   // WindowBorderSize, PopupBorderSize, PopupRounding, WindowPadding, ItemSpacing
}

bool EditorWidgets::BeginPropertyTable(const char* TableId, float LabelColumnWidth, float RevertColumnWidth)
{
    // -----------------------------------------------------------------------------------------
    // colors
    // -----------------------------------------------------------------------------------------
    const ImVec4 RowBg       = ImVec4(36.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 1.0f);
    const ImVec4 TableBorder = ImVec4(26.0f / 255.0f, 26.0f / 255.0f, 26.0f / 255.0f, 1.0f);
    const ImVec4 FrameBg     = ImVec4(15.0f / 255.0f, 15.0f / 255.0f, 15.0f / 255.0f, 1.0f);

    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(6.0f, 4.0f));

    ImGui::PushStyleColor(ImGuiCol_TableRowBg, RowBg);
    ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, RowBg);

    ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, TableBorder);
    ImGui::PushStyleColor(ImGuiCol_TableBorderLight, TableBorder);

    ImGui::PushStyleColor(ImGuiCol_FrameBg, FrameBg);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, FrameBg);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, FrameBg);

    const ImGuiTableFlags Flags =
        ImGuiTableFlags_SizingStretchProp |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_NoSavedSettings |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_NoPadOuterX |
        ImGuiTableFlags_BordersInnerH |
        ImGuiTableFlags_BordersInnerV;

    ImVec2 HeaderMin = ImGui::GetItemRectMin();
    ImVec2 HeaderMax = ImGui::GetItemRectMax();

    if (HeaderMax.x <= HeaderMin.x)
    {
        const ImVec2 CursorScreenPos = ImGui::GetCursorScreenPos();
        HeaderMin = CursorScreenPos;
        HeaderMax = ImVec2(CursorScreenPos.x + ImGui::GetContentRegionAvail().x, CursorScreenPos.y);
    }

    ImGui::SetCursorScreenPos(ImVec2(HeaderMin.x, HeaderMax.y));

    const float TableWidth = HeaderMax.x - HeaderMin.x;
    const ImVec2 OuterSize = ImVec2(TableWidth, 0.0f);

    ImGuiStorage* Storage = ImGui::GetStateStorage();

    const ImVec2 BorderMin = ImGui::GetCursorScreenPos();
    Storage->SetFloat(ImGui::GetID("##LastPropTableX"), BorderMin.x);
    Storage->SetFloat(ImGui::GetID("##LastPropTableY"), BorderMin.y);
    Storage->SetFloat(ImGui::GetID("##LastPropTableW"), TableWidth);

    if (!ImGui::BeginTable(TableId, 3, Flags, OuterSize))
    {
        ImGui::PopStyleColor(7);
        ImGui::PopStyleVar();
        return false;
    }

    ImGui::TableSetupColumn("##Label", ImGuiTableColumnFlags_WidthFixed, LabelColumnWidth);
    ImGui::TableSetupColumn("##Value", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("##Revert", ImGuiTableColumnFlags_WidthFixed, RevertColumnWidth);
    return true;
}

void EditorWidgets::EndPropertyTable()
{
    ImGui::EndTable();

    ImGuiStorage* Storage = ImGui::GetStateStorage();
    
    const float X0 = Storage->GetFloat(ImGui::GetID("##LastPropTableX"), 0.0f);
    const float Y0 = Storage->GetFloat(ImGui::GetID("##LastPropTableY"), 0.0f);
    const float W  = Storage->GetFloat(ImGui::GetID("##LastPropTableW"), 0.0f);

    if (W > 0.0f)
    {
        const ImGuiStyle& Style = ImGui::GetStyle();

        const ImVec2 CursorAfterTable = ImGui::GetCursorScreenPos();
        float BottomY = CursorAfterTable.y - Style.ItemSpacing.y;

        const ImU32 Color     = ImGui::GetColorU32(ImGuiCol_TableBorderStrong);
        const float Thickness = 2.0f;

        const float X1      = X0 + W;
        const float YTop    = Math::Floor(Y0) + 0.5f;
        const float YBottom = Math::Ceil(BottomY) - 0.5f;

        ImDrawList* DrawList = ImGui::GetWindowDrawList();
        DrawList->AddLine(ImVec2(X0, YTop),    ImVec2(X1, YTop),    Color, Thickness);
        DrawList->AddLine(ImVec2(X0, YBottom), ImVec2(X1, YBottom), Color, Thickness);

        ImGui::SetCursorScreenPos(ImVec2(CursorAfterTable.x, BottomY + Thickness * 0.5f));
    }

    ImGui::PopStyleColor(7);
    ImGui::PopStyleVar();
}

void EditorWidgets::PropertySeparatorRow(float PaddingY)
{
    ImGuiTable* Table = ImGui::GetCurrentTable();
    if (!Table)
    {
        ImGui::Separator();
        return;
    }

    ImGui::TableNextRow();

    const int32 ColumnCount = ImGui::TableGetColumnCount();
    for (int32 ColumnIndex = 0; ColumnIndex < ColumnCount; ++ColumnIndex)
    {
        ImGui::TableSetColumnIndex(ColumnIndex);
        ImGui::Separator();
    }
}

void EditorWidgets::PropertyRowLabel(const char* Label)
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(Label);

    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-FLT_MIN); // Fill available width
}

struct EditorIcon
{
    void Reset()
    {
        Texture      = nullptr;
        ImGuiTexture = nullptr;
    }

    FTextureRef               Texture      = nullptr;
    TUniquePtr<FImGuiTexture> ImGuiTexture = nullptr;
};

struct EditorIconsInternal
{
    inline static EditorIcon UndoIcon            = EditorIcon();
    inline static EditorIcon SearchIcon          = EditorIcon();
    inline static EditorIcon LockedIcon          = EditorIcon();
    inline static EditorIcon UnlockedIcon        = EditorIcon();
    inline static EditorIcon FolderIcon          = EditorIcon();
    inline static EditorIcon FolderSmallIcon     = EditorIcon();
    inline static EditorIcon FolderOpenSmallIcon = EditorIcon();
    inline static EditorIcon DocumentIcon        = EditorIcon();
    inline static EditorIcon DocumentSmallIcon   = EditorIcon();
    inline static EditorIcon Checkmark           = EditorIcon();
};

ImTextureID EditorIcons::UndoIcon            = nullptr;
ImTextureID EditorIcons::SearchIcon          = nullptr;
ImTextureID EditorIcons::LockedIcon          = nullptr;
ImTextureID EditorIcons::UnlockedIcon        = nullptr;
ImTextureID EditorIcons::FolderIcon          = nullptr;
ImTextureID EditorIcons::FolderSmallIcon     = nullptr;
ImTextureID EditorIcons::FolderOpenSmallIcon = nullptr;
ImTextureID EditorIcons::DocumentIcon        = nullptr;
ImTextureID EditorIcons::DocumentSmallIcon   = nullptr;
ImTextureID EditorIcons::Checkmark           = nullptr;

static bool LoadEditorIcon(const CHAR* InRelativePath, ImTextureID& OutIconID, EditorIcon& OutIcon, bool bEnableBlending = true, bool bEnableLinearSampler = true)
{
    OutIcon.Reset();
    OutIconID = nullptr;

    FString FullPath = FPaths::GetAssetDir();
    if (!FullPath.EndsWith("/"))
    {
        FullPath += "/";
    }

    FullPath += InRelativePath;

    FTextureRef Texture = FAssetManager::Get().LoadTexture(FullPath, false);
    if (!Texture)
    {
        LOG_ERROR("[EditorIcons]: Failed to load icon texture '%s'", *FullPath);
        return false;
    }

    OutIcon.Texture = Texture;

    FTexture2D* Texture2D = Texture->GetTexture2D();
    if (!Texture2D)
    {
        LOG_ERROR("[EditorIcons]: Icon '%s' is not a 2D texture.", *FullPath);
        return false;
    }

    FRHITextureRef TextureRHI = Texture2D->GetRHITexture();
    if (!TextureRHI)
    {
        LOG_ERROR("[EditorIcons]: Icon '%s' has no RHI texture.", *FullPath);
        return false;
    }

    OutIcon.ImGuiTexture = MakeUniquePtr<FImGuiTexture>(TextureRHI, EResourceAccess::PixelShaderResource);
    if (!OutIcon.ImGuiTexture)
    {
        LOG_ERROR("[EditorIcons]: Failed to create ImGui texture wrapper for '%s'.", *FullPath);
        return false;
    }
    else
    {
        OutIcon.ImGuiTexture->bEnableBlending      = bEnableBlending;
        OutIcon.ImGuiTexture->bEnableLinearSampler = bEnableLinearSampler;

        OutIconID = reinterpret_cast<ImTextureID>(OutIcon.ImGuiTexture.Get());
    }

    return true;
}

static void UnloadEditorIcon(ImTextureID& OutIconID, EditorIcon& OutIcon)
{
    if (OutIcon.Texture)
    {
        FAssetManager::Get().UnloadTexture(OutIcon.Texture);
    }

    OutIcon.Reset();
    OutIconID = nullptr;
}

bool EditorIcons::Initialize()
{
    bool bResult = true;

    bResult &= LoadEditorIcon("Editor/Icons/Undo.png", UndoIcon, EditorIconsInternal::UndoIcon);
    bResult &= LoadEditorIcon("Editor/Icons/Search.png", SearchIcon, EditorIconsInternal::SearchIcon);
    bResult &= LoadEditorIcon("Editor/Icons/Locked.png", LockedIcon, EditorIconsInternal::LockedIcon);
    bResult &= LoadEditorIcon("Editor/Icons/Unlocked.png", UnlockedIcon, EditorIconsInternal::UnlockedIcon);
    bResult &= LoadEditorIcon("Editor/Icons/Folder.png", FolderIcon, EditorIconsInternal::FolderIcon);
    bResult &= LoadEditorIcon("Editor/Icons/FolderSmall.png", FolderSmallIcon, EditorIconsInternal::FolderSmallIcon);
    bResult &= LoadEditorIcon("Editor/Icons/FolderOpenSmall.png", FolderOpenSmallIcon, EditorIconsInternal::FolderOpenSmallIcon);
    bResult &= LoadEditorIcon("Editor/Icons/Document.png", DocumentIcon, EditorIconsInternal::DocumentIcon);
    bResult &= LoadEditorIcon("Editor/Icons/DocumentSmall.png", DocumentSmallIcon, EditorIconsInternal::DocumentSmallIcon);
    bResult &= LoadEditorIcon("Editor/Icons/Checkmark.png", Checkmark, EditorIconsInternal::Checkmark);

    return bResult;
}

void EditorIcons::Release()
{
    UnloadEditorIcon(UndoIcon, EditorIconsInternal::UndoIcon);
    UnloadEditorIcon(SearchIcon, EditorIconsInternal::SearchIcon);
    UnloadEditorIcon(LockedIcon, EditorIconsInternal::LockedIcon);
    UnloadEditorIcon(UnlockedIcon, EditorIconsInternal::UnlockedIcon);
    UnloadEditorIcon(FolderIcon, EditorIconsInternal::FolderIcon);
    UnloadEditorIcon(FolderSmallIcon, EditorIconsInternal::FolderSmallIcon);
    UnloadEditorIcon(FolderOpenSmallIcon, EditorIconsInternal::FolderOpenSmallIcon);
    UnloadEditorIcon(DocumentIcon, EditorIconsInternal::DocumentIcon);
    UnloadEditorIcon(DocumentSmallIcon, EditorIconsInternal::DocumentSmallIcon);
    UnloadEditorIcon(Checkmark, EditorIconsInternal::Checkmark);
}

ImFont* EditorFonts::DefaultFont = nullptr;
ImFont* EditorFonts::SegoeUI_18  = nullptr;
ImFont* EditorFonts::Consola_14  = nullptr;

static ImFont* LoadEditorFont(const CHAR* InRelativePath, float SizePixels, const ImFontConfig* FontCfgTemplate = nullptr, const ImWchar* GlyphRanges = nullptr)
{
    FString FullPath = FPaths::GetAssetDir();
    if (!FullPath.EndsWith("/"))
    {
        FullPath += "/";
    }

    FullPath += InRelativePath;

    ImGuiIO& State = ImGui::GetIO();
    return State.Fonts->AddFontFromFileTTF(*FullPath, SizePixels, FontCfgTemplate, GlyphRanges);
}

bool EditorFonts::Initialize()
{
    if (!IImguiPlugin::IsEnabled())
    {
        return false;
    }

    ImGuiIO& State = ImGui::GetIO();
    State.Fonts->Clear();
    
    DefaultFont = State.Fonts->AddFontDefault();
    SegoeUI_18  = LoadEditorFont("Editor/Fonts/segoeui.ttf", 18.0f);
    Consola_14  = LoadEditorFont("Editor/Fonts/consola.ttf", 14.0f);

    IImguiPlugin& ImGuiPlugin = IImguiPlugin::Get();
    if (!ImGuiPlugin.UpdateFontAtlas())
    {
        return false;
    }

    if (SegoeUI_18)
    {
        State.FontDefault = SegoeUI_18;
    }

    return true;
}

void EditorFonts::Release()
{
    DefaultFont = nullptr;
    SegoeUI_18  = nullptr;
    Consola_14  = nullptr;
}
