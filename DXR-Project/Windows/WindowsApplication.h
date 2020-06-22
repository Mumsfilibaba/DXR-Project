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

	std::shared_ptr<WindowsWindow> CreateWindow(Uint16 InWidth, Uint16 InHeight);

	bool Tick();
	
	void SetCursor(std::shared_ptr<WindowsCursor> InCursor);
	void SetActiveWindow(std::shared_ptr<WindowsWindow>& InActiveWindow);
	void SetCapture(std::shared_ptr<WindowsWindow> InCaptureWindow);

	ModifierKeyState				GetModifierKeyState()			const;
	std::shared_ptr<WindowsWindow>	GetWindowFromHWND(HWND hWindow)	const;
	std::shared_ptr<WindowsWindow>	GetActiveWindow()				const;
	std::shared_ptr<WindowsWindow>	GetCapture()					const;

	void SetCursorPos(std::shared_ptr<WindowsWindow>& InRelativeWindow, Int32 InX, Int32 InY);
	void GetCursorPos(std::shared_ptr<WindowsWindow>& InRelativeWindow, Int32& OutX, Int32& OutY) const;

	FORCEINLINE void SetEventHandler(std::shared_ptr<EventHandler>& InEventHandler)
	{
		MessageEventHandler = InEventHandler;
	}

	FORCEINLINE std::shared_ptr<EventHandler> GetEventHandler() const
	{
		return MessageEventHandler;
	}

	FORCEINLINE HINSTANCE GetInstance() const
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