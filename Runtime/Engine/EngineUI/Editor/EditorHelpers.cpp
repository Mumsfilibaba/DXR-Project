#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "Core/Containers/StaticArray.h"
#include "Core/Misc/Paths.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Templates/CString.h"
#include "Engine/Assets/AssetManager.h"
#include "ImGuiPlugin/ImGuiRenderer.h"
#include <imgui_internal.h>

float  EditorStyleVars::MainMenuBarHeight = 28.0f;

ImVec2 EditorStyleVars::InputFieldFramePadding    = ImVec2(12.0f, 6.0f);
float  EditorStyleVars::InputFieldBorderThickness = 2.0f;
float  EditorStyleVars::InputFieldBorderRounding  = 16.0f;
ImU32  EditorStyleVars::InputFieldBorderColor     = IM_COL32(100, 136, 234, 255);
ImVec4 EditorStyleVars::InputFieldSelectionColor  = ImVec4(0.0f / 255.0f, 112.0f / 255.0f, 224.0f / 255.0f, 1.0f);

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
    Window->DrawList->AddRect(Min, Max, Color, Rounding, ImDrawFlags_None, Thickness);
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

static constexpr float FixedRevertButtonSize = 20.0f;

static bool ResetIconButton(float InSize = 0.0f)
{
    float ButtonSizePx = InSize;
    if (ButtonSizePx <= 0.0f)
    {
        ButtonSizePx = FixedRevertButtonSize;
    }

    ButtonSizePx = Math::Max(1.0f, ButtonSizePx);
    const ImVec2 ButtonSize = ImVec2(ButtonSizePx, ButtonSizePx);

    ImGuiID UniqueSeedId = ImGui::GetItemID();
    if (UniqueSeedId == 0)
    {
        UniqueSeedId = ImGui::GetID("##ResetIconButtonSeed");
    }

    ImGui::PushID(static_cast<int32>(UniqueSeedId));

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
        const int32 Alpha255 = static_cast<int32>(Alpha01 * 255.0f);

        const ImU32 IconTintColor = IM_COL32(IconTintValue, IconTintValue, IconTintValue, Alpha255);
        WindowDrawList->AddImage(EditorIcons::UndoIcon, IconRectMin, IconRectMax, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), IconTintColor);
    }
    else
    {
        const CHAR* FallbackText = "R";

        const ImVec2 FallbackTextSize     = ImGui::CalcTextSize(FallbackText);
        const ImVec2 FallbackTextPosition = ImVec2((ButtonRectMin.x + ButtonRectMax.x) * 0.5f - FallbackTextSize.x * 0.5f, (ButtonRectMin.y + ButtonRectMax.y) * 0.5f - FallbackTextSize.y * 0.5f);

        const ImU32 TextColor = ImGui::GetColorU32(ImGuiCol_Text);
        WindowDrawList->AddText(FallbackTextPosition, TextColor, FallbackText);
    }

    ImGui::PopID();
    return bWasPressed;
}

static void DrawSuffixAfterTempInputText(ImGuiID InItemId, const CHAR* InSuffix, ImU32 InColor, float InGap = 1.0f)
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

    const CHAR* EditText = Context.InputTextState.TextA.Data;
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
    const float DesiredX   = TextStartX + TextSize.x + InGap;

    if (DesiredX + SuffixSize.x > RightLimit)
    {
        return;
    }

    const float Y = ItemMin.y + (ItemMax.y - ItemMin.y - SuffixSize.y) * 0.5f;
    Window->DrawList->AddText(ImVec2(DesiredX, Y), InColor, InSuffix);
}

static FORCEINLINE int32 ClampInt32(int32 V, int32 MinV, int32 MaxV)
{
    return (V < MinV) ? MinV : (V > MaxV ? MaxV : V);
}

static FORCEINLINE void NormalizeSelection(RichTextSelectionPoint& A, RichTextSelectionPoint& B)
{
    if (A.Line > B.Line || (A.Line == B.Line && A.Column > B.Column))
    {
        RichTextSelectionPoint Temp = A;
        A = B;
        B = Temp;
    }
}

static FORCEINLINE bool SelectionIntersectsLine(const RichTextSelectionPoint& SelA, const RichTextSelectionPoint& SelB, int32 LineIndex, int32& OutColStart, int32& OutColEnd, int32 LineCharCount)
{
    OutColStart = 0;
    OutColEnd   = 0;

    if (LineIndex < SelA.Line || LineIndex > SelB.Line)
    {
        return false;
    }

    if (SelA.Line == SelB.Line)
    {
        OutColStart = ClampInt32(SelA.Column, 0, LineCharCount);
        OutColEnd   = ClampInt32(SelB.Column, 0, LineCharCount);
        return OutColEnd > OutColStart;
    }

    if (LineIndex == SelA.Line)
    {
        OutColStart = ClampInt32(SelA.Column, 0, LineCharCount);
        OutColEnd   = LineCharCount;
        return OutColEnd > OutColStart;
    }

    if (LineIndex == SelB.Line)
    {
        OutColStart = 0;
        OutColEnd   = ClampInt32(SelB.Column, 0, LineCharCount);
        return OutColEnd > OutColStart;
    }

    OutColStart = 0;
    OutColEnd   = LineCharCount;
    return LineCharCount > 0;
}

static String BuildSelectedText(const RichTextViewContext& Ctx)
{
    if (!Ctx.bHasSelection || Ctx.Lines.IsEmpty())
    {
        return String();
    }

    RichTextSelectionPoint A = Ctx.SelStart;
    RichTextSelectionPoint B = Ctx.SelEnd;
    NormalizeSelection(A, B);

    String Result;

    const int32 LineMin = ClampInt32(A.Line, 0, Ctx.Lines.Size() - 1);
    const int32 LineMax = ClampInt32(B.Line, 0, Ctx.Lines.Size() - 1);

    for (int32 L = LineMin; L <= LineMax; ++L)
    {
        const RichTextLine& Line = Ctx.Lines[L];

        String FullLine;
        for (int32 s = 0; s < Line.Spans.Size(); ++s)
        {
            FullLine += Line.Spans[s].Text;
        }

        const CHAR* Full = *FullLine;
        const int32 FullLen = static_cast<int32>(CString::Strlen(Full));

        int32 SelColStart = 0;
        int32 SelColEnd   = 0;

        RichTextSelectionPoint NA = A;
        RichTextSelectionPoint NB = B;

        if (!SelectionIntersectsLine(NA, NB, L, SelColStart, SelColEnd, FullLen))
        {
            continue;
        }

        SelColStart = ClampInt32(SelColStart, 0, FullLen);
        SelColEnd   = ClampInt32(SelColEnd, 0, FullLen);

        if (SelColEnd > SelColStart)
        {
            const int32 SubLen = SelColEnd - SelColStart;

            String Sub;
            Sub.Reserve(SubLen + 1);

            for (int32 i = 0; i < SubLen; ++i)
            {
                const CHAR C = Full[SelColStart + i];
                Sub += C;
            }

            Result += Sub;
            if (L != LineMax)
            {
                Result += "\n";
            }
        }
    }

    return Result;
}

static RichTextSelectionPoint GetMouseSelectionPoint(const RichTextViewContext& Ctx, const ImVec2& MousePos)
{
    RichTextSelectionPoint P{};
    P.Line   = 0;
    P.Column = 0;

    const float StartY = Ctx.ContentStart.y + Ctx.Padding.y;
    const float StartX = Ctx.ContentStart.x + Ctx.Padding.x;
    const float LocalY = (MousePos.y - StartY);
    const float LocalX = (MousePos.x - StartX);

    const int32 LineIndex   = (Ctx.LineHeight > 0.0f) ? static_cast<int32>(LocalY / Ctx.LineHeight) : 0;
    const int32 MaxLine     = Math::Max(0, Ctx.Lines.Size() - 1);
    const int32 ClampedLine = ClampInt32(LineIndex, 0, MaxLine);

    P.Line = ClampedLine;

    const int32 LineChars = Ctx.Lines.IsValidIndex(ClampedLine) ? Ctx.Lines[ClampedLine].TotalChars : 0;
    const int32 Col       = (Ctx.CharWidth > 0.0f) ? static_cast<int32>(LocalX / Ctx.CharWidth) : 0;

    P.Column = ClampInt32(Col, 0, LineChars);
    return P;
}

bool EditorWidgets::DrawFloat3Control(const CHAR* Label, Vector3& OutValue, float Speed, const Vector3* InRevertValue, EVector3ControlType InType)
{
    ImGuiTable* CurrentTable = ImGui::GetCurrentTable();
    if (!CurrentTable)
    {
        return false;
    }

    ImGuiStyle& Style = ImGui::GetStyle();

    const float AxisGap     = 2.0f;
    const float FrameHeight = ImGui::GetFontSize() + Style.FramePadding.y * 2.0f;
    const float RowHeight   = FrameHeight;

    ImGui::TableNextRow(0, RowHeight);
    
    bool bAnyValueChanged = false;
    bool bRowHovered      = BeginFullRowHoverCatcher(RowHeight);

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

    const auto GetDynamicFormatString = [](float Value, TStaticArray<CHAR, 8>& OutFormat) -> const CHAR*
    {
        static const TStaticArray<float, 4> Pow10Table = { 10.0f, 100.0f, 1000.0f, 10000.0f };

        const float AbsoluteValue = Math::Abs(Value);

        int32 DecimalsToShow = 1;
        for (int32 DecimalIndex = 1; DecimalIndex <= 4; ++DecimalIndex)
        {
            const float ScaleFactor       = Pow10Table[DecimalIndex - 1];
            const float RoundedToDecimals = Math::Round(AbsoluteValue * ScaleFactor) / ScaleFactor;
            const float Tolerance         = 1e-5f * (AbsoluteValue + 1.0f);

            if (Math::Abs(AbsoluteValue - RoundedToDecimals) <= Tolerance)
            {
                DecimalsToShow = DecimalIndex;
                break;
            }

            DecimalsToShow = 4;
        }

        CString::Snprintf(OutFormat.Data(), static_cast<int32>(OutFormat.Size()), "%%.%df", DecimalsToShow);
        return OutFormat.Data();
    };

    ImGui::TableSetColumnIndex(0);

    const float LabelIndent = 24.0f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + LabelIndent);

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
            const float  CenteredY    = CursorScreen.y + (RowHeight - IconButtonSizePx) * 0.5f;
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
            const int32 Alpha255 = static_cast<int32>(Alpha01 * 255.0f);
            const ImU32 Tint     = IM_COL32(255, 255, 255, Alpha255);

            ImTextureID Icon = bUniformScaleEnabled ? EditorIcons::LockedIcon : EditorIcons::UnlockedIcon;
            if (Icon)
            {
                DrawList->AddImage(Icon, IconMin, IconMax, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), Tint);
            }
            else
            {
                const CHAR* FallbackText = bUniformScaleEnabled ? "L" : "U";

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

    const float AvailableWidth = ImGui::GetContentRegionAvail().x;
    const float TotalAxisGaps  = 2.0f * AxisGap;

    float AxisFieldWidth = (AvailableWidth - TotalAxisGaps) / 3.0f;
    AxisFieldWidth = Math::Max(AxisFieldWidth, 1.0f);

    bool bXChanged = false;
    bool bYChanged = false;
    bool bZChanged = false;

    const auto DrawAxisField = [&](const CHAR* DragWidgetId, float& InOutAxisValue, ImU32 AxisIndicatorColor, float FieldWidth, bool bPlaceOnSameLine) -> bool
    {
        if (bPlaceOnSameLine)
        {
            ImGui::SameLine(0.0f, AxisGap);
        }

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(AxisGap, 0.0f));
        ImGui::SetNextItemWidth(FieldWidth);

        bool bAxisValueChanged = false;

        TStaticArray<CHAR, 8> FormatBuffer;
        const CHAR* FormatString = GetDynamicFormatString(InOutAxisValue, FormatBuffer);

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
            TStaticArray<CHAR, 8> FormatBufferAfter;
            const CHAR* FormatStringAfter = GetDynamicFormatString(InOutAxisValue, FormatBufferAfter);

            TStaticArray<CHAR, 64> ValueTextBuffer;
            CString::Snprintf(ValueTextBuffer.Data(), static_cast<int32>(ValueTextBuffer.Size()), FormatStringAfter, InOutAxisValue);

            ImGuiWindow* Window = ImGui::GetCurrentWindow();
            if (Window && !Window->SkipItems)
            {
                const ImVec2 ItemMin         = ImGui::GetItemRectMin();
                const ImVec2 ItemMax         = ImGui::GetItemRectMax();
                const ImU32  BackgroundColor = ImGui::GetColorU32(bIsItemActive ? ImGuiCol_FrameBgActive : (bIsItemHovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg));

                Window->DrawList->AddRectFilled(ItemMin, ItemMax, BackgroundColor, ImGui::GetStyle().FrameRounding);

                const float  TextStartX    = ItemMin.x + ImGui::GetStyle().FramePadding.x;
                const ImVec2 ValueTextSize = ImGui::CalcTextSize(ValueTextBuffer.Data());
                const float  TextPosY      = ItemMin.y + (ItemMax.y - ItemMin.y - ValueTextSize.y) * 0.5f;

                Window->DrawList->AddText(ImVec2(TextStartX, TextPosY), ImGui::GetColorU32(ImGuiCol_Text), ValueTextBuffer.Data());

                if (bIsRotationDegrees)
                {
                    const float DegreeGap = 1.0f;
                    const float DegreePosX  = TextStartX + ValueTextSize.x + DegreeGap;

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

    bXChanged = DrawAxisField("##X", OutValue.X, XAxisColor, AxisFieldWidth, false);
    bYChanged = DrawAxisField("##Y", OutValue.Y, YAxisColor, AxisFieldWidth, true);
    bZChanged = DrawAxisField("##Z", OutValue.Z, ZAxisColor, AxisFieldWidth, true);

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
    {
        const float PadY = Math::Max((ImGui::GetFrameHeight() - FixedRevertButtonSize) * 0.5f, 0.0f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + PadY);
    }

    const bool bHasRevertValue         = InRevertValue != nullptr;
    const bool bShouldShowRevertButton = bHasRevertValue && (OutValue != *InRevertValue);

    const float RevertSlotSize = FrameHeight;
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
        ImGui::Dummy(ImVec2(RevertSlotSize, RevertSlotSize));
    }

    ApplyHoveredRowBg(bRowHovered);
    return bAnyValueChanged;
}

bool EditorWidgets::DrawFloatProperty(const CHAR* Label, float& InOutValue, float Speed, float MinValue, float MaxValue, const CHAR* Format, bool bUseSlider, const float* InRevertValue, bool bEnabled)
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
    
    const float LabelIndent = 24.0f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + LabelIndent);

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(Label);

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
    {
        const float PadY = Math::Max((ImGui::GetFrameHeight() - FixedRevertButtonSize) * 0.5f, 0.0f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + PadY);
    }

    const bool bCanRevert = bEnabled && (InRevertValue != nullptr) && (InOutValue != *InRevertValue);
    if (bCanRevert)
    {
        if (ResetIconButton())
        {
            InOutValue = *InRevertValue;
            bResult    = true;
        }

        bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    }
    else
    {
        ImGui::Dummy(ImVec2(FixedRevertButtonSize, FixedRevertButtonSize));
    }

    ApplyHoveredRowBg(bRowHovered);
    return bEnabled && bResult;
}

bool EditorWidgets::DrawIntProperty(const CHAR* Label, int32& InOutValue, float Speed, int32 MinValue, int32 MaxValue, const CHAR* Format, bool bUseSlider, const int32* InRevertValue, bool bEnabled)
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

    const float LabelIndent = 24.0f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + LabelIndent);

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(Label);

    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(Label);

    if (!bEnabled)
    {
        ImGui::BeginDisabled();
    }

    ImGui::SetNextItemWidth(-FLT_MIN);
    if (bUseSlider)
    {
        bResult = ImGui::SliderInt("##Value", &InOutValue, MinValue, MaxValue, Format);
    }
    else
    {
        bResult = ImGui::DragInt("##Value", &InOutValue, Speed, MinValue, MaxValue, Format);
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

    ImGui::TableSetColumnIndex(2);
    {
        const float PadY = Math::Max((ImGui::GetFrameHeight() - FixedRevertButtonSize) * 0.5f, 0.0f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + PadY);
    }

    const bool bCanRevert = bEnabled && (InRevertValue != nullptr) && (InOutValue != *InRevertValue);
    if (bCanRevert)
    {
        if (ResetIconButton())
        {
            InOutValue = *InRevertValue;
            bResult    = true;
        }

        bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    }
    else
    {
        ImGui::Dummy(ImVec2(FixedRevertButtonSize, FixedRevertButtonSize));
    }

    ApplyHoveredRowBg(bRowHovered);
    return bEnabled && bResult;
}

bool EditorWidgets::DrawComboProperty(const CHAR* Label, int32& InOutValue, const CHAR* const* Items, int32 ItemCount, const int32* InRevertValue, bool bEnabled)
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

    const float LabelIndent = 24.0f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + LabelIndent);

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(Label);

    ImGui::TableSetColumnIndex(1);
    ImGui::PushID(Label);

    if (!bEnabled)
    {
        ImGui::BeginDisabled();
    }

    const ImU32 FieldBgU32     = IM_COL32(15, 15, 15, 255);
    const ImU32 FieldTextU32   = IM_COL32(170, 170, 170, 255);
    const ImU32 ArrowU32       = IM_COL32(192, 192, 192, 255);
    const ImU32 PopupBgU32     = IM_COL32(26, 26, 26, 255);
    const ImU32 PopupBorderU32 = IM_COL32(0, 90, 173, 255);
    const ImU32 SelectBgU32    = IM_COL32(0, 112, 224, 255);
    const ImU32 PopupTextU32   = IM_COL32(255, 255, 255, 255);

    const ImGuiStyle& Style = ImGui::GetStyle();
    const float FieldRounding = Style.FrameRounding;
    
    constexpr float PopupRounding = 0.0f;

    const int32  ClampedIndex = Math::Clamp<int32>(InOutValue, 0, Math::Max(0, ItemCount - 1));
    const CHAR*  PreviewText  = (Items && ItemCount > 0) ? Items[ClampedIndex] : "";
    const float  FrameHeight  = ImGui::GetFrameHeight();
    const ImVec2 FieldSize    = ImVec2(ImGui::GetContentRegionAvail().x, FrameHeight);

    const bool bPressed      = ImGui::InvisibleButton("##Value", FieldSize);
    const bool bFieldHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    const bool bFieldActive  = ImGui::IsItemActive();

    bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    bRowHovered |= ImGui::IsItemActive();

    const ImVec2 FieldMin = ImGui::GetItemRectMin();
    const ImVec2 FieldMax = ImGui::GetItemRectMax();

    ImGuiWindow* Window = ImGui::GetCurrentWindow();
    if (Window && !Window->SkipItems)
    {
        Window->DrawList->AddRectFilled(FieldMin, FieldMax, FieldBgU32, FieldRounding);

        const float  ArrowZoneW     = FrameHeight;
        const ImVec2 TextSize       = ImGui::CalcTextSize(PreviewText);
        const float  TextY          = FieldMin.y + (FrameHeight - TextSize.y) * 0.5f;
        const float  ArrowPadX      = 8.0f;
        const float  TextStartX     = FieldMin.x + Style.FramePadding.x;
        const float  TextMaxX       = FieldMax.x - ArrowZoneW - ArrowPadX;
        const ImVec2 ClipMin        = ImVec2(TextStartX, FieldMin.y);
        const ImVec2 ClipMax        = ImVec2(TextMaxX, FieldMax.y);
        const ImU32  PreviewTextU32 = (bFieldHovered || bFieldActive) ? IM_COL32(255, 255, 255, 255) : FieldTextU32;
        const ImU32  ArrowColorU32  = (bFieldHovered || bFieldActive) ? IM_COL32(255, 255, 255, 255) : ArrowU32;

        Window->DrawList->PushClipRect(ClipMin, ClipMax, true);
        Window->DrawList->AddText(ImVec2(TextStartX, TextY), PreviewTextU32, PreviewText);
        Window->DrawList->PopClipRect();

        const float ArrowSize = 6.0f;
        const float CenterX   = FieldMax.x - ArrowZoneW * 0.5f;
        const float CenterY   = (FieldMin.y + FieldMax.y) * 0.5f + 1.0f;
        const ImVec2 P0       = ImVec2(CenterX - ArrowSize, CenterY - ArrowSize * 0.5f);
        const ImVec2 P1       = ImVec2(CenterX + ArrowSize, CenterY - ArrowSize * 0.5f);
        const ImVec2 P2       = ImVec2(CenterX,            CenterY + ArrowSize * 0.6f);
        Window->DrawList->AddTriangleFilled(P0, P1, P2, ArrowColorU32);
    }

    DrawInputBorderLastItem(FieldRounding);

    if (bPressed && bEnabled)
    {
        ImGui::OpenPopup("##ComboPopup");
    }

    constexpr float PopupTextPadX = 6.0f;
    float PopupWidth = FieldSize.x;
    if (Items && ItemCount > 0)
    {
        float MaxItemTextWidth = 0.0f;
        for (int32 ItemIndex = 0; ItemIndex < ItemCount; ++ItemIndex)
        {
            const ImVec2 TextSize = ImGui::CalcTextSize(Items[ItemIndex] ? Items[ItemIndex] : "");
            MaxItemTextWidth = Math::Max(MaxItemTextWidth, TextSize.x);
        }

        PopupWidth = Math::Max(PopupWidth, MaxItemTextWidth + PopupTextPadX * 2.0f);
    }

    float  PopupX       = FieldMin.x;
    ImVec2 WorkMin      = ImVec2(0.0f, 0.0f);
    ImVec2 WorkMax      = ImVec2(0.0f, 0.0f);
    bool   bHasWorkRect = false;

    ImGuiPlatformIO& PlatformIO = ImGui::GetPlatformIO();
    if (PlatformIO.Monitors.Size > 0)
    {
        const ImVec2 FieldCenter = ImVec2((FieldMin.x + FieldMax.x) * 0.5f, (FieldMin.y + FieldMax.y) * 0.5f);
        
        int32 MonitorIndex = 0;
        for (int32 i = 0; i < PlatformIO.Monitors.Size; ++i)
        {
            const ImGuiPlatformMonitor& Monitor = PlatformIO.Monitors[i];

            const ImVec2 Min = Monitor.WorkPos;
            const ImVec2 Max = ImVec2(Monitor.WorkPos.x + Monitor.WorkSize.x, Monitor.WorkPos.y + Monitor.WorkSize.y);
            
            if (FieldCenter.x >= Min.x && FieldCenter.x <= Max.x && FieldCenter.y >= Min.y && FieldCenter.y <= Max.y)
            {
                MonitorIndex = i;
                break;
            }
        }

        const ImGuiPlatformMonitor& Monitor = PlatformIO.Monitors[MonitorIndex];
        WorkMin      = Monitor.WorkPos;
        WorkMax      = ImVec2(Monitor.WorkPos.x + Monitor.WorkSize.x, Monitor.WorkPos.y + Monitor.WorkSize.y);
        bHasWorkRect = (Monitor.WorkSize.x > 0.0f && Monitor.WorkSize.y > 0.0f);
    }

    if (!bHasWorkRect)
    {
        if (ImGuiViewport* Viewport = ImGui::GetWindowViewport())
        {
            WorkMin      = Viewport->WorkPos;
            WorkMax      = ImVec2(Viewport->WorkPos.x + Viewport->WorkSize.x, Viewport->WorkPos.y + Viewport->WorkSize.y);
            bHasWorkRect = (Viewport->WorkSize.x > 0.0f && Viewport->WorkSize.y > 0.0f);
        }
    }

    if (bHasWorkRect)
    {
        const float MaxWidth = Math::Max(1.0f, WorkMax.x - WorkMin.x);
        PopupWidth = Math::Min(PopupWidth, MaxWidth);

        if ((PopupX + PopupWidth) > WorkMax.x)
        {
            PopupX = FieldMax.x - PopupWidth;
        }

        PopupX = Math::Clamp(PopupX, WorkMin.x, WorkMax.x - PopupWidth);
    }

    ImGui::SetNextWindowPos(ImVec2(PopupX, FieldMax.y));
    ImGui::SetNextWindowSize(ImVec2(PopupWidth, 0.0f));

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, PopupRounding);

    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImGui::ColorConvertU32ToFloat4(PopupBgU32));
    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(PopupTextU32));
    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::ColorConvertU32ToFloat4(PopupBgU32));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImGui::ColorConvertU32ToFloat4(SelectBgU32));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImGui::ColorConvertU32ToFloat4(SelectBgU32));

    if (ImGui::BeginPopup("##ComboPopup"))
    {
        ImGuiWindow* PopupWindow   = ImGui::GetCurrentWindow();
        const float  ItemHeight    = ImGui::GetFrameHeight();
        const float  FullWidth     = PopupWindow ? PopupWindow->Size.x : ImGui::GetContentRegionAvail().x;
        ImDrawList*  PopupDrawList = PopupWindow ? PopupWindow->DrawList : ImGui::GetWindowDrawList();
        
        constexpr float SelectedInsetX   = 2.0f;
        constexpr float SelectedInsetY   = 1.0f;
        constexpr float SelectedRounding = 4.0f;

        for (int32 ItemIndex = 0; ItemIndex < ItemCount; ++ItemIndex)
        {
            const bool bSelected = (ItemIndex == InOutValue);

            ImGui::PushID(ItemIndex);
            ImGui::SetCursorPosX(0.0f);

            const bool bItemPressed = ImGui::InvisibleButton("##Item", ImVec2(FullWidth, ItemHeight));
            const bool bItemHovered = ImGui::IsItemHovered();
            const bool bItemHeld    = ImGui::IsItemActive();

            const ImVec2 ItemMin = ImGui::GetItemRectMin();
            const ImVec2 ItemMax = ImGui::GetItemRectMax();

            if ((bItemHovered || bItemHeld) && PopupDrawList)
            {
                PopupDrawList->AddRectFilled(ItemMin, ItemMax, SelectBgU32, 0.0f);
            }

            const CHAR*  ItemText     = (Items && ItemIndex < ItemCount) ? Items[ItemIndex] : "";
            const ImVec2 ItemTextSize = ImGui::CalcTextSize(ItemText);
            const float  ItemTextY    = ItemMin.y + (ItemHeight - ItemTextSize.y) * 0.5f;
            const float  ItemTextX    = ItemMin.x + PopupTextPadX;

            if (PopupDrawList)
            {
                PopupDrawList->AddText(ImVec2(ItemTextX, ItemTextY), PopupTextU32, ItemText);
            }

            if (bSelected && PopupDrawList)
            {
                const ImVec2 SelMin = ImVec2(ItemMin.x + SelectedInsetX, ItemMin.y + SelectedInsetY);
                const ImVec2 SelMax = ImVec2(ItemMax.x - SelectedInsetX, ItemMax.y - SelectedInsetY);
                PopupDrawList->AddRect(SelMin, SelMax, SelectBgU32, SelectedRounding, 0, 1.5f);
            }

            if (bItemPressed)
            {
                InOutValue = ItemIndex;
                bResult    = true;

                ImGui::CloseCurrentPopup();
            }

            if (bSelected)
            {
                ImGui::SetItemDefaultFocus();
            }

            ImGui::PopID();
        }

        if (PopupWindow && !PopupWindow->SkipItems)
        {
            const ImVec2 PopupMin = PopupWindow->Pos;
            const ImVec2 PopupMax = ImVec2(PopupWindow->Pos.x + PopupWindow->Size.x, PopupWindow->Pos.y + PopupWindow->Size.y);
            PopupWindow->DrawList->AddRect(PopupMin, PopupMax, PopupBorderU32, PopupRounding, 0, 1.0f);
        }

        ImGui::EndPopup();
    }

    ImGui::PopStyleColor(5);
    ImGui::PopStyleVar(3);

    if (!bEnabled)
    {
        ImGui::EndDisabled();
    }

    ImGui::PopID();

    ImGui::TableSetColumnIndex(2);
    {
        const float PadY = Math::Max((ImGui::GetFrameHeight() - FixedRevertButtonSize) * 0.5f, 0.0f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + PadY);
    }

    const bool bCanRevert = bEnabled && (InRevertValue != nullptr) && (InOutValue != *InRevertValue);
    if (bCanRevert)
    {
        if (ResetIconButton())
        {
            InOutValue = *InRevertValue;
            bResult    = true;
        }
        
        bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    }
    else
    {
        ImGui::Dummy(ImVec2(FixedRevertButtonSize, FixedRevertButtonSize));
    }

    ApplyHoveredRowBg(bRowHovered);
    return bEnabled && bResult;
}

bool EditorWidgets::DrawCheckboxProperty(const CHAR* Label, bool& InOutValue, const bool* InRevertValue, bool bEnabled)
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

    const float LabelIndent = 24.0f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + LabelIndent);

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
    {
        const float PadY = Math::Max((ImGui::GetFrameHeight() - FixedRevertButtonSize) * 0.5f, 0.0f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + PadY);
    }

    const bool bCanRevert = bEnabled && (InRevertValue != nullptr) && (InOutValue != *InRevertValue);
    if (bCanRevert)
    {
        if (ResetIconButton())
        {
            InOutValue = *InRevertValue;
            bResult    = true;
        }

        bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    }
    else
    {
        ImGui::Dummy(ImVec2(FixedRevertButtonSize, FixedRevertButtonSize));
    }

    ApplyHoveredRowBg(bRowHovered);
    return bEnabled && bResult;
}

void EditorWidgets::DrawTextProperty(const CHAR* Label, const CHAR* ValueText)
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
        const float LabelIndent = 24.0f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + LabelIndent);
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

void EditorWidgets::DrawReadOnlyFloat3Property(const CHAR* Label, const Vector3& Value)
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
        const float LabelIndent = 24.0f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + LabelIndent);
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(Label);
    }

    ImGui::TableSetColumnIndex(1);

    TStaticArray<float, 3> Temp = { Value.X, Value.Y, Value.Z };
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputFloat3("##Value", Temp.Data(), "%.3f", ImGuiInputTextFlags_ReadOnly);

    ImGuiStyle& Style = ImGui::GetStyle();
    DrawInputBorderLastItem(Style.FrameRounding);

    bRowHovered |= ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

    ImGui::TableSetColumnIndex(2);

    ApplyHoveredRowBg(bRowHovered);
}

bool EditorWidgets::DrawColor3Property(const CHAR* Label, float* InOutColor, const float* InRevertColor, bool bEnabled, ImGuiColorEditFlags Flags)
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
        const float LabelIndent = 24.0f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + LabelIndent);

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
    const auto DrawComponentDragFloat = [&](const CHAR* Id, float& OutValue, bool bSameLine)
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
    {
        const float PadY = Math::Max((ImGui::GetFrameHeight() - FixedRevertButtonSize) * 0.5f, 0.0f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + PadY);
    }

    const bool bCanRevert = (InRevertColor != nullptr);
    if (!bCanRevert || !bEnabled)
    {
        ImGui::BeginDisabled();
    }

    if (ResetIconButton() && bCanRevert)
    {
        static constexpr uint64 SizeInBytes = sizeof(TStaticArray<float, 3>);
        Memory::Memcpy(InOutColor, InRevertColor, SizeInBytes);
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

const CHAR* EditorHelpers::GetTrimmedQuery(const CHAR* InText, CHAR* OutBuf, int32 OutBufSize)
{
    if (!OutBuf || OutBufSize <= 0)
    {
        return nullptr;
    }

    OutBuf[0] = 0;

    if (!InText)
    {
        return nullptr;
    }

    const CHAR* Start = InText;
    while (*Start && CharTraits::IsWhitespace(*Start))
    {
        ++Start;
    }

    const CHAR* End = Start;
    while (*End)
    {
        ++End;
    }

    while (End > Start && CharTraits::IsWhitespace(End[-1]))
    {
        --End;
    }

    const int32 Len = static_cast<int32>(End - Start);
    if (Len <= 0)
    {
        return nullptr;
    }

    if (*End == 0)
    {
        return Start;
    }

    const int32 CopyLen = Math::Min(Len, OutBufSize - 1);
    CString::Strncpy(OutBuf, Start, CopyLen + 1);
    OutBuf[CopyLen] = 0;
    return OutBuf[0] ? OutBuf : nullptr;
}

void EditorHelpers::FormatBytes(int64 Bytes, char* OutBuffer, int32 BufferSize)
{
    const double AbsBytes = static_cast<double>(Bytes < 0 ? -Bytes : Bytes);
    if (AbsBytes >= 1024.0 * 1024.0 * 1024.0)
    {
        snprintf(OutBuffer, BufferSize, "%.2f GB", static_cast<double>(Bytes) / (1024.0 * 1024.0 * 1024.0));
    }
    else if (AbsBytes >= 1024.0 * 1024.0)
    {
        snprintf(OutBuffer, BufferSize, "%.2f MB", static_cast<double>(Bytes) / (1024.0 * 1024.0));
    }
    else if (AbsBytes >= 1024.0)
    {
        snprintf(OutBuffer, BufferSize, "%.2f KB", static_cast<double>(Bytes) / 1024.0);
    }
    else
    {
        snprintf(OutBuffer, BufferSize, "%lld B", static_cast<long long>(Bytes));
    }
}

void EditorWidgets::DrawErrorWindow(ErrorWindowContext& InOutContext)
{
    if (!InOutContext.bVisible)
    {
        return;
    }

    const CHAR* TitleText  = !InOutContext.Title.IsEmpty() ? *InOutContext.Title : "Failed Renames";
    const CHAR* HeaderText = !InOutContext.HeaderText.IsEmpty() ? *InOutContext.HeaderText : "The following files could not be moved";

    if (ImGuiViewport* Viewport = ImGui::GetMainViewport())
    {
        ImGui::SetNextWindowPos(Viewport->GetCenter(), ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
    }

    ImGui::SetNextWindowSize(ImVec2(780.0f, 420.0f), ImGuiCond_FirstUseEver);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 16.0f));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(26, 26, 26, 255));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, IM_COL32(26, 26, 26, 255));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, IM_COL32(26, 26, 26, 255));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(44, 44, 44, 255));

    const ImGuiWindowFlags WindowFlags =
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings;

    bool bClosePressed = false;
    const bool bBeginWindow = ImGui::Begin(TitleText, nullptr, WindowFlags);
    if (bBeginWindow)
    {
        ImGui::TextUnformatted(HeaderText);
        ImGui::Spacing();
        ImGui::Spacing();

        const float LineHeight     = ImGui::GetTextLineHeightWithSpacing();
        const float MinHeight      = 140.0f;
        const float EntriesHeight  = LineHeight * InOutContext.Entries.Size();
        const float DesiredHeight  = Math::Clamp(MinHeight + EntriesHeight, 140.0f, 260.0f);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(26, 26, 26, 255));

        const ImGuiChildFlags ChildFlags = ImGuiChildFlags_AlwaysUseWindowPadding;
        ImGui::BeginChild("##FailedMoveList", ImVec2(0.0f, DesiredHeight), ChildFlags, ImGuiWindowFlags_None);

        if (InOutContext.Entries.IsEmpty())
        {
            ImGui::TextUnformatted("No files listed.");
        }
        else
        {
            for (int32 Index = 0; Index < InOutContext.Entries.Size(); ++Index)
            {
                ImGui::TextUnformatted(*InOutContext.Entries[Index]);
            }
        }

        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);

        const ImVec2 ListMin = ImGui::GetItemRectMin();
        const ImVec2 ListMax = ImGui::GetItemRectMax();
        ImGui::GetWindowDrawList()->AddRect(ListMin, ListMax, IM_COL32(36, 36, 36, 255), 0.0f, ImDrawFlags_None, 3.0f);

        ImGui::Spacing();
        ImGui::Spacing();

        const float ButtonWidth  = 120.0f;
        const float ButtonHeight = ImGui::GetFrameHeight();
        const float AvailableX   = ImGui::GetContentRegionAvail().x;
        const float CursorX      = ImGui::GetCursorPosX();
        const float ButtonPosX   = CursorX + Math::Max(0.0f, AvailableX - ButtonWidth);
        ImGui::SetCursorPosX(ButtonPosX);

        const ImVec2 ButtonSize(ButtonWidth, ButtonHeight);
        bClosePressed = DrawDialogButton("Close", ButtonSize);
    }

    ImGui::End();
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(4);

    if (bClosePressed)
    {
        InOutContext.bVisible = false;
        InOutContext.Entries.Clear();
        InOutContext.HeaderText.Clear();
        InOutContext.Title.Clear();
    }
}

bool EditorWidgets::DrawConfirmDialog(ConfirmDialogContext& InOutContext)
{
    if (!InOutContext.bVisible)
    {
        return false;
    }

    const CHAR* TitleText   = !InOutContext.Title.IsEmpty() ? *InOutContext.Title : "Confirm";
    const CHAR* MessageText = !InOutContext.Message.IsEmpty() ? *InOutContext.Message : "Are you sure?";

    if (ImGuiViewport* Viewport = ImGui::GetMainViewport())
    {
        // Use Appearing so the window is centered when opened but can be moved by the user
        ImGui::SetNextWindowPos(Viewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    }

    // Set minimum width only; AlwaysAutoResize handles the rest
    ImGui::SetNextWindowSizeConstraints(ImVec2(360.0f, 0.0f), ImVec2(FLT_MAX, FLT_MAX));

    // Match error window style
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowTitleAlign, ImVec2(0.5f, 0.5f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20.0f, 16.0f));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(26, 26, 26, 255));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, IM_COL32(26, 26, 26, 255));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, IM_COL32(26, 26, 26, 255));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(44, 44, 44, 255));

    const ImGuiWindowFlags WindowFlags =
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoSavedSettings;

    bool bConfirmed = false;

    if (ImGui::Begin(TitleText, &InOutContext.bVisible, WindowFlags))
    {
        const float ButtonWidth  = 100.0f;
        const float ButtonHeight = ImGui::GetFrameHeight();
        const float Gap          = 12.0f;

        const float AvailableX = ImGui::GetContentRegionAvail().x;

        // Center the message text on the x-axis
        const ImVec2 MessageTextSize = ImGui::CalcTextSize(MessageText, nullptr, true, AvailableX);
        const float MessageStartX   = ImGui::GetCursorPosX() + Math::Max(0.0f, (AvailableX - MessageTextSize.x) * 0.5f);
        ImGui::SetCursorPosX(MessageStartX);

        ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + AvailableX);
        ImGui::TextUnformatted(MessageText);
        ImGui::PopTextWrapPos();

        ImGui::Spacing();
        ImGui::Spacing();

        const float TwoButtonsWidth = ButtonWidth * 2.0f + Gap;
        const float StartX          = ImGui::GetCursorPosX() + Math::Max(0.0f, (AvailableX - TwoButtonsWidth) * 0.5f);

        ImGui::SetCursorPosX(StartX);
        if (DrawDialogButton("Yes", ImVec2(ButtonWidth, ButtonHeight)))
        {
            bConfirmed = true;
            InOutContext.bVisible = false;
        }
        ImGui::SameLine(0.0f, Gap);
        if (DrawDialogButton("No", ImVec2(ButtonWidth, ButtonHeight)))
        {
            InOutContext.bVisible = false;
        }
    }
    else
    {
        InOutContext.bVisible = false;
    }

    ImGui::End();
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(4);

    return bConfirmed;
}

bool EditorWidgets::DrawDialogButton(const CHAR* Label, const ImVec2& Size)
{
    ImGui::PushID(Label);

    const bool bPressed = ImGui::InvisibleButton("##DialogButton", Size);
    const bool bHovered = ImGui::IsItemHovered();
    const bool bHeld    = ImGui::IsItemActive();

    const ImVec2 Min = ImGui::GetItemRectMin();
    const ImVec2 Max = ImGui::GetItemRectMax();

    const ImU32 BgIdle    = IM_COL32(56, 56, 56, 255);
    const ImU32 BgHover   = IM_COL32(87, 87, 87, 255);
    const ImU32 BgHeld    = IM_COL32(47, 47, 47, 255);
    const ImU32 BgColor   = bHeld ? BgHeld : (bHovered ? BgHover : BgIdle);
    const ImU32 TextColor = IM_COL32(255, 255, 255, 255);

    const float Rounding        = 4.0f;
    const float BorderThickness = 1.5f;

    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    DrawList->AddRectFilled(Min, Max, BgColor, Rounding);
    DrawList->AddRect(Min, Max, IM_COL32(15, 15, 15, 255), Rounding, ImDrawFlags_None, BorderThickness);

    ImGui::PushFont(EditorFonts::SegoeUI_22);

    const ImVec2 TextSize = ImGui::CalcTextSize(Label);
    const ImVec2 TextPos  = ImVec2(Min.x + (Size.x - TextSize.x) * 0.5f, Min.y + (Size.y - TextSize.y) * 0.5f);
    DrawList->AddText(TextPos, TextColor, Label);

    ImGui::PopFont();

    ImGui::PopID();
    return bPressed;
}

bool EditorWidgets::DrawSearchField(const CHAR* InId, const CHAR* InHint, CHAR* InOutBuffer, int32 InBufferSize, float InWidth, bool bDrawBorder)
{
    if (!InId || !InHint || !InOutBuffer || InBufferSize <= 0)
    {
        return false;
    }

    ImGuiWindow* Window = ImGui::GetCurrentWindow();
    if (!Window || Window->SkipItems)
    {
        return false;
    }

    // -------------------------------------------------------------------------------------
    // Layout constants
    // -------------------------------------------------------------------------------------

    const ImVec2 BasePadding = EditorStyleVars::InputFieldFramePadding;

    const float IconGap     = 6.0f;
    const float IconSize    = 16.0f;
    const float Rounding    = EditorStyleVars::InputFieldBorderRounding;
    const float BorderThick = EditorStyleVars::InputFieldBorderThickness;
    const float TotalWidth  = (InWidth <= 0.0f) ? ImGui::GetContentRegionAvail().x : InWidth;
    const float TotalHeight = ImGui::GetFontSize() + BasePadding.y * 2.0f;

    if (TotalWidth <= 1.0f)
    {
        ImGui::Dummy(ImVec2(1.0f, TotalHeight));
        return false;
    }

    const ImVec2 Start         = ImGui::GetCursorScreenPos();
    const ImVec2 End           = ImVec2(Start.x + TotalWidth, Start.y + TotalHeight);
    const ImRect FullRect      = ImRect(Start, End);
    const float  IconAreaWidth = BasePadding.x + IconSize + IconGap;

    // -------------------------------------------------------------------------------------
    // Colors
    // -------------------------------------------------------------------------------------

    ImDrawList* DrawList = ImGui::GetWindowDrawList();

    const ImU32  BgColor            = IM_COL32(15, 15, 15, 255);
    const ImU32  BorderColorNormal  = IM_COL32(51, 51, 51, 255);
    const ImU32  BorderColorHovered = IM_COL32(74, 74, 74, 255);
    const ImU32  BorderColorActive  = IM_COL32(9, 92, 176, 255);
    const ImVec4 HintTextInactive   = ImVec4(76.0f / 255.0f, 76.0f / 255.0f, 76.0f / 255.0f, 1.0f);
    const ImVec4 HintTextActive     = ImVec4(97.0f / 255.0f, 97.0f / 255.0f, 97.0f / 255.0f, 1.0f);
    const ImVec4 InputTextColor     = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    const ImU32  IconInactive       = IM_COL32(192, 192, 192, 255);
    const ImU32  IconActive         = IM_COL32(255, 255, 255, 255);

    const ImGuiID PendingInputId  = ImGui::GetID(InId);
    const bool    bInputWasActive = ImGui::GetActiveID() == PendingInputId;
    const bool    bInputClicked   = ImGui::IsMouseHoveringRect(FullRect.Min, FullRect.Max, true) && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    const bool    bUseActiveHint  = bInputWasActive || bInputClicked;
    const ImVec4  HintTextColor   = bUseActiveHint ? HintTextActive : HintTextInactive;
    DrawList->AddRectFilled(Start, End, BgColor, Rounding);

    // -------------------------------------------------------------------------------------
    // Input
    // -------------------------------------------------------------------------------------

    ImGui::SetNextItemWidth(TotalWidth);

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(IconAreaWidth, BasePadding.y));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Text, InputTextColor);
    ImGui::PushStyleColor(ImGuiCol_TextDisabled, HintTextColor);
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, EditorStyleVars::InputFieldSelectionColor);

    bool bChanged = ImGui::InputTextWithHint(InId, InHint, InOutBuffer, static_cast<size_t>(InBufferSize));

    ImGui::PopStyleColor(6);
    ImGui::PopStyleVar(3);

    const ImGuiID InputId = ImGui::GetItemID();

    // -------------------------------------------------------------------------------------
    // Icon rect
    // -------------------------------------------------------------------------------------

    const float  IconY    = Start.y + (TotalHeight - IconSize) * 0.5f;
    const ImVec2 IconMin  = ImVec2(Start.x + BasePadding.x, IconY);
    const ImVec2 IconMax  = ImVec2(IconMin.x + IconSize, IconMin.y + IconSize);
    const ImRect IconRect = ImRect(IconMin, IconMax);

    const bool bHasText = InOutBuffer[0] != '\0';

    // -------------------------------------------------------------------------------------
    // Clearing behavior
    // -------------------------------------------------------------------------------------

    bool bClearedThisFrame = false;

    if (bHasText && EditorIcons::CloseIcon)
    {
        const bool bIconHovered = ImGui::IsMouseHoveringRect(IconRect.Min, IconRect.Max, true);
        const bool bIconPressed = bIconHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);

        if (bIconHovered)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        }

        const ImU32 Tint = bIconHovered ? IconActive : IconInactive;
        DrawList->AddImage(EditorIcons::CloseIcon, IconMin, IconMax, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), Tint);

        if (bIconPressed)
        {
            InOutBuffer[0]    = '\0';
            bClearedThisFrame = true;
            bChanged          = true;

            if (ImGuiInputTextState* State = ImGui::GetInputTextState(InputId))
            {
                State->ClearText();
                State->CursorClamp();
            }

            ImGui::SetActiveID(InputId, Window);
            ImGui::SetFocusID(InputId, Window);
            ImGui::FocusWindow(Window);
        }
    }
    else if (EditorIcons::SearchIcon)
    {
        const bool bActive = ImGui::GetActiveID() == InputId;
        const ImU32 Tint = bActive ? IconActive : IconInactive;
        DrawList->AddImage(EditorIcons::SearchIcon, IconMin, IconMax, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), Tint);
    }

    // -------------------------------------------------------------------------------------
    // Border
    // -------------------------------------------------------------------------------------

    if (bDrawBorder)
    {
        const bool bActive = ImGui::GetActiveID() == InputId;
        const bool bHover  = ImGui::IsMouseHoveringRect(FullRect.Min, FullRect.Max, true);

        const ImU32 BorderColor = bActive ? BorderColorActive : (bHover ? BorderColorHovered : BorderColorNormal);
        DrawList->AddRect(Start, End, BorderColor, Rounding, ImDrawFlags_None, BorderThick);
    }

    ImGui::SetCursorScreenPos(ImVec2(Start.x, Start.y + TotalHeight));
    return bChanged || bClearedThisFrame;
}

void EditorWidgets::DrawTextWithSearchHighlight(ImDrawList* DrawList, const ImVec2& TextPos, const CHAR* Text, const CHAR* FilterText, ImU32 BaseTextU32, float HighlightPadX, float HighlightPadY, const ImVec2* ClampMin, const ImVec2* ClampMax)
{
    if (!DrawList || !Text)
    {
        return;
    }

    const CHAR* Query = (FilterText && *FilterText != 0) ? FilterText : nullptr;

    int32 MatchStart = -1;
    int32 MatchLen   = 0;

    if (Query)
    {
        if (const CHAR* MatchPtr = CString::Stristr(Text, Query))
        {
            MatchStart = static_cast<int32>(MatchPtr - Text);
            MatchLen   = static_cast<int32>(CString::Strlen(Query));
        }
    }

    if (MatchStart >= 0 && MatchLen > 0)
    {
        const float Scale = ImGui::GetIO().DisplayFramebufferScale.x;
        const float PadX  = HighlightPadX * Scale;
        const float PadY  = HighlightPadY * Scale;

        const ImU32 HighlightBgU32   = IM_COL32(139, 194, 74, 255);
        const ImU32 HighlightTextU32 = IM_COL32(0, 0, 0, 255);

        const ImVec2 PrefixSize = ImGui::CalcTextSize(Text, Text + MatchStart);
        const ImVec2 MatchSize  = ImGui::CalcTextSize(Text + MatchStart, Text + MatchStart + MatchLen);

        const ImVec2 PrefixPos = TextPos;
        const ImVec2 MatchPos  = ImVec2(TextPos.x + PrefixSize.x, TextPos.y);
        const ImVec2 SuffixPos = ImVec2(MatchPos.x + MatchSize.x, TextPos.y);

        if (MatchStart > 0)
        {
            DrawList->AddText(PrefixPos, BaseTextU32, Text, Text + MatchStart);
        }

        ImVec2 HighlightMin = ImVec2(MatchPos.x - PadX, MatchPos.y - PadY);
        ImVec2 HighlightMax = ImVec2(MatchPos.x + MatchSize.x + PadX, MatchPos.y + MatchSize.y + PadY);

        if (ClampMin && ClampMax)
        {
            HighlightMin.x = ImMax(HighlightMin.x, ClampMin->x);
            HighlightMin.y = ImMax(HighlightMin.y, ClampMin->y);
            HighlightMax.x = ImMin(HighlightMax.x, ClampMax->x);
            HighlightMax.y = ImMin(HighlightMax.y, ClampMax->y);
        }

        DrawList->AddRectFilled(HighlightMin, HighlightMax, HighlightBgU32, 0.0f);
        DrawList->AddText(MatchPos, HighlightTextU32, Text + MatchStart, Text + MatchStart + MatchLen);

        const CHAR* Suffix = Text + MatchStart + MatchLen;
        if (Suffix && *Suffix != 0)
        {
            DrawList->AddText(SuffixPos, BaseTextU32, Suffix);
        }
    }
    else
    {
        DrawList->AddText(TextPos, BaseTextU32, Text);
    }
}

void EditorWidgets::MenuSeparator(float Thickness, float PaddingY)
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

void EditorWidgets::MenuLabeledSeparator(const CHAR* Label, float Thickness, float PaddingY)
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

    // Use same background as menu popup (rgb 56,56,56) so separator has no different background
    const ImU32 MenuSeparatorBg = IM_COL32(56, 56, 56, 255);

    constexpr float InsetX = 20.0f;

    const ImU32 LineColor  = IM_COL32(106, 106, 106, 255);
    const ImU32 LabelColor = IM_COL32(160, 160, 160, 255);

    const CHAR* LabelToDraw = "";
    TStaticArray<CHAR, 128> UpperLabel{};

    if (Label && Label[0] != '\0')
    {
        int32 i = 0;
        const int32 UpperLabelMax = static_cast<int32>(UpperLabel.Size()) - 1;
        for (; Label[i] != '\0' && i < UpperLabelMax; ++i)
        {
            const CHAR Ch = static_cast<CHAR>(Label[i]);
            UpperLabel[i] = static_cast<CHAR>(CharTraits::ToUpper(Ch));
        }

        UpperLabel[i] = '\0';
        LabelToDraw   = UpperLabel.Data();
    }

    const ImVec2 LabelSize = ImGui::CalcTextSize(LabelToDraw);
    const float  RowHeight = Math::Max(LabelSize.y, Thickness);
    const float  YLine     = CursorMin.y + (RowHeight * 0.5f) + ((Thickness <= 1.0f) ? 0.5f : 0.0f);
    const float  LabelX    = CursorMin.x + InsetX;
    const float  LabelY    = CursorMin.y + (RowHeight - LabelSize.y) * 0.5f;

    DrawList->AddRectFilled(CursorMin, ImVec2(CursorMin.x + Width, CursorMin.y + RowHeight), MenuSeparatorBg, 0.0f);

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
        const ImVec2 PadMin = ImVec2(CursorMin.x, CursorMin.y + RowHeight);
        const ImVec2 PadMax = ImVec2(CursorMin.x + Width, CursorMin.y + RowHeight + PaddingY);
        DrawList->AddRectFilled(PadMin, PadMax, MenuSeparatorBg, 0.0f);
        ImGui::Dummy(ImVec2(0.0f, PaddingY));
    }
}

bool EditorWidgets::MenuItem(const CHAR* Label, const CHAR* Shortcut, bool bSelected, bool bEnabled, bool bDrawBorder)
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
        const ImU32  BorderColor  = EditorHelpers::MakeBrighterColorU32(HoveredColor, 0.20f);
        DrawList->AddRect(RectMin, RectMax, BorderColor, 0.0f, ImDrawFlags_None, 1.0f);
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

    const CHAR* ShortcutToDraw = Shortcut;

    TStaticArray<CHAR, 128> UpperShortcut{};
    if (bHasShortcut)
    {
        int32 i = 0;
        const int32 UpperShortcutMax = static_cast<int32>(UpperShortcut.Size()) - 1;
        for (; Shortcut[i] != '\0' && i < UpperShortcutMax; ++i)
        {
            const CHAR Ch    = static_cast<CHAR>(Shortcut[i]);
            UpperShortcut[i] = static_cast<CHAR>(CharTraits::ToUpper(Ch));
        }
        UpperShortcut[i] = '\0';
        ShortcutToDraw   = UpperShortcut.Data();
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
        if (EditorIcons::CheckmarkIcon)
        {
            const ImVec2 IconMin = CheckPos;
            const ImVec2 IconMax = ImVec2(CheckPos.x + CheckSize, CheckPos.y + CheckSize);

            const ImU32 Tint = ImGui::GetColorU32(ImGuiCol_Text);
            DrawList->AddImage(EditorIcons::CheckmarkIcon, IconMin, IconMax, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), Tint);
        }
        else
        {
            // Fallback if icon not loaded
            DrawCheckMark(DrawList, CheckPos, ImGui::GetColorU32(ImGuiCol_Text), CheckSize);
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

namespace
{
    enum class EMenuFloatWidget : uint8
    {
        Slider = 0,
        Drag   = 1,
    };
}

static bool MenuFloatRow(
    EMenuFloatWidget WidgetType,
    const CHAR*      Label,
    float&           InOutValue,
    float            Speed,
    float            MinValue,
    float            MaxValue,
    const CHAR*      Format,
    float            ValueWidth,
    bool             bEnabled)
{
    ImGuiWindow* Window = ImGui::GetCurrentWindow();
    if (!Window || Window->SkipItems)
    {
        return false;
    }

    constexpr float MenuIndentX = 20.0f;
    constexpr float PaddingY    = 4.0f;

    const float RowWidth  = ImGui::GetContentRegionAvail().x;
    const float RowHeight = Math::Max(ImGui::GetFontSize() + PaddingY * 2.0f, ImGui::GetFrameHeight());

    ImGui::PushID(Label);

    const ImVec2 RectMin = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(RowWidth, RowHeight));
    const ImVec2 RectMax = ImVec2(RectMin.x + RowWidth, RectMin.y + RowHeight);

    ImDrawList* DrawList = ImGui::GetWindowDrawList();
    DrawList->AddRectFilled(RectMin, RectMax, ImGui::GetColorU32(ImGuiCol_Header), 0.0f);

    const ImVec2 LabelSize = ImGui::CalcTextSize(Label);
    const float  LabelY    = RectMin.y + (RowHeight - LabelSize.y) * 0.5f + 0.5f;

    const float FieldWidth = Math::Min(ValueWidth, Math::Max(RowWidth - MenuIndentX * 2.0f, 1.0f));
    const float FieldX     = RectMax.x - MenuIndentX - FieldWidth;
    const float FieldY     = RectMin.y + (RowHeight - ImGui::GetFrameHeight()) * 0.5f;

    DrawList->PushClipRect(ImVec2(RectMin.x + MenuIndentX, RectMin.y), ImVec2(Math::Max(FieldX - 8.0f, RectMin.x + MenuIndentX + 1.0f), RectMax.y), true);
    DrawList->AddText(ImVec2(RectMin.x + MenuIndentX, LabelY), ImGui::GetColorU32(bEnabled ? ImGuiCol_Text : ImGuiCol_TextDisabled), Label);
    DrawList->PopClipRect();

    ImGui::SetCursorScreenPos(ImVec2(FieldX, FieldY));
    ImGui::SetNextItemWidth(FieldWidth);

    if (!bEnabled)
    {
        ImGui::BeginDisabled();
    }

    bool bChanged = false;
    if (WidgetType == EMenuFloatWidget::Slider)
    {
        bChanged = ImGui::SliderFloat("##Value", &InOutValue, MinValue, MaxValue, Format);
    }
    else
    {
        bChanged = ImGui::DragFloat("##Value", &InOutValue, Speed, MinValue, MaxValue, Format);
    }

    DrawInputBorderLastItem(ImGui::GetStyle().FrameRounding);

    if (!bEnabled)
    {
        ImGui::EndDisabled();
    }

    // The field was placed by hand, so put the cursor back on the row the Dummy reserved.
    ImGui::SetCursorScreenPos(ImVec2(RectMin.x, RectMax.y));
    ImGui::PopID();

    return bEnabled && bChanged;
}

bool EditorWidgets::MenuSliderFloat(const CHAR* Label, float& InOutValue, float MinValue, float MaxValue, const CHAR* Format, float ValueWidth, bool bEnabled)
{
    return MenuFloatRow(EMenuFloatWidget::Slider, Label, InOutValue, 0.0f, MinValue, MaxValue, Format, ValueWidth, bEnabled);
}

bool EditorWidgets::MenuDragFloat(const CHAR* Label, float& InOutValue, float Speed, float MinValue, float MaxValue, const CHAR* Format, float ValueWidth, bool bEnabled)
{
    return MenuFloatRow(EMenuFloatWidget::Drag, Label, InOutValue, Speed, MinValue, MaxValue, Format, ValueWidth, bEnabled);
}

void EditorWidgets::MenuButton(const CHAR* Label, const CHAR* PopupId, bool bAnyPopupOpen, float ButtonHeight, PopupAnchor& OutAnchor, bool bDrawBorder)
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
        DrawList->AddRect(OutAnchor.Min, OutAnchor.Max, BorderColor, 0.0f, ImDrawFlags_None, 1.0f);
    }

    if (bThisPopupOpen)
    {
        ImGui::PopStyleColor(3);
    }
}

static void PopMenuPopupStyle()
{
    ImGui::PopFont();
    ImGui::PopStyleColor(7); // PopupBg, Border, Text, TextDisabled, Header, HeaderHovered, HeaderActive
    ImGui::PopStyleVar(5);   // WindowBorderSize, PopupBorderSize, PopupRounding, WindowPadding, ItemSpacing
}

bool EditorWidgets::BeginMenuPopup(const CHAR* PopupId, const PopupAnchor& Anchor, float MinWidth)
{
    if (Anchor.bRequestPosition || ImGui::IsPopupOpen(PopupId, ImGuiPopupFlags_None))
    {
        ImGui::SetNextWindowPos(ImVec2(Anchor.Min.x, Anchor.Max.y), ImGuiCond_Always);
    }

    // -----------------------------------------------------------------------------------------
    // Styling Vars
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

    // -----------------------------------------------------------------------------------------
    // Font
    // -----------------------------------------------------------------------------------------

    ImFont* Font = EditorFonts::SegoeUI_18 ? EditorFonts::SegoeUI_18 : EditorFonts::DefaultFont;
    CHECK(Font != nullptr);

    ImGui::PushFont(Font);

    // -----------------------------------------------------------------------------------------
    // Open menu
    // -----------------------------------------------------------------------------------------

    ImGui::SetNextWindowSizeConstraints(ImVec2(FinalMinWidth, 0.0f), ImVec2(FLT_MAX, FLT_MAX));

    const bool bOpen = ImGui::BeginPopup(PopupId);
    if (bOpen)
    {
        ImDrawList* DrawList = ImGui::GetWindowDrawList();

        const ImVec2 WinPos  = ImGui::GetWindowPos();
        const ImVec2 WinSize = ImGui::GetWindowSize();
        const ImVec2 Min     = ImVec2(WinPos.x + 1.0f, WinPos.y + 1.0f);
        const ImVec2 Max     = ImVec2(WinPos.x + WinSize.x - 1.0f, WinPos.y + WinSize.y - 1.0f);
        DrawList->AddRect(Min, Max, IM_COL32(50, 50, 50, 255), 0.0f, ImDrawFlags_None, 1.0f);
    }
    else
    {
        PopMenuPopupStyle();
    }

    return bOpen;
}

void EditorWidgets::EndMenuPopup()
{
    PopMenuPopupStyle();
    ImGui::EndPopup();
}

static constexpr float ContextMenuPopupPadY      = 10.0f;
static constexpr float ContextMenuContentIndentX = 20.0f;
static constexpr float ContextMenuMinWidth       = 180.0f;
static constexpr float ContextMenuFinalMinWidth  = ContextMenuMinWidth + (ContextMenuContentIndentX * 2.0f);

static void PushContextMenuStyle()
{
    // -----------------------------------------------------------------------------------------
    // Styling Vars
    // -----------------------------------------------------------------------------------------

    const ImVec4 ContextMenuPopupBg       = ImVec4(56.0f / 255.0f, 56.0f / 255.0f, 56.0f / 255.0f, 1.0f);
    const ImVec4 ContextMenuPopupBorder   = ImVec4(63.0f / 255.0f, 63.0f / 255.0f, 63.0f / 255.0f, 1.0f);
    const ImVec4 ContextMenuTextColor     = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    const ImVec4 ContextMenuShortcutColor = ImVec4(175.0f / 255.0f, 175.0f / 255.0f, 175.0f / 255.0f, 1.0f);
    const ImVec4 ContextMenuHoverBlue     = ImVec4(0.0f / 255.0f, 112.0f / 255.0f, 224.0f / 255.0f, 1.0f);

    // -----------------------------------------------------------------------------------------
    // Style vars
    // -----------------------------------------------------------------------------------------

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, ContextMenuPopupPadY));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));

    // -----------------------------------------------------------------------------------------
    // Style colors
    // -----------------------------------------------------------------------------------------

    ImGui::PushStyleColor(ImGuiCol_PopupBg, ContextMenuPopupBg);
    ImGui::PushStyleColor(ImGuiCol_Border, ContextMenuPopupBorder);
    ImGui::PushStyleColor(ImGuiCol_Text, ContextMenuTextColor);
    ImGui::PushStyleColor(ImGuiCol_TextDisabled, ContextMenuShortcutColor);
    ImGui::PushStyleColor(ImGuiCol_Header, ContextMenuPopupBg);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ContextMenuHoverBlue);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ContextMenuHoverBlue);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ContextMenuPopupBg);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ContextMenuPopupBg);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ContextMenuPopupBg);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ContextMenuPopupBg);

    // -----------------------------------------------------------------------------------------
    // Font
    // -----------------------------------------------------------------------------------------

    ImFont* Font = EditorFonts::SegoeUI_18 ? EditorFonts::SegoeUI_18 : EditorFonts::DefaultFont;
    CHECK(Font != nullptr);

    ImGui::PushFont(Font);
}

static void PopContextMenuStyle()
{
    ImGui::PopFont();
    ImGui::PopStyleColor(11);
    ImGui::PopStyleVar(6);
}

bool EditorWidgets::BeginPopupContextWindow(const CHAR* PopupId, ImGuiPopupFlags Flags)
{
    PushContextMenuStyle();

    ImGui::SetNextWindowSizeConstraints(ImVec2(ContextMenuFinalMinWidth, 0.0f), ImVec2(FLT_MAX, FLT_MAX));

    const bool bOpen = ImGui::BeginPopupContextWindow(PopupId, Flags);
    if (bOpen)
    {
        // Set cursor to arrow when context menu is open
        ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

        const ImVec2 WinPos  = ImGui::GetWindowPos();
        const ImVec2 WinSize = ImGui::GetWindowSize();
        const ImVec2 Min     = ImVec2(WinPos.x + 1.0f, WinPos.y + 1.0f);
        const ImVec2 Max     = ImVec2(WinPos.x + WinSize.x - 1.0f, WinPos.y + WinSize.y - 1.0f);

        ImDrawList* DrawList = ImGui::GetWindowDrawList();
        DrawList->AddRect(Min, Max, IM_COL32(50, 50, 50, 255), 0.0f, ImDrawFlags_None, 1.0f);
    }
    else
    {
        PopContextMenuStyle();
    }

    return bOpen;
}

bool EditorWidgets::BeginPopupContextItem(const CHAR* PopupId)
{
    PushContextMenuStyle();

    ImGui::SetNextWindowSizeConstraints(ImVec2(ContextMenuFinalMinWidth, 0.0f), ImVec2(FLT_MAX, FLT_MAX));

    const bool bOpen = ImGui::BeginPopupContextItem(PopupId);
    if (bOpen)
    {
        // Set cursor to arrow when context menu is open
        ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

        const ImVec2 WinPos  = ImGui::GetWindowPos();
        const ImVec2 WinSize = ImGui::GetWindowSize();
        const ImVec2 Min     = ImVec2(WinPos.x + 1.0f, WinPos.y + 1.0f);
        const ImVec2 Max     = ImVec2(WinPos.x + WinSize.x - 1.0f, WinPos.y + WinSize.y - 1.0f);

        ImDrawList* DrawList = ImGui::GetWindowDrawList();
        DrawList->AddRect(Min, Max, IM_COL32(50, 50, 50, 255), 0.0f, ImDrawFlags_None, 1.0f);
    }
    else
    {
        PopContextMenuStyle();
    }

    return bOpen;
}

void EditorWidgets::EndPopupContext()
{
    PopContextMenuStyle();
    ImGui::EndPopup();
}

bool EditorWidgets::BeginPropertyTable(const CHAR* TableId, float LabelColumnWidth, float RevertColumnWidth)
{
    // -----------------------------------------------------------------------------------------
    // Colors
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
    const float TableWidth = Storage->GetFloat(ImGui::GetID("##LastPropTableW"), 0.0f);

    if (TableWidth > 0.0f)
    {
        const ImGuiStyle& Style = ImGui::GetStyle();

        const ImVec2 CursorAfterTable = ImGui::GetCursorScreenPos();
        float BottomY = CursorAfterTable.y - Style.ItemSpacing.y;

        const ImU32 Color     = ImGui::GetColorU32(ImGuiCol_TableBorderStrong);
        const float Thickness = 2.0f;

        const float X1      = X0 + TableWidth;
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

void EditorWidgets::PropertyRowLabel(const CHAR* Label)
{
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);

    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(Label);

    ImGui::TableSetColumnIndex(1);
    ImGui::SetNextItemWidth(-FLT_MIN); // Fill available width
}

void EditorWidgets::PropertySeparatorRow(float PaddingY)
{
    UNREFERENCED_VARIABLE(PaddingY);
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

bool EditorWidgets::BeginRichTextView(const CHAR* InId, const ImVec2& InSize, RichTextViewContext& InOutContext, ImGuiWindowFlags InFlags, bool bWithContextMenu)
{
    InOutContext.ClearForNewFrame();

    const ImGuiWindowFlags Flags = InFlags | ImGuiWindowFlags_HorizontalScrollbar;

    InOutContext.ViewId = ImGui::GetID(InId);

    const bool bOpen = ImGui::BeginChild(InId, InSize, ImGuiChildFlags_Border, Flags);
    if (!bOpen)
    {
        return false;
    }

    InOutContext.bActive      = true;
    InOutContext.LineHeight   = ImGui::GetTextLineHeight();
    InOutContext.CharWidth    = ImGui::CalcTextSize("A").x;
    InOutContext.ContentStart = ImGui::GetCursorScreenPos();

    if (bWithContextMenu && EditorWidgets::BeginPopupContextWindow("##RichTextViewContext", ImGuiPopupFlags_MouseButtonRight))
    {
        EditorWidgets::MenuLabeledSeparator("Log");
        const String Selected = BuildSelectedText(InOutContext);
        if (EditorWidgets::MenuItem("Copy Selection", nullptr, false, InOutContext.bHasSelection && !Selected.IsEmpty()))
        {
            ImGui::SetClipboardText(*Selected);
        }
        if (EditorWidgets::MenuItem("Copy All"))
        {
            String All;
            for (int32 L = 0; L < InOutContext.Lines.Size(); ++L)
            {
                const RichTextLine& Line = InOutContext.Lines[L];
                for (int32 s = 0; s < Line.Spans.Size(); ++s)
                {
                    All += Line.Spans[s].Text;
                }
                All += "\n";
            }
            ImGui::SetClipboardText(*All);
        }
        if (EditorWidgets::MenuItem("Clear Selection", nullptr, false, InOutContext.bHasSelection))
        {
            InOutContext.bHasSelection = false;
            InOutContext.bSelecting = false;
        }
        EditorWidgets::EndPopupContext();
    }

    return true;
}

void EditorWidgets::RichTextSelectAll(RichTextViewContext& InOutContext)
{
    if (InOutContext.Lines.IsEmpty())
    {
        InOutContext.bHasSelection = false;
        InOutContext.bSelecting    = false;
        return;
    }
    const int32 LastLineIndex = InOutContext.Lines.Size() - 1;
    const int32 LastCol       = InOutContext.Lines[LastLineIndex].TotalChars;
    InOutContext.bHasSelection = true;
    InOutContext.bSelecting    = false;
    InOutContext.SelStart.Line   = 0;
    InOutContext.SelStart.Column = 0;
    InOutContext.SelEnd.Line     = LastLineIndex;
    InOutContext.SelEnd.Column   = LastCol;
}

String EditorWidgets::GetSelectedRichText(const RichTextViewContext& InContext)
{
    return BuildSelectedText(InContext);
}

void EditorWidgets::RichTextNewLine(RichTextViewContext& InOutContext)
{
    if (!InOutContext.bActive)
    {
        return;
    }

    RichTextLine Line;
    Line.TotalChars = 0;
    InOutContext.Lines.Add(Line);
}

void EditorWidgets::RichTextAddText(RichTextViewContext& InOutContext, const CHAR* InText, ImU32 InTextColor)
{
    if (!InOutContext.bActive || InOutContext.Lines.IsEmpty() || !InText)
    {
        return;
    }

    RichTextLine& Line = InOutContext.Lines[InOutContext.Lines.Size() - 1];

    RichTextSpan Span;
    Span.Text           = InText;
    Span.TextColor      = InTextColor;
    Span.bHasBackground = false;

    Line.TotalChars += static_cast<int32>(CString::Strlen(InText));
    Line.Spans.Add(Span);
}

void EditorWidgets::RichTextAddTextBg(RichTextViewContext& InOutContext, const CHAR* InText, ImU32 InTextColor, ImU32 InBackgroundColor)
{
    if (!InOutContext.bActive || InOutContext.Lines.IsEmpty() || !InText)
    {
        return;
    }

    RichTextLine& Line = InOutContext.Lines[InOutContext.Lines.Size() - 1];

    RichTextSpan Span;
    Span.Text            = InText;
    Span.TextColor       = InTextColor;
    Span.bHasBackground  = true;
    Span.BackgroundColor = InBackgroundColor;

    Line.TotalChars += static_cast<int32>(CString::Strlen(InText));
    Line.Spans.Add(Span);
}

void EditorWidgets::EndRichTextView(RichTextViewContext& InOutContext)
{
    if (!InOutContext.bActive)
    {
        ImGui::EndChild();
        return;
    }

    ImGuiIO& State = ImGui::GetIO();

    ImDrawList*  DrawList      = ImGui::GetWindowDrawList();
    ImGuiWindow* CurrentWindow = ImGui::GetCurrentWindow();

    const float FullLineHeight = InOutContext.LineHeight;
    const float TotalHeight    = InOutContext.Padding.y + static_cast<float>(InOutContext.Lines.Size()) * FullLineHeight + InOutContext.Padding.y;

    {
        const ImVec2 SavedCursorPos = ImGui::GetCursorPos();
        ImGui::Dummy(ImVec2(0.0f, TotalHeight));
        ImGui::SetCursorPos(SavedCursorPos);
    }

    const auto MarkKeyboardCaptureActive = [&]()
    {
        ImGui::SetWindowFocus();
        ImGui::SetNextFrameWantCaptureKeyboard(true);
    };

    // -----------------------------------------------------------------------------------------
    // Selection begin
    // -----------------------------------------------------------------------------------------

    const bool bHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_NoPopupHierarchy);
    if (bHovered)
    {
        const ImGuiStyle& Style = ImGui::GetStyle();

        const bool bHasVScroll = ImGui::GetScrollMaxY() > 0.0f;
        const bool bHasHScroll = ImGui::GetScrollMaxX() > 0.0f;

        const ImVec2 WinPos  = ImGui::GetWindowPos();
        const ImVec2 WinSize = ImGui::GetWindowSize();
        const ImVec2 Mouse   = State.MousePos;

        const float MaxX = WinPos.x + WinSize.x - (bHasVScroll ? Style.ScrollbarSize : 0.0f);
        const float MaxY = WinPos.y + WinSize.y - (bHasHScroll ? Style.ScrollbarSize : 0.0f);

        if (Mouse.x < MaxX && Mouse.y < MaxY)
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_TextInput);
        }
    }

    if (bHovered && (ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle)))
    {
        MarkKeyboardCaptureActive();
    }

    if (bHovered && (State.MouseWheel != 0.0f || State.MouseWheelH != 0.0f))
    {
        MarkKeyboardCaptureActive();
    }

    if (bHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        InOutContext.bSelecting    = true;
        InOutContext.bHasSelection = true;

        InOutContext.SelStart = GetMouseSelectionPoint(InOutContext, State.MousePos);
        InOutContext.SelEnd   = InOutContext.SelStart;

        MarkKeyboardCaptureActive();
    }

    // -----------------------------------------------------------------------------------------
    // Select all (Ctrl + A)
    // -----------------------------------------------------------------------------------------

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && State.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A, false))
    {
        ImGui::SetNextFrameWantCaptureKeyboard(true);

        if (!InOutContext.Lines.IsEmpty())
        {
            const int32 LastLineIndex = InOutContext.Lines.Size() - 1;
            const int32 LastCol       = InOutContext.Lines[LastLineIndex].TotalChars;

            InOutContext.bHasSelection = true;
            InOutContext.bSelecting    = false;

            InOutContext.SelStart.Line   = 0;
            InOutContext.SelStart.Column = 0;
            InOutContext.SelEnd.Line     = LastLineIndex;
            InOutContext.SelEnd.Column   = LastCol;
        }
        else
        {
            InOutContext.bHasSelection = false;
            InOutContext.bSelecting    = false;
        }
    }

    // -----------------------------------------------------------------------------------------
    // Selection update
    // -----------------------------------------------------------------------------------------

    if (InOutContext.bSelecting && ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        InOutContext.SelEnd = GetMouseSelectionPoint(InOutContext, State.MousePos);

        {
            const float TopY              = CurrentWindow->InnerRect.Min.y;
            const float BottomY           = CurrentWindow->InnerRect.Max.y;
            const float EdgeInside      = 10.0f;
            const float RampDistance    = 100.0f;
            const float BaseSpeedPxPerSec = 20.0f;
            const float MaxSpeedPxPerSec  = 2000.0f;

            float ScrollDir         = 0.0f;
            float DistPastTrigger = 0.0f;

            if (State.MousePos.y <= TopY + EdgeInside)
            {
                ScrollDir = -1.0f;

                const float TriggerY = TopY + EdgeInside;
                DistPastTrigger = TriggerY - State.MousePos.y;
            }
            else if (State.MousePos.y >= BottomY - EdgeInside)
            {
                ScrollDir = 1.0f;

                const float TriggerY = BottomY - EdgeInside;
                DistPastTrigger = State.MousePos.y - TriggerY;
            }

            DistPastTrigger = Math::Max(0.0f, DistPastTrigger);

            const float ScrollMaxY = ImGui::GetScrollMaxY();
            if (ScrollDir != 0.0f && ScrollMaxY > 0.0f)
            {
                const float DeltaTime = (State.DeltaTime > 0.0f) ? State.DeltaTime : (1.0f / 60.0f);

                float T = DistPastTrigger / RampDistance;
                T = Math::Clamp(T, 0.0f, 1.0f);

                T = T * T * (3.0f - 2.0f * T);

                const float Speed = BaseSpeedPxPerSec + (MaxSpeedPxPerSec - BaseSpeedPxPerSec) * T;
                float Delta = ScrollDir * Speed * DeltaTime;

                const float MinDelta = 1.0f;
                if (Delta > -MinDelta && Delta < MinDelta)
                {
                    Delta = (ScrollDir < 0.0f) ? -MinDelta : MinDelta;
                }

                float ScrollY = ImGui::GetScrollY();
                ScrollY = Math::Clamp(ScrollY + Delta, 0.0f, ScrollMaxY);
                ImGui::SetScrollY(ScrollY);

                InOutContext.SelEnd = GetMouseSelectionPoint(InOutContext, State.MousePos);
            }
        }
    }

    // -----------------------------------------------------------------------------------------
    // Selection end
    // -----------------------------------------------------------------------------------------

    if (InOutContext.bSelecting && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        InOutContext.bSelecting = false;

        if (InOutContext.SelStart.Line == InOutContext.SelEnd.Line && InOutContext.SelStart.Column == InOutContext.SelEnd.Column)
        {
            InOutContext.bHasSelection = false;
        }
    }

    // -----------------------------------------------------------------------------------------
    // Copy (Ctrl + C)
    // -----------------------------------------------------------------------------------------

    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && State.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C, false))
    {
        ImGui::SetNextFrameWantCaptureKeyboard(true);

        if (InOutContext.bHasSelection)
        {
            const String Selected = BuildSelectedText(InOutContext);
            if (!Selected.IsEmpty())
            {
                ImGui::SetClipboardText(*Selected);
            }
        }
        else
        {
            const RichTextSelectionPoint P = GetMouseSelectionPoint(InOutContext, State.MousePos);
            if (InOutContext.Lines.IsValidIndex(P.Line))
            {
                String LineText;
                for (int32 s = 0; s < InOutContext.Lines[P.Line].Spans.Size(); ++s)
                {
                    LineText += InOutContext.Lines[P.Line].Spans[s].Text;
                }

                ImGui::SetClipboardText(*LineText);
            }
        }
    }

    // -----------------------------------------------------------------------------------------
    // Draw
    // -----------------------------------------------------------------------------------------

    ImGuiListClipper Clipper;
    Clipper.Begin(InOutContext.Lines.Size(), FullLineHeight);

    RichTextSelectionPoint SelA = InOutContext.SelStart;
    RichTextSelectionPoint SelB = InOutContext.SelEnd;
    NormalizeSelection(SelA, SelB);

    const ImU32 SelectionBg = ImGui::GetColorU32(ImGuiCol_TextSelectedBg);

    while (Clipper.Step())
    {
        for (int32 LineIndex = Clipper.DisplayStart; LineIndex < Clipper.DisplayEnd; ++LineIndex)
        {
            if (!InOutContext.Lines.IsValidIndex(LineIndex))
            {
                continue;
            }

            const RichTextLine& Line = InOutContext.Lines[LineIndex];

            const float Y = InOutContext.ContentStart.y + InOutContext.Padding.y + LineIndex * FullLineHeight;
            const float X = InOutContext.ContentStart.x + InOutContext.Padding.x;

            const ImVec2 LineMin = ImVec2(CurrentWindow->WorkRect.Min.x, Y);
            const ImVec2 LineMax = ImVec2(CurrentWindow->WorkRect.Max.x, Y + FullLineHeight);

            if (InOutContext.bHasSelection)
            {
                int32 ColStart = 0;
                int32 ColEnd   = 0;

                if (SelectionIntersectsLine(SelA, SelB, LineIndex, ColStart, ColEnd, Line.TotalChars))
                {
                    const float SelX0 = X + ColStart * InOutContext.CharWidth;
                    const float SelX1 = X + ColEnd   * InOutContext.CharWidth;

                    DrawList->AddRectFilled(ImVec2(SelX0, LineMin.y), ImVec2(SelX1, LineMax.y), SelectionBg, 0.0f);
                }
            }

            float CursorX = X;
            for (int32 s = 0; s < Line.Spans.Size(); ++s)
            {
                const RichTextSpan& Span = Line.Spans[s];

                const CHAR* Text = *Span.Text;
                if (!Text || Text[0] == 0)
                {
                    continue;
                }

                const float SpanWidth = ImGui::CalcTextSize(Text).x;
                if (Span.bHasBackground)
                {
                    const float HighlightPadY    = 2.0f;
                    const float HighlightMarginY = 2.0f;
                    const float HighlightTopY    = LineMin.y + HighlightPadY - HighlightMarginY;
                    const float HighlightBotY    = LineMax.y - HighlightPadY + HighlightMarginY;
                    DrawList->AddRectFilled(ImVec2(CursorX, HighlightTopY), ImVec2(CursorX + SpanWidth, HighlightBotY), Span.BackgroundColor, 0.0f);
                }

                DrawList->AddText(ImVec2(CursorX, LineMin.y), Span.TextColor, Text);
                CursorX += SpanWidth;
            }
        }
    }

    // -----------------------------------------------------------------------------------------
    // Scroll to bottom
    // -----------------------------------------------------------------------------------------

    const float CurrentScrollMaxY = ImGui::GetScrollMaxY();
    InOutContext.bIsScrolledToBottom = (CurrentScrollMaxY <= 0.0f) || (ImGui::GetScrollY() >= CurrentScrollMaxY - 1.0f);

    const bool bStayPinned = InOutContext.bAutoScroll && InOutContext.bIsScrolledToBottom;

    if ((InOutContext.bScrollToBottom || bStayPinned) && !InOutContext.bSelecting)
    {
        ImGui::SetScrollFromPosY(ImGui::GetCursorStartPos().y + TotalHeight, 1.0f);

        InOutContext.bScrollToBottom     = false;
        InOutContext.bIsScrolledToBottom = true;
    }

    ImGui::EndChild();
}

bool EditorWidgets::DrawButtonCenteredOnLine(const CHAR* Label, float Alignment)
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

void EditorWidgets::DrawCheckMark(ImDrawList* DrawList, ImVec2 Position, ImU32 Color, float CheckMarkSize)
{
    const float Thickness = ImMax(1.0f, CheckMarkSize / 6.0f);

    const ImVec2 PointA = ImVec2(Position.x + CheckMarkSize * 0.15f, Position.y + CheckMarkSize * 0.55f);
    const ImVec2 PointB = ImVec2(Position.x + CheckMarkSize * 0.40f, Position.y + CheckMarkSize * 0.80f);
    const ImVec2 PointC = ImVec2(Position.x + CheckMarkSize * 0.85f, Position.y + CheckMarkSize * 0.20f);

    DrawList->AddLine(PointA, PointB, Color, Thickness);
    DrawList->AddLine(PointB, PointC, Color, Thickness);
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
    inline static EditorIcon UndoIcon             = EditorIcon();
    inline static EditorIcon SearchIcon           = EditorIcon();
    inline static EditorIcon LockedIcon           = EditorIcon();
    inline static EditorIcon UnlockedIcon         = EditorIcon();
    inline static EditorIcon FolderIcon           = EditorIcon();
    inline static EditorIcon FolderSmallIcon      = EditorIcon();
    inline static EditorIcon FolderSmall2Icon     = EditorIcon();
    inline static EditorIcon FolderOpenSmallIcon  = EditorIcon();
    inline static EditorIcon DocumentIcon         = EditorIcon();
    inline static EditorIcon DocumentSmallIcon    = EditorIcon();
    inline static EditorIcon CheckmarkIcon        = EditorIcon();
    inline static EditorIcon ForbiddenIcon        = EditorIcon();
    inline static EditorIcon CircledCheckmarkIcon = EditorIcon();
    inline static EditorIcon NextIcon             = EditorIcon();
    inline static EditorIcon PreviousIcon         = EditorIcon();
    inline static EditorIcon CloseIcon            = EditorIcon();
    inline static EditorIcon FilterIcon           = EditorIcon();
    inline static EditorIcon RightArrowIcon       = EditorIcon();
    inline static EditorIcon DownArrowIcon        = EditorIcon();
    inline static EditorIcon CollapseArrowDown    = EditorIcon();
    inline static EditorIcon CollapseArrowRight   = EditorIcon();
};

ImTextureID EditorIcons::UndoIcon             = nullptr;
ImTextureID EditorIcons::SearchIcon           = nullptr;
ImTextureID EditorIcons::LockedIcon           = nullptr;
ImTextureID EditorIcons::UnlockedIcon         = nullptr;
ImTextureID EditorIcons::FolderIcon           = nullptr;
ImTextureID EditorIcons::FolderSmallIcon      = nullptr;
ImTextureID EditorIcons::FolderSmall2Icon     = nullptr;
ImTextureID EditorIcons::FolderOpenSmallIcon  = nullptr;
ImTextureID EditorIcons::DocumentIcon         = nullptr;
ImTextureID EditorIcons::DocumentSmallIcon    = nullptr;
ImTextureID EditorIcons::CheckmarkIcon        = nullptr;
ImTextureID EditorIcons::ForbiddenIcon        = nullptr;
ImTextureID EditorIcons::CircledCheckmarkIcon = nullptr;
ImTextureID EditorIcons::NextIcon             = nullptr;
ImTextureID EditorIcons::PreviousIcon         = nullptr;
ImTextureID EditorIcons::CloseIcon            = nullptr;
ImTextureID EditorIcons::FilterIcon           = nullptr;
ImTextureID EditorIcons::RightArrowIcon       = nullptr;
ImTextureID EditorIcons::DownArrowIcon        = nullptr;
ImTextureID EditorIcons::CollapseArrowDown    = nullptr;
ImTextureID EditorIcons::CollapseArrowRight   = nullptr;

static bool LoadEditorIcon(const CHAR* InRelativePath, ImTextureID& OutIconID, EditorIcon& OutIcon, bool bEnableBlending = true, bool bEnableLinearSampler = true)
{
    OutIcon.Reset();
    OutIconID = nullptr;

    String FullPath = Paths::GetAssetDir();
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

    OutIcon.ImGuiTexture = MakeUniquePtr<FImGuiTexture>(TextureRHI);
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
    bResult &= LoadEditorIcon("Editor/Icons/FolderSmall2.png", FolderSmall2Icon, EditorIconsInternal::FolderSmall2Icon);
    bResult &= LoadEditorIcon("Editor/Icons/FolderOpenSmall.png", FolderOpenSmallIcon, EditorIconsInternal::FolderOpenSmallIcon);
    bResult &= LoadEditorIcon("Editor/Icons/Document.png", DocumentIcon, EditorIconsInternal::DocumentIcon);
    bResult &= LoadEditorIcon("Editor/Icons/DocumentSmall.png", DocumentSmallIcon, EditorIconsInternal::DocumentSmallIcon);
    bResult &= LoadEditorIcon("Editor/Icons/Checkmark.png", CheckmarkIcon, EditorIconsInternal::CheckmarkIcon);
    bResult &= LoadEditorIcon("Editor/Icons/Forbidden.png", ForbiddenIcon, EditorIconsInternal::ForbiddenIcon);
    bResult &= LoadEditorIcon("Editor/Icons/CircledCheckmark.png", CircledCheckmarkIcon, EditorIconsInternal::CircledCheckmarkIcon);
    bResult &= LoadEditorIcon("Editor/Icons/Next.png", NextIcon, EditorIconsInternal::NextIcon);
    bResult &= LoadEditorIcon("Editor/Icons/Previous.png", PreviousIcon, EditorIconsInternal::PreviousIcon);
    bResult &= LoadEditorIcon("Editor/Icons/Close.png", CloseIcon, EditorIconsInternal::CloseIcon);
    bResult &= LoadEditorIcon("Editor/Icons/Filter.png", FilterIcon, EditorIconsInternal::FilterIcon);
    bResult &= LoadEditorIcon("Editor/Icons/RightArrow.png", RightArrowIcon, EditorIconsInternal::RightArrowIcon);
    bResult &= LoadEditorIcon("Editor/Icons/DownArrow.png", DownArrowIcon, EditorIconsInternal::DownArrowIcon);
    bResult &= LoadEditorIcon("Editor/Icons/CollapseArrowDown.png", CollapseArrowDown, EditorIconsInternal::CollapseArrowDown);
    bResult &= LoadEditorIcon("Editor/Icons/CollapseArrowRight.png", CollapseArrowRight, EditorIconsInternal::CollapseArrowRight);

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
    UnloadEditorIcon(FolderSmall2Icon, EditorIconsInternal::FolderSmall2Icon);
    UnloadEditorIcon(FolderOpenSmallIcon, EditorIconsInternal::FolderOpenSmallIcon);
    UnloadEditorIcon(DocumentIcon, EditorIconsInternal::DocumentIcon);
    UnloadEditorIcon(DocumentSmallIcon, EditorIconsInternal::DocumentSmallIcon);
    UnloadEditorIcon(CheckmarkIcon, EditorIconsInternal::CheckmarkIcon);
    UnloadEditorIcon(ForbiddenIcon, EditorIconsInternal::ForbiddenIcon);
    UnloadEditorIcon(CircledCheckmarkIcon, EditorIconsInternal::CircledCheckmarkIcon);
    UnloadEditorIcon(NextIcon, EditorIconsInternal::NextIcon);
    UnloadEditorIcon(PreviousIcon, EditorIconsInternal::PreviousIcon);
    UnloadEditorIcon(CloseIcon, EditorIconsInternal::CloseIcon);
    UnloadEditorIcon(FilterIcon, EditorIconsInternal::FilterIcon);
    UnloadEditorIcon(RightArrowIcon, EditorIconsInternal::RightArrowIcon);
    UnloadEditorIcon(DownArrowIcon, EditorIconsInternal::DownArrowIcon);
    UnloadEditorIcon(CollapseArrowDown, EditorIconsInternal::CollapseArrowDown);
    UnloadEditorIcon(CollapseArrowRight, EditorIconsInternal::CollapseArrowRight);
}

ImFont* EditorFonts::DefaultFont = nullptr;
ImFont* EditorFonts::SegoeUI_18  = nullptr;
ImFont* EditorFonts::SegoeUI_22  = nullptr;
ImFont* EditorFonts::Consola_16  = nullptr;

static ImFont* LoadEditorFont(const CHAR* InRelativePath, float SizePixels, const ImFontConfig* FontCfgTemplate = nullptr, const ImWchar* GlyphRanges = nullptr)
{
    String FullPath = Paths::GetAssetDir();
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

    // Default font
    DefaultFont = State.Fonts->AddFontDefault();

    // Load fonts from file
    SegoeUI_18 = LoadEditorFont("Editor/Fonts/segoeui.ttf", 18.0f);
    SegoeUI_22 = LoadEditorFont("Editor/Fonts/segoeui.ttf", 22.0f);
    Consola_16 = LoadEditorFont("Editor/Fonts/consola.ttf", 16.0f);

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
    SegoeUI_22  = nullptr;
    Consola_16  = nullptr;
}
