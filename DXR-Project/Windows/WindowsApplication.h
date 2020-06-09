#pragma once
#include "Windows.h"

#include "Types.h"

#include <memory>
#include <vector>

class WindowsWindow;
class GenericEventHandler;

class WindowsApplication
{
public:
	~WindowsApplication();

	WindowsWindow* CreateWindow(Uint16 Width, Uint16 Height);
	WindowsWindow* GetWindowFromHWND(HWND hWindow);

	bool Tick();

	void SetEventHandler(GenericEventHandler* NewEventHandler);
	
	GenericEventHandler* GetEventHandler() const
	{
		return EventHandler;
	}

	HINSTANCE GetInstance()
	{
		return hInstance;
	}

	static WindowsApplication* Create(HINSTANCE hInstance);
	static WindowsApplication* Get();

private:
	WindowsApplication(HINSTANCE hInstance);
	
	bool Create();

	void AddWindow(WindowsWindow* NewWindow);

	bool RegisterWindowClass();

	LRESULT ApplicationProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

	static LRESULT MessageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

private:
	HINSTANCE				hInstance		= 0;
	GenericEventHandler*	EventHandler	= nullptr;

	std::vector<WindowsWindow*> Windows;

	static std::unique_ptr<WindowsApplication> WinApplication;
};