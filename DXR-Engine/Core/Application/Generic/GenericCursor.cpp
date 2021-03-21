#include "GenericCursor.h"

#include "Core/Application/Platform/PlatformApplication.h"

TRef<GenericCursor> GenericCursor::Arrow;
TRef<GenericCursor> GenericCursor::TextInput;
TRef<GenericCursor> GenericCursor::ResizeAll;
TRef<GenericCursor> GenericCursor::ResizeEW;
TRef<GenericCursor> GenericCursor::ResizeNS;
TRef<GenericCursor> GenericCursor::ResizeNESW;
TRef<GenericCursor> GenericCursor::ResizeNWSE;
TRef<GenericCursor> GenericCursor::Hand;
TRef<GenericCursor> GenericCursor::NotAllowed;

void GenericCursor::ReleaseSystemCursors()
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
