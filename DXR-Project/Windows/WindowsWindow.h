#pragma once
#include "Windows.h"

#include "Types.h"

struct WindowShape
{
public:
	Uint16	Width;
	Uint16	Height;
	Int16	x;
	Int16	y;
};

class WindowsWindow
{
public:
	WindowsWindow();
	~WindowsWindow();

	bool Init(Uint16 Width, Uint16 Height);

	void Show();

	void GetWindowShape(WindowShape& OutWindowShape);
	
	HWND GetHandle() const
	{
		return hWindow;
	}

private:
	HWND	hWindow = 0;
	DWORD	dwStyle = WS_OVERLAPPEDWINDOW;
};