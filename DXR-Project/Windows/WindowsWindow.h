#pragma once
#include "Windows.h"
#include "Defines.h"
#include "Types.h"

#include "Application/Generic/GenericWindow.h"

struct WindowProperties
{
	std::string Title;
	Uint16		Width;
	Uint16		Height;
	Uint32		Style;
};

/*
* WindowsWindow
*/

class WindowsApplication;

class WindowsWindow : public GenericWindow
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