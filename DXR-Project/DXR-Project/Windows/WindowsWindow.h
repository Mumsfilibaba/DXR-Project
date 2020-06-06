#pragma once
#include "Windows.h"

#include "../Types.h"

class WindowsWindow
{
	WindowsWindow(WindowsWindow&& Other)		= delete;
	WindowsWindow(const WindowsWindow& Other)	= delete;

	WindowsWindow& operator=(WindowsWindow&& Other)			= delete;
	WindowsWindow& operator=(const WindowsWindow& Other)	= delete;

public:
	WindowsWindow();
	~WindowsWindow();

	bool Create(Uint16 Width, Uint16 Height);

	void Show();

	HWND GetHandle()
	{
		return hWindow;
	}

private:
	HWND hWindow = 0;
};