#pragma once
#include "Core/Application/Generic/Cursor.h"

#include "Windows.h"

class Window;

class WindowsCursor : public Cursor
{
public:
    WindowsCursor();
    ~WindowsCursor();

    bool Init(LPCSTR InCursorName);

    virtual void* GetNativeHandle() const override final
    {
        return reinterpret_cast<void*>(hCursor);
    }

    HCURSOR GetHandle() const { return hCursor; }

    static Cursor* Create(LPCSTR CursorName);

private:
    HCURSOR hCursor;
    LPCSTR  CursorName;
};