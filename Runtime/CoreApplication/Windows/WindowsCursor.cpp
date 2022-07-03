#include "WindowsCursor.h"
#include "WindowsWindow.h"
#include "Windows.h"

#include "Core/Containers/SharedRef.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FWindowsCursor

FWindowsCursor* FWindowsCursor::CreateWindowsCursor()
{
    return dbg_new FWindowsCursor();
}

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

void FWindowsCursor::SetPosition(FGenericWindow* RelativeWindow, int32 x, int32 y) const
{
    POINT CursorPos = { x, y };
    if (RelativeWindow)
    {
        TSharedRef<FWindowsWindow> WinWindow = MakeSharedRef<FWindowsWindow>(RelativeWindow);

        HWND hRelative = WinWindow->GetWindowHandle();
        if (!ClientToScreen(hRelative, &CursorPos))
        {
            return;
        }
    }

    ::SetCursorPos(CursorPos.x, CursorPos.y);
}

void FWindowsCursor::GetPosition(FGenericWindow* RelativeWindow, int32& OutX, int32& OutY) const
{
    POINT CursorPos = { };
    if (!GetCursorPos(&CursorPos))
    {
        return;
    }

    if (RelativeWindow)
    {
        TSharedRef<FWindowsWindow> WinRelative = MakeSharedRef<FWindowsWindow>(RelativeWindow);

        HWND Relative = WinRelative->GetWindowHandle();
        if (!ScreenToClient(Relative, &CursorPos))
        {
            return;
        }
    }

    OutX = CursorPos.x;
    OutY = CursorPos.y;
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
