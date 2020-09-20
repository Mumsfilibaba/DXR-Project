#include "GenericCursor.h"
#include "GenericApplication.h"

/*
* GlobalCursors
*/
TSharedPtr<GenericCursor> GlobalCursors::Arrow;
TSharedPtr<GenericCursor> GlobalCursors::TextInput;
TSharedPtr<GenericCursor> GlobalCursors::ResizeAll;
TSharedPtr<GenericCursor> GlobalCursors::ResizeEastWest;
TSharedPtr<GenericCursor> GlobalCursors::ResizeNorthSouth;
TSharedPtr<GenericCursor> GlobalCursors::ResizeNorthEastSouthWest;
TSharedPtr<GenericCursor> GlobalCursors::ResizeNorthWestSouthEast;
TSharedPtr<GenericCursor> GlobalCursors::Hand;
TSharedPtr<GenericCursor> GlobalCursors::NotAllowed;

bool GlobalCursors::Initialize()
{
	Arrow = GlobalPlatformApplication->MakeCursor();
	if (!Arrow->Initialize(CursorInitializer(EPlatformCursor::CURSOR_ARROW)))
	{
		return false;
	}

	TextInput = GlobalPlatformApplication->MakeCursor();
	if (!TextInput->Initialize(CursorInitializer(EPlatformCursor::CURSOR_TEXT_INPUT)))
	{
		return false;
	}

	ResizeAll = GlobalPlatformApplication->MakeCursor();
	if (!ResizeAll->Initialize(CursorInitializer(EPlatformCursor::CURSOR_RESIZE_ALL)))
	{
		return false;
	}

	ResizeEastWest = GlobalPlatformApplication->MakeCursor();
	if (!ResizeEastWest->Initialize(CursorInitializer(EPlatformCursor::CURSOR_RESIZE_EW)))
	{
		return false;
	}

	ResizeNorthSouth = GlobalPlatformApplication->MakeCursor();
	if (!ResizeNorthSouth->Initialize(CursorInitializer(EPlatformCursor::CURSOR_RESIZE_NS)))
	{
		return false;
	}

	ResizeNorthEastSouthWest = GlobalPlatformApplication->MakeCursor();
	if (!ResizeNorthEastSouthWest->Initialize(CursorInitializer(EPlatformCursor::CURSOR_RESIZE_NESW)))
	{
		return false;
	}

	ResizeNorthWestSouthEast = GlobalPlatformApplication->MakeCursor();
	if (!ResizeNorthWestSouthEast->Initialize(CursorInitializer(EPlatformCursor::CURSOR_RESIZE_NWSE)))
	{
		return false;
	}

	Hand = GlobalPlatformApplication->MakeCursor();
	if (!Hand->Initialize(CursorInitializer(EPlatformCursor::CURSOR_HAND)))
	{
		return false;
	}

	NotAllowed = GlobalPlatformApplication->MakeCursor();
	if (!NotAllowed->Initialize(CursorInitializer(EPlatformCursor::CURSOR_NOT_ALLOWED)))
	{
		return false;
	}

	return true;
}
