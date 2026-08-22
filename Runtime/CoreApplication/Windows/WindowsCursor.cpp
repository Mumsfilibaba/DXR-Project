#include "Core/Windows/Windows.h"
#include "Core/Containers/SharedRef.h"
#include "CoreApplication/Windows/WindowsCursor.h"
#include "CoreApplication/Windows/WindowsWindow.h"

static LPSTR ResolveCursorName(ECursor Cursor)
{
    switch (Cursor)
    {
    case ECursor::TextInput:
        return IDC_IBEAM;

    case ECursor::ResizeAll:
        return IDC_SIZEALL;

    case ECursor::ResizeEW:
        return IDC_SIZEWE;

    case ECursor::ResizeNS:
        return IDC_SIZENS;

    case ECursor::ResizeNESW:
        return IDC_SIZENESW;

    case ECursor::ResizeNWSE:
        return IDC_SIZENWSE;

    case ECursor::Hand:
        return IDC_HAND;

    case ECursor::NotAllowed:
        return IDC_NO;

    default:
        return IDC_ARROW;
    }
}

FWindowsCursor::FWindowsCursor()
    : CurrentCursor(ECursor::Arrow)
    , bIsVisible(true)
{
}

void FWindowsCursor::SetCursor(ECursor Cursor)
{
    CurrentCursor = Cursor;

    if (Cursor == ECursor::None)
    {
        ::SetCursor(nullptr);
        return;
    }

    HCURSOR CursorHandle = ::LoadCursor(nullptr, ResolveCursorName(Cursor));
    if (!CursorHandle)
    {
        CursorHandle = ::LoadCursor(nullptr, IDC_ARROW);
    }

    ::SetCursor(CursorHandle);
}

void FWindowsCursor::SetPosition(int32 x, int32 y)
{
    ::SetCursorPos(x, y);
}

IntVector2 FWindowsCursor::GetPosition() const
{
    POINT CursorPos = { 0, 0 };
    ::GetCursorPos(&CursorPos);
    return IntVector2(CursorPos.x, CursorPos.y);
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
