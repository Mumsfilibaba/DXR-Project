#pragma once
#include "Windows/WindowsConsoleOutput.h"

#define LOG_ERROR(Message) \
	VALIDATE(GlobalOutputHandle != nullptr); \
	GlobalOutputHandle->SetColor(EConsoleColor::CONSOLE_COLOR_RED); \
	GlobalOutputHandle->Print(std::string(Message) + "\n"); \
	GlobalOutputHandle->SetColor(EConsoleColor::CONSOLE_COLOR_WHITE) \

#define LOG_WARNING(Message) \
	VALIDATE(GlobalOutputHandle != nullptr); \
	GlobalOutputHandle->SetColor(EConsoleColor::CONSOLE_COLOR_YELLOW); \
	GlobalOutputHandle->Print(std::string(Message) + "\n"); \
	GlobalOutputHandle->SetColor(EConsoleColor::CONSOLE_COLOR_WHITE) \

#define LOG_INFO(Message) \
	VALIDATE(GlobalOutputHandle != nullptr); \
	GlobalOutputHandle->SetColor(EConsoleColor::CONSOLE_COLOR_GREEN); \
	GlobalOutputHandle->Print(std::string(Message) + "\n"); \
	GlobalOutputHandle->SetColor(EConsoleColor::CONSOLE_COLOR_WHITE) \