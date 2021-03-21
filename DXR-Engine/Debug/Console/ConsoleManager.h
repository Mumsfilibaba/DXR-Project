#pragma once
#include "ConsoleVariable.h"
#include "ConsoleCommand.h"

class ConsoleManager
{
public:
	void RegisterVariable(const String& Name, ConsoleVariable* Variable);
	void RegisterCommand(const String& Name, ConsoleCommand* Cmd);
};