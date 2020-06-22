#include "WindowsCursor.h"

/*
* WindowsCursor
*/
WindowsCursor::WindowsCursor(LPCSTR InCursorName)
	: CursorName(InCursorName)
{
	Cursor = ::LoadCursor(0, InCursorName);
}

WindowsCursor::WindowsCursor(HCURSOR InCursorHandle)
	: Cursor(InCursorHandle)
{
}

WindowsCursor::~WindowsCursor()
{
	if (!CursorName)
	{
		::DestroyCursor(Cursor);
	}
}

/*
* Initialize
*/

std::shared_ptr<WindowsCursor> CursorArrow;
std::shared_ptr<WindowsCursor> CursorTextInput;
std::shared_ptr<WindowsCursor> CursorResizeAll;
std::shared_ptr<WindowsCursor> CursorResizeEastWest;
std::shared_ptr<WindowsCursor> CursorResizeNorthSouth;
std::shared_ptr<WindowsCursor> CursorResizeNorthEastSouthWest;
std::shared_ptr<WindowsCursor> CursorResizeNorthWestSouthEast;
std::shared_ptr<WindowsCursor> CursorHand;
std::shared_ptr<WindowsCursor> CursorNotAllowed;

void InitializeCursors()
{
	CursorArrow						= std::shared_ptr<WindowsCursor>(new WindowsCursor(IDC_ARROW));
	CursorTextInput					= std::shared_ptr<WindowsCursor>(new WindowsCursor(IDC_IBEAM));
	CursorResizeAll					= std::shared_ptr<WindowsCursor>(new WindowsCursor(IDC_SIZEALL));
	CursorResizeEastWest			= std::shared_ptr<WindowsCursor>(new WindowsCursor(IDC_SIZEWE));
	CursorResizeNorthSouth			= std::shared_ptr<WindowsCursor>(new WindowsCursor(IDC_SIZENS));
	CursorResizeNorthEastSouthWest	= std::shared_ptr<WindowsCursor>(new WindowsCursor(IDC_SIZENESW));
	CursorResizeNorthWestSouthEast	= std::shared_ptr<WindowsCursor>(new WindowsCursor(IDC_SIZENWSE));
	CursorHand						= std::shared_ptr<WindowsCursor>(new WindowsCursor(IDC_HAND));
	CursorNotAllowed				= std::shared_ptr<WindowsCursor>(new WindowsCursor(IDC_NO));
}
