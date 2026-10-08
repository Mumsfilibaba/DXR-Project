#include "FileTests.h"

#include <Core/Filesystem/File.h>
#include <Core/Containers/String.h>
#include <Core/Image/ImageView.h>
#include <Core/Image/PngWriter.h>
#include <Core/Json/Json.h>
#include <Core/Platform/PlatformFile.h>
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

    TEST_SECTION("ExtractFilepath returns the directory part only");
    {
        TEST_EXPECT(File::ExtractFilepath("Dir/Sub/File.txt").Equals("Dir/Sub"));
        TEST_EXPECT(File::ExtractFilepath("Dir/File").Equals("Dir"));
        TEST_EXPECT(File::ExtractFilepath("File.txt").IsEmpty());
        TEST_EXPECT(File::ExtractFilepath("").IsEmpty());
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

    TEST_EXPECT(File::DeleteDirectoryTree(Root));
    TEST_EXPECT(!FPlatformFile::IsDirectory(*Root));

    TEST_END();
}
