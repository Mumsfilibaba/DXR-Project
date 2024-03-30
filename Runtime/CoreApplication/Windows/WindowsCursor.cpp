#include "WindowsCursor.h"
#include "WindowsWindow.h"
#include "Core/Windows/Windows.h"
#include "Core/Containers/SharedRef.h"

void FWindowsCursor::SetCursor(ECursor Cursor)
{
    LPSTR CursorName = NULL;
    switch (Cursor)
    {
    case ECursor::Arrow:
        CursorName = IDC_ARROW;
        break;
    case ECursor::TextInput:
        CursorName = IDC_IBEAM;
        break;
    case ECursor::ResizeAll:
        CursorName = IDC_SIZEALL;
        break;
    case ECursor::ResizeEW:
        CursorName = IDC_SIZEWE;
        break;
    case ECursor::ResizeNS:
        CursorName = IDC_SIZENS;
        break;
    case ECursor::ResizeNESW:
        CursorName = IDC_SIZENESW;
        break;
    case ECursor::ResizeNWSE:
        CursorName = IDC_SIZENWSE;
        break;
    case ECursor::Hand:
        CursorName = IDC_HAND;
        break;
    case ECursor::NotAllowed:
        CursorName = IDC_NO;
        break;
    default:
        CursorName = NULL;
        break;
    }

    HCURSOR CursorHandle = LoadCursor(NULL, CursorName);
    if (CursorHandle)
    {
        ::SetCursor(CursorHandle);
    }

    // TODO: Log error
}

void FWindowsCursor::SetPosition(int32 x, int32 y) const
{
    ::SetCursorPos(x, y);
}

FIntVector2 FWindowsCursor::GetPosition() const
{
    POINT CursorPos = { 0, 0 };
    ::GetCursorPos(&CursorPos);
    return FIntVector2(CursorPos.x, CursorPos.y);
}

void FWindowsCursor::SetVisibility(bool bVisible)
{
    // TODO: Investigate if we need to do more in order to keep track of the ShowCursor calls

    if (bVisible)
    {
        if (!bIsVisible)
        {
            ShowCursor(true);
            bIsVisible = true;
        }
    }
    else
    {
        if (bIsVisible)
        {
            ShowCursor(false);
            bIsVisible = false;
        }
    }
}
