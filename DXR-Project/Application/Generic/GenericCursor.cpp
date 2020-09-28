#include "GenericCursor.h"
#include "GenericApplication.h"

#include "Engine/EngineGlobals.h"

/*
* GlobalCursors
*/
TSharedRef<GenericCursor> GlobalCursors::Arrow;
TSharedRef<GenericCursor> GlobalCursors::TextInput;
TSharedRef<GenericCursor> GlobalCursors::ResizeAll;
TSharedRef<GenericCursor> GlobalCursors::ResizeEastWest;
TSharedRef<GenericCursor> GlobalCursors::ResizeNorthSouth;
TSharedRef<GenericCursor> GlobalCursors::ResizeNorthEastSouthWest;
TSharedRef<GenericCursor> GlobalCursors::ResizeNorthWestSouthEast;
TSharedRef<GenericCursor> GlobalCursors::Hand;
TSharedRef<GenericCursor> GlobalCursors::NotAllowed;

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
