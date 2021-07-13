#pragma once
#include "Core/Application/Generic/GenericCursor.h"

#include "Windows.h"

class GenericWindow;

class WindowsCursor : public GenericCursor
{
public:
    WindowsCursor();
    ~WindowsCursor();

    bool Init( LPCSTR InCursorName );

    virtual void* GetNativeHandle() const override final
    {
        return reinterpret_cast<void*>(Cursor);
    }

    HCURSOR GetHandle() const
    {
        return Cursor;
    }

    static GenericCursor* Create( LPCSTR CursorName );

private:
    HCURSOR Cursor;
    LPCSTR  CursorName;
};