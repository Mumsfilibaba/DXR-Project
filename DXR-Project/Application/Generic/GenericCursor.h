#pragma once
#include "Core.h"

#include "Core/RefCountedObject.h"

/*
* EPlatformCursor
*/

enum class EPlatformCursor : UInt32
{
	PlatformCursor_None			= 0,
	PlatformCursor_TextInput	= 1,
	PlatformCursor_ResizeAll	= 2,
	PlatformCursor_ResizeEW		= 3,
	PlatformCursor_ResizeNS		= 4,
	PlatformCursor_ResizeNESW	= 5,
	PlatformCursor_ResizeNWSE	= 6,
	PlatformCursor_Hand			= 7,
	PlatformCursor_NotAllowed	= 8,
	PlatformCursor_Arrow		= 9,
};

/*
* GenericCursor
*/

struct CursorCreateInfo
{
	CursorCreateInfo()
		: PlatformCursor(EPlatformCursor::PlatformCursor_None)
		, IsPlatformCursor(false)
	{
	}

	CursorCreateInfo(EPlatformCursor InPlatformCursor)
		: PlatformCursor(InPlatformCursor)
		, IsPlatformCursor(true)
	{
	}

	EPlatformCursor PlatformCursor;
	Bool IsPlatformCursor;
};

/*
* GenericCursor
*/

class GenericCursor : public RefCountedObject
{
public:
	virtual ~GenericCursor() = default;

	virtual Bool Init(const CursorCreateInfo& InCreateInfo) = 0;

	virtual Void* GetNativeHandle() const
	{
		return nullptr;
	}

protected:
	CursorCreateInfo CreateInfo;
};

/*
* Pre-Defined Cursors
*/

struct GlobalCursors
{
	static TSharedRef<GenericCursor> Arrow;
	static TSharedRef<GenericCursor> TextInput;
	static TSharedRef<GenericCursor> ResizeAll;
	static TSharedRef<GenericCursor> ResizeEW;
	static TSharedRef<GenericCursor> ResizeNS;
	static TSharedRef<GenericCursor> ResizeNESW;
	static TSharedRef<GenericCursor> ResizeNWSE;
	static TSharedRef<GenericCursor> Hand;
	static TSharedRef<GenericCursor> NotAllowed;

	static Bool Init();
	static void Release();
};
