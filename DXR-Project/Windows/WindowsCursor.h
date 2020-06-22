#pragma once
#include "Windows.h"

#include <memory>

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
		return Cursor;
	}

private:
	HCURSOR Cursor		= 0;
	LPCSTR	CursorName	= nullptr;
};

/*
* Pre-Defined Cursors
*/
extern std::shared_ptr<WindowsCursor> CursorArrow;
extern std::shared_ptr<WindowsCursor> CursorTextInput;
extern std::shared_ptr<WindowsCursor> CursorResizeAll;
extern std::shared_ptr<WindowsCursor> CursorResizeEastWest;
extern std::shared_ptr<WindowsCursor> CursorResizeNorthSouth;
extern std::shared_ptr<WindowsCursor> CursorResizeNorthEastSouthWest;
extern std::shared_ptr<WindowsCursor> CursorResizeNorthWestSouthEast;
extern std::shared_ptr<WindowsCursor> CursorHand;
extern std::shared_ptr<WindowsCursor> CursorNotAllowed;

void InitializeCursors();