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

	virtual bool Initialize() override final;
	virtual bool Tick() override final;

	virtual TSharedPtr<GenericWindow> MakeWindow() override final;
	virtual TSharedPtr<GenericCursor> MakeCursor() override final;
	
	virtual void SetCursor(TSharedPtr<GenericCursor> Cursor) override final;
	virtual void SetActiveWindow(TSharedPtr<GenericWindow> Window) override final;
	virtual void SetCapture(TSharedPtr<GenericWindow> Window) override final;

	virtual ModifierKeyState GetModifierKeyState() const override final;
	virtual TSharedPtr<GenericWindow> GetActiveWindow() const override final;
	virtual TSharedPtr<GenericWindow> GetCapture() const override final;
	virtual TSharedPtr<GenericCursor> GetCursor() const override final;
	
	TSharedPtr<WindowsWindow> GetWindowFromHWND(HWND Window) const;

	virtual void SetCursorPos(TSharedPtr<GenericWindow> RelativeWindow, Int32 X, Int32 Y) override final;
	virtual void GetCursorPos(TSharedPtr<GenericWindow> RelativeWindow, Int32& OutX, Int32& OutY) const override final;

	FORCEINLINE HINSTANCE GetInstance() const
	{
		return InstanceHandle;
	}

	static GenericApplication* Make(HINSTANCE hInstance);

private:
	WindowsApplication(HINSTANCE hInstance);
	
	void AddWindow(TSharedPtr<WindowsWindow>& Window);

	bool RegisterWindowClass();

	LRESULT ApplicationProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
	static LRESULT MessageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

private:
	HINSTANCE InstanceHandle = 0;
	TSharedPtr<WindowsCursor> CurrentCursor;
	TArray<TSharedPtr<WindowsWindow>> Windows;
};

extern WindowsApplication* GlobalWindowsApplication;