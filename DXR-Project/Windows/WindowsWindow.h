#pragma once
#include "Windows.h"
#include "Defines.h"
#include "Types.h"

enum EWindowStyleFlag : Uint32
{
	WINDOW_STYLE_FLAG_NONE			= 0x00,
	WINDOW_STYLE_FLAG_TITLED		= FLAG(1),
	WINDOW_STYLE_FLAG_CLOSABLE		= FLAG(2),
	WINDOW_STYLE_FLAG_MINIMIZABLE	= FLAG(3),
	WINDOW_STYLE_FLAG_MAXIMIZABLE	= FLAG(4),
	WINDOW_STYLE_FLAG_RESIZEABLE	= FLAG(5),
};

struct WindowShape
{
	Uint16	Width;
	Uint16	Height;
	Int16	X;
	Int16	Y;
};

struct WindowProperties
{
	std::string Title;
	Uint16		Width;
	Uint16		Height;
	Uint32		Style;
};

class WindowsApplication;

class WindowsWindow
{
public:
	WindowsWindow();
	~WindowsWindow();

	bool Initialize(WindowsApplication* InOwnerApplication, const WindowProperties& Properties);

	void Show();

	void GetWindowShape(WindowShape& OutWindowShape);
	
	FORCEINLINE HWND GetHandle() const
	{
		return hWindow;
	}

private:
	WindowsApplication* OwnerApplication = nullptr;

	HWND	hWindow = 0;
	DWORD	Style	= 0;
};