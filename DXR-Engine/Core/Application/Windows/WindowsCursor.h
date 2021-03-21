#pragma once
#include "Core/Application/Generic/GenericCursor.h"

#include "Windows.h"

class GenericWindow;
class WindowsApplication;

class WindowsCursor : public GenericCursor
{
public:
    WindowsCursor();
    ~WindowsCursor();

    bool Init(LPCSTR InCursorName);

    virtual void* GetNativeHandle() const override final
    {
        return reinterpret_cast<void*>(Cursor);
    }

    static bool InitSystemCursors();

    static void SetCursor(GenericCursor* Cursor);
    static GenericCursor* GetCursor();

    static void SetCursorPos(GenericWindow* RelativeWindow, int32 x, int32 y);
    static void GetCursorPos(GenericWindow* RelativeWindow, int32& OutX, int32& OutY);

    HCURSOR GetHandle() const { return Cursor; }

private:
    HCURSOR Cursor;
    LPCSTR  CursorName;

    static TRef<WindowsCursor> CurrentCursor;
};