#include "WindowsCursor.h"
#include "WindowsApplication.h"

static GenericCursor* CreateCursor(LPCSTR CursorName)
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

TRef<WindowsCursor> WindowsCursor::CurrentCursor;

WindowsCursor::WindowsCursor()
    : GenericCursor()
    , Cursor(0)
    , CursorName(nullptr)
{
}

WindowsCursor::~WindowsCursor()
{
    if (!CursorName)
    {
        DestroyCursor(Cursor);
    }
}

bool WindowsCursor::InitSystemCursors()
{
    if (!(Arrow = CreateCursor(IDC_ARROW)))
    {
        return false;
    }
    if (!(TextInput = CreateCursor(IDC_IBEAM)))
    {
        return false;
    }
    if (!(ResizeAll = CreateCursor(IDC_SIZEALL)))
    {
        return false;
    }
    if (!(ResizeEW = CreateCursor(IDC_SIZEWE)))
    {
        return false;
    }
    if (!(ResizeNS = CreateCursor(IDC_SIZENS)))
    {
        return false;
    }
    if (!(ResizeNESW = CreateCursor(IDC_SIZENESW)))
    {
        return false;
    }
    if (!(ResizeNWSE = CreateCursor(IDC_SIZENWSE)))
    {
        return false;
    }
    if (!(Hand = CreateCursor(IDC_HAND)))
    {
        return false;
    }
    if (!(NotAllowed = CreateCursor(IDC_NO)))
    {
        return false;
    }

    return true;
}

void WindowsCursor::SetCursor(GenericCursor* Cursor)
{
    if (Cursor)
    {
        TRef<WindowsCursor> WinCursor = MakeSharedRef<WindowsCursor>(Cursor);
        CurrentCursor = WinCursor;

        HCURSOR Cursorhandle = WinCursor->GetHandle();
        ::SetCursor(Cursorhandle);
    }
    else
    {
        ::SetCursor(NULL);
    }
}

GenericCursor* WindowsCursor::GetCursor()
{
    HCURSOR Cursor = ::GetCursor();
    if (CurrentCursor)
    {
        WindowsCursor* WinCursor = static_cast<WindowsCursor*>(CurrentCursor.Get());
        if (WinCursor->GetHandle() == Cursor)
        {
            CurrentCursor->AddRef();
            return CurrentCursor.Get();
        }
        else
        {
            CurrentCursor.Reset();
        }
    }

    return nullptr;
}

void WindowsCursor::SetCursorPos(GenericWindow* RelativeWindow, int32 x, int32 y)
{
    if (RelativeWindow)
    {
        TRef<WindowsWindow> WinWindow = MakeSharedRef<WindowsWindow>(RelativeWindow);
        HWND hRelative = WinWindow->GetHandle();

        POINT CursorPos = { x, y };
        if (ClientToScreen(hRelative, &CursorPos))
        {
            ::SetCursorPos(CursorPos.x, CursorPos.y);
        }
    }
}

void WindowsCursor::GetCursorPos(GenericWindow* RelativeWindow, int32& OutX, int32& OutY)
{
    POINT CursorPos = { };
    if (!::GetCursorPos(&CursorPos))
    {
        return;
    }

    TRef<WindowsWindow> WinRelative = MakeSharedRef<WindowsWindow>(RelativeWindow);
    if (WinRelative)
    {
        HWND Relative = WinRelative->GetHandle();
        if (ScreenToClient(Relative, &CursorPos))
        {
            OutX = CursorPos.x;
            OutY = CursorPos.y;
        }
    }
}

bool WindowsCursor::Init(LPCSTR InCursorName)
{
    CursorName = InCursorName;
    if (CursorName)
    {
        Cursor = LoadCursor(0, CursorName);
        return true;
    }
    else
    {
        return false;
    }
}