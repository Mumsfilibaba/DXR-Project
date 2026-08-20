#include "Core/Templates/CString.h"
#include "Core/Misc/FrameProfiler.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Engine/EngineUI/Editor/EditorOutputLogWidget.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "ImGuiPlugin/ImGuiCore.h"

static constexpr int32 MaxLogMessages = 5000;
static constexpr int32 LogTrimBlock   = 512;

static constexpr ImU32 LogWarningTextColor   = IM_COL32(255, 255, 0, 255);
static constexpr ImU32 LogErrorTextColor     = IM_COL32(255, 0, 0, 255);
static constexpr ImU32 LogHighlightTextColor = IM_COL32(192, 192, 192, 255);
static constexpr ImU32 LogHighlightBgColor   = IM_COL32(13, 59, 105, 255);

FEditorOutputLogWidget::FEditorOutputLogWidget()
    : IOutputDevice()
    , SearchFilterBuffer()
    , Messages()
    , MessagesCS()
    , RichTextCtx()
    , ImGuiDelegateHandle()
    , TotalMessagesAdded(0)
    , TotalMessagesRemoved(0)
    , BuiltSearchFilter()
    , BuiltMessagesAdded(0)
    , BuiltMessagesRemoved(0)
    , bBuiltFilterInfo(true)
    , bBuiltFilterWarning(true)
    , bBuiltFilterError(true)
    , bAutoScroll(true)
    , bFocusSearchField(false)
    , bFilterInfo(true)
    , bFilterWarning(true)
    , bFilterError(true)
    , bVisible(true)
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
    BuiltSearchFilter.Fill(0);

    Messages.Reserve(MaxLogMessages + 1);

    RichTextCtx.Padding     = ImVec2(12.0f, 8.0f);
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

    Messages.Emplace(FLogMessage{ Message, Severity });
    ++TotalMessagesAdded;

    if (Messages.Size() > MaxLogMessages)
    {
        const int32 Overflow = Messages.Size() - (MaxLogMessages - LogTrimBlock);
        Messages.RemoveAt(0, Overflow);

        TotalMessagesRemoved += static_cast<uint64>(Overflow);
    }
}

void FEditorOutputLogWidget::Draw()
{
    if (!bVisible)
    {
        return;
    }

    TRACE_SCOPE("Output Log");

    ImGuiStyle& Style = ImGui::GetStyle();

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(Style.ItemSpacing.x, 0.0f));

    if (EditorWidgets::BeginEditorWindow("Output Log", &bVisible))
    {
        // ImGuiMod_Ctrl resolves to Cmd on macOS, and the default focused routing keeps this off the other panels
        if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_F))
        {
            bFocusSearchField = true;
        }

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

        if (ImGui::BeginChild("##OutputLogHeader", ImVec2(ChildWidth, HeaderHeight), ImGuiChildFlags_Border, OutputLogHeaderFlags))
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

        if (ImGui::BeginChild("##OutputLogOuter", ImVec2(ChildWidth, OutputHeight), ImGuiChildFlags_Border, OuterLogFlags))
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

    EditorWidgets::EndEditorWindow();

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

    if (bFocusSearchField)
    {
        ImGui::SetKeyboardFocusHere();
        bFocusSearchField = false;
    }

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

        EditorWidgets::DrawIcon(DrawList, EditorIcons::FilterIcon, LeftIconMin, LeftIconMax, White);

        const float TextX = LeftIconMax.x + IconTextGap;
        const float TextY = Min.y + (ButtonHeight - TextSize.y) * 0.5f;

        DrawList->AddText(EditorFonts::SegoeUI_22, EditorFonts::SegoeUI_22->FontSize, ImVec2(TextX, TextY), White, Label);

        const float  RightIconX   = TextX + TextSize.x + TextArrowGap;
        const float  RightIconY   = Min.y + (ButtonHeight - ArrowIconSize) * 0.5f;
        const ImVec2 RightIconMin = ImVec2(RightIconX, RightIconY);
        const ImVec2 RightIconMax = ImVec2(RightIconMin.x + ArrowIconSize, RightIconMin.y + ArrowIconSize);

        EditorWidgets::DrawIcon(DrawList, EditorIcons::DownArrowIcon, RightIconMin, RightIconMax, White);

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

    FPopupAnchor FilterAnchor;
    FilterAnchor.Min = ImGui::GetItemRectMin();
    FilterAnchor.Max = ImGui::GetItemRectMax();

    EditorWidgets::SetNextBeginPopupPos("LogFilterMenu", FilterAnchor, EPopupPlacement::BelowAnchor);
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

bool FEditorOutputLogWidget::IsSeverityVisible(ELogSeverity Severity) const
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
}

ImU32 FEditorOutputLogWidget::GetSeverityColor(ELogSeverity Severity) const
{
    if (Severity == ELogSeverity::Warning)
    {
        return LogWarningTextColor;
    }
    else if (Severity == ELogSeverity::Error)
    {
        return LogErrorTextColor;
    }

    return ImGui::GetColorU32(ImGuiCol_Text);
}

void FEditorOutputLogWidget::AppendMessageLine(const FLogMessage& Message)
{
    if (!IsSeverityVisible(Message.Severity))
    {
        return;
    }

    const CHAR* Line = *Message.Message;
    if (!Line)
    {
        return;
    }

    const CHAR* Search = SearchFilterBuffer.Data();
    const CHAR* Match  = (Search[0] != 0) ? CString::Stristr(Line, Search) : nullptr;

    if (Search[0] != 0 && !Match)
    {
        return;
    }

    EditorWidgets::RichTextNewLine(RichTextCtx);

    const ImU32 LineColor = GetSeverityColor(Message.Severity);
    if (!Match)
    {
        EditorWidgets::RichTextAddText(RichTextCtx, Line, LineColor);
        return;
    }

    const int32 MatchLength = static_cast<int32>(CString::Strlen(Search));

    if (Match > Line)
    {
        EditorWidgets::RichTextAddText(RichTextCtx, Line, static_cast<int32>(Match - Line), LineColor);
    }

    EditorWidgets::RichTextAddTextBg(RichTextCtx, Match, MatchLength, LogHighlightTextColor, LogHighlightBgColor);

    if (Match[MatchLength] != 0)
    {
        EditorWidgets::RichTextAddText(RichTextCtx, Match + MatchLength, LineColor);
    }
}

void FEditorOutputLogWidget::RebuildRichTextIfDirty()
{
    SCOPED_LOCK(MessagesCS);

    const bool bFilterChanged =
        bBuiltFilterInfo    != bFilterInfo    ||
        bBuiltFilterWarning != bFilterWarning ||
        bBuiltFilterError   != bFilterError   ||
        CString::Strcmp(BuiltSearchFilter.Data(), SearchFilterBuffer.Data()) != 0;

    const bool bRebuildAll = bFilterChanged || (BuiltMessagesRemoved != TotalMessagesRemoved);

    int32 FirstMessage = 0;
    if (bRebuildAll)
    {
        RichTextCtx.Clear();
    }
    else
    {
        FirstMessage = static_cast<int32>(BuiltMessagesAdded - TotalMessagesRemoved);
        if (FirstMessage >= Messages.Size())
        {
            return;
        }
    }

    for (int32 Index = FirstMessage; Index < Messages.Size(); ++Index)
    {
        AppendMessageLine(Messages[Index]);
    }

    BuiltMessagesAdded   = TotalMessagesAdded;
    BuiltMessagesRemoved = TotalMessagesRemoved;
    bBuiltFilterInfo     = bFilterInfo;
    bBuiltFilterWarning  = bFilterWarning;
    bBuiltFilterError    = bFilterError;

    CString::Strncpy(BuiltSearchFilter.Data(), SearchFilterBuffer.Data(), BuiltSearchFilter.Size());
}

void FEditorOutputLogWidget::DrawLogListRichText()
{
    RichTextCtx.bAutoScroll = bAutoScroll;

    ImGui::PushFont(EditorFonts::Consola_16);
    ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, ImVec4(0.0f / 255.0f, 112.0f / 255.0f, 224.0f / 255.0f, 1.0f));

    if (EditorWidgets::BeginRichTextView("##OutputLogRichText", ImVec2(-1.0f, -1.0f), RichTextCtx, 0, false))
    {
        RebuildRichTextIfDirty();

        if (EditorWidgets::BeginPopupContextWindow("OutputLogContextMenu"))
        {
            EditorWidgets::MenuLabeledSeparator("Output Log");

            if (EditorWidgets::MenuItem("Select All", EDITOR_SHORTCUT_MOD "+A", false, true))
            {
                EditorWidgets::RichTextSelectAll(RichTextCtx);
            }

            if (EditorWidgets::MenuItem("Copy", EDITOR_SHORTCUT_MOD "+C", false, RichTextCtx.bHasSelection))
            {
                const String Selected = EditorWidgets::GetSelectedRichText(RichTextCtx);
                if (!Selected.IsEmpty())
                {
                    ImGui::SetClipboardText(*Selected);
                }
            }

            if (EditorWidgets::MenuItem("Copy All", nullptr, false, !RichTextCtx.Lines.IsEmpty()))
            {
                const String All = EditorWidgets::GetAllRichText(RichTextCtx);
                if (!All.IsEmpty())
                {
                    ImGui::SetClipboardText(*All);
                }
            }

            if (EditorWidgets::MenuItem("Find", EDITOR_SHORTCUT_MOD "+F", false, true))
            {
                bFocusSearchField = true;
            }

            if (EditorWidgets::MenuItem("Clear log", nullptr, false, true))
            {
                SCOPED_LOCK(MessagesCS);

                TotalMessagesRemoved += static_cast<uint64>(Messages.Size());
                Messages.Clear();
            }
            
            EditorWidgets::EndPopupContext();
        }

        EditorWidgets::EndRichTextView(RichTextCtx);
    }

    ImGui::PopStyleColor();
    ImGui::PopFont();
}
