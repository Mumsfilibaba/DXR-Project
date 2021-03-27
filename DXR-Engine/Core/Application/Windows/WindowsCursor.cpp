#include "WindowsCursor.h"

Cursor* WindowsCursor::Create(LPCSTR CursorName)
{
    TRef<WindowsCursor> NewCursor = DBG_NEW WindowsCursor();
    if (!NewCursor->Init(CursorName))
    {
        return nullptr;
    }
    else
    {
        return NewCursor.ReleaseOwnership();
    }
}

WindowsCursor::WindowsCursor()
    : Cursor()
    , hCursor(0)
    , CursorName(nullptr)
{
}

WindowsCursor::~WindowsCursor()
{
    if (!CursorName)
    {
        DestroyCursor(hCursor);
    }
}

bool WindowsCursor::Init(LPCSTR InCursorName)
{
    CursorName = InCursorName;
    if (CursorName)
    {
        hCursor = LoadCursor(0, CursorName);
        return true;
    }
    else
    {
        return false;
    }
}