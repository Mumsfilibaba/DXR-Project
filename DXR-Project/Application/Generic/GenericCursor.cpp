#include "GenericCursor.h"
#include "GenericApplication.h"

#include "Engine/EngineGlobals.h"

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
	Arrow = EngineGlobals::PlatformApplication->MakeCursor();
	if (!Arrow->Initialize(CursorInitializer(EPlatformCursor::CURSOR_ARROW)))
	{
		return false;
	}

	TextInput = EngineGlobals::PlatformApplication->MakeCursor();
	if (!TextInput->Initialize(CursorInitializer(EPlatformCursor::CURSOR_TEXT_INPUT)))
	{
		return false;
	}

	ResizeAll = EngineGlobals::PlatformApplication->MakeCursor();
	if (!ResizeAll->Initialize(CursorInitializer(EPlatformCursor::CURSOR_RESIZE_ALL)))
	{
		return false;
	}

	ResizeEastWest = EngineGlobals::PlatformApplication->MakeCursor();
	if (!ResizeEastWest->Initialize(CursorInitializer(EPlatformCursor::CURSOR_RESIZE_EW)))
	{
		return false;
	}

	ResizeNorthSouth = EngineGlobals::PlatformApplication->MakeCursor();
	if (!ResizeNorthSouth->Initialize(CursorInitializer(EPlatformCursor::CURSOR_RESIZE_NS)))
	{
		return false;
	}

	ResizeNorthEastSouthWest = EngineGlobals::PlatformApplication->MakeCursor();
	if (!ResizeNorthEastSouthWest->Initialize(CursorInitializer(EPlatformCursor::CURSOR_RESIZE_NESW)))
	{
		return false;
	}

	ResizeNorthWestSouthEast = EngineGlobals::PlatformApplication->MakeCursor();
	if (!ResizeNorthWestSouthEast->Initialize(CursorInitializer(EPlatformCursor::CURSOR_RESIZE_NWSE)))
	{
		return false;
	}

	Hand = EngineGlobals::PlatformApplication->MakeCursor();
	if (!Hand->Initialize(CursorInitializer(EPlatformCursor::CURSOR_HAND)))
	{
		return false;
	}

	NotAllowed = EngineGlobals::PlatformApplication->MakeCursor();
	if (!NotAllowed->Initialize(CursorInitializer(EPlatformCursor::CURSOR_NOT_ALLOWED)))
	{
		return false;
	}

	return true;
}
