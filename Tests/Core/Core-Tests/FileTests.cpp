#include "FileTests.h"

#include <Core/Filesystem/File.h>
#include <Core/Containers/String.h>
#include <Core/Image/ImageView.h>
#include <Core/Image/PngWriter.h>
#include <Core/Json/Json.h>
#include <Core/Platform/PlatformFile.h>
#include <Core/Templates/CString.h>

#include "TestCommon/TestMacros.h"

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

    TEST_END();
}
