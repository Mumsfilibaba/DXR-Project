#include "Console.h"

#include "Rendering/DebugUI.h"

/*
* Console
*/

Console::Console()
{
}

Console::~Console()
{
}

void Console::Init()
{
}

void Console::Tick()
{
	static Int32 n = 0;
	
	if (n < 250)
	{
		n++;
		History.EmplaceBack("Text" + std::to_string(n));
	}

	DebugUI::DrawUI([]()
	{
		GlobalConsole.DrawUI();
	});
}

void Console::RegisterCommand(const ConsoleCommand& Cmd)
{
}

void Console::DrawUI()
{
	const UInt32 WindowWidth	= GlobalMainWindow->GetWidth();
	const UInt32 WindowHeight	= GlobalMainWindow->GetHeight();
	const Float Width			= Math::Max(WindowWidth * 0.6f, 400.0f);
	const Float Height			= WindowHeight * 0.15f;

	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.3f, 0.3f, 0.3f, 0.6f));
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ResizeGrip, 0);
	ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, 0);
	ImGui::PushStyleColor(ImGuiCol_ResizeGripActive, 0);

	ImGui::SetNextWindowPos(
		ImVec2(10.0f, 10.0f),
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
		ImGuiWindowFlags_NoMove			|
		ImGuiWindowFlags_NoTitleBar		| 
		ImGuiWindowFlags_NoScrollbar	| 
		ImGuiWindowFlags_NoCollapse		|
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

	ImGui::PushItemWidth(TextWindowWidth * 0.5f);
	ImGui::Text(">");
	ImGui::SameLine();
	ImGui::InputText("###Input", Buffer.Data(), Buffer.Size());
	ImGui::PopItemWidth();

	ImGui::PopStyleColor();

	ImGui::PopStyleColor();
	ImGui::PopStyleColor();
	ImGui::PopStyleColor();
	ImGui::PopStyleColor();
	ImGui::PopStyleColor();
	ImGui::PopStyleColor();

	ImGui::End();
}
