#include "Console.h"

#include "Rendering/DebugUI.h"

#include "Main/EngineLoop.h"

/*
* Globals
*/

Bool GlobalIsConsoleActive = false;

/*
* Console
*/

void Console::Init()
{
	GlobalEventDispatcher->RegisterEventHandler(this);
}

void Console::Tick()
{
	if (IsActive)
	{
		DebugUI::DrawUI([]()
		{
			const UInt32 WindowWidth	= GlobalMainWindow->GetWidth();
			const UInt32 WindowHeight	= GlobalMainWindow->GetHeight();
			const Float Width			= WindowWidth;
			const Float Height			= WindowHeight * 0.15f;

			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.3f, 0.3f, 0.3f, 0.8f));
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ResizeGrip, 0);
			ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, 0);
			ImGui::PushStyleColor(ImGuiCol_ResizeGripActive, 0);

			ImGui::SetNextWindowPos(
				ImVec2(0.0f, 0.0f),
				ImGuiCond_Always,
				ImVec2(0.0f, 0.0f));

			ImGui::SetNextWindowSize(
				ImVec2(Width, 0.0f),
				ImGuiCond_FirstUseEver);

			ImGui::SetNextWindowSizeConstraints(
				ImVec2(Width, 140),
				ImVec2(Width, WindowHeight * 0.5f));

			ImGui::Begin(
				"Console Window",
				nullptr,
				ImGuiWindowFlags_NoMove				|
				ImGuiWindowFlags_NoTitleBar			|
				ImGuiWindowFlags_NoScrollbar		|
				ImGuiWindowFlags_NoCollapse			|
				ImGuiWindowFlags_NoScrollWithMouse	|
				ImGuiWindowFlags_NoSavedSettings);

			ImGui::Text("Console:");

			ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.3f, 0.3f, 0.3f, 0.6f));

			const ImVec2 ParentSize			= ImGui::GetWindowSize();
			const Float TextWindowWidth		= Width * 0.985f;
			const Float TextWindowHeight	= ParentSize.y - 55.0f;
			ImGui::BeginChild(
				"##TextWindow",
				ImVec2(TextWindowWidth, TextWindowHeight),
				false,
				ImGuiWindowFlags_None);

			for (const std::string& Text : GlobalConsole.History)
			{
				ImGui::Text("%s", Text.c_str());
			}

			ImGui::SetScrollHereY();

			ImGui::EndChild();

			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));

			ImGui::Text(">");
			ImGui::SameLine();
			
			ImGui::PushItemWidth(TextWindowWidth - 25.0f);

			const ImGuiInputTextFlags InputFlags =
				ImGuiInputTextFlags_EnterReturnsTrue	|
				ImGuiInputTextFlags_CallbackCompletion	|
				ImGuiInputTextFlags_CallbackAlways		|
				ImGuiInputTextFlags_CallbackHistory;

			auto Callback = [](ImGuiTextEditCallbackData* Data)->Int32
			{
				Console* This = reinterpret_cast<Console*>(Data->UserData);
				return This->TextCallback(Data);
			};

			const Bool Result = ImGui::InputText(
				"###Input",
				GlobalConsole.Buffer.Data(), GlobalConsole.Buffer.Size(),
				InputFlags,
				Callback,
				reinterpret_cast<void*>(&GlobalConsole));

			if (Result)
			{
				const std::string Text = std::string(GlobalConsole.Buffer.Data());
				GlobalConsole.History.EmplaceBack(Text);

				if (Text == "ShowDebug")
				{
					GlobalDrawTextureDebugger = true;
				}
				else if (Text == "Fullscreen")
				{
					GlobalMainWindow->ToggleFullscreen();
				}
				else if (Text == "Fullscreen")
				{
					GlobalEngineLoop.Exit();
				}
			}

			if (ImGui::IsWindowFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))
			{
				ImGui::SetKeyboardFocusHere(-1);
			}

			ImGui::PopItemWidth();

			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();
			ImGui::PopStyleColor();

			ImGui::End();
		});
	}
}

void Console::RegisterCommand(const ConsoleCommand& Cmd)
{
}

Bool Console::OnEvent(const Event& Event)
{
	if (!IsEventOfType<KeyPressedEvent>(Event))
	{
		return false;
	}

	const KeyPressedEvent& KeyEvent = CastEvent<KeyPressedEvent>(Event);
	if (!KeyEvent.IsRepeat && KeyEvent.Key == EKey::Key_GraveAccent)
	{
		IsActive = !IsActive;
	}

	return true;
}

Int32 Console::TextCallback(ImGuiInputTextCallbackData* Data)
{
	if (UpdateCursorPosition)
	{
		Data->CursorPos = Int32(PopupSelectedText.length());
		Data->InsertChars(Data->CursorPos, " ");
		
		UpdateCursorPosition = false;
		PopupSelectedText.clear();
	}

	switch (Data->EventFlag)
	{
		case ImGuiInputTextFlags_CallbackEdit:
		{
			const Char* WordEnd		= Data->Buf + Data->CursorPos;
			const Char* WordStart	= WordEnd;
			while (WordStart > Data->Buf)
			{
				const Char c = WordStart[-1];
				if (c == ' ' || c == '\t' || c == ',' || c == ';')
				{
					break;
				}

				WordStart--;
			}

			//m_Candidates.Clear();
			//for (auto& cmd : m_CommandMap)
			//{
			//	const char* command = cmd.first.c_str();
			//	int32 d = -1;
			//	int32 n = (int32)(WordEnd - WordStart);
			//	const char* s1 = command;
			//	const char* s2 = WordStart;
			//	while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1)
			//	{
			//		s1++;
			//		s2++;
			//		n--;
			//	}
			//	if (d == 0)
			//	{
			//		m_Candidates.PushBack(command);
			//	}
			//}

			break;
		}
		case ImGuiInputTextFlags_CallbackCompletion:
		{
			const Char* WordEnd		= Data->Buf + Data->CursorPos;
			const Char* WordStart	= WordEnd;
			while (WordStart > Data->Buf)
			{
				const Char c = WordStart[-1];
				if (c == ' ' || c == '\t' || c == ',' || c == ';')
				{
					break;
				}

				WordStart--;
			}

			//// Build a list of candidates
			//TArray<const char*> candidates;
			////m_Candidates.Clear();
			//for (auto& cmd : m_CommandMap)
			//{
			//	const char* command = cmd.first.c_str();
			//	int32 d = 0;
			//	int32 n = (int32)(word_end - word_start);
			//	const char* s1 = command;
			//	const char* s2 = word_start;
			//	while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1)
			//	{
			//		s1++;
			//		s2++;
			//		n--;
			//	}
			//	if (d == 0)
			//	{
			//		candidates.PushBack(command);
			//	}
			//}

			//if (candidates.GetSize() == 0)
			//{
			//	// No match
			//	m_ActivePopupIndex = -1;
			//}
			//else if (candidates.GetSize() == 1)
			//{
			//	// Single match. Delete the beginning of the word and replace it entirely so we've got nice casing.
			//	data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
			//	data->InsertChars(data->CursorPos, candidates[0]);
			//	data->InsertChars(data->CursorPos, " ");
			//}
			//else if (m_ActivePopupIndex != -1)
			//{
			//	data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
			//	data->InsertChars(data->CursorPos, m_PopupSelectedText.c_str());
			//	data->InsertChars(data->CursorPos, " ");
			//	m_ActivePopupIndex = -1;
			//	m_PopupSelectedText = "";
			//	m_Candidates.Clear();
			//}

			break;
		}
		case ImGuiInputTextFlags_CallbackHistory:
		{
			//if (m_Candidates.GetSize() == 0)
			//{
			//	// Show history when nothing is typed (no candidates)
			//	const int prevHistoryIndex = m_HistoryIndex;
			//	if (data->EventKey == ImGuiKey_UpArrow)
			//	{
			//		if (m_HistoryIndex == -1)
			//			m_HistoryIndex = m_History.GetSize() - 1;
			//		else if (m_HistoryIndex > 0)
			//			m_HistoryIndex--;
			//	}
			//	else if (data->EventKey == ImGuiKey_DownArrow)
			//	{
			//		if (m_HistoryIndex != -1)
			//			if (++m_HistoryIndex >= (int)m_History.GetSize())
			//				m_HistoryIndex = -1;
			//	}

			//	if (prevHistoryIndex != m_HistoryIndex)
			//	{
			//		const char* historyStr = (m_HistoryIndex >= 0) ? m_History[m_HistoryIndex].c_str() : "";
			//		data->DeleteChars(0, data->BufTextLen);
			//		data->InsertChars(0, historyStr);
			//	}
			//}
			//else
			{
				// Navigate candidates list
				//if (data->EventKey == ImGuiKey_UpArrow)
				//{
				//	if (m_ActivePopupIndex <= 0)
				//	{
				//		m_ActivePopupIndex = ((int32)(m_Candidates.GetSize()) - 1);
				//		m_PopupSelectionChanged = true;
				//	}
				//	else
				//	{
				//		m_ActivePopupIndex--;
				//		m_PopupSelectionChanged = true;
				//	}
				//}
				//else if (data->EventKey == ImGuiKey_DownArrow)
				//{
				//	if (m_ActivePopupIndex >= ((int32)(m_Candidates.GetSize()) - 1))
				//	{
				//		m_ActivePopupIndex = 0;
				//		m_PopupSelectionChanged = true;
				//	}
				//	else
				//	{
				//		m_ActivePopupIndex++;
				//		m_PopupSelectionChanged = true;
				//	}
				//}
			}

			break;
		}
	}

	return 0;
}
