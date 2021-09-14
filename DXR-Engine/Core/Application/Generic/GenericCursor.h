#pragma once
#include "Core.h"

#include "Core/RefCounted.h"
#include "Core/Containers/SharedRef.h"

#ifdef COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable
#endif

class GenericWindow;

class GenericCursor : public CRefCounted
{
public:
    virtual ~GenericCursor() = default;

    virtual void* GetNativeHandle() const
    {
        return nullptr;
    }

public:
    static TSharedRef<GenericCursor> Arrow;
    static TSharedRef<GenericCursor> TextInput;
    static TSharedRef<GenericCursor> ResizeAll;
    static TSharedRef<GenericCursor> ResizeEW;
    static TSharedRef<GenericCursor> ResizeNS;
    static TSharedRef<GenericCursor> ResizeNESW;
    static TSharedRef<GenericCursor> ResizeNWSE;
    static TSharedRef<GenericCursor> Hand;
    static TSharedRef<GenericCursor> NotAllowed;
};

#ifdef COMPILER_MSVC
#pragma warning(pop)
#endif