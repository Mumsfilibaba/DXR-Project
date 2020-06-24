#pragma once
#include "Windows.h"
#include "Defines.h"
#include "Types.h"

#include "Application/InputCodes.h"

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

	std::shared_ptr<WindowsWindow> MakeWindow(const struct WindowProperties& Properties);

	bool Tick();
	
	void SetCursor(std::shared_ptr<WindowsCursor> Cursor);
	void SetActiveWindow(std::shared_ptr<WindowsWindow>& ActiveWindow);
	void SetCapture(std::shared_ptr<WindowsWindow> CaptureWindow);

	ModifierKeyState				GetModifierKeyState()			const;
	std::shared_ptr<WindowsWindow>	GetWindowFromHWND(HWND Window)	const;
	std::shared_ptr<WindowsWindow>	GetActiveWindow()				const;
	std::shared_ptr<WindowsWindow>	GetCapture()					const;

	void SetCursorPos(std::shared_ptr<WindowsWindow>& RelativeWindow, Int32 X, Int32 Y);
	void GetCursorPos(std::shared_ptr<WindowsWindow>& RelativeWindow, Int32& OutX, Int32& OutY) const;

	FORCEINLINE void SetEventHandler(std::shared_ptr<EventHandler>& InMessageHandler)
	{
		MessageHandler = InMessageHandler;
	}

	FORCEINLINE std::shared_ptr<EventHandler> GetEventHandler() const
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

	void AddWindow(std::shared_ptr<WindowsWindow>& Window);

	bool RegisterWindowClass();

	LRESULT ApplicationProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

	static LRESULT MessageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

private:
	HINSTANCE						InstanceHandle			= 0;
	std::shared_ptr<EventHandler>	MessageHandler	= nullptr;

	std::vector<std::shared_ptr<WindowsWindow>> Windows;
};

extern WindowsApplication* GlobalWindowsApplication;