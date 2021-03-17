#pragma once
#include "Core.h"

#include "Core/RefCountedObject.h"
#include "Core/Ref.h"

enum class EPlatformCursor : uint32
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
    bool IsPlatformCursor;
};

class GenericCursor : public RefCountedObject
{
public:
    virtual ~GenericCursor() = default;

    virtual bool Init(const CursorCreateInfo& InCreateInfo) = 0;

    virtual void* GetNativeHandle() const { return nullptr; }

protected:
    CursorCreateInfo CreateInfo;
};

struct GlobalCursors
{
    static TRef<GenericCursor> Arrow;
    static TRef<GenericCursor> TextInput;
    static TRef<GenericCursor> ResizeAll;
    static TRef<GenericCursor> ResizeEW;
    static TRef<GenericCursor> ResizeNS;
    static TRef<GenericCursor> ResizeNESW;
    static TRef<GenericCursor> ResizeNWSE;
    static TRef<GenericCursor> Hand;
    static TRef<GenericCursor> NotAllowed;

    static bool Init();
    static void Release();
};
