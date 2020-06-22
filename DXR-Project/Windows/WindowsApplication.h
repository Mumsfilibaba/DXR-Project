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

	std::shared_ptr<WindowsWindow> CreateWindow(Uint16 InWidth, Uint16 InHeight);

	bool Tick();

	void SetActiveWindow(std::shared_ptr<WindowsWindow>& InActiveWindow);
	void SetCapture(std::shared_ptr<WindowsWindow>& InCaptureWindow);

	std::shared_ptr<WindowsWindow> GetWindowFromHWND(HWND hWindow)	const;
	std::shared_ptr<WindowsWindow> GetActiveWindow()				const;
	std::shared_ptr<WindowsWindow> GetCapture()						const;

	void SetEventHandler(std::shared_ptr<EventHandler>& InEventHandler)
	{
		MessageEventHandler = InEventHandler;
	}

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

	void AddWindow(std::shared_ptr<WindowsWindow>& InWindow);

	bool RegisterWindowClass();

	LRESULT ApplicationProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

	static LRESULT MessageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

private:
	HINSTANCE						hInstance			= 0;
	std::shared_ptr<EventHandler>	MessageEventHandler	= nullptr;

	std::vector<std::shared_ptr<WindowsWindow>> Windows;
};

extern WindowsApplication* GlobalWindowsApplication;