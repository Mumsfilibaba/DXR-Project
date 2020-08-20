#pragma once
#include "Windows.h"

/*
* WindowsCursor
*/
class WindowsCursor
{
public:
	WindowsCursor(LPCSTR InCursorName);
	WindowsCursor(HCURSOR InCursorHandle);
	~WindowsCursor();

	FORCEINLINE HCURSOR GetCursor() const
	{
		return CursorHandle;
	}

private:
	HCURSOR CursorHandle	= 0;
	LPCSTR	CursorName		= nullptr;
};

/*
* Pre-Defined Cursors
*/
extern TSharedPtr<WindowsCursor> CursorArrow;
extern TSharedPtr<WindowsCursor> CursorTextInput;
extern TSharedPtr<WindowsCursor> CursorResizeAll;
extern TSharedPtr<WindowsCursor> CursorResizeEastWest;
extern TSharedPtr<WindowsCursor> CursorResizeNorthSouth;
extern TSharedPtr<WindowsCursor> CursorResizeNorthEastSouthWest;
extern TSharedPtr<WindowsCursor> CursorResizeNorthWestSouthEast;
extern TSharedPtr<WindowsCursor> CursorHand;
extern TSharedPtr<WindowsCursor> CursorNotAllowed;

void InitializeCursors();