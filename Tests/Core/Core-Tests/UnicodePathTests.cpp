#include "UnicodePathTests.h"

#include <Core/Containers/String.h>
#include <Core/Containers/StringConversion.h>
#include <Core/Filesystem/File.h>
#include <Core/Misc/Paths.h>
#include <Core/Platform/PlatformFile.h>
#include <Core/Platform/PlatformLibrary.h>
#include <Core/Platform/PlatformMisc.h>
#include <Core/Platform/PlatformProcess.h>
#include <Core/Platform/PlatformThreadMisc.h>
#include <Core/Memory/Memory.h>

#include "TestCommon/TestMacros.h"

static constexpr int32 PROCESS_TIMEOUT_MILLISECONDS = 30 * 1000;

static bool WriteText(const String& Filename, const CHAR_T* Text)
{
    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForWrite(Filename);
    return FileHandle && File::WriteTextFile(FileHandle.Get(), String(Text));
}

static String ReadText(const String& Filename)
{
    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForRead(Filename);
    if (!FileHandle)
    {
        return String();
    }

    TArray<CHAR_T> Text;
    return File::ReadTextFile(FileHandle.Get(), Text) ? String(Text.Data()) : String();
}

static bool ContainsReplacementCharacter(const String& Text)
{
    const TStringConversion<WIDECHAR, CHAR_T> WideText = StringCast<WIDECHAR>(Text);
    for (int32 Index = 0; Index < WideText.Length(); ++Index)
    {
        if (static_cast<uint32>(WideText.Get()[Index]) == FStringConversion::ReplacementCharacter)
        {
            return true;
        }
    }

    return false;
}

bool UnicodePath_Test()
{
    TEST_BEGIN();

    const String Root      = Paths::GetUserSettingsDir() + TEXT("/Tests/UnicodePathTests.Scratch");
    const String Directory = Root + TEXT("/Åäö 日本語 😀");
    const String Filename  = Directory + TEXT("/Fil ö.txt");

    File::DeleteDirectoryTree(Root);
    TEST_EXPECT(File::CreateDirectoryTree(Directory));

    TEST_SECTION("Directories and files with non-ASCII names are created, found, read and listed");
    {
        TEST_EXPECT(FPlatformFile::IsDirectory(*Directory));
        TEST_EXPECT(WriteText(Filename, TEXT("Innehåll 日本語")));
        TEST_EXPECT(FPlatformFile::IsFile(*Filename));
        TEST_EXPECT(ReadText(Filename).Equals(TEXT("Innehåll 日本語")));

        const FFileInfo Info = FPlatformFile::GetFileInfo(*Filename);
        TEST_EXPECT(Info.bExists && !Info.bIsDirectory);
        TEST_EXPECT_EQ(Info.Size, 19);

        TArray<FDirectoryEntry> Entries;
        TEST_EXPECT(FPlatformFile::IterateDirectory(*Directory, Entries));
        TEST_EXPECT_EQ(Entries.Size(), 1);
        TEST_EXPECT(Entries.Size() == 1 && Entries[0].Name.Equals(TEXT("Fil ö.txt")));

        TArray<FDirectoryEntry> RootEntries;
        TEST_EXPECT(FPlatformFile::IterateDirectory(*Root, RootEntries));
        TEST_EXPECT(RootEntries.Size() == 1 && RootEntries[0].Name.Equals(TEXT("Åäö 日本語 😀")) && RootEntries[0].bIsDirectory);
    }

    TEST_SECTION("Text files hold UTF-8, and a byte-order mark at the start is not part of the text");
    {
        const String BomFile = Directory + TEXT("/Bom.txt");
        {
            TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForWrite(BomFile);
            const uint8 Bytes[] = { 0xEF, 0xBB, 0xBF, 'a', 0xC3, 0xA5 };
            TEST_EXPECT(FileHandle && FileHandle->Write(Bytes, sizeof(Bytes)) == static_cast<int32>(sizeof(Bytes)));
        }

        TEST_EXPECT(ReadText(BomFile).Equals(TEXT("aå")));

        {
            TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForRead(BomFile);
            TArray<CHAR_T> Text;
            TEST_EXPECT(FileHandle && FileHandle->SeekFromStart(3) && File::ReadTextFile(FileHandle.Get(), Text));
            TEST_EXPECT(String(Text.Data()).Equals(TEXT("aå")));
        }

        TArray<uint8> Written;
        TEST_EXPECT(WriteText(BomFile, TEXT("Åä 日")));
        {
            TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForRead(BomFile);
            TEST_EXPECT(FileHandle && File::ReadFile(FileHandle.Get(), Written));
        }

        const uint8 Expected[] = { 0xC3, 0x85, 0xC3, 0xA4, ' ', 0xE6, 0x97, 0xA5 };
        TEST_EXPECT(Written.Size() == static_cast<int32>(sizeof(Expected)) && Memory::Memcmp(Written.Data(), Expected, sizeof(Expected)) == 0);
        TEST_EXPECT(FPlatformFile::DeleteFile(*BomFile));
    }

    TEST_SECTION("Files and directories with non-ASCII names are copied, moved and deleted");
    {
        const String Copied = Directory + TEXT("/Kopia ä.txt");
        TEST_EXPECT(FPlatformFile::CopyFile(*Filename, *Copied));
        TEST_EXPECT(ReadText(Copied).Equals(TEXT("Innehåll 日本語")));

        const String Moved = Directory + TEXT("/Flyttad 本.txt");
        TEST_EXPECT(FPlatformFile::MoveFile(*Copied, *Moved));
        TEST_EXPECT(!FPlatformFile::IsFile(*Copied) && FPlatformFile::IsFile(*Moved));

        TEST_EXPECT(FPlatformFile::DeleteFile(*Moved));
        TEST_EXPECT(!FPlatformFile::IsFile(*Moved));

        const String MovedDirectory = Root + TEXT("/Flyttad mapp ö");
        TEST_EXPECT(FPlatformFile::MoveDirectory(*Directory, *MovedDirectory));
        TEST_EXPECT(FPlatformFile::IsFile(*(MovedDirectory + TEXT("/Fil ö.txt"))));
        TEST_EXPECT(FPlatformFile::MoveDirectory(*MovedDirectory, *Directory));
        TEST_EXPECT(!FPlatformFile::IsPathRelative(*Directory));
    }

    TEST_SECTION("A process starts in a working directory with a non-ASCII name");
    {
        FProcessDesc Desc;
    #if PLATFORM_WINDOWS
        Desc.Executable = TEXT("cmd.exe");
        Desc.Arguments.Add(String(TEXT("/c")));
        Desc.Arguments.Add(String(TEXT("exit 7")));
    #else
        Desc.Executable = TEXT("/bin/sh");
        Desc.Arguments.Add(String(TEXT("-c")));
        Desc.Arguments.Add(String(TEXT("exit 7")));
    #endif
        Desc.WorkingDirectory = Directory;

        TUniquePtr<IPlatformProcessHandle> Process = FPlatformProcess::LaunchProcess(Desc);
        TEST_EXPECT(Process != nullptr);
        if (Process)
        {
            TEST_EXPECT(Process->Wait(PROCESS_TIMEOUT_MILLISECONDS));

            int32 ExitCode = -1;
            TEST_EXPECT(Process->GetExitCode(ExitCode));
            TEST_EXPECT_EQ(ExitCode, 7);
        }
    }

#if PLATFORM_WINDOWS
    TEST_SECTION("A library is loaded and its symbols found by name");
    {
        void* Kernel = FPlatformLibrary::GetLoadedHandle(TEXT("kernel32"));
        TEST_EXPECT(Kernel != nullptr);
        TEST_EXPECT(FPlatformLibrary::LoadSymbol(TEXT("GetTickCount64"), Kernel) != nullptr);
    }

    TEST_SECTION("System error messages arrive as valid UTF-8 in any system language");
    {
        TEST_EXPECT(FPlatformFile::OpenForRead(Directory + TEXT("/Saknas ö.txt")) == nullptr);

        String Error;
        TEST_EXPECT(FPlatformMisc::GetLastErrorString(Error) != 0);
        TEST_EXPECT(!Error.IsEmpty());
        TEST_EXPECT(!ContainsReplacementCharacter(Error));
    }
#endif

    TEST_SECTION("The executable path and working directory are valid UTF-8");
    {
        TEST_EXPECT(!ContainsReplacementCharacter(String(FPlatformFile::GetExecutablePath())));
        TEST_EXPECT(!ContainsReplacementCharacter(FPlatformFile::GetCurrentWorkingDirectory()));
    }

    TEST_EXPECT(File::DeleteDirectoryTree(Root));
    TEST_EXPECT(!FPlatformFile::IsDirectory(*Root));
    FPlatformFile::RemoveDirectory(*File::GetDirectoryOf(Root));

    TEST_END();
}
