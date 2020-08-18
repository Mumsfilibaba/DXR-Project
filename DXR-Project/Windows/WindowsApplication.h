#pragma once
#include "Windows.h"
#include "Defines.h"
#include "Types.h"

#include "Application/InputCodes.h"
#include "Application/EventHandler.h"

#include <memory>
#include <vector>

class WindowsWindow;
class EventHandler;
class WindowsCursor;

/*
* ModifierKeyState
*/
class ModifierKeyState
{
public:
	ModifierKeyState(Uint32 InModifierMask)
		: ModifierMask(InModifierMask)
	{
	}

	bool IsCtrlDown() const
	{
		return (ModifierMask & MODIFIER_FLAG_CTRL);
	}

	bool IsAltDown() const
	{
		return (ModifierMask & MODIFIER_FLAG_ALT);
	}

	bool IsShiftDown() const
	{
		return (ModifierMask & MODIFIER_FLAG_SHIFT);
	}

	bool IsCapsLockDown() const
	{
		return (ModifierMask & MODIFIER_FLAG_CAPS_LOCK);
	}

	bool IsSuperKeyDown() const
	{
		return (ModifierMask & MODIFIER_FLAG_SUPER);
	}

	bool IsNumPadDown() const
	{
		return (ModifierMask & MODIFIER_FLAG_NUM_LOCK);
	}

private:
	Uint32 ModifierMask = 0;
};

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

	FORCEINLINE void SetEventHandler(TSharedPtr<EventHandler> InMessageHandler)
	{
		MessageHandler = InMessageHandler;
	}

	FORCEINLINE TSharedPtr<EventHandler> GetEventHandler() const
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
	HINSTANCE					InstanceHandle	= 0;
	TSharedPtr<EventHandler>	MessageHandler	= nullptr;

	TArray<TSharedPtr<WindowsWindow>> Windows;
};

extern WindowsApplication* GlobalWindowsApplication;