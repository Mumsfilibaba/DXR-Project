#pragma once
#include "Windows.h"

class WindowsConsoleOutput
{
public:
	WindowsConsoleOutput();
	~WindowsConsoleOutput();

	void Print(const std::string& Message);

private:
	HANDLE OutputHandle = 0;
};

extern WindowsConsoleOutput* GlobalOutputHandle;