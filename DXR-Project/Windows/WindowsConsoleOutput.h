#pragma once
#include "Windows.h"

enum class EConsoleColor : Uint8
{
	CONSOLE_COLOR_RED		= 0,
	CONSOLE_COLOR_GREEN		= 1,
	CONSOLE_COLOR_YELLOW	= 2,
	CONSOLE_COLOR_WHITE		= 3
};

/*
* WindowsConsoleOutput
*/
class WindowsConsoleOutput
{
public:
	WindowsConsoleOutput();
	~WindowsConsoleOutput();

	void Print(const std::string& Message);
	void Clear();

	void SetTitle(const std::string& Title);
	void SetColor(EConsoleColor Color);

private:
	HANDLE OutputHandle = 0;
};

extern WindowsConsoleOutput* GlobalOutputHandle;