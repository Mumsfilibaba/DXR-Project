#pragma once
#include "Application/Events/EventDispatcher.h"

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

class Console : public IEventHandler
{
public:
	Console()	= default;
	~Console()	= default;

	void Init();

	void Tick();

	void RegisterCommand(const ConsoleCommand& Cmd);

	virtual Bool OnEvent(const Event& Event) override final;

private:
	Int32 TextCallback(ImGuiInputTextCallbackData* Data);

	TArray<std::string>		History;
	TStaticArray<Char, 256>	Buffer;

	std::string PopupSelectedText;
	
	Bool UpdateCursorPosition	= false;
	Bool IsActive				= false;
};