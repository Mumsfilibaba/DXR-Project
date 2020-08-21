#pragma once
#include "Defines.h"
#include "Types.h"
#include "Windows.h"

#include "Application/InputCodes.h"
#include "Application/Events/ApplicationEventHandler.h"

class WindowsWindow;
class ApplicationEventHandler;
class WindowsCursor;

/*
* WindowsApplication
*/
class WindowsApplication
{
public:
	~WindowsApplication();

	TSharedPtr<WindowsWindow> MakeWindow(const struct WindowProperties& Properties);

	bool Tick();
	
	void SetCursor(TSharedPtr<WindowsCursor> Cursor);
	void SetActiveWindow(TSharedPtr<WindowsWindow>& ActiveWindow);
	void SetCapture(TSharedPtr<WindowsWindow> CaptureWindow);

	ModifierKeyState			GetModifierKeyState()			const;
	TSharedPtr<WindowsWindow>	GetWindowFromHWND(HWND Window)	const;
	TSharedPtr<WindowsWindow>	GetActiveWindow()				const;
	TSharedPtr<WindowsWindow>	GetCapture()					const;

	void SetCursorPos(TSharedPtr<WindowsWindow>& RelativeWindow, Int32 X, Int32 Y);
	void GetCursorPos(TSharedPtr<WindowsWindow>& RelativeWindow, Int32& OutX, Int32& OutY) const;

	FORCEINLINE void SetEventHandler(TSharedPtr<ApplicationEventHandler> InMessageHandler)
	{
		MessageHandler = InMessageHandler;
	}

	FORCEINLINE TSharedPtr<ApplicationEventHandler> GetEventHandler() const
	{
		return MessageHandler;
	}

	FORCEINLINE HINSTANCE GetInstance() const
	{
		return InstanceHandle;
	}

	static WindowsApplication* Make(HINSTANCE hInstance);

private:
	WindowsApplication(HINSTANCE hInstance);
	
	bool Initialize();

	void AddWindow(TSharedPtr<WindowsWindow>& Window);

	bool RegisterWindowClass();

	LRESULT ApplicationProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

	static LRESULT MessageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

private:
	HINSTANCE InstanceHandle = 0;
	TSharedPtr<ApplicationEventHandler>	MessageHandler	= nullptr;
	TArray<TSharedPtr<WindowsWindow>> Windows;
};

extern WindowsApplication* GlobalWindowsApplication;