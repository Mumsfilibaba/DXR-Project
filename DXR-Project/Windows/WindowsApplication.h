#pragma once
#include "Windows.h"

#include "Types.h"

#include <memory>
#include <vector>

class WindowsWindow;
class EventHandler;

class WindowsApplication
{
public:
	~WindowsApplication();

	std::shared_ptr<WindowsWindow> CreateWindow(Uint16 Width, Uint16 Height);
	std::shared_ptr<WindowsWindow> GetWindowFromHWND(HWND hWindow);

	bool Tick();

	void SetEventHandler(std::shared_ptr<EventHandler> NewEventHandler);
	
	std::shared_ptr<EventHandler> GetEventHandler() const
	{
		return MessageEventHandler;
	}

	HINSTANCE GetInstance()
	{
		return hInstance;
	}

	static WindowsApplication* Create(HINSTANCE hInstance);

private:
	WindowsApplication(HINSTANCE hInstance);
	
	bool Create();

	void AddWindow(std::shared_ptr<WindowsWindow>& NewWindow);

	bool RegisterWindowClass();

	LRESULT ApplicationProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

	static LRESULT MessageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

private:
	HINSTANCE						hInstance			= 0;
	std::shared_ptr<EventHandler>	MessageEventHandler	= nullptr;

	std::vector<std::shared_ptr<WindowsWindow>> Windows;
};

extern WindowsApplication* GlobalWindowsApplication;