#include "FileOutputDeviceTests.h"

#include <Core/Filesystem/File.h>
#include <Core/Misc/FileOutputDevice.h>
#include <Core/Platform/PlatformFile.h>
#include <Core/Tasks/ParallelFor.h>

#include "TestCommon/TestMacros.h"

#include <cstdio>

static bool ReadClosedFile(const FString& Path, TArray<CHAR_T>& OutText)
{
    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForRead(Path);
    if (!FileHandle)
    {
        return false;
    }

    return File::ReadTextFile(FileHandle.Get(), OutText);
}

static int32 CountOccurrences(const CHAR_T* Text, const CHAR_T* Substring)
{
    int32 NumFound = 0;
    for (const CHAR_T* Found = CString::Strstr(Text, Substring); Found != nullptr; Found = CString::Strstr(Found + 1, Substring))
    {
        NumFound++;
    }

    return NumFound;
}

bool FileOutputDevice_Test()
{
    TEST_BEGIN();

    const FString TempPath("FileOutputDeviceTests.tmp.log");

    TEST_SECTION("Lines logged from many threads all reach the file exactly once");
    {
        ::remove(*TempPath);

        constexpr int32 NumWriters        = 64;
        constexpr int32 NumLinesPerWriter = 16;

        {
            FFileOutputDevice Device(TempPath);
            TEST_EXPECT(Device.IsValid());

            Tasks::ParallelFor(NumWriters, [&](int32 Writer)
            {
                for (int32 Index = 0; Index < NumLinesPerWriter; Index++)
                {
                    Device.Log(ELogSeverity::Info, FString::Printf("FileOutputDeviceTests W%03dL%03d", Writer, Index));
                }
            });
        }

        TArray<CHAR_T> Text;
        TEST_EXPECT(ReadClosedFile(TempPath, Text));

        int32 NumWrong = 0;
        for (int32 Writer = 0; Writer < NumWriters; Writer++)
        {
            for (int32 Index = 0; Index < NumLinesPerWriter; Index++)
            {
                const FString Expected = FString::Printf("FileOutputDeviceTests W%03dL%03d", Writer, Index);
                if (CountOccurrences(Text.Data(), *Expected) != 1)
                {
                    NumWrong++;
                }
            }
        }

        TEST_EXPECT_EQ(NumWrong, 0);

        ::remove(*TempPath);
    }

    TEST_SECTION("Warnings, which flush blocking, keep their lines when threads contend");
    {
        ::remove(*TempPath);

        constexpr int32 NumWriters        = 32;
        constexpr int32 NumLinesPerWriter = 16;

        {
            FFileOutputDevice Device(TempPath);
            TEST_EXPECT(Device.IsValid());

            Tasks::ParallelFor(NumWriters, [&](int32 Writer)
            {
                for (int32 Index = 0; Index < NumLinesPerWriter; Index++)
                {
                    const ELogSeverity Severity = (Index % 2 == 0) ? ELogSeverity::Warning : ELogSeverity::Info;
                    Device.Log(Severity, FString::Printf("FileOutputDeviceTests Mixed W%03dL%03d", Writer, Index));
                }
            });
        }

        TArray<CHAR_T> Text;
        TEST_EXPECT(ReadClosedFile(TempPath, Text));

        int32 NumWrong = 0;
        for (int32 Writer = 0; Writer < NumWriters; Writer++)
        {
            for (int32 Index = 0; Index < NumLinesPerWriter; Index++)
            {
                const FString Expected = FString::Printf("FileOutputDeviceTests Mixed W%03dL%03d", Writer, Index);
                if (CountOccurrences(Text.Data(), *Expected) != 1)
                {
                    NumWrong++;
                }
            }
        }

        TEST_EXPECT_EQ(NumWrong, 0);

        ::remove(*TempPath);
    }

    TEST_SECTION("A device that could not open its file absorbs lines instead of growing a backlog");
    {
        FFileOutputDevice Device(FString("FileOutputDeviceTests_NoSuchDirectory/Output.log"));
        TEST_EXPECT(!Device.IsValid());

        for (int32 Index = 0; Index < 512; Index++)
        {
            Device.Log(ELogSeverity::Info, FString::Printf("FileOutputDeviceTests Dropped%03d", Index));
        }

        Device.Log(ELogSeverity::Error, FString("FileOutputDeviceTests Dropped error"));
        Device.Flush();
    }

    TEST_SECTION("A line is written once however often the device is flushed");
    {
        ::remove(*TempPath);

        constexpr int32 NumLines = 32;

        {
            FFileOutputDevice Device(TempPath);
            TEST_EXPECT(Device.IsValid());

            for (int32 Index = 0; Index < NumLines; Index++)
            {
                Device.Log(ELogSeverity::Info, FString::Printf("FileOutputDeviceTests Once%03d", Index));
                Device.Flush();
                Device.Flush();
            }
        }

        TArray<CHAR_T> Text;
        TEST_EXPECT(ReadClosedFile(TempPath, Text));

        int32 NumWrong = 0;
        for (int32 Index = 0; Index < NumLines; Index++)
        {
            const FString Expected = FString::Printf("FileOutputDeviceTests Once%03d", Index);
            if (CountOccurrences(Text.Data(), *Expected) != 1)
            {
                NumWrong++;
            }
        }

        TEST_EXPECT_EQ(NumWrong, 0);

        ::remove(*TempPath);
    }

    TEST_END();
}
