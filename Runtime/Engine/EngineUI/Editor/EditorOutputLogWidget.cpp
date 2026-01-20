#include "Engine/EngineUI/Editor/EditorOutputLogWidget.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Templates/CString.h"
#include <imgui.h>

FEditorOutputLogWidget::FEditorOutputLogWidget()
    : IOutputDevice()
    , bVisible(true)
    , bAutoScroll(true)
    , bScrollToBottom(false)
    , bFilterInfo(true)
    , bFilterWarning(true)
    , bFilterError(true)
    , SearchFilterBuffer()
    , Messages()
{
    if (FOutputDeviceLogger* Logger = FOutputDeviceLogger::Get())
    {
        Logger->RegisterOutputDevice(this);
    }

    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDelegate(FImGuiDelegate::CreateRaw(this, &FEditorOutputLogWidget::Draw));
    }

    SearchFilterBuffer.Fill(0);
}

FEditorOutputLogWidget::~FEditorOutputLogWidget()
{
    if (FOutputDeviceLogger* Logger = FOutputDeviceLogger::Get())
    {
        Logger->UnregisterOutputDevice(this);
    }

    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().RemoveDelegate(ImGuiDelegateHandle);
    }
}

void FEditorOutputLogWidget::Log(const FString& Message)
{
    Log(ELogSeverity::Info, Message);
}

void FEditorOutputLogWidget::Log(ELogSeverity Severity, const FString& Message)
{
    SCOPED_LOCK(MessagesCS);

    constexpr int32 MaxMessages = 5000;
    Messages.Emplace(FLogMessage{ Message, Severity });
    
    if (Messages.Size() > MaxMessages)
    {
        const int32 Overflow = Messages.Size() - MaxMessages;
        Messages.RemoveAt(0, Overflow);
    }
    
    if (bAutoScroll)
    {
        bScrollToBottom = true;
    }
}

void FEditorOutputLogWidget::DrawFilterBar()
{
    ImGuiStyle& Style = ImGui::GetStyle();

    const float GapX             = 8.0f;
    const float IconSizePx       = 18.0f;
    const float ArrowIconSizePx  = 14.0f;
    const float IconTextGap      = 6.0f;
    const float TextArrowGap     = 6.0f;
    const float ButtonRoundingPx = 6.0f;

    // -------------------------------------------------------------------------------------------
    // Search Field
    // -------------------------------------------------------------------------------------------

    const float AvailableX     = ImGui::GetContentRegionAvail().x;
    const float MaxSearchWidth = AvailableX * 0.25f;

    float SearchWidth = MaxSearchWidth;
    EditorWidgets::EditorSearchField("##LogSearch", "Search Log", SearchFilterBuffer.Data(), SearchFilterBuffer.Size(), SearchWidth, true);

    // Ensure filter button matches search field height exactly
    float SearchBarHeight = ImGui::GetItemRectSize().y;
    if (SearchBarHeight <= 0.0f)
    {
        SearchBarHeight = ImGui::GetFrameHeight();
    }

    ImGui::SameLine(0.0f, GapX);

    // -------------------------------------------------------------------------------------------
    // Filter button
    // -------------------------------------------------------------------------------------------

    const auto DrawFilterButton = [&]() -> bool
    {
        ImGui::PushFont(EditorFonts::SegoeUI_22);

        const char* Label = "Filter";

        const ImVec2 TextSize = ImGui::CalcTextSize(Label);

        // IMPORTANT: Match the search-bar height exactly
        const float ButtonHeight = SearchBarHeight;

        const float ButtonWidth =
            IconSizePx +
            IconTextGap +
            TextSize.x +
            TextArrowGap +
            ArrowIconSizePx +
            Style.FramePadding.x * 2.0f;

        const ImVec2 ButtonSize = ImVec2(ButtonWidth, ButtonHeight);

        const bool bPressed = ImGui::InvisibleButton("##FilterButton", ButtonSize);
        const bool bHovered = ImGui::IsItemHovered();
        const bool bHeld    = ImGui::IsItemActive();

        const ImVec2 Min = ImGui::GetItemRectMin();
        const ImVec2 Max = ImGui::GetItemRectMax();

        const float Alpha01  = (Style.Alpha < 0.0f) ? 0.0f : (Style.Alpha > 1.0f ? 1.0f : Style.Alpha);
        const int32 Alpha255 = (int32)(Alpha01 * 255.0f);

        const ImU32 BgIdle  = IM_COL32(36, 36, 36, Alpha255);
        const ImU32 BgHover = IM_COL32(56, 56, 56, Alpha255);
        const ImU32 BgColor = (bHeld || bHovered) ? BgHover : BgIdle;
        const ImU32 White   = IM_COL32(255, 255, 255, Alpha255);

        ImDrawList* DrawList = ImGui::GetWindowDrawList();
        DrawList->AddRectFilled(Min, Max, BgColor, ButtonRoundingPx);

        // Left icon
        const float  LeftIconY   = Min.y + (ButtonHeight - IconSizePx) * 0.5f;
        const ImVec2 LeftIconMin = ImVec2(Min.x + Style.FramePadding.x, LeftIconY);
        const ImVec2 LeftIconMax = ImVec2(LeftIconMin.x + IconSizePx, LeftIconMin.y + IconSizePx);

        if (EditorIcons::FilterIcon)
        {
            DrawList->AddImage(EditorIcons::FilterIcon, LeftIconMin, LeftIconMax, ImVec2(0, 0), ImVec2(1, 1), White);
        }

        // Text
        const float TextX = LeftIconMax.x + IconTextGap;
        const float TextY = Min.y + (ButtonHeight - TextSize.y) * 0.5f;

        DrawList->AddText(EditorFonts::SegoeUI_22, EditorFonts::SegoeUI_22->FontSize, ImVec2(TextX, TextY), White, Label);

        // Right icon (your current one)
        const float  RightIconX   = TextX + TextSize.x + TextArrowGap;
        const float  RightIconY   = Min.y + (ButtonHeight - ArrowIconSizePx) * 0.5f;
        const ImVec2 RightIconMin = ImVec2(RightIconX, RightIconY);
        const ImVec2 RightIconMax = ImVec2(RightIconMin.x + ArrowIconSizePx, RightIconMin.y + ArrowIconSizePx);

        if (EditorIcons::DownArrowIcon)
        {
            DrawList->AddImage(EditorIcons::DownArrowIcon, RightIconMin, RightIconMax, ImVec2(0, 0), ImVec2(1, 1), White);
        }

        ImGui::PopFont();
        return bPressed;
    };

    if (DrawFilterButton())
    {
        ImGui::OpenPopup("LogFilterMenu");
    }

    // -------------------------------------------------------------------------------------------
    // Popup menu
    // -------------------------------------------------------------------------------------------

    const ImVec2 ButtonMin = ImGui::GetItemRectMin();
    const ImVec2 ButtonMax = ImGui::GetItemRectMax();

    ImGui::SetNextWindowPos(ImVec2(ButtonMin.x, ButtonMax.y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(180.0f, 0.0f), ImGuiCond_Appearing);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 2.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.0f);

    ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32(56, 56, 56, 255));
    ImGui::PushStyleColor(ImGuiCol_Border,  IM_COL32(69, 69, 69, 255));

    // Selectable colors (Selectable uses Header colors)
    ImGui::PushStyleColor(ImGuiCol_Header,        IM_COL32(56, 56, 56, 255)); // not hovered
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(87, 87, 87, 255)); // hovered
    ImGui::PushStyleColor(ImGuiCol_HeaderActive,  IM_COL32(87, 87, 87, 255)); // pressed

    if (ImGui::BeginPopup("LogFilterMenu"))
    {
        EditorWidgets::EditorMenuLabeledSeparator("Verbosity", 1.0f, 4.0f);

        if (EditorWidgets::EditorMenuItem("Messages", nullptr, bFilterInfo))
        {
            bFilterInfo = !bFilterInfo;
        }

        if (EditorWidgets::EditorMenuItem("Warnings", nullptr, bFilterWarning))
        {
            bFilterWarning = !bFilterWarning;
        }

        if (EditorWidgets::EditorMenuItem("Errors", nullptr, bFilterError))
        {
            bFilterError = !bFilterError;
        }

        ImGui::EndPopup();
    }

    ImGui::PopStyleColor(3); // Header, HeaderHovered, HeaderActive
    ImGui::PopStyleColor(2); // PopupBg, Border
    ImGui::PopStyleVar(3);   // WindowPadding, PopupBorderSize, PopupRounding
}

void FEditorOutputLogWidget::DrawLogList()
{
    const auto IsSeverityMatching = [this](ELogSeverity Severity)
    {
        if (Severity == ELogSeverity::Info)
        {
            return bFilterInfo;
        }
        else if (Severity == ELogSeverity::Warning)
        {
            return bFilterWarning;
        }
        else if (Severity == ELogSeverity::Error)
        {
            return bFilterError;
        }

        return true;
    };

    // Copy under lock once per frame
    TArray<FLogMessage> LocalMessages;
    {
        SCOPED_LOCK(MessagesCS);
        LocalMessages = Messages;
    }

    ImGui::PushFont(EditorFonts::Consola_16);

    const float PaddingX = 8.0f;
    const float PaddingY = 4.0f;

    const bool  bHasSearch = (SearchFilterBuffer[0] != 0);
    const char* Search     = SearchFilterBuffer.Data();
    for (int32 i = 0; i < LocalMessages.Size(); ++i)
    {
        const FLogMessage& Message = LocalMessages[i];
        if (!IsSeverityMatching(Message.Severity))
        {
            continue;
        }

        if (bHasSearch && !FCString::Stristr(*Message.Message, Search))
        {
            continue;
        }

        ImVec4 TextColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        switch (Message.Severity)
        {
            case ELogSeverity::Warning:
            {
                TextColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
                break;
            }
            case ELogSeverity::Error:
            {
                TextColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                break;
            }
            default:
            {
                break;
            }
        }

        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + PaddingY);

        ImGui::Indent(PaddingX);
        ImGui::TextColored(TextColor, "%s", *Message.Message);
        ImGui::Unindent(PaddingX);
    }

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + PaddingY);

    if (bScrollToBottom)
    {
        ImGui::SetScrollHereY(1.0f);
        bScrollToBottom = false;
    }

    ImGui::PopFont();
}

void FEditorOutputLogWidget::Draw()
{
    if (!bVisible)
    {
        return;
    }

    ImGuiStyle& Style = ImGui::GetStyle();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(Style.ItemSpacing.x, 0.0f));

    const ImGuiWindowFlags OutputLogFlags = ImGuiWindowFlags_NoCollapse;
    if (ImGui::Begin("Output Log", &bVisible, OutputLogFlags))
    {
        const float OuterPadX        = 4.0f;
        const float OuterPadTop      = 10.0f;
        const float OuterPadBottom   = 4.0f;
        const float GapBetweenPanels = 6.0f;
        const float HeaderInnerPadX  = 8.0f;
        const float HeaderInnerPadY  = 8.0f;
        const float OutputRoundingPx = 4.0f;
        const float HeaderRowH       = ImGui::GetFrameHeight();
        const float HeaderHeight     = HeaderRowH + GapBetweenPanels;

        const ImVec2 ContentMin = ImGui::GetWindowContentRegionMin();
        const ImVec2 ContentMax = ImGui::GetWindowContentRegionMax();
        const float  ContentW   = (ContentMax.x - ContentMin.x);
        const float  ContentH   = (ContentMax.y - ContentMin.y);

        float ChildWidth = ContentW - OuterPadX * 2.0f;
        if (ChildWidth < 1.0f)
        {
            ChildWidth = 1.0f;
        }

        const float HeaderX = ContentMin.x + OuterPadX;
        const float HeaderY = ContentMin.y + OuterPadTop;

        ImGui::SetCursorPos(ImVec2(HeaderX, HeaderY));

        ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(36, 36, 36, 255));

        const ImGuiWindowFlags OutputLogHeaderFlags = 
            ImGuiWindowFlags_NoScrollbar;

        if (ImGui::BeginChild("##OutputLogHeader", ImVec2(ChildWidth, HeaderHeight), true, OutputLogHeaderFlags))
        {
            DrawFilterBar();
        }

        ImGui::EndChild();

        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);

        const float OutputX = HeaderX;
        const float OutputY = HeaderY + HeaderHeight;

        ImGui::SetCursorPos(ImVec2(OutputX, OutputY));

        float OutputHeight = (ContentMin.y + ContentH) - OutputY - OuterPadBottom;
        if (OutputHeight < 1.0f)
        {
            OutputHeight = 1.0f;
        }

        const ImU32  OutputBgU32 = IM_COL32(26, 26, 26, 255);
        const ImVec4 OutputBg    = ImVec4(26.0f / 255.0f, 26.0f / 255.0f, 26.0f / 255.0f, 1.0f);

        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, OutputRoundingPx);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, OutputBgU32);
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, OutputBg);

        const ImGuiWindowFlags OutputLogScrollFlags = 
            ImGuiWindowFlags_HorizontalScrollbar;

        if (ImGui::BeginChild("##OutputLogScroll", ImVec2(ChildWidth, OutputHeight), true, OutputLogScrollFlags))
        {
            DrawLogList();
        }

        ImGui::EndChild();

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);
    }

    ImGui::End();

    ImGui::PopStyleVar(2);
}
