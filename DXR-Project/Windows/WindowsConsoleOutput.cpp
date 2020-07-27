#include "WindowsConsoleOutput.h"

/*
* Global Handle
*/

WindowsConsoleOutput* GlobalOutputHandle = nullptr;

/*
* WindowsConsoleOutput
*/

WindowsConsoleOutput::WindowsConsoleOutput()
{
	if (::AllocConsole())
	{
		OutputHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
		::SetConsoleTitleA("Console Output");
	}
}

WindowsConsoleOutput::~WindowsConsoleOutput()
{
	if (OutputHandle)
	{
		::FreeConsole();
		OutputHandle = 0;
	}
}

void WindowsConsoleOutput::Print(const std::string& Message)
{
	if (OutputHandle)
	{
		::WriteConsoleA(OutputHandle, Message.c_str(), Message.size(), 0, NULL);
	}
}

void WindowsConsoleOutput::Clear()
{
	if (OutputHandle)
	{
		CONSOLE_SCREEN_BUFFER_INFO CSBI = { };
		if (::GetConsoleScreenBufferInfo(OutputHandle, &CSBI))
		{
			COORD		Dest		= { 0, -CSBI.dwSize.Y };
			CHAR_INFO	FillInfo	= { '\0', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE };
			::ScrollConsoleScreenBufferA(OutputHandle, &CSBI.srWindow, nullptr, Dest, &FillInfo);

			COORD CursorPos = { 0, 0 };
			::SetConsoleCursorPosition(OutputHandle, CursorPos);
		}
	}
}

void WindowsConsoleOutput::SetTitle(const std::string& Title)
{
	if (OutputHandle)
	{
		::SetConsoleTitleA(Title.c_str());
	}
}

void WindowsConsoleOutput::SetColor(EConsoleColor Color)
{
	if (OutputHandle)
	{
		WORD wColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
		switch (Color)
		{
		case EConsoleColor::CONSOLE_COLOR_RED:
			wColor = FOREGROUND_RED | FOREGROUND_INTENSITY;
			break;

		case EConsoleColor::CONSOLE_COLOR_GREEN:
			wColor = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
			break;

		case EConsoleColor::CONSOLE_COLOR_YELLOW:
			wColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
			break;

		case EConsoleColor::CONSOLE_COLOR_WHITE:
			break;
		}

		::SetConsoleTextAttribute(OutputHandle, wColor);
	}
}
