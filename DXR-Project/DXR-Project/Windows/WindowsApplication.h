#pragma once
#include "Windows.h"

class WindowsApplication
{
public:
	static WindowsApplication* Create(HINSTANCE hInstance);
	static WindowsApplication* Get();

private:
	WindowsApplication(WindowsApplication&& Other)		= delete;
	WindowsApplication(const WindowsApplication& Other)	= delete;

	WindowsApplication& operator=(WindowsApplication&& Other)		= delete;
	WindowsApplication& operator=(const WindowsApplication& Other)	= delete;

	WindowsApplication(HINSTANCE hInstance);
	~WindowsApplication();

	LRESULT ApplicationProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

	static LRESULT MessageProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

private:
	HINSTANCE hInstance = 0;

	static WindowsApplication* WinApplication;
};