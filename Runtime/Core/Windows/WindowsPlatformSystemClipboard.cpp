#include "Core/Windows/WindowsPlatformSystemClipboard.h"
#include "Core/Windows/Windows.h"
#include "Core/Memory/Memory.h"

static bool OpenClipboardScoped(HWND InOwner)
{
    return ::OpenClipboard(InOwner) != 0;
}

bool FWindowsPlatformSystemClipboard::HasText()
{
    return ::IsClipboardFormatAvailable(CF_UNICODETEXT) != 0;
}

bool FWindowsPlatformSystemClipboard::GetText(String& OutText)
{
    OutText.Clear();

    if (!HasText())
    {
        return false;
    }

    if (!OpenClipboardScoped(nullptr))
    {
        return false;
    }

    HANDLE DataHandle = ::GetClipboardData(CF_UNICODETEXT);
    if (!DataHandle)
    {
        ::CloseClipboard();
        return false;
    }

    const WIDECHAR* WideText = reinterpret_cast<const WIDECHAR*>(::GlobalLock(DataHandle));
    if (!WideText)
    {
        ::CloseClipboard();
        return false;
    }

    OutText = WideToChar(StringViewWide(WideText));

    ::GlobalUnlock(DataHandle);
    ::CloseClipboard();
    return true;
}

bool FWindowsPlatformSystemClipboard::SetText(const String& InText)
{
    if (!OpenClipboardScoped(nullptr))
    {
        return false;
    }

    ::EmptyClipboard();

    const StringWide WideText = CharToWide(StringView(InText));

    const WIDECHAR* WidePtr = *WideText;
    if (!WidePtr)
    {
        ::CloseClipboard();
        return false;
    }

    const SIZE_T CharCount = CStringWide::Strlen(WidePtr) + 1;
    const SIZE_T ByteCount = CharCount * sizeof(WIDECHAR);

    HGLOBAL GlobalHandle = ::GlobalAlloc(GMEM_MOVEABLE, ByteCount);
    if (!GlobalHandle)
    {
        ::CloseClipboard();
        return false;
    }

    void* Dest = ::GlobalLock(GlobalHandle);
    if (!Dest)
    {
        ::GlobalFree(GlobalHandle);
        ::CloseClipboard();
        return false;
    }

    Memory::Memcpy(Dest, WidePtr, ByteCount);

    ::GlobalUnlock(GlobalHandle);

    if (!::SetClipboardData(CF_UNICODETEXT, GlobalHandle))
    {
        ::GlobalFree(GlobalHandle);
        ::CloseClipboard();
        return false;
    }

    ::CloseClipboard();
    return true;
}

void FWindowsPlatformSystemClipboard::Clear()
{
    if (!OpenClipboardScoped(nullptr))
    {
        return;
    }

    ::EmptyClipboard();
    ::CloseClipboard();
}
