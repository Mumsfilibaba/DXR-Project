#include "Engine/EngineUI/Editor/EditorLogOutputWidget.h"
#include "Engine/EngineUI/Editor/EditorHelpers.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Templates/CString.h"
#include <imgui.h>

FEditorLogOutputWidget::FEditorLogOutputWidget()
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
        ImGuiDelegateHandle = IImguiPlugin::Get().AddDelegate(FImGuiDelegate::CreateRaw(this, &FEditorLogOutputWidget::Draw));
    }

    SearchFilterBuffer.Fill(0);
}

FEditorLogOutputWidget::~FEditorLogOutputWidget()
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

void FEditorLogOutputWidget::Log(const FString& Message)
{
    Log(ELogSeverity::Info, Message);
}

void FEditorLogOutputWidget::Log(ELogSeverity Severity, const FString& Message)
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

void FEditorLogOutputWidget::DrawFilterBar()
{
	ImGuiStyle& Style = ImGui::GetStyle();

	// -------------------------------------------------------------------------------------------
	// Search Field
	// -------------------------------------------------------------------------------------------

	const char*  FilterButtonLabel    = "Filters";
	const ImVec2 FilterButtonTextSize = ImGui::CalcTextSize(FilterButtonLabel);
	const float  FilterButtonWidth    = FilterButtonTextSize.x + Style.FramePadding.x * 2.0f;

	// Add some spacing before the input field
	ImGui::Dummy(ImVec2(1.0f, 0.0f));
	ImGui::SameLine();

	const float InputFieldWidth = 512.0f;
	ImGui::SetNextItemWidth(InputFieldWidth);

	{
		const ImVec2 BasePadding = EditorStyleVars::InputFieldFramePadding;
		const float  FrameHeight = ImGui::GetFontSize() + BasePadding.y * 2.0f;
		const float  IconGapPx   = 6.0f;
		const float  IconSizePx  = 16.0f;
		const float  PaddedX     = BasePadding.x + IconSizePx + IconGapPx;

		// Reserve room for the icon by increasing left padding.
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(PaddedX, BasePadding.y));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, EditorStyleVars::InputFieldBorderRounding);

		ImGui::InputTextWithHint("##LogSearch", "Search Log", SearchFilterBuffer.Data(), SearchFilterBuffer.Size());

		// Draw search icon inside the input, before the hint/text.
		if (EditorIcons::SearchIcon)
		{
			const ImVec2 ItemMin = ImGui::GetItemRectMin();
			const ImVec2 ItemMax = ImGui::GetItemRectMax();
			const float  ItemH   = ItemMax.y - ItemMin.y;
			const ImVec2 IconMin = ImVec2(ItemMin.x + BasePadding.x, ItemMin.y + (ItemH - IconSizePx) * 0.5f);
			const ImVec2 IconMax = ImVec2(IconMin.x + IconSizePx, IconMin.y + IconSizePx);
			const ImU32  Tint    = IM_COL32(255, 255, 255, 255);

			ImDrawList* DrawList = ImGui::GetWindowDrawList();
			DrawList->AddImage(EditorIcons::SearchIcon, IconMin, IconMax, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f), Tint);
		}

		ImGui::PopStyleVar(2);
	}

	// Cache input rect in absolute screen coords, this is later used to draw the candidate window
	const ImVec2 ItemMin = ImGui::GetItemRectMin();
	const ImVec2 ItemMax = ImGui::GetItemRectMax();

	// Always draw a border around the search bar with state colors:
	// Normal:  RGB(51,51,51)
	// Hovered: RGB(74,74,74)
	// Active:  RGB(9,92,176)
	{
		const bool bActive  = ImGui::IsItemActive();
		const bool bHovered = ImGui::IsItemHovered();

		const ImU32 BorderColorNormal  = IM_COL32(51, 51, 51, 255);
		const ImU32 BorderColorHovered = IM_COL32(74, 74, 74, 255);
		const ImU32 BorderColorActive  = IM_COL32(9, 92, 176, 255);
		const ImU32 BorderColor        = bActive ? BorderColorActive : (bHovered ? BorderColorHovered : BorderColorNormal);

		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		DrawList->AddRect(
			ItemMin,
			ItemMax,
			BorderColor,
			EditorStyleVars::InputFieldBorderRounding,
			0,
			EditorStyleVars::InputFieldBorderThickness);
	}

	// -------------------------------------------------------------------------------------------
	// Filters Button
	// -------------------------------------------------------------------------------------------

	ImGui::SameLine();

	const bool bIsFiltering = !bFilterInfo || !bFilterWarning || !bFilterError;
	if (bIsFiltering)
	{
		const ImVec4 Active = Style.Colors[ImGuiCol_ButtonActive];
		ImGui::PushStyleColor(ImGuiCol_Button, Active);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Active);
	}

	if (ImGui::Button(FilterButtonLabel, ImVec2(FilterButtonWidth, 0.0f)))
	{
		ImGui::OpenPopup("LogFilterMenu");
	}

	const ImVec2 ButtonMin = ImGui::GetItemRectMin();
	const ImVec2 ButtonMax = ImGui::GetItemRectMax();

	ImGui::SetNextWindowPos(ImVec2(ButtonMin.x, ButtonMax.y), ImGuiCond_Always);

	const float PopupWidth = 128.0f;
	ImGui::SetNextWindowSize(ImVec2(PopupWidth, 0.0f), ImGuiCond_Appearing);

	const ImVec4 PopupColor = Style.Colors[ImGuiCol_Button];
	ImGui::PushStyleColor(ImGuiCol_PopupBg, PopupColor);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 8.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(Style.FramePadding.x * 1.6f, Style.FramePadding.y * 1.6f));
	ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0.0f);

	if (ImGui::BeginPopup("LogFilterMenu"))
	{
		ImVec2 Cursor = ImGui::GetCursorPos();
		ImGui::Dummy(ImVec2(PopupWidth, 0.0f));
		ImGui::SetCursorPos(Cursor);

		ImGui::TextUnformatted("Verbosity");
		ImGui::Separator();

		ImGui::MenuItem("Messages", nullptr, &bFilterInfo);
		ImGui::MenuItem("Warnings", nullptr, &bFilterWarning);
		ImGui::MenuItem("Errors", nullptr, &bFilterError);

		ImGui::EndPopup();
	}

	ImGui::PopStyleVar(3);
	ImGui::PopStyleColor();

	if (bIsFiltering)
	{
		ImGui::PopStyleColor(2);
	}
}

void FEditorLogOutputWidget::DrawLogList()
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
    
	const float PaddingX = 8.0f;
	const float PaddingY = 4.0f;
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + PaddingY);

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

		ImGui::PushFont(EditorFonts::Consola_14);
        ImGui::TextColored(TextColor, "%s", *Message.Message);
		ImGui::PopFont();

        ImGui::Unindent(PaddingX);
    }

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + PaddingY);

    if (bScrollToBottom)
    {
        ImGui::SetScrollHereY(1.0f);
        bScrollToBottom = false;
    }
}

void FEditorLogOutputWidget::Draw()
{
    if (!bVisible)
    {
        return;
    }
 
    ImGuiStyle& Style = ImGui::GetStyle();
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));

    const ImGuiWindowFlags OutputLogFlags = ImGuiWindowFlags_NoCollapse;
    if (ImGui::Begin("Output Log", &bVisible, OutputLogFlags))
    {
        DrawFilterBar();

        const ImVec4 LogBackground = Style.Colors[ImGuiCol_ChildBg];
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.0f, 2.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.0f);
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, LogBackground);

        const float FooterReserve = 0.0f;
        if (ImGui::BeginChild("##OutputLogScroll", ImVec2(0, -FooterReserve), true, ImGuiWindowFlags_HorizontalScrollbar))
        {
            DrawLogList();
        }
    
        ImGui::EndChild(); // Log Child Window (Text Area)

        ImGui::PopStyleColor();
		ImGui::PopStyleVar();
		ImGui::PopStyleVar();
    }

    ImGui::End(); // Output Log Window

    ImGui::PopStyleVar();
}
