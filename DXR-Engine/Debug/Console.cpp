#include "Console.h"

#include "Rendering/DebugUI.h"

#include "Main/EngineLoop.h"

#include <regex>

/*
* Globals
*/

Bool GlobalIsConsoleActive = false;

/*
* Console
*/

void Console::Init()
{
	auto EventHandler = [](const Event& Event)->Bool
	{
		if (!IsEventOfType<KeyPressedEvent>(Event))
		{
			return false;
		}

		const KeyPressedEvent& KeyEvent = CastEvent<KeyPressedEvent>(Event);
		if (!KeyEvent.IsRepeat && KeyEvent.Key == EKey::Key_GraveAccent)
		{
			GlobalConsole.IsActive = !GlobalConsole.IsActive;
		}

		return true;
	};

	GlobalEventDispatcher->RegisterEventHandler(EventHandler);
}

void Console::Tick()
{
	if (IsActive)
	{
		DebugUI::DrawUI([]()
		{
			const UInt32 WindowWidth	= GlobalMainWindow->GetWidth();
			const UInt32 WindowHeight	= GlobalMainWindow->GetHeight();
			const Float Width			= Float(WindowWidth);
			const Float Height			= Float(WindowHeight) * 0.15f;

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

			ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.3f, 0.3f, 0.3f, 0.6f));

			const ImVec2 ParentSize			= ImGui::GetWindowSize();
			const Float TextWindowWidth		= Width * 0.985f;
			const Float TextWindowHeight	= ParentSize.y - 40.0f;
			ImGui::BeginChild(
				"##TextWindow",
				ImVec2(TextWindowWidth, TextWindowHeight),
				false,
				ImGuiWindowFlags_None);

			for (const OutputHistory& Text : GlobalConsole.History)
			{
				ImGui::TextColored(Text.Color, "%s", Text.String.c_str());
			}

			if (GlobalConsole.ScrollDown)
			{
				ImGui::SetScrollHereY();
				GlobalConsole.ScrollDown = false;
			}

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
				GlobalConsole.TextBuffer.Data(), GlobalConsole.TextBuffer.Size(),
				InputFlags,
				Callback,
				reinterpret_cast<void*>(&GlobalConsole));

			if (Result && GlobalConsole.TextBuffer[0] != 0)
			{
				const std::string Text = std::string(GlobalConsole.TextBuffer.Data());
				GlobalConsole.HandleCommand(Text);
				GlobalConsole.TextBuffer[0] = 0;

				GlobalConsole.ScrollDown = true;
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

void Console::RegisterCommand(
	const std::string& CmdName, 
	ConsoleCommand Cmd)
{
	ConsoleCommand CurrCmd = FindCommand(CmdName);
	if (CurrCmd)
	{
		LOG_WARNING("ConsoleCommand '" + CmdName + "' is already registered");
		return;
	}

	CmdIndexMap[CmdName]		= NextCommandIndex;
	Commands[NextCommandIndex]	= Cmd;

	NextCommandIndex++;
}

ConsoleVariable* Console::RegisterVariable(
	const std::string& VarName, 
	EConsoleVariableType Type)
{
	ConsoleVariable* Var = FindVariable(VarName);
	if (Var)
	{
		LOG_WARNING("ConsoleVariable '" + VarName + "' is already registered");
		return Var;
	}

	const UInt32 Index = NextVariableIndex;
	NextVariableIndex++;

	// TODO: Better way of initializing a consolevar
	VarIndexMap[VarName] = Index;
	Var = &Variables[Index];
	Var->Type			= Type;
	Var->StringValue	= nullptr;
	Var->Length			= 0;
	return Var;
}

ConsoleCommand Console::FindCommand(const std::string& CmdName)
{
	auto CmdIndex = CmdIndexMap.find(CmdName.c_str());
	if (CmdIndex != CmdIndexMap.end())
	{
		VALIDATE(CmdIndex->second >= 0);
		return Commands[CmdIndex->second];
	}

	return nullptr;
}

ConsoleVariable* Console::FindVariable(const std::string& VarName)
{
	auto VarIndex = VarIndexMap.find(VarName.c_str());
	if (VarIndex != VarIndexMap.end())
	{
		VALIDATE(VarIndex->second >= 0);
		return &Variables[VarIndex->second];
	}

	return nullptr;
}

void Console::PrintMessage(const std::string& Message)
{
	GlobalConsole.History.EmplaceBack(Message, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
}

void Console::PrintWarning(const std::string& Message)
{
	GlobalConsole.History.EmplaceBack(Message, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
}

void Console::PrintError(const std::string& Message)
{
	GlobalConsole.History.EmplaceBack(Message, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
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
			{
				//const Int32 PrevHistoryIndex = m_HistoryIndex;
				//if (Data->EventKey == ImGuiKey_UpArrow)
				//{
				//	if (m_HistoryIndex == -1)
				//		m_HistoryIndex = m_History.GetSize() - 1;
				//	else if (m_HistoryIndex > 0)
				//		m_HistoryIndex--;
				//}
				//else if (Data->EventKey == ImGuiKey_DownArrow)
				//{
				//	if (m_HistoryIndex != -1)
				//		if (++m_HistoryIndex >= (int)m_History.GetSize())
				//			m_HistoryIndex = -1;
				//}

				//if (prevHistoryIndex != m_HistoryIndex)
				//{
				//	const char* historyStr = (m_HistoryIndex >= 0) ? m_History[m_HistoryIndex].c_str() : "";
				//	data->DeleteChars(0, data->BufTextLen);
				//	data->InsertChars(0, historyStr);
				//}
			}
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

void Console::HandleCommand(const std::string& CmdString)
{
	PrintMessage(CmdString);

	size_t Pos = CmdString.find_first_of(" ");
	if (Pos == std::string::npos)
	{
		auto CmdIndex = CmdIndexMap.find(CmdString);
		if (CmdIndex != CmdIndexMap.end())
		{
			ConsoleCommand Cmd = Commands[CmdIndex->second];
			VALIDATE(Cmd != nullptr);

			Cmd();
		}
		else
		{
			const std::string Message = "'" + CmdString + "' is not a registered command";
			PrintError(Message);
		}
	}
	else
	{
		std::string Variable(CmdString.c_str(), Pos);
		auto VarIndex = VarIndexMap.find(Variable);
		if (VarIndex == VarIndexMap.end())
		{
			PrintError("'" + CmdString + "' is not a registered variable");
			return;
		}

		ConsoleVariable* Var = &Variables[VarIndex->second];
		Pos++;

		std::string Value(CmdString.c_str() + Pos, CmdString.length() - Pos);
		if (std::regex_match(Value, std::regex("-[0-9]+")) && Var->CanBeInteger())
		{
			const Int32 IntValue = std::stoi(Value);
			Var->SetAndConvertInt32(IntValue);
		}
		else if (std::regex_match(Value, std::regex("[0-9]+")) && Var->CanBeInteger())
		{
			const Int32 IntValue = std::stoi(Value);
			Var->SetAndConvertInt32(IntValue);
		}
		else if (std::regex_match(Value, std::regex("(-[0-9]*\\.[0-9]+)|(-[0-9]+\\.[0-9]*)")) && Var->IsFloat())
		{
			const Float FloatValue = std::stof(Value);
			Var->SetFloat(FloatValue);
		}
		else if (std::regex_match(Value, std::regex("([0-9]*\\.[0-9]+)|([0-9]+\\.[0-9]*)")) && Var->IsFloat())
		{
			const Float FloatValue = std::stof(Value);
			Var->SetFloat(FloatValue);
		}
		else 
		{
			if (Var->IsBool())
			{
				for (Char& c : Value)
				{
					c = (Char)tolower(c);
				}

				if (std::regex_match(Value, std::regex("(false)|(true)")))
				{
					const Bool BoolValue = (Value == "false" ? false : true);
					Var->SetBool(BoolValue);
					return;
				}
			}
			else if (Var->IsString())
			{
				Var->SetString(Value.c_str());
				return;
			}
			
			const std::string Message = "'" + Value + "' Is an invalid value for '" + Variable + "'";
			PrintError(Message);
		}
	}
}
