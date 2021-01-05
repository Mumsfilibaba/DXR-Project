#pragma once
#include "Core.h"

/*
* EConsoleColor
*/

enum class EConsoleColor : UInt8
{
	ConsoleColor_Red	= 0,
	ConsoleColor_Green	= 1,
	ConsoleColor_Yellow	= 2,
	ConsoleColor_White	= 3
};

/*
* GenericConsoleOutput
*/

class GenericOutputDevice
{
public:
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