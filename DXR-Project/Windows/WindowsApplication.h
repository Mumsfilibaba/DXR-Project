#pragma once
#include "Windows.h"

#include "Types.h"

#include <memory>
#include <vector>

class WindowsWindow;
class EventHandler;

class WindowsApplication
{
	WindowsApplication(WindowsApplication&& Other)		= delete;
	WindowsApplication(const WindowsApplication& Other)	= delete;

	WindowsApplication& operator=(WindowsApplication&& Other)		= delete;
	WindowsApplication& operator=(const WindowsApplication& Other)	= delete;

public:
	~WindowsApplication();

	WindowsWindow* CreateWindow(Uint16 Width, Uint16 Height);
	WindowsWindow* GetWindowFromHWND(HWND hWindow);

	bool Tick();

	void SetEventHandler(EventHandler* NewMessageHandler);
	
	EventHandler* GetEventHandler() const
	{
		return MessageHandler;
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
	HINSTANCE		hInstance		= 0;
	EventHandler*	MessageHandler	= nullptr;

	std::vector<WindowsWindow*> Windows;

	static std::unique_ptr<WindowsApplication> WinApplication;
};