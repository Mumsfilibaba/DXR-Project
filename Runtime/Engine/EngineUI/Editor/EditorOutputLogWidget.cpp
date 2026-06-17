#include "Core/Templates/CString.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Engine/EngineUI/Editor/EditorOutputLogWidget.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "ImGuiPlugin/ImGuiCore.h"

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
    , MessagesCS()
    , ImGuiDelegateHandle()
    , RichTextCtx()
{
    if (FOutputDeviceLogger* Logger = FOutputDeviceLogger::Get())
    {
        Logger->RegisterOutputDevice(this);
    }

    if (IImguiPlugin::IsEnabled())
    {
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDrawDelegate(FImGuiDelegate::CreateRaw(this, &FEditorOutputLogWidget::Draw));
    }

    SearchFilterBuffer.Fill(0);

    RichTextCtx.Padding     = ImVec2(8.0f, 4.0f);
    RichTextCtx.bAutoScroll = bAutoScroll;
}

FEditorOutputLogWidget::~FEditorOutputLogWidget()
{
    if (FOutputDeviceLogger* Logger = FOutputDeviceLogger::Get())
    {
        Logger->UnregisterOutputDevice(this);
    }

    if (IImguiPlugin::IsEnabled())
    {
        IImguiPlugin::Get().RemoveDrawDelegate(ImGuiDelegateHandle);
    }
}

void FEditorOutputLogWidget::Log(const String& Message)
{
    Log(ELogSeverity::Info, Message);
}

void FEditorOutputLogWidget::Log(ELogSeverity Severity, const String& Message)
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
        const float OutputRounding = 4.0f;
        const float HeaderRowHeight  = ImGui::GetFrameHeight();
        const float HeaderHeight     = HeaderRowHeight + GapBetweenPanels;

        const ImVec2 ContentMin    = ImGui::GetWindowContentRegionMin();
        const ImVec2 ContentMax    = ImGui::GetWindowContentRegionMax();
        const float  ContentWidth  = (ContentMax.x - ContentMin.x);
        const float  ContentHeight = (ContentMax.y - ContentMin.y);

        float ChildWidth = ContentWidth - OuterPadX * 2.0f;
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

        const ImGuiWindowFlags OutputLogHeaderFlags = ImGuiWindowFlags_NoScrollbar;

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

        float OutputHeight = (ContentMin.y + ContentHeight) - OutputY - OuterPadBottom;
        if (OutputHeight < 1.0f)
        {
            OutputHeight = 1.0f;
        }

        // -------------------------------------------------------------------------------------
        // Log view
        // -------------------------------------------------------------------------------------

        const ImU32  OutputBgU32 = IM_COL32(26, 26, 26, 255);
        const ImVec4 OutputBg    = ImVec4(26.0f / 255.0f, 26.0f / 255.0f, 26.0f / 255.0f, 1.0f);

        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, OutputRounding);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, OutputBgU32);
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, OutputBg);

        const ImGuiWindowFlags OuterLogFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

        if (ImGui::BeginChild("##OutputLogOuter", ImVec2(ChildWidth, OutputHeight), true, OuterLogFlags))
        {
            DrawLogListRichText();
        }

        ImGui::EndChild();

        const ImVec2 LogOuterMin = ImGui::GetItemRectMin();
        const ImVec2 LogOuterMax = ImGui::GetItemRectMax();

        if (RichTextCtx.bHasSelection && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            const ImVec2 MousePos = ImGui::GetIO().MousePos;

            const bool bMouseInLogOuter =
                MousePos.x >= LogOuterMin.x &&
                MousePos.x <= LogOuterMax.x &&
                MousePos.y >= LogOuterMin.y &&
                MousePos.y <= LogOuterMax.y;

            if (!bMouseInLogOuter)
            {
                RichTextCtx.bHasSelection = false;
                RichTextCtx.bSelecting    = false;
            }
        }

        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);
    }

    ImGui::End();

    ImGui::PopStyleVar(2);
}

void FEditorOutputLogWidget::DrawFilterBar()
{
    ImGuiStyle& Style = ImGui::GetStyle();

    const float GapX = 8.0f;

    // -------------------------------------------------------------------------------------------
    // Search Field
    // -------------------------------------------------------------------------------------------

    const float AvailableX     = ImGui::GetContentRegionAvail().x;
    const float MaxSearchWidth = AvailableX * 0.25f;

    float SearchWidth = MaxSearchWidth;
    EditorWidgets::DrawSearchField("##LogSearch", "Search Log", SearchFilterBuffer.Data(), SearchFilterBuffer.Size(), SearchWidth, true);

    float SearchBarHeight = ImGui::GetItemRectSize().y;
    if (SearchBarHeight <= 0.0f)
    {
        SearchBarHeight = ImGui::GetFrameHeight();
    }

    ImGui::SameLine(0.0f, GapX);

    // -------------------------------------------------------------------------------------------
    // Filter button
    // -------------------------------------------------------------------------------------------

    const float IconSize       = 18.0f;
    const float ArrowIconSize  = 14.0f;
    const float IconTextGap    = 6.0f;
    const float TextArrowGap   = 6.0f;
    const float ButtonRounding = 6.0f;

    const auto DrawFilterButton = [&]() -> bool
    {
        ImGui::PushFont(EditorFonts::SegoeUI_22);

        const CHAR* Label = "Filter";

        const ImVec2 TextSize     = ImGui::CalcTextSize(Label);
        const float  ButtonHeight = SearchBarHeight;
        const float  ButtonWidth  = IconSize + IconTextGap + TextSize.x + TextArrowGap + ArrowIconSize + Style.FramePadding.x * 2.0f;
        const ImVec2 ButtonSize   = ImVec2(ButtonWidth, ButtonHeight);

        const bool bPressed = ImGui::InvisibleButton("##FilterButton", ButtonSize);
        const bool bHovered = ImGui::IsItemHovered();
        const bool bHeld    = ImGui::IsItemActive();

        const ImVec2 Min = ImGui::GetItemRectMin();
        const ImVec2 Max = ImGui::GetItemRectMax();

        const float Alpha01  = (Style.Alpha < 0.0f) ? 0.0f : (Style.Alpha > 1.0f ? 1.0f : Style.Alpha);
        const int32 Alpha255 = static_cast<int32>(Alpha01 * 255.0f);

        const ImU32 BgIdle  = IM_COL32(36, 36, 36, Alpha255);
        const ImU32 BgHover = IM_COL32(56, 56, 56, Alpha255);
        const ImU32 BgColor = (bHeld || bHovered) ? BgHover : BgIdle;
        const ImU32 White   = IM_COL32(255, 255, 255, Alpha255);

        ImDrawList* DrawList = ImGui::GetWindowDrawList();
        DrawList->AddRectFilled(Min, Max, BgColor, ButtonRounding);

        const float  LeftIconY   = Min.y + (ButtonHeight - IconSize) * 0.5f;
        const ImVec2 LeftIconMin = ImVec2(Min.x + Style.FramePadding.x, LeftIconY);
        const ImVec2 LeftIconMax = ImVec2(LeftIconMin.x + IconSize, LeftIconMin.y + IconSize);

        if (EditorIcons::FilterIcon)
        {
            DrawList->AddImage(EditorIcons::FilterIcon, LeftIconMin, LeftIconMax, ImVec2(0, 0), ImVec2(1, 1), White);
        }

        const float TextX = LeftIconMax.x + IconTextGap;
        const float TextY = Min.y + (ButtonHeight - TextSize.y) * 0.5f;

        DrawList->AddText(EditorFonts::SegoeUI_22, EditorFonts::SegoeUI_22->FontSize, ImVec2(TextX, TextY), White, Label);

        const float  RightIconX   = TextX + TextSize.x + TextArrowGap;
        const float  RightIconY   = Min.y + (ButtonHeight - ArrowIconSize) * 0.5f;
        const ImVec2 RightIconMin = ImVec2(RightIconX, RightIconY);
        const ImVec2 RightIconMax = ImVec2(RightIconMin.x + ArrowIconSize, RightIconMin.y + ArrowIconSize);

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
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(69, 69, 69, 255));
    ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(56, 56, 56, 255));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(87, 87, 87, 255));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, IM_COL32(87, 87, 87, 255));

    if (ImGui::BeginPopup("LogFilterMenu"))
    {
        EditorWidgets::MenuLabeledSeparator("Verbosity", 1.0f, 4.0f);

        if (EditorWidgets::MenuItem("Messages", nullptr, bFilterInfo))
        {
            bFilterInfo = !bFilterInfo;
        }

        if (EditorWidgets::MenuItem("Warnings", nullptr, bFilterWarning))
        {
            bFilterWarning = !bFilterWarning;
        }

        if (EditorWidgets::MenuItem("Errors", nullptr, bFilterError))
        {
            bFilterError = !bFilterError;
        }

        ImGui::EndPopup();
    }

    ImGui::PopStyleColor(3); // Header, HeaderHovered, HeaderActive
    ImGui::PopStyleColor(2); // PopupBg, Border
    ImGui::PopStyleVar(3);   // WindowPadding, PopupBorderSize, PopupRounding
}

void FEditorOutputLogWidget::DrawLogListRichText()
{
    TArray<FLogMessage> LocalMessages;
    {
        SCOPED_LOCK(MessagesCS);
        LocalMessages = Messages;
    }

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

    const bool  bHasSearch = (SearchFilterBuffer[0] != 0);
    const CHAR* Search     = SearchFilterBuffer.Data();

    RichTextCtx.bAutoScroll = bAutoScroll;

    if (bScrollToBottom)
    {
        RichTextCtx.bScrollToBottom = true;
        bScrollToBottom = false;
    }

    const ImU32 DefaultTextU32   = ImGui::GetColorU32(ImGuiCol_Text);
    const ImU32 WarningTextU32   = IM_COL32(255, 255, 0, 255);
    const ImU32 ErrorTextU32     = IM_COL32(255, 0, 0, 255);
    const ImU32 HighlightBgU32   = IM_COL32(13, 59, 105, 255);
    const ImU32 HighlightTextU32 = IM_COL32(192, 192, 192, 255);

    ImGui::PushFont(EditorFonts::Consola_16);
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, ImVec4(0.0f / 255.0f, 112.0f / 255.0f, 224.0f / 255.0f, 1.0f));

    if (EditorWidgets::BeginRichTextView("##OutputLogRichText", ImVec2(-1.0f, -1.0f), RichTextCtx, 0, false))
    {
        for (int32 i = 0; i < LocalMessages.Size(); ++i)
        {
            const FLogMessage& Msg = LocalMessages[i];

            if (!IsSeverityMatching(Msg.Severity))
            {
                continue;
            }

            const CHAR* Line = *Msg.Message;
            if (!Line)
            {
                continue;
            }

            if (bHasSearch && !CString::Stristr(Line, Search))
            {
                continue;
            }

            ImU32 LineColor = DefaultTextU32;
            if (Msg.Severity == ELogSeverity::Warning)
            {
                LineColor = WarningTextU32;
            }
            else if (Msg.Severity == ELogSeverity::Error)
            {
                LineColor = ErrorTextU32;
            }

            EditorWidgets::RichTextNewLine(RichTextCtx);

            if (bHasSearch)
            {
                const int32 MatchStart = StringView(Line).Find(Search, EStringCaseType::NoCase);
                const int32 MatchLen   = static_cast<int32>(CString::Strlen(Search));

                if (MatchStart >= 0 && MatchLen > 0)
                {
                    if (MatchStart > 0)
                    {
                        String Prefix;
                        Prefix.Reserve(MatchStart + 1);

                        for (int32 c = 0; c < MatchStart; ++c)
                        {
                            Prefix += Line[c];
                        }

                        EditorWidgets::RichTextAddText(RichTextCtx, *Prefix, LineColor);
                    }

                    String Match;
                    Match.Reserve(MatchLen + 1);

                    for (int32 c = 0; c < MatchLen; ++c)
                    {
                        Match += Line[MatchStart + c];
                    }

                    EditorWidgets::RichTextAddTextBg(RichTextCtx, *Match, HighlightTextU32, HighlightBgU32);

                    const int32 LineLen     = static_cast<int32>(CString::Strlen(Line));
                    const int32 SuffixStart = MatchStart + MatchLen;

                    if (SuffixStart < LineLen)
                    {
                        String Suffix;
                        Suffix.Reserve(LineLen - SuffixStart + 1);
                        for (int32 c = SuffixStart; c < LineLen; ++c)
                        {
                            Suffix += Line[c];
                        }

                        EditorWidgets::RichTextAddText(RichTextCtx, *Suffix, LineColor);
                    }
                }
                else
                {
                    EditorWidgets::RichTextAddText(RichTextCtx, Line, LineColor);
                }
            }
            else
            {
                EditorWidgets::RichTextAddText(RichTextCtx, Line, LineColor);
            }
        }

        if (EditorWidgets::BeginPopupContextWindow("OutputLogContextMenu"))
        {
            EditorWidgets::MenuLabeledSeparator("Output Log");

            if (EditorWidgets::MenuItem("Select All", nullptr, false, true))
            {
                EditorWidgets::RichTextSelectAll(RichTextCtx);
            }

            if (EditorWidgets::MenuItem("Copy", nullptr, false, RichTextCtx.bHasSelection))
            {
                const String Selected = EditorWidgets::GetSelectedRichText(RichTextCtx);
                if (!Selected.IsEmpty())
                {
                    ImGui::SetClipboardText(*Selected);
                }
            }

            if (EditorWidgets::MenuItem("Clear log", nullptr, false, true))
            {
                SCOPED_LOCK(MessagesCS);
                Messages.Clear();
            }
            
            EditorWidgets::EndPopupContext();
        }

        EditorWidgets::EndRichTextView(RichTextCtx);
    }

    ImGui::PopStyleColor();
    ImGui::PopFont();
}
