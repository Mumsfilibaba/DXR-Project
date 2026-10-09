#include "PlatformProcessTests.h"

#include <Core/Filesystem/File.h>
#include <Core/Platform/PlatformFile.h>
#include <Core/Platform/PlatformProcess.h>
#include <Core/Platform/PlatformThreadMisc.h>
#include <Core/Containers/String.h>

#include "TestCommon/TestMacros.h"

static constexpr int32 PROCESS_TIMEOUT_MILLISECONDS = 30 * 1000;

static bool RunShell(const String& Command, const String& WorkingDirectory, String& OutOutput, int32& OutExitCode)
{
    FProcessDesc Desc;
#if PLATFORM_WINDOWS
    Desc.Executable = "cmd.exe";
    Desc.Arguments.Add(String("/c"));
#else
    Desc.Executable = "/bin/sh";
    Desc.Arguments.Add(String("-c"));
#endif
    Desc.Arguments.Add(Command);
    Desc.WorkingDirectory = WorkingDirectory;
    Desc.bCaptureOutput   = true;

    TUniquePtr<IPlatformProcessHandle> Process = FPlatformProcess::LaunchProcess(Desc);
    if (!Process)
    {
        return false;
    }

    for (int32 Elapsed = 0; Process->ReadOutput(OutOutput); ++Elapsed)
    {
        if (Elapsed > PROCESS_TIMEOUT_MILLISECONDS)
        {
            Process->Kill();
            return false;
        }

        FPlatformThreadMisc::Sleep(FTimespan::Milliseconds(1));
    }

    return Process->GetExitCode(OutExitCode);
}

static String TrimLineEnd(const String& Text)
{
    int32 Length = Text.Length();
    while ((Length > 0) && ((Text[Length - 1] == '\n') || (Text[Length - 1] == '\r')))
    {
        --Length;
    }

    return String(*Text, Length);
}

bool PlatformProcess_Test()
{
    TEST_BEGIN();

    TEST_SECTION("Output and the exit code of a finished process are collected");
    {
        String Output;
        int32  ExitCode = -1;
        TEST_EXPECT(RunShell("echo Hello", String(), Output, ExitCode));
        TEST_EXPECT(TrimLineEnd(Output).Equals("Hello"));
        TEST_EXPECT(ExitCode == 0);
    }

    TEST_SECTION("A non-zero exit code is reported");
    {
        String Output;
        int32  ExitCode = -1;
        TEST_EXPECT(RunShell("exit 3", String(), Output, ExitCode));
        TEST_EXPECT(ExitCode == 3);
    }

    TEST_SECTION("Standard error is captured with standard output");
    {
        String Output;
        int32  ExitCode = -1;
        TEST_EXPECT(RunShell("echo Problem>&2", String(), Output, ExitCode));
        TEST_EXPECT(TrimLineEnd(Output).Equals("Problem"));
    }

    TEST_SECTION("The process starts in the requested working directory");
    {
        const String Directory = File::MakeAbsolute("PlatformProcessTests.Scratch");
        TEST_EXPECT(File::CreateDirectoryTree(Directory));

        String Output;
        int32  ExitCode = -1;
#if PLATFORM_WINDOWS
        TEST_EXPECT(RunShell("cd", Directory, Output, ExitCode));
#else
        TEST_EXPECT(RunShell("pwd", Directory, Output, ExitCode));
#endif
        TEST_EXPECT(File::NormalizePath(TrimLineEnd(Output)).Equals(Directory));
        TEST_EXPECT(File::DeleteDirectoryTree(Directory));
    }

    TEST_SECTION("A process that is still running can be waited for and killed");
    {
        FProcessDesc Desc;
#if PLATFORM_WINDOWS
        Desc.Executable = "cmd.exe";
        Desc.Arguments.Add(String("/c"));
        Desc.Arguments.Add(String("ping -n 30 127.0.0.1 > nul"));
#else
        Desc.Executable = "/bin/sleep";
        Desc.Arguments.Add(String("30"));
#endif

        TUniquePtr<IPlatformProcessHandle> Process = FPlatformProcess::LaunchProcess(Desc);
        TEST_EXPECT(Process != nullptr);

        if (Process)
        {
            int32 ExitCode = 0;
            TEST_EXPECT(Process->IsRunning());
            TEST_EXPECT(!Process->Wait(10));
            TEST_EXPECT(!Process->GetExitCode(ExitCode));

            Process->Kill();
            TEST_EXPECT(Process->Wait(PROCESS_TIMEOUT_MILLISECONDS));
            TEST_EXPECT(!Process->IsRunning());
        }
    }

    TEST_SECTION("Starting a program that does not exist fails cleanly");
    {
        FProcessDesc Desc;
        Desc.Executable     = "ThisProgramDoesNotExist.exe";
        Desc.bCaptureOutput = true;
        TEST_EXPECT(FPlatformProcess::LaunchProcess(Desc) == nullptr);
    }

    TEST_END();
}
