#include "GenericCursor.h"
#include "GenericApplication.h"

TRef<GenericCursor> GlobalCursors::Arrow;
TRef<GenericCursor> GlobalCursors::TextInput;
TRef<GenericCursor> GlobalCursors::ResizeAll;
TRef<GenericCursor> GlobalCursors::ResizeEW;
TRef<GenericCursor> GlobalCursors::ResizeNS;
TRef<GenericCursor> GlobalCursors::ResizeNESW;
TRef<GenericCursor> GlobalCursors::ResizeNWSE;
TRef<GenericCursor> GlobalCursors::Hand;
TRef<GenericCursor> GlobalCursors::NotAllowed;

Bool GlobalCursors::Init()
{
    Arrow = gApplication->MakeCursor();
    if (!Arrow->Init(CursorCreateInfo(EPlatformCursor::Arrow)))
    {
        return false;
    }

    TextInput = gApplication->MakeCursor();
    if (!TextInput->Init(CursorCreateInfo(EPlatformCursor::TextInput)))
    {
        return false;
    }

    ResizeAll = gApplication->MakeCursor();
    if (!ResizeAll->Init(CursorCreateInfo(EPlatformCursor::ResizeAll)))
    {
        return false;
    }

    ResizeEW = gApplication->MakeCursor();
    if (!ResizeEW->Init(CursorCreateInfo(EPlatformCursor::ResizeEW)))
    {
        return false;
    }

    ResizeNS = gApplication->MakeCursor();
    if (!ResizeNS->Init(CursorCreateInfo(EPlatformCursor::ResizeNS)))
    {
        return false;
    }

    ResizeNESW = gApplication->MakeCursor();
    if (!ResizeNESW->Init(CursorCreateInfo(EPlatformCursor::ResizeNESW)))
    {
        return false;
    }

    ResizeNWSE = gApplication->MakeCursor();
    if (!ResizeNWSE->Init(CursorCreateInfo(EPlatformCursor::ResizeNWSE)))
    {
        return false;
    }

    Hand = gApplication->MakeCursor();
    if (!Hand->Init(CursorCreateInfo(EPlatformCursor::Hand)))
    {
        return false;
    }

    NotAllowed = gApplication->MakeCursor();
    if (!NotAllowed->Init(CursorCreateInfo(EPlatformCursor::NotAllowed)))
    {
        return false;
    }

    return true;
}

void GlobalCursors::Release()
{
    Arrow.Reset();
    TextInput.Reset();
    ResizeAll.Reset();
    ResizeEW.Reset();
    ResizeNS.Reset();
    ResizeNESW.Reset();
    ResizeNWSE.Reset();
    Hand.Reset();
    NotAllowed.Reset();
}
