#pragma once
#include "Core.h"

#include "Core/RefCountedObject.h"
#include "Core/Ref.h"

#ifdef COMPILER_VISUAL_STUDIO
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#endif

class GenericWindow;

class GenericCursor : public RefCountedObject
{
public:
    virtual ~GenericCursor() = default;

    virtual void* GetNativeHandle() const { return nullptr; }

    static bool InitSystemCursors() { return false; }
    static void ReleaseSystemCursors();

    static void SetCursor(GenericCursor* Cursor) { }
    static GenericCursor* GetCursor() { return nullptr; }

    static void SetCursorPos(GenericWindow* RelativeWindow, int32 x, int32 y) {}
    static void GetCursorPos(GenericWindow* RelativeWindow, int32& OutX, int32& OutY) {}

public:
    static TRef<GenericCursor> Arrow;
    static TRef<GenericCursor> TextInput;
    static TRef<GenericCursor> ResizeAll;
    static TRef<GenericCursor> ResizeEW;
    static TRef<GenericCursor> ResizeNS;
    static TRef<GenericCursor> ResizeNESW;
    static TRef<GenericCursor> ResizeNWSE;
    static TRef<GenericCursor> Hand;
    static TRef<GenericCursor> NotAllowed;
};

#ifdef COMPILER_VISUAL_STUDIO
    #pragma warning(pop)
#endif