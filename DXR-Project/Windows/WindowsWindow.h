#pragma once
#include "Windows.h"
#include "Defines.h"
#include "Types.h"

struct WindowShape
{
public:
	Uint16	Width;
	Uint16	Height;
	Int16	x;
	Int16	y;
};

class WindowsApplication;

class WindowsWindow
{
public:
	WindowsWindow();
	~WindowsWindow();

	bool Initialize(WindowsApplication* InOwnerApplication, Uint16 InWidth, Uint16 InHeight);

	void Show();

	void GetWindowShape(WindowShape& OutWindowShape);
	
	FORCEINLINE HWND GetHandle() const
	{
		return hWindow;
	}

private:
	WindowsApplication* OwnerApplication = nullptr;

	HWND	hWindow = 0;
	DWORD	dwStyle = WS_OVERLAPPEDWINDOW;
};