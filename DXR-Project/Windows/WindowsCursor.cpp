#include "WindowsCursor.h"

/*
* WindowsCursor
*/
WindowsCursor::WindowsCursor(LPCSTR InCursorName)
	: CursorName(InCursorName)
{
	CursorHandle = ::LoadCursor(0, InCursorName);
}

WindowsCursor::WindowsCursor(HCURSOR InCursorHandle)
	: CursorHandle(InCursorHandle)
{
}

WindowsCursor::~WindowsCursor()
{
	if (!CursorName)
	{
		::DestroyCursor(CursorHandle);
	}
}

/*
* Initialize
*/

TSharedPtr<WindowsCursor> CursorArrow;
TSharedPtr<WindowsCursor> CursorTextInput;
TSharedPtr<WindowsCursor> CursorResizeAll;
TSharedPtr<WindowsCursor> CursorResizeEastWest;
TSharedPtr<WindowsCursor> CursorResizeNorthSouth;
TSharedPtr<WindowsCursor> CursorResizeNorthEastSouthWest;
TSharedPtr<WindowsCursor> CursorResizeNorthWestSouthEast;
TSharedPtr<WindowsCursor> CursorHand;
TSharedPtr<WindowsCursor> CursorNotAllowed;

void InitializeCursors()
{
	CursorArrow						= TSharedPtr<WindowsCursor>(new WindowsCursor(IDC_ARROW));
	CursorTextInput					= TSharedPtr<WindowsCursor>(new WindowsCursor(IDC_IBEAM));
	CursorResizeAll					= TSharedPtr<WindowsCursor>(new WindowsCursor(IDC_SIZEALL));
	CursorResizeEastWest			= TSharedPtr<WindowsCursor>(new WindowsCursor(IDC_SIZEWE));
	CursorResizeNorthSouth			= TSharedPtr<WindowsCursor>(new WindowsCursor(IDC_SIZENS));
	CursorResizeNorthEastSouthWest	= TSharedPtr<WindowsCursor>(new WindowsCursor(IDC_SIZENESW));
	CursorResizeNorthWestSouthEast	= TSharedPtr<WindowsCursor>(new WindowsCursor(IDC_SIZENWSE));
	CursorHand						= TSharedPtr<WindowsCursor>(new WindowsCursor(IDC_HAND));
	CursorNotAllowed				= TSharedPtr<WindowsCursor>(new WindowsCursor(IDC_NO));
}
