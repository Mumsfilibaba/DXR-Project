#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Templates/CString.h"
#include "Application/Application.h"
#include "Engine/EngineUI/Editor/EditorFooterWidget.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "ImGuiPlugin/ImGuiExtensions.h"
#include <imgui.h>

FEditorFooterWidget::FEditorFooterWidget(const TSharedPtr<IOutputDevice>& InOutputDevice)
    : OutputDevice(InOutputDevice)
    , Candidates()
    , SelectedCandidateIndex(InvalidIndex)
    , HistoryIndex(InvalidIndex)
    , LastCursorPosition(0)
    , PendingCursorPosition(0)
    , bCandidateSelectionChanged(false)
    , bRequestCursorPosition(false)
    , bRequestInputFocus(false)
    , bUpdateCursorPosition(false)
    , bScrollToBottom(false)
    , bCandidatesOverlayOpen(false)
{
    if (FApplication::IsInitialized())
    {
        InputHandler = MakeSharedPtr<FConsoleInputHandler>();
        FApplication::Get().RegisterInputHandler(InputHandler);
    }

    TextBuffer.Fill(0);
}

FEditorFooterWidget::~FEditorFooterWidget()
{
    if (FApplication::IsInitialized())
    {
        FApplication::Get().UnregisterInputHandler(InputHandler);
    }
}

void FEditorFooterWidget::Draw()
{
    const ImVec2 FrameBufferScale = ImGuiExtensions::GetDisplayFramebufferScale();

    ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(36, 36, 36, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 6.0f));

    const ImGuiWindowFlags ConsoleWindowFlags =
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_AlwaysAutoResize;

    const ImGuiChildFlags ConsoleChildWindowFlags = ImGuiChildFlags_None;

    ImVec2 InputRectMin = ImVec2(0.0f, 0.0f);
    ImVec2 InputRectMax = ImVec2(0.0f, 0.0f);

    bool bIsInputFieldActive    = false;
    bool bShowCandidatesOverlay = false;

    ImFont* FontToUse = EditorFonts::Consola_16 ? EditorFonts::Consola_16 : ImGui::GetFont();
    ImGui::PushFont(FontToUse);

    if (ImGui::BeginChild("Console", ImVec2(0.0f, GetHeight()), ConsoleChildWindowFlags, ConsoleWindowFlags))
    {
        const ImGuiInputTextFlags ConsoleInputFlags =
            ImGuiInputTextFlags_EnterReturnsTrue |
            ImGuiInputTextFlags_CallbackCompletion |
            ImGuiInputTextFlags_CallbackHistory |
            ImGuiInputTextFlags_CallbackAlways |
            ImGuiInputTextFlags_CallbackEdit;

        const auto InputCallback = [](ImGuiInputTextCallbackData* Data)
        {
            return reinterpret_cast<FEditorFooterWidget*>(Data->UserData)->InputTextCallback(Data);
        };

        ImGui::SameLine();

        const float InputFieldWidth = 512.0f;
        ImGui::SetNextItemWidth(InputFieldWidth);

        const bool bWantsInputFocus = bRequestInputFocus;
        if (bRequestInputFocus)
        {
            ImGui::SetKeyboardFocusHere();
            bRequestInputFocus = false;
        }

        const ImVec2 BasePadding   = EditorStyleVars::InputFieldFramePadding;
        const float  InputRounding = 4.0f;

        const ImU32  BgColor            = IM_COL32(15, 15, 15, 255);
        const ImU32  BorderColorNormal  = IM_COL32(51, 51, 51, 255);
        const ImU32  BorderColorHovered = IM_COL32(74, 74, 74, 255);
        const ImU32  BorderColorActive  = IM_COL32(9, 92, 176, 255);
        const ImVec4 HintTextInactive   = ImVec4(77.0f / 255.0f, 77.0f / 255.0f, 77.0f / 255.0f, 1.0f);
        const ImVec4 HintTextActive     = ImVec4(99.0f / 255.0f, 99.0f / 255.0f, 99.0f / 255.0f, 1.0f);
        const ImVec4 InputTextColor     = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

        const ImVec2 InputStart = ImGui::GetCursorScreenPos();
        const float  InputHeight = ImGui::GetFontSize() + BasePadding.y * 2.0f;
        const ImVec2 InputEnd    = ImVec2(InputStart.x + InputFieldWidth, InputStart.y + InputHeight);

        const ImGuiID ConsoleInputId  = ImGui::GetID("##ConsoleInput");
        const bool    bInputWasActive = ImGui::GetActiveID() == ConsoleInputId;
        const bool    bInputClicked   = ImGui::IsMouseHoveringRect(InputStart, InputEnd, true) && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        const bool    bUseActiveHint  = bInputWasActive || bWantsInputFocus || bInputClicked;
        const ImVec4  HintTextColor   = bUseActiveHint ? HintTextActive : HintTextInactive;

        ImDrawList* DrawList = ImGui::GetWindowDrawList();
        DrawList->AddRectFilled(InputStart, InputEnd, BgColor, InputRounding);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, BasePadding);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, InputTextColor);
        ImGui::PushStyleColor(ImGuiCol_TextDisabled, HintTextColor);
        ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, EditorStyleVars::InputFieldSelectionColor);

        const bool bDidEnterInput = ImGui::InputTextWithHint("##ConsoleInput", "Console Input", TextBuffer.Data(), TextBuffer.Size(), ConsoleInputFlags, InputCallback, this);

        ImGui::PopStyleColor(6);
        ImGui::PopStyleVar(3);

        InputRectMin = ImGui::GetItemRectMin();
        InputRectMax = ImGui::GetItemRectMax();
        
        bIsInputFieldActive = ImGui::IsItemActive();

        {
            const bool bActive  = ImGui::IsItemActive();
            const bool bHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

            const ImU32 BorderColor = bActive ? BorderColorActive : (bHovered ? BorderColorHovered : BorderColorNormal);
            DrawList->AddRect(InputRectMin, InputRectMax, BorderColor, InputRounding, ImDrawListFlags_AntiAliasedLines, EditorStyleVars::InputFieldBorderThickness);
        }

        if (InputHandler)
        {
            InputHandler->bConsoleToggled = bIsInputFieldActive;
        }

        bShowCandidatesOverlay = !Candidates.IsEmpty();

        if (bDidEnterInput)
        {
            if (TextBuffer[0] != 0)
            {
                if (SelectedCandidateIndex >= 0 && !Candidates.IsEmpty())
                {
                    const FString& NewText = Candidates[SelectedCandidateIndex].Second;
                    FCString::Strncpy(TextBuffer.Data(), *NewText, Math::Min(TextBuffer.Size(), NewText.Size()));
                    bUpdateCursorPosition = true;
                }
                else
                {
                    const FString Command(TextBuffer.Data());
                    FConsoleManager::Get().ExecuteCommand(*OutputDevice, Command);

                    TextBuffer[0]   = 0;
                    bScrollToBottom = true;
                }
            }

            InvalidateCandidates();
        }
    }

    ImGui::EndChild();

    ImGui::PopStyleVar(); // WindowPadding
    ImGui::PopStyleColor(); // ChildBg

    // -------------------------------------------------------------------------------------------
    // Candidates overlay window
    // -------------------------------------------------------------------------------------------

    if (!bShowCandidatesOverlay)
    {
        bCandidatesOverlayOpen = false;
    }

    if (bIsInputFieldActive && bShowCandidatesOverlay)
    {
        bCandidatesOverlayOpen = true;
    }

    bool bMouseInsideOverlayRect = false;

    float OverlayMaxNameWidth = 0.0f;
    ImVec2 OverlayWindowSize  = ImVec2(0.0f, 0.0f);
    ImVec2 OverlayWindowPos   = ImVec2(0.0f, 0.0f);

    if (bShowCandidatesOverlay)
    {
        const ImGuiStyle& Style = ImGui::GetStyle();

        const float  RowHeight      = 20.0f;
        const int32  MaxVisibleRows = 20;
        const float  PanelOffsetY   = 4.0f;
        const float  Scale          = FrameBufferScale.x;
        const float  TotalHeight    = RowHeight * MaxVisibleRows;
        const ImVec2 WindowPadding  = ImVec2(10.0f * Scale, 4.0f * Scale);

        Candidates.Foreach([&](const TPair<IConsoleObject*, FString>& Candidate)
        {
            OverlayMaxNameWidth = Math::Max(OverlayMaxNameWidth, ImGui::CalcTextSize(*Candidate.Second).x);
        });

        const float  ReservedScrollbarWidth = Style.ScrollbarSize;
        const float  ExtraRightPadding      = 6.0f * Scale;
        const float  TotalWidth             = OverlayMaxNameWidth + (WindowPadding.x * 2.0f) + ReservedScrollbarWidth + ExtraRightPadding;

        OverlayWindowSize = ImVec2(TotalWidth, TotalHeight);
        OverlayWindowPos  = ImVec2(InputRectMin.x, InputRectMin.y - PanelOffsetY);

        const ImVec2 OverlayMin = ImVec2(OverlayWindowPos.x, OverlayWindowPos.y - OverlayWindowSize.y);
        const ImVec2 OverlayMax = ImVec2(OverlayWindowPos.x + OverlayWindowSize.x, OverlayWindowPos.y);

        bMouseInsideOverlayRect = ImGui::IsMouseHoveringRect(OverlayMin, OverlayMax, false);

        if (bCandidatesOverlayOpen && bMouseInsideOverlayRect && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            bCandidatesOverlayOpen = true;
        }

        const bool bMouseInsideInput = ImGui::IsMouseHoveringRect(InputRectMin, InputRectMax, false);
        if (!bIsInputFieldActive && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !bMouseInsideOverlayRect && !bMouseInsideInput)
        {
            bCandidatesOverlayOpen = false;
        }
    }

    if (bShowCandidatesOverlay && (bIsInputFieldActive || bCandidatesOverlayOpen))
    {
        const ImGuiStyle& Style = ImGui::GetStyle();

        const float  RowHeight      = 20.0f;
        const int32  MaxVisibleRows = 20;
        const float  PanelOffsetY   = 4.0f;
        const float  Scale          = FrameBufferScale.x;
        const float  TotalHeight    = RowHeight * MaxVisibleRows;
        const ImVec2 WindowPadding  = ImVec2(10.0f * Scale, 4.0f * Scale);

        const ImVec4 WindowBgColor             = ImVec4(26.0f / 255.0f, 26.0f / 255.0f, 26.0f / 255.0f, 1.0f);
        const ImVec4 BorderColor               = ImVec4(56.0f / 255.0f, 56.0f / 255.0f, 56.0f / 255.0f, 1.0f);
        const ImVec4 SelectedBgColor           = ImVec4(64.0f / 255.0f, 87.0f / 255.0f, 111.0f / 255.0f, 1.0f);
        const ImVec4 ScrollbarGrabColor        = ImVec4(87.0f / 255.0f, 87.0f / 255.0f, 87.0f / 255.0f, 1.0f);
        const ImVec4 ScrollbarGrabHoveredColor = ImVec4(128.0f / 255.0f, 128.0f / 255.0f, 128.0f / 255.0f, 1.0f);
        const ImVec4 TextSelectedColor         = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        const ImVec4 TextNormalColor           = ImVec4(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f);
        const ImU32  HighlightBgU32            = IM_COL32(139, 194, 74, 255);
        const ImU32  HighlightTextU32          = IM_COL32(0, 0, 0, 255);

        float MaxNameWidth = 0.0f;
        Candidates.Foreach([&](const TPair<IConsoleObject*, FString>& Candidate)
        {
            MaxNameWidth = Math::Max(MaxNameWidth, ImGui::CalcTextSize(*Candidate.Second).x);
        });

        const float  ReservedScrollbarWidth = Style.ScrollbarSize;
        const float  ExtraRightPadding      = 6.0f * Scale;
        const float  TotalWidth             = MaxNameWidth + (WindowPadding.x * 2.0f) + ReservedScrollbarWidth + ExtraRightPadding;
        const ImVec2 WindowSize             = ImVec2(TotalWidth, TotalHeight);
        const ImVec2 WindowPosition         = ImVec2(InputRectMin.x, InputRectMin.y - PanelOffsetY);

        ImGui::PushStyleColor(ImGuiCol_ResizeGrip, 0);
        ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, 0);
        ImGui::PushStyleColor(ImGuiCol_ResizeGripActive, 0);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, WindowBgColor);
        ImGui::PushStyleColor(ImGuiCol_Border, BorderColor);
        ImGui::PushStyleColor(ImGuiCol_Header, SelectedBgColor);
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, SelectedBgColor);
        ImGui::PushStyleColor(ImGuiCol_HeaderActive, SelectedBgColor);
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, WindowBgColor);
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ScrollbarGrabColor);
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ScrollbarGrabHoveredColor);
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ScrollbarGrabHoveredColor);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, WindowPadding);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 999.0f);

        ImGui::SetNextWindowSize(WindowSize, ImGuiCond_Always);
        ImGui::SetNextWindowPos(WindowPosition, ImGuiCond_Always, ImVec2(0.0f, 1.0f));

        const ImGuiWindowFlags OverlayFlags =
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoNavFocus;

        if (ImGui::Begin("ConsoleCandidates", nullptr, OverlayFlags))
        {
            ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(0.0f, 0.5f));

            const bool bHasVerticalScrollbar = ImGui::GetScrollMaxY() > 0.0f;

            const auto DrawCandidateTooltip = [&](const TPair<IConsoleObject*, FString>& Candidate, const ImRect& InItemRect)
            {
                const ImVec4 TooltipBg        = ImVec4(56.0f / 255.0f, 56.0f / 255.0f, 56.0f / 255.0f, 1.0f);
                const ImVec4 TooltipBorder    = ImVec4(71.0f / 255.0f, 71.0f / 255.0f, 71.0f / 255.0f, 1.0f);
                const ImVec4 TooltipTextWhite = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                const ImVec4 TooltipTextGrey  = ImVec4(192.0f / 255.0f, 192.0f / 255.0f, 192.0f / 255.0f, 1.0f);

                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 2.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 2.0f);

                ImGui::PushStyleColor(ImGuiCol_PopupBg, TooltipBg);
                ImGui::PushStyleColor(ImGuiCol_Border, TooltipBorder);
                ImGui::PushStyleColor(ImGuiCol_Separator, TooltipBorder);

                const float TooltipOffsetX  = (10.0f * Scale) + (bHasVerticalScrollbar ? Style.ScrollbarSize : 0.0f);
                const float TooltipMaxWidth = 420.0f * Scale;

                ImGui::SetNextWindowPos(ImVec2(InItemRect.Max.x + TooltipOffsetX, InItemRect.Min.y), ImGuiCond_Always);
                ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f), ImVec2(TooltipMaxWidth, FLT_MAX));

                ImGui::BeginTooltip();

                ImGui::PushStyleColor(ImGuiCol_Text, TooltipTextWhite);
                ImGui::TextUnformatted(*Candidate.Second);
                ImGui::PopStyleColor();

                ImGui::Dummy(ImVec2(0.0f, 4.0f * Scale));

                ImGui::PushStyleColor(ImGuiCol_Text, TooltipTextGrey);

                if (IConsoleVariable* Var = Candidate.First->AsVariable())
                {
                    const CHAR* TypeText = "Variable";
                    if (Var->IsVariableBool())
                    {
                        TypeText = "Bool";
                    }
                    else if (Var->IsVariableInt())
                    {
                        TypeText = "Int";
                    }
                    else if (Var->IsVariableFloat())
                    {
                        TypeText = "Float";
                    }
                    else if (Var->IsVariableString())
                    {
                        TypeText = "String";
                    }

                    const FString ValueString = Var->GetString();

                    ImGui::Text("Type: %s", TypeText);
                    ImGui::Text("Value: %s", *ValueString);

                    const EConsoleVariableFlags VariableFlags = static_cast<EConsoleVariableFlags>(Var->GetFlags() & EConsoleVariableFlags::SetByMask);
                    ImGui::Text("Set By: %s", SetByFlagToString(VariableFlags));
                }
                else if (Candidate.First->AsCommand())
                {
                    ImGui::TextUnformatted("Type: Command");
                }

                const CHAR* HelpString = Candidate.First->GetHelpString();
                if (HelpString && HelpString[0] != 0)
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, TooltipTextWhite);

                    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + TooltipMaxWidth - 20.0f * Scale);
                    ImGui::TextUnformatted(HelpString);
                    ImGui::PopTextWrapPos();

                    ImGui::PopStyleColor();
                }

                ImGui::PopStyleColor();

                ImGui::EndTooltip();

                ImGui::PopStyleColor(3);
                ImGui::PopStyleVar(4);
            };

            const float ContentWidth = ImGui::GetContentRegionAvail().x;
            int32 ClickedCandidateIndex = InvalidIndex;
            for (int32 CandidateIndex = 0; CandidateIndex < Candidates.Size(); ++CandidateIndex)
            {
                const TPair<IConsoleObject*, FString>& Candidate = Candidates[CandidateIndex];
                const bool bIsActiveIndex = (SelectedCandidateIndex == CandidateIndex);

                ImGui::PushID(CandidateIndex);

                const ImVec2 SelectableSize(ContentWidth, RowHeight);
                const bool bCandidateClicked = ImGui::Selectable("##CandidateSelectable", bIsActiveIndex, ImGuiSelectableFlags_None, SelectableSize);
                if (bCandidateClicked)
                {
                    ClickedCandidateIndex = CandidateIndex;
                }

                const bool bIsSelectableVisible = ImGuiExtensions::IsItemFullyVisible();

                const ImRect ItemRect    = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
                const ImVec2 TextStart   = ImVec2(ItemRect.Min.x + 6.0f * Scale, ItemRect.Min.y + (RowHeight - ImGui::GetTextLineHeight()) * 0.5f);
                const ImU32  BaseTextU32 = ImGui::GetColorU32(bIsActiveIndex ? TextSelectedColor : TextNormalColor);

                const CHAR* NameText   = *Candidate.Second;
                const CHAR* FilterText = CandidateFilter.IsEmpty() ? nullptr : *CandidateFilter;

                int32 MatchStart = -1;
                int32 MatchLen   = 0;

                if (FilterText && FilterText[0] != 0)
                {
                    MatchStart = FStringView(NameText).Find(FilterText, EStringCaseType::NoCase);
                    MatchLen   = static_cast<int32>(FCString::Strlen(FilterText));
                }

                ImDrawList* DrawList = ImGui::GetWindowDrawList();
                if (MatchStart >= 0 && MatchLen > 0)
                {
                    const ImVec2 PrefixSize = ImGui::CalcTextSize(NameText, NameText + MatchStart);
                    const ImVec2 MatchSize  = ImGui::CalcTextSize(NameText + MatchStart, NameText + MatchStart + MatchLen);
                    const ImVec2 PrefixPos  = TextStart;
                    const ImVec2 MatchPos   = ImVec2(TextStart.x + PrefixSize.x, TextStart.y);
                    const ImVec2 SuffixPos  = ImVec2(MatchPos.x + MatchSize.x, TextStart.y);

                    DrawList->AddText(PrefixPos, BaseTextU32, NameText, NameText + MatchStart);

                    const float  HighlightPadY = 2.0f * Scale;
                    const ImVec2 HighlightMin  = ImVec2(MatchPos.x - 1.0f * Scale, ItemRect.Min.y + HighlightPadY);
                    const ImVec2 HighlightMax  = ImVec2(MatchPos.x + MatchSize.x + 1.0f * Scale, ItemRect.Max.y - HighlightPadY);
                    DrawList->AddRectFilled(HighlightMin, HighlightMax, HighlightBgU32, 0.0f);

                    DrawList->AddText(MatchPos, HighlightTextU32, NameText + MatchStart, NameText + MatchStart + MatchLen);
                    DrawList->AddText(SuffixPos, BaseTextU32, NameText + MatchStart + MatchLen);
                }
                else
                {
                    DrawList->AddText(TextStart, BaseTextU32, NameText);
                }

                const bool bHoveredByMouse = ImGui::IsMouseHoveringRect(ItemRect.Min, ItemRect.Max, false);
                if (bHoveredByMouse)
                {
                    DrawCandidateTooltip(Candidate, ItemRect);
                }

                if (bIsActiveIndex && bCandidateSelectionChanged)
                {
                    if (!bIsSelectableVisible)
                    {
                        ImGui::SetScrollHereY(0.0f);
                    }

                    bCandidateSelectionChanged = false;
                }

                ImGui::PopID();
            }

            if (ClickedCandidateIndex != InvalidIndex)
            {
                ApplyCandidateToBuffer(ClickedCandidateIndex);
                bRequestInputFocus = true;
                InvalidateCandidates();
            }

            ImGui::PopStyleVar(); // SelectableTextAlign
        }

        ImGui::End(); // Console Candidates

        ImGui::PopStyleVar(5);
        ImGui::PopStyleColor(12);
    }

    ImGui::PopFont();
}

void FEditorFooterWidget::ApplyCandidateToBuffer(int32 CandidateIndex)
{
    if (!Candidates.IsValidIndex(CandidateIndex))
    {
        return;
    }

    const CHAR* Buffer = TextBuffer.Data();
    const int32 BufferLength = FCString::Strlen(Buffer);

    int32 CursorPos = LastCursorPosition;
    if (CursorPos < 0)
    {
        CursorPos = 0;
    }
    else if (CursorPos > BufferLength)
    {
        CursorPos = BufferLength;
    }

    const CHAR* WordEnd   = Buffer + CursorPos;
    const CHAR* WordStart = WordEnd;

    while (WordStart > Buffer)
    {
        const CHAR CurrentChar = WordStart[-1];
        if (CurrentChar == ' ' || CurrentChar == '\t' || CurrentChar == ',' || CurrentChar == ';')
        {
            break;
        }

        WordStart--;
    }

    const int32 WordLength = static_cast<int32>(WordEnd - WordStart);
    if (WordLength <= 0)
    {
        return;
    }

    const FString Prefix(Buffer, static_cast<int32>(WordStart - Buffer));
    const FString Suffix(WordEnd);

    const FString& CandidateText = Candidates[CandidateIndex].Second;
    const FString NewBuffer = Prefix + CandidateText + Suffix;

    const int32 CopyLen = Math::Min(TextBuffer.Size(), NewBuffer.Size());
    FCString::Strncpy(TextBuffer.Data(), *NewBuffer, CopyLen);
    TextBuffer[TextBuffer.Size() - 1] = 0;

    PendingCursorPosition = Prefix.Size() + CandidateText.Size();
    bRequestCursorPosition = true;
}

void FEditorFooterWidget::InvalidateCandidates()
{
    SelectedCandidateIndex     = InvalidIndex;
    bCandidateSelectionChanged = true;

    Candidates.Clear();
    CandidateFilter.Clear();
}

int32 FEditorFooterWidget::InputTextCallback(ImGuiInputTextCallbackData* CallbackData)
{
    if (bRequestCursorPosition)
    {
        CallbackData->CursorPos = PendingCursorPosition;
        if (CallbackData->CursorPos < 0)
        {
            CallbackData->CursorPos = 0;
        }
        else if (CallbackData->CursorPos > CallbackData->BufTextLen)
        {
            CallbackData->CursorPos = CallbackData->BufTextLen;
        }

        bRequestCursorPosition = false;
    }
    else if (bUpdateCursorPosition)
    {
        CallbackData->CursorPos = CallbackData->BufTextLen;
        bUpdateCursorPosition = false;
    }

    switch (CallbackData->EventFlag)
    {
        // This callback is called whenever we edit the text in the InputText field
        case ImGuiInputTextFlags_CallbackEdit:
        {
            const CHAR* WordEnd   = CallbackData->Buf + CallbackData->CursorPos;
            const CHAR* WordStart = WordEnd;

            while (WordStart > CallbackData->Buf)
            {
                const CHAR CurrentChar = WordStart[-1];
                if (CurrentChar == ' ' || CurrentChar == '\t' || CurrentChar == ',' || CurrentChar == ';')
                {
                    break;
                }

                WordStart--;
            }

            // When we edit we want to search for new candidates, we do this by "reseting" candidate index and array
            InvalidateCandidates();

            const int32 WordLength = static_cast<int32>(WordEnd - WordStart);
            if (WordLength > 0)
            {
                const FStringView CandidateName(WordStart, WordLength);
                CandidateFilter = FString(WordStart, WordLength);
                FConsoleManager::Get().FindCandidates(CandidateName, Candidates);

                // If we found any candidates, then want to reset the history index, otherwise the index will be the 
                // the same if we erase the text in the input-field and start iterating through the history.
                if (!Candidates.IsEmpty())
                {
                    HistoryIndex = InvalidIndex;
                }
            }

            break;
        }

        // This callback is called when we press TAB when the console InputText field has keyboard- focus
        case ImGuiInputTextFlags_CallbackCompletion:
        {
            const CHAR* WordEnd   = CallbackData->Buf + CallbackData->CursorPos;
            const CHAR* WordStart = WordEnd;

            if (CallbackData->BufTextLen > 0)
            {
                while (WordStart > CallbackData->Buf)
                {
                    const CHAR CurrentChar = WordStart[-1];
                    if (CurrentChar == ' ' || CurrentChar == '\t' || CurrentChar == ',' || CurrentChar == ';')
                    {
                        break;
                    }

                    WordStart--;
                }
            }

            const int32 WordLength = static_cast<int32>(WordEnd - WordStart);
            if (WordLength > 0)
            {
                // We might not have a selection-index if we have not pressed the up- or down-key
                if (!Candidates.IsEmpty() && SelectedCandidateIndex > InvalidIndex)
                {
                    const int32 Pos   = static_cast<int32>(WordStart - CallbackData->Buf);
                    const int32 Count = WordLength;

                    const FString& NewTextData = Candidates[SelectedCandidateIndex].Second;
                    CallbackData->DeleteChars(Pos, Count);
                    CallbackData->InsertChars(CallbackData->CursorPos, *NewTextData);

                    // If we used tried to complete the command and there actually was text, then we want to clear the candidate array
                    InvalidateCandidates();

                    // TODO: Do we need to communicate with the log to scroll here?
                }
            }

            break;
        }

        // We have two options when this callback is called (I.e when the user presses either up or down arrows on the keyboard)
        //  1. We want to select one of the options that fits what we have type (auto-completion)
        //  2. We want to use a previous command
        case ImGuiInputTextFlags_CallbackHistory:
        {
            // If the candidates are empty, that means that we have not types anything into the input line
            // so this will route the arrow-keys to flip through the history (The commands previously used)
            if (Candidates.IsEmpty())
            {
                // If we have no candidates we should have and invalid candidate-index
                CHECK(SelectedCandidateIndex == InvalidIndex);

                const int32 PrevHistoryIndex = HistoryIndex;

                const TArray<FString>& History = FConsoleManager::Get().GetHistory();
                if (!History.IsEmpty())
                {
                    // If we have any console-history then we can go through the history by pressing the down-arrow key
                    if (CallbackData->EventKey == ImGuiKey_UpArrow)
                    {
                        // If we have a history-index (non-invalid index), this means that we have already started to go through the 
                        // console history and we can just decrement the index.
                        if (HistoryIndex != InvalidIndex)
                        {
                            HistoryIndex--;

                            // If we try and go further than we have indices, then we clamp the index to zero
                            if (HistoryIndex < 0)
                            {
                                HistoryIndex = 0;
                            }
                        }
                        else
                        {
                            // If we currently have not pressed the history, then we set the index to the last history entry
                            HistoryIndex = History.LastElementIndex();
                        }
                    }
                    else if (CallbackData->EventKey == ImGuiKey_DownArrow)
                    {
                        // if the up arrow is pressed, then we need to have already pressed the down-arrow,
                        // otherwise we do not do anything.

                        if (HistoryIndex != InvalidIndex)
                        {
                            HistoryIndex++;

                            // If we try and go beyond the history array then we "exit" going through the history
                            if (HistoryIndex >= History.Size())
                            {
                                HistoryIndex = InvalidIndex;
                            }
                        }
                    }
                }
                else
                {
                    // If we have no history, then we set the index to invalid
                    HistoryIndex = InvalidIndex;
                }

                // If the index changed then we insert the new history text into the text-field
                if (PrevHistoryIndex != HistoryIndex)
                {
                    // If the history-index is invalid, then we clear the input-text field
                    const CHAR* HistoryStr = (HistoryIndex >= 0) ? *History[HistoryIndex] : "";
                    CallbackData->DeleteChars(0, CallbackData->BufTextLen);
                    CallbackData->InsertChars(0, HistoryStr);
                }
            }
            else
            {
                // If we have candidates we should have an invalid history-index
                CHECK(HistoryIndex == InvalidIndex);

                const int32 PreviousSelectedCandidateIndex = SelectedCandidateIndex;
                if (CallbackData->EventKey == ImGuiKey_UpArrow)
                {
                    if (SelectedCandidateIndex <= 0)
                    {
                        SelectedCandidateIndex = Candidates.LastElementIndex();
                    }
                    else
                    {
                        SelectedCandidateIndex--;
                    }
                }
                else if (CallbackData->EventKey == ImGuiKey_DownArrow)
                {
                    if (SelectedCandidateIndex >= Candidates.LastElementIndex())
                    {
                        SelectedCandidateIndex = 0;
                    }
                    else
                    {
                        SelectedCandidateIndex++;
                    }
                }

                if (PreviousSelectedCandidateIndex != SelectedCandidateIndex)
                {
                    bCandidateSelectionChanged = true;
                }
            }

            break;
        }
    }

    LastCursorPosition = CallbackData->CursorPos;

    return 0;
}
