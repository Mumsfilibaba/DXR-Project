#pragma once
#include "Windows.h"

#include "Types.h"

class WindowsWindow
{
public:
	WindowsWindow();
	~WindowsWindow();

	bool Init(Uint16 Width, Uint16 Height);

	void Show();

	HWND GetHandle() const
	{
		return hWindow;
	}

private:
	HWND hWindow = 0;
};