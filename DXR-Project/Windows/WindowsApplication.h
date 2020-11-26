#pragma once
#include "Application/InputCodes.h"
#include "Application/Events/ApplicationEventHandler.h"
#include "Application/Generic/GenericApplication.h"

#include "Windows.h"

class WindowsWindow;
class ApplicationEventHandler;
class WindowsCursor;

/*
* WindowsApplication
*/

class WindowsApplication : public GenericApplication
{
public:
	~WindowsApplication();

	TSharedRef<WindowsWindow> GetWindowFromHWND(HWND Window) const;

	FORCEINLINE HINSTANCE GetInstance() const
	{
		return InstanceHandle;
	}

public:
	// Generic application
	virtual TSharedRef<GenericWindow> MakeWindow() override final;
	virtual TSharedRef<GenericCursor> MakeCursor() override final;

	virtual bool Initialize() override final;
	virtual bool Tick() override final;
	
	virtual void SetCursor(TSharedRef<GenericCursor> Cursor) override final;
	virtual void SetActiveWindow(TSharedRef<GenericWindow> Window) override final;
	virtual void SetCapture(TSharedRef<GenericWindow> Window) override final;

	virtual ModifierKeyState GetModifierKeyState() const override final;
	virtual TSharedRef<GenericWindow> GetActiveWindow() const override final;
	virtual TSharedRef<GenericWindow> GetCapture() const override final;
	virtual TSharedRef<GenericCursor> GetCursor() const override final;
	
	virtual void SetCursorPos(TSharedRef<GenericWindow> RelativeWindow, int32 X, int32 Y) override final;
	virtual void GetCursorPos(TSharedRef<GenericWindow> RelativeWindow, int32& OutX, int32& OutY) const override final;
	
	static GenericApplication* Make();

private:
	WindowsApplication(HINSTANCE hInstance);
	
	void AddWindow(TSharedRef<WindowsWindow>& Window);

	bool RegisterWindowClass();

	LRESULT ApplicationProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
	static LRESULT MessageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

private:
	HINSTANCE InstanceHandle = 0;
	
	TSharedRef<WindowsCursor>			CurrentCursor;
	TArray<TSharedRef<WindowsWindow>>	Windows;
};

extern WindowsApplication* GlobalWindowsApplication;