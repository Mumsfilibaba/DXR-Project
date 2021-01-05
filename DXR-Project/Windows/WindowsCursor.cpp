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

bool WindowsCursor::Initialize(const CursorCreateInfo& InCreateInfo)
{
	if (InCreateInfo.IsPlatformCursor)
	{
		switch (InCreateInfo.PlatformCursor)
		{
		case EPlatformCursor::PlatformCursor_Arrow:			CursorName = IDC_ARROW;		break;
		case EPlatformCursor::PlatformCursor_Hand:			CursorName = IDC_HAND;		break;
		case EPlatformCursor::PlatformCursor_NotAllowed:	CursorName = IDC_NO;		break;
		case EPlatformCursor::PlatformCursor_ResizeAll:		CursorName = IDC_SIZEALL;	break;
		case EPlatformCursor::PlatformCursor_ResizeEW:		CursorName = IDC_SIZEWE;	break;
		case EPlatformCursor::PlatformCursor_ResizeNS:		CursorName = IDC_SIZENS;	break;
		case EPlatformCursor::PlatformCursor_ResizeNESW:	CursorName = IDC_SIZENESW;	break;
		case EPlatformCursor::PlatformCursor_ResizeNWSE:	CursorName = IDC_SIZENWSE;	break;
		case EPlatformCursor::PlatformCursor_TextInput:		CursorName = IDC_IBEAM;		break;
		default:											CursorName = nullptr;		break;
		}

		CreateInfo = InCreateInfo;
		if (CursorName)
		{
			hCursor = ::LoadCursor(0, CursorName);
			return true;
		}
		else
		{
			return false;
		}
	}

	return false;
}