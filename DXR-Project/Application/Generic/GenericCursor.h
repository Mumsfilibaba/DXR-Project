#pragma once
#include "Defines.h"

#include "Core/RefCountedObject.h"

/*
* EPlatformCursor
*/

enum class EPlatformCursor : UInt32
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
		, IsPlatformCursor(false)
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

class GenericCursor : public RefCountedObject
{
public:
	virtual ~GenericCursor() = default;

	virtual bool Initialize(const CursorInitializer& InInitializer) = 0;

	virtual Void* GetNativeHandle() const
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
	static TSharedRef<GenericCursor> Arrow;
	static TSharedRef<GenericCursor> TextInput;
	static TSharedRef<GenericCursor> ResizeAll;
	static TSharedRef<GenericCursor> ResizeEastWest;
	static TSharedRef<GenericCursor> ResizeNorthSouth;
	static TSharedRef<GenericCursor> ResizeNorthEastSouthWest;
	static TSharedRef<GenericCursor> ResizeNorthWestSouthEast;
	static TSharedRef<GenericCursor> Hand;
	static TSharedRef<GenericCursor> NotAllowed;

	static bool Initialize();
};
