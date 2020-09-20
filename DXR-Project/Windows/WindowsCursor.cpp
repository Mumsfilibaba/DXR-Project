#include "WindowsCursor.h"
#include "WindowsApplication.h"

/*
* WindowsCursor
*/
WindowsCursor::WindowsCursor(WindowsApplication* InApplication)
	: Application(InApplication)
	, hCursor(0)
	, CursorName(nullptr)
{
}

WindowsCursor::~WindowsCursor()
{
	if (!CursorName)
	{
		::DestroyCursor(hCursor);
	}
}

bool WindowsCursor::Initialize(const CursorInitializer& InInitializer)
{
	if (InInitializer.IsPlatformCursor)
	{
		switch (InInitializer.PlatformCursor)
		{
		case EPlatformCursor::CURSOR_ARROW:			CursorName = IDC_ARROW;		break;
		case EPlatformCursor::CURSOR_HAND:			CursorName = IDC_HAND;		break;
		case EPlatformCursor::CURSOR_NOT_ALLOWED:	CursorName = IDC_NO;		break;
		case EPlatformCursor::CURSOR_RESIZE_ALL:	CursorName = IDC_SIZEALL;	break;
		case EPlatformCursor::CURSOR_RESIZE_EW:		CursorName = IDC_SIZEWE;	break;
		case EPlatformCursor::CURSOR_RESIZE_NS:		CursorName = IDC_SIZENS;	break;
		case EPlatformCursor::CURSOR_RESIZE_NESW:	CursorName = IDC_SIZENESW;	break;
		case EPlatformCursor::CURSOR_RESIZE_NWSE:	CursorName = IDC_SIZENWSE;	break;
		case EPlatformCursor::CURSOR_TEXT_INPUT:	CursorName = IDC_IBEAM;		break;
		default:									CursorName = nullptr;		break;
		}

		Initializer = InInitializer;
		if (CursorName)
		{
			HINSTANCE hInstance = Application->GetInstance();
			::LoadCursor(hInstance, CursorName);
			return true;
		}
		else
		{
			return false;
		}
	}

	return false;
}