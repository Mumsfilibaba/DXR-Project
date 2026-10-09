#include "Core/Windows/WindowsPlatformProcess.h"
#include "Core/Windows/WindowsPlatformMisc.h"
#include "Core/Memory/Memory.h"
#include "Core/Misc/OutputDeviceLogger.h"

static constexpr DWORD OUTPUT_PIPE_SIZE = 64 * 1024;

static void AppendQuotedArgument(String& CommandLine, const String& Argument)
{
    const bool bNeedsQuotes = Argument.IsEmpty() || Argument.Contains(' ') || Argument.Contains('\t') || Argument.Contains('"');
    if (!bNeedsQuotes)
    {
        CommandLine.Append(Argument);
        return;
    }

    CommandLine.Append('"');

    for (int32 Index = 0; ; ++Index)
    {
        int32 NumBackslashes = 0;
        while ((Index < Argument.Length()) && (Argument[Index] == '\\'))
        {
            ++NumBackslashes;
            ++Index;
        }

        const bool  bAtEnd        = Index == Argument.Length();
        const bool  bBeforeQuote  = bAtEnd || (Argument[Index] == '"');
        const int32 NumToWrite    = bBeforeQuote ? (NumBackslashes * 2) : NumBackslashes;
        const int32 NumQuoteSlash = (!bAtEnd && (Argument[Index] == '"')) ? 1 : 0;

        for (int32 Count = 0; Count < (NumToWrite + NumQuoteSlash); ++Count)
        {
            CommandLine.Append('\\');
        }

        if (bAtEnd)
        {
            break;
        }

        CommandLine.Append(Argument[Index]);
    }

    CommandLine.Append('"');
}

FWindowsPlatformProcessHandle::FWindowsPlatformProcessHandle(HANDLE InProcess, HANDLE InOutputPipe)
    : IPlatformProcessHandle()
    , Process(InProcess)
    , OutputPipe(InOutputPipe)
{
}

FWindowsPlatformProcessHandle::~FWindowsPlatformProcessHandle()
{
    if (OutputPipe)
    {
        ::CloseHandle(OutputPipe);
    }

    ::CloseHandle(Process);
}

bool FWindowsPlatformProcessHandle::ReadOutput(String& OutText)
{
    const bool bHasExited = !IsRunning();
    if (OutputPipe)
    {
        while (true)
        {
            DWORD NumAvailable = 0;
            if (!::PeekNamedPipe(OutputPipe, nullptr, 0, nullptr, &NumAvailable, nullptr) || (NumAvailable == 0))
            {
                break;
            }

            CHAR        Buffer[4096];
            const DWORD NumToRead = (NumAvailable < sizeof(Buffer)) ? NumAvailable : sizeof(Buffer);

            DWORD NumRead = 0;
            if (!::ReadFile(OutputPipe, Buffer, NumToRead, &NumRead, nullptr) || (NumRead == 0))
            {
                break;
            }

            OutText.Append(Buffer, static_cast<int32>(NumRead));
        }
    }

    return !bHasExited;
}

bool FWindowsPlatformProcessHandle::IsRunning()
{
    return ::WaitForSingleObject(Process, 0) == WAIT_TIMEOUT;
}

bool FWindowsPlatformProcessHandle::Wait(int32 TimeoutMilliseconds)
{
    const DWORD Timeout = (TimeoutMilliseconds < 0) ? INFINITE : static_cast<DWORD>(TimeoutMilliseconds);
    return ::WaitForSingleObject(Process, Timeout) == WAIT_OBJECT_0;
}

bool FWindowsPlatformProcessHandle::GetExitCode(int32& OutExitCode)
{
    if (IsRunning())
    {
        return false;
    }

    DWORD ExitCode = 0;
    if (!::GetExitCodeProcess(Process, &ExitCode))
    {
        return false;
    }

    OutExitCode = static_cast<int32>(ExitCode);
    return true;
}

void FWindowsPlatformProcessHandle::Kill()
{
    ::TerminateProcess(Process, 1);
}

TUniquePtr<IPlatformProcessHandle> FWindowsPlatformProcess::LaunchProcess(const FProcessDesc& Desc)
{
    String Executable = Desc.Executable;
    Executable.ReplaceAll('/', '\\');

    String CommandLine;
    AppendQuotedArgument(CommandLine, Executable);
    for (const String& Argument : Desc.Arguments)
    {
        CommandLine.Append(' ');
        AppendQuotedArgument(CommandLine, Argument);
    }

    STARTUPINFOA StartupInfo;
    Memory::Memzero(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);

    HANDLE ReadPipe  = nullptr;
    HANDLE WritePipe = nullptr;

    if (Desc.bCaptureOutput)
    {
        SECURITY_ATTRIBUTES Security;
        Memory::Memzero(&Security, sizeof(Security));

        Security.nLength        = sizeof(Security);
        Security.bInheritHandle = TRUE;

        if (!::CreatePipe(&ReadPipe, &WritePipe, &Security, OUTPUT_PIPE_SIZE))
        {
            LOG_ERROR("[FWindowsPlatformProcess]: Failed to create the output pipe for '%s'", *Desc.Executable);
            return nullptr;
        }

        ::SetHandleInformation(ReadPipe, HANDLE_FLAG_INHERIT, 0);

        StartupInfo.dwFlags   |= STARTF_USESTDHANDLES;
        StartupInfo.hStdOutput = WritePipe;
        StartupInfo.hStdError  = WritePipe;
    }

    String WorkingDirectory = Desc.WorkingDirectory;
    WorkingDirectory.ReplaceAll('/', '\\');

    const DWORD Flags = Desc.bHidden ? CREATE_NO_WINDOW : 0;

    PROCESS_INFORMATION ProcessInfo;
    Memory::Memzero(&ProcessInfo, sizeof(ProcessInfo));

    const BOOL bCreated = ::CreateProcessA(
        nullptr,
        CommandLine.Data(),
        nullptr,
        nullptr,
        Desc.bCaptureOutput ? TRUE : FALSE,
        Flags,
        nullptr,
        WorkingDirectory.IsEmpty() ? nullptr : *WorkingDirectory,
        &StartupInfo,
        &ProcessInfo);

    if (WritePipe)
    {
        ::CloseHandle(WritePipe);
    }

    if (!bCreated)
    {
        String Error;
        const int32 ErrorCode = FWindowsPlatformMisc::GetLastErrorString(Error);
        LOG_ERROR("[FWindowsPlatformProcess]: Failed to start '%s' with error %d '%s'", *Desc.Executable, ErrorCode, *Error);

        if (ReadPipe)
        {
            ::CloseHandle(ReadPipe);
        }

        return nullptr;
    }

    ::CloseHandle(ProcessInfo.hThread);
    return TUniquePtr<IPlatformProcessHandle>(new FWindowsPlatformProcessHandle(ProcessInfo.hProcess, ReadPipe));
}
