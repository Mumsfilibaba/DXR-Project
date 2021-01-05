#pragma once
#include "Platform/PlatformOutputDevice.h"

#define LOG_ERROR(Message) \
	{ \
		VALIDATE(GlobalOutputDevices::Console != nullptr); \
		GlobalOutputDevices::Console->SetColor(EConsoleColor::ConsoleColor_Red); \
		GlobalOutputDevices::Console->Print(std::string(Message) + "\n"); \
		GlobalOutputDevices::Console->SetColor(EConsoleColor::ConsoleColor_White); \
	}

#define LOG_WARNING(Message) \
	{ \
		VALIDATE(GlobalOutputDevices::Console != nullptr); \
		GlobalOutputDevices::Console->SetColor(EConsoleColor::ConsoleColor_Yellow); \
		GlobalOutputDevices::Console->Print(std::string(Message) + "\n"); \
		GlobalOutputDevices::Console->SetColor(EConsoleColor::ConsoleColor_White); \
	}

#define LOG_INFO(Message) \
	{\
		VALIDATE(GlobalOutputDevices::Console != nullptr); \
		GlobalOutputDevices::Console->SetColor(EConsoleColor::ConsoleColor_Green); \
		GlobalOutputDevices::Console->Print(std::string(Message) + "\n"); \
		GlobalOutputDevices::Console->SetColor(EConsoleColor::ConsoleColor_White); \
	}