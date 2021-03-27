#pragma once
#include "Core.h"

#include "Core/RefCountedObject.h"
#include "Core/Ref.h"

#ifdef COMPILER_VISUAL_STUDIO
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#endif

class Window;

class Cursor : public RefCountedObject
{
public:
    virtual ~Cursor() = default;

    virtual void* GetNativeHandle() const { return nullptr; }

public:
    static TRef<Cursor> Arrow;
    static TRef<Cursor> TextInput;
    static TRef<Cursor> ResizeAll;
    static TRef<Cursor> ResizeEW;
    static TRef<Cursor> ResizeNS;
    static TRef<Cursor> ResizeNESW;
    static TRef<Cursor> ResizeNWSE;
    static TRef<Cursor> Hand;
    static TRef<Cursor> NotAllowed;
};

#ifdef COMPILER_VISUAL_STUDIO
    #pragma warning(pop)
#endif