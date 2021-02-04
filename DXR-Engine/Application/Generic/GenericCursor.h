#pragma once
#include "Core.h"

#include "Core/RefCountedObject.h"
#include "Core/TSharedRef.h"

enum class EPlatformCursor : UInt32
{
    None       = 0,
    TextInput  = 1,
    ResizeAll  = 2,
    ResizeEW   = 3,
    ResizeNS   = 4,
    ResizeNESW = 5,
    ResizeNWSE = 6,
    Hand       = 7,
    NotAllowed = 8,
    Arrow      = 9,
};

struct CursorCreateInfo
{
    CursorCreateInfo()
        : PlatformCursor(EPlatformCursor::None)
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

class GenericCursor : public RefCountedObject
{
public:
    virtual ~GenericCursor() = default;

    virtual Bool Init(const CursorCreateInfo& InCreateInfo) = 0;

    virtual Void* GetNativeHandle() const { return nullptr; }

protected:
    CursorCreateInfo CreateInfo;
};

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
