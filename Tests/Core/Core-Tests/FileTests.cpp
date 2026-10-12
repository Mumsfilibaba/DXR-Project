#include "FileTests.h"

#include <Core/Filesystem/File.h>
#include <Core/Containers/String.h>
#include <Core/Image/ImageView.h>
#include <Core/Image/PngWriter.h>
#include <Core/Json/Json.h>
#include <Core/Misc/Paths.h>
#include <Core/Platform/PlatformFile.h>
#include <Core/Platform/PlatformProcess.h>
#include <Core/Templates/CString.h>

#include "TestCommon/TestMacros.h"

static bool WriteText(const FString& Filename, const CHAR_T* Text)
{
    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForWrite(Filename);
    if (!FileHandle)
    {
        return false;
    }

    return File::WriteTextFile(FileHandle.Get(), FString(Text));
}

struct FFakeFile : public IPlatformFile
{
    FFakeFile(int64 InSize, int32 InReadResult, int32 InWriteResult)
        : FakeSize(InSize)
        , ReadResult(InReadResult)
        , WriteResult(InWriteResult)
    {
    }

    virtual bool SeekFromStart(int64) override final
    {
        return true;
    }

    virtual bool SeekFromCurrent(int64) override final
    {
        return true;
    }

    virtual bool SeekFromEnd(int64) override final
    {
        return true;
    }

    virtual int64 Size() const override final
    {
        return FakeSize;
    }

    virtual int64 Tell() const override final
    {
        return 0;
    }

    virtual int32 Read(uint8*, uint32) override final
    {
        return ReadResult;
    }

    virtual int32 Write(const uint8*, uint32) override final
    {
        return WriteResult;
    }

    virtual bool Truncate(int64) override final
    {
        return true;
    }

    virtual bool IsValid() const override final
    {
        return true;
    }

    virtual void Close() override final
    {
    }

    int64 FakeSize;
    int32 ReadResult;
    int32 WriteResult;
};

static bool CreateDirectoryLink(const FString& Link, const FString& Target)
{
    FProcessDesc Desc;
#if PLATFORM_WINDOWS
    FString WindowsLink   = Link;
    FString WindowsTarget = Target;
    WindowsLink.ReplaceAll('/', '\\');
    WindowsTarget.ReplaceAll('/', '\\');

    Desc.Executable = TEXT("cmd.exe");
    Desc.Arguments.Add(FString(TEXT("/c")));
    Desc.Arguments.Add(FString(TEXT("mklink")));
    Desc.Arguments.Add(FString(TEXT("/J")));
    Desc.Arguments.Add(WindowsLink);
    Desc.Arguments.Add(WindowsTarget);
#else
    Desc.Executable = TEXT("/bin/ln");
    Desc.Arguments.Add(FString(TEXT("-s")));
    Desc.Arguments.Add(Target);
    Desc.Arguments.Add(Link);
#endif
    Desc.bCaptureOutput = true;

    TUniquePtr<IPlatformProcessHandle> Process = FPlatformProcess::LaunchProcess(Desc);
    if (!Process || !Process->Wait(30 * 1000))
    {
        return false;
    }

    int32 ExitCode = -1;
    return Process->GetExitCode(ExitCode) && (ExitCode == 0);
}

static FString ReadText(const FString& Filename)
{
    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForRead(Filename);
    if (!FileHandle)
    {
        return FString();
    }

    TArray<CHAR_T> Text;
    if (!File::ReadTextFile(FileHandle.Get(), Text))
    {
        return FString();
    }

    return FString(Text.Data());
}

bool File_Test()
{
    TEST_BEGIN();

    TEST_SECTION(TEXT("The working directory has no trailing null-terminator"));
    {
        const FString WorkingDirectory = FPlatformFile::GetCurrentWorkingDirectory();
        TEST_EXPECT(!WorkingDirectory.IsEmpty());
        TEST_EXPECT(WorkingDirectory.Length() == static_cast<int32>(CString::Strlen(*WorkingDirectory)));
        TEST_EXPECT(FPlatformFile::IsDirectory(*WorkingDirectory));

        const FString Appended = WorkingDirectory + TEXT("/File.txt");
        TEST_EXPECT(static_cast<int32>(CString::Strlen(*Appended)) == Appended.Length());
    }

    TEST_SECTION(TEXT("GetDirectoryOf and ExtractFilename split on either separator"));
    {
        TEST_EXPECT(File::GetDirectoryOf(TEXT("Dir/Sub/File.txt")).Equals(TEXT("Dir/Sub")));
        TEST_EXPECT(File::GetDirectoryOf(TEXT("Dir/File")).Equals(TEXT("Dir")));
        TEST_EXPECT(File::GetDirectoryOf(TEXT("Dir\\Sub\\File.txt")).Equals(TEXT("Dir\\Sub")));
        TEST_EXPECT(File::GetDirectoryOf(TEXT("File.txt")).IsEmpty());
        TEST_EXPECT(File::GetDirectoryOf(TEXT("")).IsEmpty());

        TEST_EXPECT(File::ExtractFilename(TEXT("Dir/Sub/File.txt")).Equals(TEXT("File.txt")));
        TEST_EXPECT(File::ExtractFilename(TEXT("Dir\\Sub\\File.txt")).Equals(TEXT("File.txt")));
        TEST_EXPECT(File::ExtractFilename(TEXT("Dir/Mixed\\File.txt")).Equals(TEXT("File.txt")));
        TEST_EXPECT(File::ExtractFilename(TEXT("File.txt")).Equals(TEXT("File.txt")));
        TEST_EXPECT(File::ExtractFilename(TEXT("Dir/")).IsEmpty());
    }

    TEST_SECTION(TEXT("ExtractFilenameWithoutExtension only looks at the filename"));
    {
        TEST_EXPECT(File::ExtractFilenameWithoutExtension(TEXT("Dir/Sub/File.txt")).Equals(TEXT("File")));
        TEST_EXPECT(File::ExtractFilenameWithoutExtension(TEXT("File.txt")).Equals(TEXT("File")));
        TEST_EXPECT(File::ExtractFilenameWithoutExtension(TEXT("Archive.tar.gz")).Equals(TEXT("Archive.tar")));

        TEST_EXPECT(File::ExtractFilenameWithoutExtension(TEXT("Dir/File")).Equals(TEXT("File")));
        TEST_EXPECT(File::ExtractFilenameWithoutExtension(TEXT("File")).Equals(TEXT("File")));

        TEST_EXPECT(File::ExtractFilenameWithoutExtension(TEXT("Dir.v2/File")).Equals(TEXT("File")));
        TEST_EXPECT(File::ExtractFilenameWithoutExtension(TEXT("Dir.v2/File.txt")).Equals(TEXT("File")));

        TEST_EXPECT(File::ExtractFilenameWithoutExtension(TEXT("Dir/.gitignore")).Equals(TEXT(".gitignore")));
        TEST_EXPECT(File::ExtractExtension(TEXT("Dir/.gitignore")).IsEmpty());

        TEST_EXPECT(File::ExtractFilenameWithoutExtension(TEXT("")).IsEmpty());
    }

    TEST_SECTION(TEXT("Json::SaveToFile writes a file that has no directory in its name"));
    {
        const FString Filename(TEXT("FileTests.NoDirectory.json"));

        FJsonValue Document = FJsonValue::MakeObject();
        Document.AddMember(TEXT("Value"), FJsonValue(1));

        TEST_EXPECT(Json::SaveToFile(Filename, Document));
        TEST_EXPECT(!FPlatformFile::IsDirectory(*Filename));

        FJsonValue FromDisk;
        TEST_EXPECT(Json::LoadFromFile(Filename, FromDisk));
        TEST_EXPECT(Document.Equals(FromDisk));

        TEST_EXPECT(FPlatformFile::DeleteFile(*Filename));
    }

    TEST_SECTION(TEXT("FPngWriter writes a file that has no directory in its name"));
    {
        const FString Filename(TEXT("FileTests.NoDirectory.png"));
        const uint8   Pixel[FImageView::BytesPerPixel] = { 255, 0, 0, 255 };

        FPngWriter Writer;
        TEST_EXPECT(Writer.WriteToFile(Filename, FImageView(Pixel, 1, 1)));
        TEST_EXPECT(!FPlatformFile::IsDirectory(*Filename));
        TEST_EXPECT(FPlatformFile::IsFile(*Filename));

        TEST_EXPECT(FPlatformFile::DeleteFile(*Filename));
    }

    TEST_SECTION(TEXT("A short or failed read fails and leaves the output empty"));
    {
        for (const int32 ReadResult : { 4, 0, -1 })
        {
            FFakeFile Fake(10, ReadResult, 0);

            TArray<uint8> Bytes = { 1, 2, 3 };
            TEST_EXPECT(!File::ReadFile(&Fake, Bytes));
            TEST_EXPECT(Bytes.IsEmpty());

            TArray<CHAR_T> Text = { 'A' };
            TEST_EXPECT(!File::ReadTextFile(&Fake, Text));
            TEST_EXPECT(Text.IsEmpty());

            FByteInputStream Stream;
            TEST_EXPECT(!File::ReadFile(&Fake, Stream));
            TEST_EXPECT(Stream.Size() == 0);
        }
    }

    TEST_SECTION(TEXT("A partial or failed write fails"));
    {
        for (const int32 WriteResult : { 3, 0, -1 })
        {
            FFakeFile Fake(0, 0, WriteResult);
            TEST_EXPECT(!File::WriteTextFile(&Fake, FString(TEXT("0123456789"))));
        }

        FFakeFile Complete(0, 0, 10);
        TEST_EXPECT(File::WriteTextFile(&Complete, FString(TEXT("0123456789"))));
    }

    const FString Root(TEXT("FileTests.Scratch"));
    File::DeleteDirectoryTree(Root);
    TEST_EXPECT(File::CreateDirectoryTree(Root));

    TEST_SECTION(TEXT("An empty file reads as empty and an empty string writes"));
    {
        const FString Filename = Root + TEXT("/Empty.txt");
        TEST_EXPECT(WriteText(Filename, TEXT("")));
        TEST_EXPECT(FPlatformFile::GetFileInfo(*Filename).Size == 0);

        TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForRead(Filename);
        TEST_EXPECT(FileHandle.IsValid());

        if (FileHandle)
        {
            TArray<uint8> Bytes = { 1, 2, 3 };
            TEST_EXPECT(File::ReadFile(FileHandle.Get(), Bytes));
            TEST_EXPECT(Bytes.IsEmpty());

            TArray<CHAR_T> Text;
            TEST_EXPECT(File::ReadTextFile(FileHandle.Get(), Text));
            TEST_EXPECT((Text.Size() == 1) && (Text[0] == 0));

            FByteInputStream Stream;
            TEST_EXPECT(File::ReadFile(FileHandle.Get(), Stream));
            TEST_EXPECT(Stream.Size() == 0);
        }
    }

    TEST_SECTION(TEXT("Reads start at the file's current position"));
    {
        const FString Filename = Root + TEXT("/Position.txt");
        TEST_EXPECT(WriteText(Filename, TEXT("0123456789")));

        TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForRead(Filename);
        TEST_EXPECT(FileHandle.IsValid());

        if (FileHandle)
        {
            TEST_EXPECT(FileHandle->SeekFromStart(4));
            TEST_EXPECT(FileHandle->Tell() == 4);

            TEST_EXPECT(FileHandle->SeekFromCurrent(-2));
            TEST_EXPECT(FileHandle->Tell() == 2);

            TEST_EXPECT(FileHandle->SeekFromEnd(-3));
            TEST_EXPECT(FileHandle->Tell() == 7);

            TEST_EXPECT(!FileHandle->SeekFromStart(-1));
            TEST_EXPECT(FileHandle->Tell() == 7);

            TEST_EXPECT(FileHandle->SeekFromStart(4));

            TArray<CHAR_T> Text;
            TEST_EXPECT(File::ReadTextFile(FileHandle.Get(), Text));
            TEST_EXPECT(FString(Text.Data()).Equals(TEXT("456789")));
            TEST_EXPECT(FileHandle->Tell() == 10);
        }
    }

    TEST_SECTION(TEXT("IsFile is false for a directory"));
    {
        const FString Filename = Root + TEXT("/IsFile.txt");
        TEST_EXPECT(WriteText(Filename, TEXT("Text")));

        TEST_EXPECT(FPlatformFile::IsFile(*Filename));
        TEST_EXPECT(!FPlatformFile::IsFile(*Root));
        TEST_EXPECT(!FPlatformFile::IsFile(*(Root + TEXT("/Missing.txt"))));
    }

    TEST_SECTION(TEXT("GetFileInfo reports size, kind and write time"));
    {
        const FString Filename = Root + TEXT("/Info.txt");
        TEST_EXPECT(WriteText(Filename, TEXT("0123456789")));

        const FFileInfo FileInfo = FPlatformFile::GetFileInfo(*Filename);
        TEST_EXPECT(FileInfo.bExists);
        TEST_EXPECT(!FileInfo.bIsDirectory);
        TEST_EXPECT(FileInfo.Size == 10);

        // Between 2020-01-01 and 2100-01-01
        TEST_EXPECT(FileInfo.ModifiedTime > 1577836800);
        TEST_EXPECT(FileInfo.ModifiedTime < 4102444800);

        const FFileInfo DirectoryInfo = FPlatformFile::GetFileInfo(*Root);
        TEST_EXPECT(DirectoryInfo.bExists);
        TEST_EXPECT(DirectoryInfo.bIsDirectory);
        TEST_EXPECT(DirectoryInfo.Size == 0);

        const FFileInfo MissingInfo = FPlatformFile::GetFileInfo(*(Root + TEXT("/Missing.txt")));
        TEST_EXPECT(!MissingInfo.bExists);
    }

    TEST_SECTION(TEXT("CopyFile refuses to overwrite unless asked to"));
    {
        const FString Source      = Root + TEXT("/CopySource.txt");
        const FString Destination = Root + TEXT("/CopyDestination.txt");
        TEST_EXPECT(WriteText(Source, TEXT("Source")));
        TEST_EXPECT(WriteText(Destination, TEXT("Old")));

        TEST_EXPECT(!FPlatformFile::CopyFile(*Source, *Destination));
        TEST_EXPECT(ReadText(Destination).Equals(TEXT("Old")));

        TEST_EXPECT(FPlatformFile::CopyFile(*Source, *Destination, true));
        TEST_EXPECT(ReadText(Destination).Equals(TEXT("Source")));
        TEST_EXPECT(ReadText(Source).Equals(TEXT("Source")));

        const FString NewDestination = Root + TEXT("/CopyNew.txt");
        TEST_EXPECT(FPlatformFile::CopyFile(*Source, *NewDestination));
        TEST_EXPECT(ReadText(NewDestination).Equals(TEXT("Source")));
    }

    TEST_SECTION(TEXT("MoveDirectory renames a directory with its contents and refuses an existing target"));
    {
        const FString From = Root + TEXT("/MoveFrom");
        const FString To   = Root + TEXT("/MoveTo");
        TEST_EXPECT(File::CreateDirectoryTree(From + TEXT("/Nested")));
        TEST_EXPECT(WriteText(From + TEXT("/Nested/File.txt"), TEXT("Moved")));

        TEST_EXPECT(FPlatformFile::MoveDirectory(*From, *To));
        TEST_EXPECT(!FPlatformFile::IsDirectory(*From));
        TEST_EXPECT(ReadText(To + TEXT("/Nested/File.txt")).Equals(TEXT("Moved")));

        const FString Occupied = Root + TEXT("/Occupied");
        TEST_EXPECT(FPlatformFile::CreateDirectory(*Occupied));
        TEST_EXPECT(!FPlatformFile::MoveDirectory(*To, *Occupied));
        TEST_EXPECT(FPlatformFile::IsDirectory(*To));
    }

    TEST_SECTION(TEXT("DeleteDirectoryTree removes nested directories and files"));
    {
        const FString Tree = Root + TEXT("/Tree");
        TEST_EXPECT(File::CreateDirectoryTree(Tree + TEXT("/A/B/C")));
        TEST_EXPECT(WriteText(Tree + TEXT("/Top.txt"), TEXT("1")));
        TEST_EXPECT(WriteText(Tree + TEXT("/A/B/Deep.txt"), TEXT("2")));
        TEST_EXPECT(WriteText(Tree + TEXT("/A/B/C/Deeper.txt"), TEXT("3")));

        TEST_EXPECT(File::DeleteDirectoryTree(Tree));
        TEST_EXPECT(!FPlatformFile::GetFileInfo(*Tree).bExists);

        TEST_EXPECT(File::DeleteDirectoryTree(Tree));
    }

    TEST_SECTION(TEXT("DeleteDirectoryTree removes a link to a directory without touching its target"));
    {
        const FString Target = File::MakeAbsolute(Root + TEXT("/LinkTarget"));
        const FString Tree   = File::MakeAbsolute(Root + TEXT("/TreeWithLink"));
        const FString Link   = Tree + TEXT("/Link");
        TEST_EXPECT(File::CreateDirectoryTree(Target));
        TEST_EXPECT(File::CreateDirectoryTree(Tree));
        TEST_EXPECT(WriteText(Target + TEXT("/Kept.txt"), TEXT("Kept")));

        TEST_EXPECT(CreateDirectoryLink(Link, Target));
        TEST_EXPECT(FPlatformFile::IsDirectory(*Link));

        TArray<FDirectoryEntry> Entries;
        TEST_EXPECT(FPlatformFile::IterateDirectory(*Tree, Entries));
        TEST_EXPECT((Entries.Size() == 1) && Entries[0].bIsSymbolicLink);

        TEST_EXPECT(File::DeleteDirectoryTree(Tree));
        TEST_EXPECT(!FPlatformFile::GetFileInfo(*Tree).bExists);
        TEST_EXPECT(ReadText(Target + TEXT("/Kept.txt")).Equals(TEXT("Kept")));
    }

    TEST_SECTION(TEXT("CopyDirectoryTree copies nested directories, files and empty directories"));
    {
        const FString Source      = Root + TEXT("/CopyTreeSource");
        const FString Destination = Root + TEXT("/CopyTreeDestination");
        TEST_EXPECT(File::CreateDirectoryTree(Source + TEXT("/A/B")));
        TEST_EXPECT(File::CreateDirectoryTree(Source + TEXT("/Empty")));
        TEST_EXPECT(WriteText(Source + TEXT("/Top.txt"), TEXT("1")));
        TEST_EXPECT(WriteText(Source + TEXT("/A/B/Deep.txt"), TEXT("2")));

        TEST_EXPECT(File::CopyDirectoryTree(Source, Destination));
        TEST_EXPECT(ReadText(Destination + TEXT("/Top.txt")).Equals(TEXT("1")));
        TEST_EXPECT(ReadText(Destination + TEXT("/A/B/Deep.txt")).Equals(TEXT("2")));
        TEST_EXPECT(FPlatformFile::IsDirectory(*(Destination + TEXT("/Empty"))));
        TEST_EXPECT(ReadText(Source + TEXT("/Top.txt")).Equals(TEXT("1")));
    }

    TEST_SECTION(TEXT("CopyDirectoryTree refuses to overwrite unless asked to"));
    {
        const FString Source      = Root + TEXT("/CopyTreeSource");
        const FString Destination = Root + TEXT("/CopyTreeDestination");
        TEST_EXPECT(WriteText(Source + TEXT("/Top.txt"), TEXT("New")));

        TEST_EXPECT(!File::CopyDirectoryTree(Source, Destination));
        TEST_EXPECT(ReadText(Destination + TEXT("/Top.txt")).Equals(TEXT("1")));

        TEST_EXPECT(File::CopyDirectoryTree(Source, Destination, true));
        TEST_EXPECT(ReadText(Destination + TEXT("/Top.txt")).Equals(TEXT("New")));
    }

    TEST_SECTION(TEXT("CopyDirectoryTree skips what the filter rejects"));
    {
        const FString Source      = Root + TEXT("/CopyTreeSource");
        const FString Destination = Root + TEXT("/CopyTreeFiltered");

        const bool bCopied = File::CopyDirectoryTree(Source, Destination, false, [](const FString& Path, bool bIsDirectory)
        {
            return !File::ExtractFilename(Path).Equals(TEXT("Top.txt")) && !(bIsDirectory && File::ExtractFilename(Path).Equals(TEXT("A")));
        });

        TEST_EXPECT(bCopied);
        TEST_EXPECT(!FPlatformFile::GetFileInfo(*(Destination + TEXT("/Top.txt"))).bExists);
        TEST_EXPECT(!FPlatformFile::GetFileInfo(*(Destination + TEXT("/A"))).bExists);
        TEST_EXPECT(FPlatformFile::IsDirectory(*(Destination + TEXT("/Empty"))));
    }

    TEST_SECTION(TEXT("CopyDirectoryTree refuses to copy a directory into itself"));
    {
        const FString Source = Root + TEXT("/CopyTreeSource");
        TEST_EXPECT(!File::CopyDirectoryTree(Source, Source + TEXT("/Inside")));
        TEST_EXPECT(!File::CopyDirectoryTree(Source, Source));
        TEST_EXPECT(!FPlatformFile::GetFileInfo(*(Source + TEXT("/Inside"))).bExists);
    }

    TEST_SECTION(TEXT("CopyDirectoryTree skips a link to a directory"));
    {
        const FString Target      = File::MakeAbsolute(Root + TEXT("/CopyLinkTarget"));
        const FString Source      = File::MakeAbsolute(Root + TEXT("/CopyTreeWithLink"));
        const FString Destination = File::MakeAbsolute(Root + TEXT("/CopyTreeWithLinkCopy"));
        TEST_EXPECT(File::CreateDirectoryTree(Target));
        TEST_EXPECT(File::CreateDirectoryTree(Source));
        TEST_EXPECT(WriteText(Target + TEXT("/Linked.txt"), TEXT("Linked")));
        TEST_EXPECT(WriteText(Source + TEXT("/Own.txt"), TEXT("Own")));
        TEST_EXPECT(CreateDirectoryLink(Source + TEXT("/Link"), Target));

        TEST_EXPECT(File::CopyDirectoryTree(Source, Destination));
        TEST_EXPECT(ReadText(Destination + TEXT("/Own.txt")).Equals(TEXT("Own")));
        TEST_EXPECT(!FPlatformFile::GetFileInfo(*(Destination + TEXT("/Link"))).bExists);
    }

    TEST_SECTION(TEXT("CopyDirectoryTree fails for a directory that does not exist"));
    {
        TEST_EXPECT(!File::CopyDirectoryTree(Root + TEXT("/Missing"), Root + TEXT("/MissingCopy")));
    }

    TEST_EXPECT(File::DeleteDirectoryTree(Root));
    TEST_EXPECT(!FPlatformFile::IsDirectory(*Root));

    TEST_SECTION(TEXT("NormalizePath cleans up separators, '.' and '..'"));
    {
        TEST_EXPECT(File::NormalizePath(TEXT("/Engine/./Saved/../Content//A.png")).Equals(TEXT("/Engine/Content/A.png")));
        TEST_EXPECT(File::NormalizePath(TEXT("/usr//local/./bin/")).Equals(TEXT("/usr/local/bin")));
        TEST_EXPECT(File::NormalizePath(TEXT("A/B/../../C")).Equals(TEXT("C")));
        TEST_EXPECT(File::NormalizePath(TEXT("A/..")).Equals(TEXT(".")));
        TEST_EXPECT(File::NormalizePath(TEXT("../../A")).Equals(TEXT("../../A")));
        TEST_EXPECT(File::NormalizePath(TEXT("A/../../B")).Equals(TEXT("../B")));
        TEST_EXPECT(File::NormalizePath(TEXT("/..")).Equals(TEXT("/")));
        TEST_EXPECT(File::NormalizePath(TEXT("/")).Equals(TEXT("/")));
        TEST_EXPECT(File::NormalizePath(TEXT("")).IsEmpty());
    }

    TEST_SECTION(TEXT("A backslash is a separator in an engine path on every platform"));
    {
        TEST_EXPECT(File::NormalizePath(TEXT("textures_pbr\\Thorn_diffuse.tga")).Equals(TEXT("textures_pbr/Thorn_diffuse.tga")));
        TEST_EXPECT(File::NormalizePath(TEXT("\\Engine\\Content\\")).Equals(TEXT("/Engine/Content")));
        TEST_EXPECT(File::NormalizePath(TEXT("A\\B/C\\..\\D")).Equals(TEXT("A/B/D")));
    }

    TEST_SECTION(TEXT("MakeAbsolute resolves against the working directory"));
    {
        const FString WorkingDirectory = File::NormalizePath(FPlatformFile::GetCurrentWorkingDirectory());
        TEST_EXPECT(File::MakeAbsolute(TEXT("A/B.txt")).Equals(WorkingDirectory + TEXT("/A/B.txt")));
        TEST_EXPECT(File::MakeAbsolute(TEXT(".")).Equals(WorkingDirectory));
        TEST_EXPECT(File::MakeAbsolute(TEXT("/Engine/../Game")).Equals(TEXT("/Game")));
    }

    TEST_SECTION(TEXT("MakeRelative walks up and down from the base directory"));
    {
        TEST_EXPECT(File::MakeRelative(TEXT("/Engine/Content/A.png"), TEXT("/Engine/Saved")).Equals(TEXT("../Content/A.png")));
        TEST_EXPECT(File::MakeRelative(TEXT("/Engine/Content/A.png"), TEXT("/Engine")).Equals(TEXT("Content/A.png")));
        TEST_EXPECT(File::MakeRelative(TEXT("/Engine"), TEXT("/Engine/Content/Deep")).Equals(TEXT("../..")));
        TEST_EXPECT(File::MakeRelative(TEXT("/Engine"), TEXT("/Engine/")).Equals(TEXT(".")));
        TEST_EXPECT(File::MakeRelative(TEXT("\\Engine\\Content"), TEXT("/Engine")).Equals(TEXT("Content")));
        TEST_EXPECT(File::MakeRelative(TEXT("/EngineData/File.txt"), TEXT("/Engine")).Equals(TEXT("../EngineData/File.txt")));
    }

    TEST_SECTION(TEXT("IsUnderDirectory compares whole directory names"));
    {
        TEST_EXPECT(File::IsUnderDirectory(TEXT("/Engine/Content/A.png"), TEXT("/Engine")));
        TEST_EXPECT(File::IsUnderDirectory(TEXT("/Engine/Content/A.png"), TEXT("/Engine/Content/")));
        TEST_EXPECT(File::IsUnderDirectory(TEXT("/Engine"), TEXT("/Engine")));
        TEST_EXPECT(File::IsUnderDirectory(TEXT("/Engine/Content/../Saved/B.txt"), TEXT("/Engine")));
        TEST_EXPECT(!File::IsUnderDirectory(TEXT("/EngineData/A.png"), TEXT("/Engine")));
        TEST_EXPECT(!File::IsUnderDirectory(TEXT("/Engine/../Game/A.png"), TEXT("/Engine")));
        TEST_EXPECT(!File::IsUnderDirectory(TEXT("/Engine"), TEXT("/Engine/Content")));
    }

    TEST_SECTION(TEXT("The user settings directory exists and the engine's lives inside it"));
    {
        const FString Directory = FPlatformFile::GetUserSettingsDirectory();
        TEST_EXPECT(!Directory.IsEmpty());
        TEST_EXPECT(!Directory.Contains('\\'));
        TEST_EXPECT(FPlatformFile::IsDirectory(*Directory));
        TEST_EXPECT(File::IsUnderDirectory(Paths::GetUserSettingsDir(), Directory));
    }

#if PLATFORM_WINDOWS
    TEST_SECTION(TEXT("Windows drive letters and UNC roots are roots"));
    {
        TEST_EXPECT(File::NormalizePath(TEXT("C:\\Engine\\Content\\")).Equals(TEXT("C:/Engine/Content")));
        TEST_EXPECT(File::NormalizePath(TEXT("C:/..")).Equals(TEXT("C:/")));
        TEST_EXPECT(File::NormalizePath(TEXT("\\\\Server\\Share\\Dir")).Equals(TEXT("//Server/Share/Dir")));
        TEST_EXPECT(File::MakeAbsolute(TEXT("C:\\Engine\\..\\Game")).Equals(TEXT("C:/Game")));
        TEST_EXPECT(File::MakeAbsolute(TEXT("//Server/Share/Dir")).Equals(TEXT("//Server/Share/Dir")));
        TEST_EXPECT(File::MakeRelative(TEXT("C:/Engine/Content/A.png"), TEXT("C:/Engine/Saved")).Equals(TEXT("../Content/A.png")));
        TEST_EXPECT(File::IsUnderDirectory(TEXT("C:/Engine/Content/A.png"), TEXT("C:/Engine")));
    }

    TEST_SECTION(TEXT("Paths on different Windows drives have nothing in common"));
    {
        TEST_EXPECT(File::MakeRelative(TEXT("D:/Other/File.txt"), TEXT("C:/Engine")).Equals(TEXT("D:/Other/File.txt")));
        TEST_EXPECT(!File::IsUnderDirectory(TEXT("D:/Engine/A.png"), TEXT("C:/Engine")));
        TEST_EXPECT(!File::IsUnderDirectory(TEXT("//Server/Share/A.png"), TEXT("C:/Engine")));
    }

    TEST_SECTION(TEXT("Windows paths compare without regard to case"));
    {
        TEST_EXPECT(File::IsUnderDirectory(TEXT("c:/engine/content/a.png"), TEXT("C:/Engine")));
        TEST_EXPECT(File::MakeRelative(TEXT("c:/engine/Content"), TEXT("C:/Engine")).Equals(TEXT("Content")));
    }
#else
    TEST_SECTION(TEXT("Outside Windows a leading '//' is the same root as '/'"));
    {
        TEST_EXPECT(File::NormalizePath(TEXT("//usr/bin")).Equals(TEXT("/usr/bin")));
        TEST_EXPECT(File::IsUnderDirectory(TEXT("//usr/bin"), TEXT("/usr")));
    }

    TEST_SECTION(TEXT("Outside Windows 'C:' is an ordinary name"));
    {
        const FString WorkingDirectory = File::NormalizePath(FPlatformFile::GetCurrentWorkingDirectory());
        TEST_EXPECT(File::NormalizePath(TEXT("C:/Engine/..")).Equals(TEXT("C:")));
        TEST_EXPECT(File::MakeAbsolute(TEXT("C:/Engine")).Equals(WorkingDirectory + TEXT("/C:/Engine")));
    }

    TEST_SECTION(TEXT("Outside Windows paths compare with regard to case"));
    {
        TEST_EXPECT(!File::IsUnderDirectory(TEXT("/engine/content/a.png"), TEXT("/Engine")));
        TEST_EXPECT(File::MakeRelative(TEXT("/engine/Content"), TEXT("/Engine")).Equals(TEXT("../engine/Content")));
    }
#endif

    TEST_END();
}
