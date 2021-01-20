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

private:
	TArray<std::string>		History;
	TStaticArray<Char, 256>	Buffer;
};