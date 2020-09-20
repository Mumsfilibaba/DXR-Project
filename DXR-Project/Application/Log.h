#pragma once
#include "Platform/PlatformOutputDevice.h"

#define LOG_ERROR(Message) \
	{ \
		VALIDATE(GlobalOutputHandle != nullptr); \
		GlobalOutputDevices::Console->SetColor(EConsoleColor::CONSOLE_COLOR_RED); \
		GlobalOutputDevices::Console->Print(std::string(Message) + "\n"); \
		GlobalOutputDevices::Console->SetColor(EConsoleColor::CONSOLE_COLOR_WHITE); \
	}

#define LOG_WARNING(Message) \
	{ \
		VALIDATE(GlobalOutputHandle != nullptr); \
		GlobalOutputDevices::Console->SetColor(EConsoleColor::CONSOLE_COLOR_YELLOW); \
		GlobalOutputDevices::Console->Print(std::string(Message) + "\n"); \
		GlobalOutputDevices::Console->SetColor(EConsoleColor::CONSOLE_COLOR_WHITE); \
	}

#define LOG_INFO(Message) \
	{\
		VALIDATE(GlobalOutputDevices::Console != nullptr); \
		GlobalOutputDevices::Console->SetColor(EConsoleColor::CONSOLE_COLOR_GREEN); \
		GlobalOutputDevices::Console->Print(std::string(Message) + "\n"); \
		GlobalOutputDevices::Console->SetColor(EConsoleColor::CONSOLE_COLOR_WHITE); \
	}