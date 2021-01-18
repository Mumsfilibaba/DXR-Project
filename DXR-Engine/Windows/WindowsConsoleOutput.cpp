#include "WindowsConsoleOutput.h"

/*
* WindowsConsoleOutput
*/

WindowsConsoleOutput::WindowsConsoleOutput()
{
	if (::AllocConsole())
	{
		ConsoleHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
		::SetConsoleTitleA("Console Output");
	}
}

WindowsConsoleOutput::~WindowsConsoleOutput()
{
	if (ConsoleHandle)
	{
		::FreeConsole();
		ConsoleHandle = 0;
	}
}

void WindowsConsoleOutput::Print(const std::string& Message)
{
	if (ConsoleHandle)
	{
		::WriteConsoleA(ConsoleHandle, Message.c_str(), static_cast<DWORD>(Message.size()), 0, NULL);
	}
}

void WindowsConsoleOutput::Clear()
{
	if (ConsoleHandle)
	{
		CONSOLE_SCREEN_BUFFER_INFO CSBI = { };
		if (::GetConsoleScreenBufferInfo(ConsoleHandle, &CSBI))
		{
			COORD		Dest		= { 0, -CSBI.dwSize.Y };
			CHAR_INFO	FillInfo	= { '\0', FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE };
			::ScrollConsoleScreenBufferA(ConsoleHandle, &CSBI.srWindow, nullptr, Dest, &FillInfo);

			COORD CursorPos = { 0, 0 };
			::SetConsoleCursorPosition(ConsoleHandle, CursorPos);
		}
	}
}

void WindowsConsoleOutput::SetTitle(const std::string& Title)
{
	if (ConsoleHandle)
	{
		::SetConsoleTitleA(Title.c_str());
	}
}

void WindowsConsoleOutput::SetColor(EConsoleColor Color)
{
	if (ConsoleHandle)
	{
		WORD wColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
		switch (Color)
		{
		case EConsoleColor::ConsoleColor_Red:
			wColor = FOREGROUND_RED | FOREGROUND_INTENSITY;
			break;

		case EConsoleColor::ConsoleColor_Green:
			wColor = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
			break;

		case EConsoleColor::ConsoleColor_Yellow:
			wColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
			break;

		case EConsoleColor::ConsoleColor_White:
			break;
		}

		::SetConsoleTextAttribute(ConsoleHandle, wColor);
	}
}

GenericOutputDevice* WindowsConsoleOutput::Make()
{
	return DBG_NEW WindowsConsoleOutput();
}
