#include "WindowsConsoleOutput.h"

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
		::WriteConsoleA(OutputHandle, Message.c_str(), static_cast<DWORD>(Message.size()), 0, NULL);
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

		::SetConsoleTextAttribute(OutputHandle, wColor);
	}
}

GenericOutputDevice* WindowsConsoleOutput::Make()
{
	return new WindowsConsoleOutput();
}
