#pragma once
#include <unordered_map>

/*
* ConsoleCommand
*/

struct ConsoleCommand
{

};

/*
* Console
*/

class Console
{
public:
	Console();
	~Console();

	void Init();

	void Tick();

	void RegisterCommand(const ConsoleCommand& Cmd);

	void DrawUI();

	Int32 TextCallback(ImGuiInputTextCallbackData* Data);

private:
	TArray<std::string>		History;
	TStaticArray<Char, 256>	Buffer;

	std::string PopupSelectedText;
	
	Bool UpdateCursorPosition = false;
};