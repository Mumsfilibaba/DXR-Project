#pragma once
#include "Defines.h"
#include "Types.h"

/*
* EPlatformCursor
*/
enum class EPlatformCursor : Uint32
{
	CURSOR_NONE			= 0,
	CURSOR_TEXT_INPUT	= 1,
	CURSOR_RESIZE_ALL	= 2,
	CURSOR_RESIZE_EW	= 3,
	CURSOR_RESIZE_NS	= 4,
	CURSOR_RESIZE_NESW	= 5,
	CURSOR_RESIZE_NWSE	= 6,
	CURSOR_HAND			= 7,
	CURSOR_NOT_ALLOWED	= 8,
	CURSOR_ARROW		= 9,
};

/*
* GenericCursor
*/
struct CursorInitializer
{
	inline CursorInitializer()
		: PlatformCursor(EPlatformCursor::CURSOR_NONE)
	{
	}

	inline CursorInitializer(EPlatformCursor InPlatformCursor)
		: PlatformCursor(InPlatformCursor)
		, IsPlatformCursor(true)
	{
	}

	EPlatformCursor PlatformCursor;
	bool IsPlatformCursor;
};

/*
* GenericCursor
*/
class GenericCursor
{
public:
	GenericCursor() = default;
	~GenericCursor() = default;

	virtual bool Initialize(const CursorInitializer& InInitializer) = 0;

	virtual VoidPtr GetNativeHandle() const
	{
		return nullptr;
	}

protected:
	CursorInitializer Initializer;
};

/*
* Pre-Defined Cursors
*/
struct GlobalCursors
{
	static TSharedPtr<GenericCursor> Arrow;
	static TSharedPtr<GenericCursor> TextInput;
	static TSharedPtr<GenericCursor> ResizeAll;
	static TSharedPtr<GenericCursor> ResizeEastWest;
	static TSharedPtr<GenericCursor> ResizeNorthSouth;
	static TSharedPtr<GenericCursor> ResizeNorthEastSouthWest;
	static TSharedPtr<GenericCursor> ResizeNorthWestSouthEast;
	static TSharedPtr<GenericCursor> Hand;
	static TSharedPtr<GenericCursor> NotAllowed;

	static bool Initialize();
};
