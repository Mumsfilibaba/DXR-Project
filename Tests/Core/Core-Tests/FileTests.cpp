#include "FileTests.h"

#include <Core/Filesystem/File.h>
#include <Core/Containers/String.h>
#include <Core/Image/ImageView.h>
#include <Core/Image/PngWriter.h>
#include <Core/Json/Json.h>
#include <Core/Platform/PlatformFile.h>
#include <Core/Platform/PlatformProcess.h>
#include <Core/Templates/CString.h>

#include "TestCommon/TestMacros.h"

static bool WriteText(const String& Filename, const CHAR* Text)
{
    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForWrite(Filename);
    if (!FileHandle)
    {
        return false;
    }

    return File::WriteTextFile(FileHandle.Get(), String(Text));
}

static String ReadText(const String& Filename)
{
    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForRead(Filename);
    if (!FileHandle)
    {
        return String();
    }

    TArray<CHAR> Text;
    if (!File::ReadTextFile(FileHandle.Get(), Text))
    {
        return String();
    }

    return String(Text.Data());
}

bool File_Test()
{
    TEST_BEGIN();

    TEST_SECTION("The working directory has no trailing null-terminator");
    {
        const String WorkingDirectory = FPlatformFile::GetCurrentWorkingDirectory();
        TEST_EXPECT(!WorkingDirectory.IsEmpty());
        TEST_EXPECT(WorkingDirectory.Length() == static_cast<int32>(CString::Strlen(*WorkingDirectory)));
        TEST_EXPECT(FPlatformFile::IsDirectory(*WorkingDirectory));

        const String Appended = WorkingDirectory + "/File.txt";
        TEST_EXPECT(static_cast<int32>(CString::Strlen(*Appended)) == Appended.Length());
    }

    TEST_SECTION("GetDirectoryOf and ExtractFilename split on either separator");
    {
        TEST_EXPECT(File::GetDirectoryOf("Dir/Sub/File.txt").Equals("Dir/Sub"));
        TEST_EXPECT(File::GetDirectoryOf("Dir/File").Equals("Dir"));
        TEST_EXPECT(File::GetDirectoryOf("Dir\\Sub\\File.txt").Equals("Dir\\Sub"));
        TEST_EXPECT(File::GetDirectoryOf("File.txt").IsEmpty());
        TEST_EXPECT(File::GetDirectoryOf("").IsEmpty());

        TEST_EXPECT(File::ExtractFilename("Dir/Sub/File.txt").Equals("File.txt"));
        TEST_EXPECT(File::ExtractFilename("Dir\\Sub\\File.txt").Equals("File.txt"));
        TEST_EXPECT(File::ExtractFilename("Dir/Mixed\\File.txt").Equals("File.txt"));
        TEST_EXPECT(File::ExtractFilename("File.txt").Equals("File.txt"));
        TEST_EXPECT(File::ExtractFilename("Dir/").IsEmpty());
    }

    TEST_SECTION("ExtractFilenameWithoutExtension only looks at the filename");
    {
        TEST_EXPECT(File::ExtractFilenameWithoutExtension("Dir/Sub/File.txt").Equals("File"));
        TEST_EXPECT(File::ExtractFilenameWithoutExtension("File.txt").Equals("File"));
        TEST_EXPECT(File::ExtractFilenameWithoutExtension("Archive.tar.gz").Equals("Archive.tar"));

        TEST_EXPECT(File::ExtractFilenameWithoutExtension("Dir/File").Equals("File"));
        TEST_EXPECT(File::ExtractFilenameWithoutExtension("File").Equals("File"));

        TEST_EXPECT(File::ExtractFilenameWithoutExtension("Dir.v2/File").Equals("File"));
        TEST_EXPECT(File::ExtractFilenameWithoutExtension("Dir.v2/File.txt").Equals("File"));

        TEST_EXPECT(File::ExtractFilenameWithoutExtension("Dir/.gitignore").Equals(".gitignore"));
        TEST_EXPECT(File::ExtractExtension("Dir/.gitignore").IsEmpty());

        TEST_EXPECT(File::ExtractFilenameWithoutExtension("").IsEmpty());
    }

    TEST_SECTION("Json::SaveToFile writes a file that has no directory in its name");
    {
        const String Filename("FileTests.NoDirectory.json");

        FJsonValue Document = FJsonValue::MakeObject();
        Document.AddMember("Value", FJsonValue(1));

        TEST_EXPECT(Json::SaveToFile(Filename, Document));
        TEST_EXPECT(!FPlatformFile::IsDirectory(*Filename));

        FJsonValue FromDisk;
        TEST_EXPECT(Json::LoadFromFile(Filename, FromDisk));
        TEST_EXPECT(Document.Equals(FromDisk));

        TEST_EXPECT(FPlatformFile::DeleteFile(*Filename));
    }

    TEST_SECTION("FPngWriter writes a file that has no directory in its name");
    {
        const String Filename("FileTests.NoDirectory.png");
        const uint8  Pixel[FImageView::BytesPerPixel] = { 255, 0, 0, 255 };

        FPngWriter Writer;
        TEST_EXPECT(Writer.WriteToFile(Filename, FImageView(Pixel, 1, 1)));
        TEST_EXPECT(!FPlatformFile::IsDirectory(*Filename));
        TEST_EXPECT(FPlatformFile::IsFile(*Filename));

        TEST_EXPECT(FPlatformFile::DeleteFile(*Filename));
    }

    const String Root("FileTests.Scratch");
    File::DeleteDirectoryTree(Root);
    TEST_EXPECT(File::CreateDirectoryTree(Root));

    TEST_SECTION("IsFile is false for a directory");
    {
        const String Filename = Root + "/IsFile.txt";
        TEST_EXPECT(WriteText(Filename, "Text"));

        TEST_EXPECT(FPlatformFile::IsFile(*Filename));
        TEST_EXPECT(!FPlatformFile::IsFile(*Root));
        TEST_EXPECT(!FPlatformFile::IsFile(*(Root + "/Missing.txt")));
    }

    TEST_SECTION("GetFileInfo reports size, kind and write time");
    {
        const String Filename = Root + "/Info.txt";
        TEST_EXPECT(WriteText(Filename, "0123456789"));

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

        const FFileInfo MissingInfo = FPlatformFile::GetFileInfo(*(Root + "/Missing.txt"));
        TEST_EXPECT(!MissingInfo.bExists);
    }

    TEST_SECTION("CopyFile refuses to overwrite unless asked to");
    {
        const String Source      = Root + "/CopySource.txt";
        const String Destination = Root + "/CopyDestination.txt";
        TEST_EXPECT(WriteText(Source, "Source"));
        TEST_EXPECT(WriteText(Destination, "Old"));

        TEST_EXPECT(!FPlatformFile::CopyFile(*Source, *Destination));
        TEST_EXPECT(ReadText(Destination).Equals("Old"));

        TEST_EXPECT(FPlatformFile::CopyFile(*Source, *Destination, true));
        TEST_EXPECT(ReadText(Destination).Equals("Source"));
        TEST_EXPECT(ReadText(Source).Equals("Source"));

        const String NewDestination = Root + "/CopyNew.txt";
        TEST_EXPECT(FPlatformFile::CopyFile(*Source, *NewDestination));
        TEST_EXPECT(ReadText(NewDestination).Equals("Source"));
    }

    TEST_SECTION("MoveDirectory renames a directory with its contents and refuses an existing target");
    {
        const String From = Root + "/MoveFrom";
        const String To   = Root + "/MoveTo";
        TEST_EXPECT(File::CreateDirectoryTree(From + "/Nested"));
        TEST_EXPECT(WriteText(From + "/Nested/File.txt", "Moved"));

        TEST_EXPECT(FPlatformFile::MoveDirectory(*From, *To));
        TEST_EXPECT(!FPlatformFile::IsDirectory(*From));
        TEST_EXPECT(ReadText(To + "/Nested/File.txt").Equals("Moved"));

        const String Occupied = Root + "/Occupied";
        TEST_EXPECT(FPlatformFile::CreateDirectory(*Occupied));
        TEST_EXPECT(!FPlatformFile::MoveDirectory(*To, *Occupied));
        TEST_EXPECT(FPlatformFile::IsDirectory(*To));
    }

    TEST_SECTION("DeleteDirectoryTree removes nested directories and files");
    {
        const String Tree = Root + "/Tree";
        TEST_EXPECT(File::CreateDirectoryTree(Tree + "/A/B/C"));
        TEST_EXPECT(WriteText(Tree + "/Top.txt", "1"));
        TEST_EXPECT(WriteText(Tree + "/A/B/Deep.txt", "2"));
        TEST_EXPECT(WriteText(Tree + "/A/B/C/Deeper.txt", "3"));

        TEST_EXPECT(File::DeleteDirectoryTree(Tree));
        TEST_EXPECT(!FPlatformFile::GetFileInfo(*Tree).bExists);

        TEST_EXPECT(File::DeleteDirectoryTree(Tree));
    }

    TEST_SECTION("DeleteDirectoryTree removes a link to a directory without touching its target");
    {
        const String Target = File::MakeAbsolute(Root + "/LinkTarget");
        const String Tree   = File::MakeAbsolute(Root + "/TreeWithLink");
        const String Link   = Tree + "/Link";
        TEST_EXPECT(File::CreateDirectoryTree(Target));
        TEST_EXPECT(File::CreateDirectoryTree(Tree));
        TEST_EXPECT(WriteText(Target + "/Kept.txt", "Kept"));

        FProcessDesc Desc;
#if PLATFORM_WINDOWS
        String WindowsLink   = Link;
        String WindowsTarget = Target;
        WindowsLink.ReplaceAll('/', '\\');
        WindowsTarget.ReplaceAll('/', '\\');

        Desc.Executable = "cmd.exe";
        Desc.Arguments.Add(String("/c"));
        Desc.Arguments.Add(String("mklink"));
        Desc.Arguments.Add(String("/J"));
        Desc.Arguments.Add(WindowsLink);
        Desc.Arguments.Add(WindowsTarget);
#else
        Desc.Executable = "/bin/ln";
        Desc.Arguments.Add(String("-s"));
        Desc.Arguments.Add(Target);
        Desc.Arguments.Add(Link);
#endif
        Desc.bCaptureOutput = true;

        TUniquePtr<IPlatformProcessHandle> Process = FPlatformProcess::LaunchProcess(Desc);
        TEST_EXPECT(Process != nullptr);

        int32 ExitCode = -1;
        if (Process)
        {
            TEST_EXPECT(Process->Wait(30 * 1000));
            TEST_EXPECT(Process->GetExitCode(ExitCode));
        }

        TEST_EXPECT(ExitCode == 0);
        TEST_EXPECT(FPlatformFile::IsDirectory(*Link));

        TArray<FDirectoryEntry> Entries;
        TEST_EXPECT(FPlatformFile::IterateDirectory(*Tree, Entries));
        TEST_EXPECT((Entries.Size() == 1) && Entries[0].bIsSymbolicLink);

        TEST_EXPECT(File::DeleteDirectoryTree(Tree));
        TEST_EXPECT(!FPlatformFile::GetFileInfo(*Tree).bExists);
        TEST_EXPECT(ReadText(Target + "/Kept.txt").Equals("Kept"));
    }

    TEST_EXPECT(File::DeleteDirectoryTree(Root));
    TEST_EXPECT(!FPlatformFile::IsDirectory(*Root));

    TEST_SECTION("NormalizePath cleans up separators, '.' and '..'");
    {
        TEST_EXPECT(File::NormalizePath("/Engine/./Saved/../Content//A.png").Equals("/Engine/Content/A.png"));
        TEST_EXPECT(File::NormalizePath("/usr//local/./bin/").Equals("/usr/local/bin"));
        TEST_EXPECT(File::NormalizePath("A/B/../../C").Equals("C"));
        TEST_EXPECT(File::NormalizePath("A/..").Equals("."));
        TEST_EXPECT(File::NormalizePath("../../A").Equals("../../A"));
        TEST_EXPECT(File::NormalizePath("A/../../B").Equals("../B"));
        TEST_EXPECT(File::NormalizePath("/..").Equals("/"));
        TEST_EXPECT(File::NormalizePath("/").Equals("/"));
        TEST_EXPECT(File::NormalizePath("").IsEmpty());
    }

    TEST_SECTION("A backslash is a separator in an engine path on every platform");
    {
        TEST_EXPECT(File::NormalizePath("textures_pbr\\Thorn_diffuse.tga").Equals("textures_pbr/Thorn_diffuse.tga"));
        TEST_EXPECT(File::NormalizePath("\\Engine\\Content\\").Equals("/Engine/Content"));
        TEST_EXPECT(File::NormalizePath("A\\B/C\\..\\D").Equals("A/B/D"));
    }

    TEST_SECTION("MakeAbsolute resolves against the working directory");
    {
        const String WorkingDirectory = File::NormalizePath(FPlatformFile::GetCurrentWorkingDirectory());
        TEST_EXPECT(File::MakeAbsolute("A/B.txt").Equals(WorkingDirectory + "/A/B.txt"));
        TEST_EXPECT(File::MakeAbsolute(".").Equals(WorkingDirectory));
        TEST_EXPECT(File::MakeAbsolute("/Engine/../Game").Equals("/Game"));
    }

    TEST_SECTION("MakeRelative walks up and down from the base directory");
    {
        TEST_EXPECT(File::MakeRelative("/Engine/Content/A.png", "/Engine/Saved").Equals("../Content/A.png"));
        TEST_EXPECT(File::MakeRelative("/Engine/Content/A.png", "/Engine").Equals("Content/A.png"));
        TEST_EXPECT(File::MakeRelative("/Engine", "/Engine/Content/Deep").Equals("../.."));
        TEST_EXPECT(File::MakeRelative("/Engine", "/Engine/").Equals("."));
        TEST_EXPECT(File::MakeRelative("\\Engine\\Content", "/Engine").Equals("Content"));
        TEST_EXPECT(File::MakeRelative("/EngineData/File.txt", "/Engine").Equals("../EngineData/File.txt"));
    }

    TEST_SECTION("IsUnderDirectory compares whole directory names");
    {
        TEST_EXPECT(File::IsUnderDirectory("/Engine/Content/A.png", "/Engine"));
        TEST_EXPECT(File::IsUnderDirectory("/Engine/Content/A.png", "/Engine/Content/"));
        TEST_EXPECT(File::IsUnderDirectory("/Engine", "/Engine"));
        TEST_EXPECT(File::IsUnderDirectory("/Engine/Content/../Saved/B.txt", "/Engine"));
        TEST_EXPECT(!File::IsUnderDirectory("/EngineData/A.png", "/Engine"));
        TEST_EXPECT(!File::IsUnderDirectory("/Engine/../Game/A.png", "/Engine"));
        TEST_EXPECT(!File::IsUnderDirectory("/Engine", "/Engine/Content"));
    }

#if PLATFORM_WINDOWS
    TEST_SECTION("Windows drive letters and UNC roots are roots");
    {
        TEST_EXPECT(File::NormalizePath("C:\\Engine\\Content\\").Equals("C:/Engine/Content"));
        TEST_EXPECT(File::NormalizePath("C:/..").Equals("C:/"));
        TEST_EXPECT(File::NormalizePath("\\\\Server\\Share\\Dir").Equals("//Server/Share/Dir"));
        TEST_EXPECT(File::MakeAbsolute("C:\\Engine\\..\\Game").Equals("C:/Game"));
        TEST_EXPECT(File::MakeAbsolute("//Server/Share/Dir").Equals("//Server/Share/Dir"));
        TEST_EXPECT(File::MakeRelative("C:/Engine/Content/A.png", "C:/Engine/Saved").Equals("../Content/A.png"));
        TEST_EXPECT(File::IsUnderDirectory("C:/Engine/Content/A.png", "C:/Engine"));
    }

    TEST_SECTION("Paths on different Windows drives have nothing in common");
    {
        TEST_EXPECT(File::MakeRelative("D:/Other/File.txt", "C:/Engine").Equals("D:/Other/File.txt"));
        TEST_EXPECT(!File::IsUnderDirectory("D:/Engine/A.png", "C:/Engine"));
        TEST_EXPECT(!File::IsUnderDirectory("//Server/Share/A.png", "C:/Engine"));
    }

    TEST_SECTION("Windows paths compare without regard to case");
    {
        TEST_EXPECT(File::IsUnderDirectory("c:/engine/content/a.png", "C:/Engine"));
        TEST_EXPECT(File::MakeRelative("c:/engine/Content", "C:/Engine").Equals("Content"));
    }
#else
    TEST_SECTION("Outside Windows a leading '//' is the same root as '/'");
    {
        TEST_EXPECT(File::NormalizePath("//usr/bin").Equals("/usr/bin"));
        TEST_EXPECT(File::IsUnderDirectory("//usr/bin", "/usr"));
    }

    TEST_SECTION("Outside Windows 'C:' is an ordinary name");
    {
        const String WorkingDirectory = File::NormalizePath(FPlatformFile::GetCurrentWorkingDirectory());
        TEST_EXPECT(File::NormalizePath("C:/Engine/..").Equals("C:"));
        TEST_EXPECT(File::MakeAbsolute("C:/Engine").Equals(WorkingDirectory + "/C:/Engine"));
    }

    TEST_SECTION("Outside Windows paths compare with regard to case");
    {
        TEST_EXPECT(!File::IsUnderDirectory("/engine/content/a.png", "/Engine"));
        TEST_EXPECT(File::MakeRelative("/engine/Content", "/Engine").Equals("../engine/Content"));
    }
#endif

    TEST_END();
}
