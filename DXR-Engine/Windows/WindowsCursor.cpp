#include "WindowsCursor.h"
#include "WindowsApplication.h"

WindowsCursor::WindowsCursor(WindowsApplication* InApplication)
    : Application(InApplication)
    , hCursor(0)
    , CursorName(nullptr)
{
}

WindowsCursor::~WindowsCursor()
{
    if (!CursorName)
    {
        ::DestroyCursor(hCursor);
    }
}

Bool WindowsCursor::Init(const CursorCreateInfo& InCreateInfo)
{
    if (InCreateInfo.IsPlatformCursor)
    {
        switch (InCreateInfo.PlatformCursor)
        {
        case EPlatformCursor::Arrow:      CursorName = IDC_ARROW;    break;
        case EPlatformCursor::Hand:       CursorName = IDC_HAND;     break;
        case EPlatformCursor::NotAllowed: CursorName = IDC_NO;       break;
        case EPlatformCursor::ResizeAll:  CursorName = IDC_SIZEALL;  break;
        case EPlatformCursor::ResizeEW:   CursorName = IDC_SIZEWE;   break;
        case EPlatformCursor::ResizeNS:   CursorName = IDC_SIZENS;   break;
        case EPlatformCursor::ResizeNESW: CursorName = IDC_SIZENESW; break;
        case EPlatformCursor::ResizeNWSE: CursorName = IDC_SIZENWSE; break;
        case EPlatformCursor::TextInput:  CursorName = IDC_IBEAM;    break;
        default: CursorName = nullptr; break;
        }

        CreateInfo = InCreateInfo;
        if (CursorName)
        {
            hCursor = ::LoadCursor(0, CursorName);
            return true;
        }
        else
        {
            return false;
        }
    }

    return false;
}