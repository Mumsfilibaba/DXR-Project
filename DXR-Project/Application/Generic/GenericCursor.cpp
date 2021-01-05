#include "GenericCursor.h"
#include "GenericApplication.h"

/*
* GlobalCursors
*/

TSharedRef<GenericCursor> GlobalCursors::Arrow;
TSharedRef<GenericCursor> GlobalCursors::TextInput;
TSharedRef<GenericCursor> GlobalCursors::ResizeAll;
TSharedRef<GenericCursor> GlobalCursors::ResizeEW;
TSharedRef<GenericCursor> GlobalCursors::ResizeNS;
TSharedRef<GenericCursor> GlobalCursors::ResizeNESW;
TSharedRef<GenericCursor> GlobalCursors::ResizeNWSE;
TSharedRef<GenericCursor> GlobalCursors::Hand;
TSharedRef<GenericCursor> GlobalCursors::NotAllowed;

bool GlobalCursors::Initialize()
{
	Arrow = GlobalPlatformApplication->MakeCursor();
	if (!Arrow->Initialize(CursorCreateInfo(EPlatformCursor::PlatformCursor_Arrow)))
	{
		return false;
	}

	TextInput = GlobalPlatformApplication->MakeCursor();
	if (!TextInput->Initialize(CursorCreateInfo(EPlatformCursor::PlatformCursor_TextInput)))
	{
		return false;
	}

	ResizeAll = GlobalPlatformApplication->MakeCursor();
	if (!ResizeAll->Initialize(CursorCreateInfo(EPlatformCursor::PlatformCursor_ResizeAll)))
	{
		return false;
	}

	ResizeEW = GlobalPlatformApplication->MakeCursor();
	if (!ResizeEW->Initialize(CursorCreateInfo(EPlatformCursor::PlatformCursor_ResizeEW)))
	{
		return false;
	}

	ResizeNS = GlobalPlatformApplication->MakeCursor();
	if (!ResizeNS->Initialize(CursorCreateInfo(EPlatformCursor::PlatformCursor_ResizeNS)))
	{
		return false;
	}

	ResizeNESW = GlobalPlatformApplication->MakeCursor();
	if (!ResizeNESW->Initialize(CursorCreateInfo(EPlatformCursor::PlatformCursor_ResizeNESW)))
	{
		return false;
	}

	ResizeNWSE = GlobalPlatformApplication->MakeCursor();
	if (!ResizeNWSE->Initialize(CursorCreateInfo(EPlatformCursor::PlatformCursor_ResizeNWSE)))
	{
		return false;
	}

	Hand = GlobalPlatformApplication->MakeCursor();
	if (!Hand->Initialize(CursorCreateInfo(EPlatformCursor::PlatformCursor_Hand)))
	{
		return false;
	}

	NotAllowed = GlobalPlatformApplication->MakeCursor();
	if (!NotAllowed->Initialize(CursorCreateInfo(EPlatformCursor::PlatformCursor_NotAllowed)))
	{
		return false;
	}

	return true;
}

void GlobalCursors::Release()
{
	Arrow.Reset();
	TextInput.Reset();
	ResizeAll.Reset();
	ResizeEW.Reset();
	ResizeNS.Reset();
	ResizeNESW.Reset();
	ResizeNWSE.Reset();
	Hand.Reset();
	NotAllowed.Reset();
}
