#pragma once
#include "Defines.h"
#include "Types.h"

/*
* EConsoleColor
*/

enum class EConsoleColor : uint8
{
	CONSOLE_COLOR_RED		= 0,
	CONSOLE_COLOR_GREEN		= 1,
	CONSOLE_COLOR_YELLOW	= 2,
	CONSOLE_COLOR_WHITE		= 3
};

/*
* GenericConsoleOutput
*/

class GenericOutputDevice
{
public:
	GenericOutputDevice() = default;
	virtual ~GenericOutputDevice() = default;

	virtual void Print(const std::string& Message) = 0;
	virtual void Clear() = 0;

	virtual void SetTitle(const std::string& Title) = 0;
	virtual void SetColor(EConsoleColor Color) = 0;

	static GenericOutputDevice* Make()
	{
		return nullptr;
	}
};

/*
* GlobalOutputDevices
*/

struct GlobalOutputDevices
{
	static GenericOutputDevice* Console;

	static bool Initialize();
	static bool Release();
};