#pragma once
#include "Core/Windows/Windows.h"
#include "Core/Containers/String.h"
#include "Core/Containers/StringConversion.h"
#include "Core/PlatformInterface/IPlatformMisc.h"

struct FWindowsPlatformMisc final : public IPlatformMisc
{
    static FORCEINLINE void OutputDebugString(const CHAR* Message)
    {
        ::OutputDebugStringW(StringCast<WIDECHAR>(Message).Get());
        // Add a new line here since that makes this function consistent with OutputDebugString on other platforms
        ::OutputDebugStringW(L"\n");
    }

    static FORCEINLINE void WriteToStdOutput(const CHAR* Text, uint32 Length)
    {
        if (Length == 0)
        {
            return;
        }

        HANDLE Handle = ::GetStdHandle(STD_OUTPUT_HANDLE);
        if (Handle && (Handle != INVALID_HANDLE_VALUE))
        {
            DWORD Written = 0;
            if (::GetFileType(Handle) == FILE_TYPE_CHAR)
            {
                const TStringConversion<WIDECHAR, CHAR> WideText = StringCast<WIDECHAR>(Text, static_cast<int32>(Length));
                ::WriteConsoleW(Handle, WideText.Get(), static_cast<DWORD>(WideText.Length()), &Written, nullptr);
            }
            else
            {
                const TStringConversion<ANSICHAR, CHAR> Utf8Text = StringCast<ANSICHAR>(Text, static_cast<int32>(Length));
                ::WriteFile(Handle, Utf8Text.Get(), static_cast<DWORD>(Utf8Text.Length()), &Written, nullptr);
            }
        }
    }

    static FORCEINLINE bool IsDebuggerPresent()
    {
        return ::IsDebuggerPresent();
    }

    static FORCEINLINE EAssertDialogResult ShowAssertDialog(const CHAR* Title, const CHAR* Message)
    {
        // Matches the CRT assert dialog: Retry drops into the debugger, and is the 
        // default button so that hitting Enter does not terminate the process.
        const UINT Flags = MB_ICONERROR | MB_ABORTRETRYIGNORE | MB_DEFBUTTON2 | MB_TASKMODAL | MB_TOPMOST | MB_SETFOREGROUND;

        switch (::MessageBoxW(nullptr, StringCast<WIDECHAR>(Message).Get(), StringCast<WIDECHAR>(Title).Get(), Flags))
        {
            case IDABORT:
                return EAssertDialogResult::Abort;
            
            case IDRETRY:
                return EAssertDialogResult::Debug;

            case IDIGNORE:
                return EAssertDialogResult::Ignore;

            default:
                return EAssertDialogResult::Abort;
        }
    }

    static FORCEINLINE void MemoryBarrier()
    {
    #if PLATFORM_ARCHITECTURE_X86_64
        _mm_sfence();
    #endif
    }

    static FORCEINLINE bool GetEnvironmentVariable(const CHAR* Name, String& OutValue)
    {
        OutValue.Clear();

        if (!Name || Name[0] == '\0')
        {
            return false;
        }

        const TStringConversion<WIDECHAR, CHAR> WideName = StringCast<WIDECHAR>(Name);

        DWORD RequiredSize = ::GetEnvironmentVariableW(WideName.Get(), nullptr, 0);
        while (RequiredSize > 0)
        {
            const DWORD Length = RequiredSize - 1;

            WString WideValue;
            WideValue.Resize(static_cast<int32>(Length));

            const DWORD Copied = ::GetEnvironmentVariableW(WideName.Get(), WideValue.Data(), RequiredSize);
            if (Copied == Length)
            {
                OutValue = StringCast<CHAR>(WideValue).ToString();
                return true;
            }

            if (Copied == 0)
            {
                OutValue.Clear();
                return false;
            }

            RequiredSize = Copied;
        }

        return false;
    }

    static FORCEINLINE bool SetEnvironmentVariable(const CHAR* Name, const CHAR* Value)
    {
        if (!Name || Name[0] == '\0')
        {
            return false;
        }

        return ::SetEnvironmentVariableW(StringCast<WIDECHAR>(Name).Get(), Value ? StringCast<WIDECHAR>(Value).Get() : nullptr) != FALSE;
    }

    static CORE_API void InstallCrashHandler();

    NODISCARD static CORE_API FGuid CreateGuid();

    static FORCEINLINE int32 GetLastErrorString(String& OutErrorString)
    {
        const int32 LastError = ::GetLastError();
        const int32 Flags     = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;

        LPWSTR MessageBuffer = nullptr;

        const uint32 MessageLength = ::FormatMessageW(Flags, nullptr, LastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&MessageBuffer), 0, nullptr);
        OutErrorString = StringCast<CHAR>(MessageBuffer, static_cast<int32>(MessageLength)).ToString();
        ::LocalFree(MessageBuffer);
        return LastError;
    }
};
