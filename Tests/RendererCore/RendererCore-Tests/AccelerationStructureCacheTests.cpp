#include <Core/Filesystem/File.h>
#include <Core/Platform/PlatformFile.h>
#include <Core/Platform/PlatformThreadMisc.h>
#include <RendererCore/RayTracing/AccelerationStructureCacheBackend.h>

#include "TestCommon/TestMacros.h"

#include "AccelerationStructureCacheTests.h"

static TArray<uint8> MakeBytes(int32 Size, uint8 Value)
{
    TArray<uint8> Bytes;
    Bytes.Resize(Size);
    for (uint8& Byte : Bytes)
    {
        Byte = Value;
    }

    return Bytes;
}

static bool WriteRawFile(const FString& Filename)
{
    const TArray<uint8> Bytes = MakeBytes(8, 0xcd);

    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForWrite(Filename);
    return FileHandle && FileHandle->Write(Bytes.Data(), static_cast<uint32>(Bytes.Size())) == Bytes.Size();
}

static void WaitForNextFileTime()
{
    FPlatformThreadMisc::Sleep(FTimespan::Milliseconds(1100));
}

bool AccelerationStructureCacheBackend_Test()
{
    TEST_BEGIN();

    const FString Root(TEXT("AccelerationStructureCacheTests.Scratch"));
    File::DeleteDirectoryTree(Root);

    TEST_SECTION(TEXT("An entry round trips, and a missing key misses"));
    {
        FDiskAccelerationStructureCacheBackend Backend(Root, 0);
        Backend.Store(FStringView(TEXT("A")), MakeBytes(100, 1));

        TArray<uint8> Bytes;
        TEST_EXPECT(Backend.Load(FStringView(TEXT("A")), Bytes) && Bytes == MakeBytes(100, 1));
        TEST_EXPECT(!Backend.Load(FStringView(TEXT("B")), Bytes));
        TEST_EXPECT(Backend.GetNumEntries() == 1 && Backend.GetSizeInBytes() == 100);

        Backend.Store(FStringView(TEXT("A")), MakeBytes(40, 2));
        TEST_EXPECT(Backend.Load(FStringView(TEXT("A")), Bytes) && Bytes == MakeBytes(40, 2));
        TEST_EXPECT(Backend.GetNumEntries() == 1 && Backend.GetSizeInBytes() == 40);
    }

    TEST_SECTION(TEXT("Opening the folder again finds the entries, deletes unfinished writes and ignores other files"));
    {
        TEST_EXPECT(WriteRawFile(Root + TEXT("/B.rtas.tmp")));
        TEST_EXPECT(WriteRawFile(Root + TEXT("/Notes.txt")));

        FDiskAccelerationStructureCacheBackend Backend(Root, 0);

        TArray<uint8> Bytes;
        TEST_EXPECT(Backend.GetNumEntries() == 1 && Backend.Load(FStringView(TEXT("A")), Bytes) && Bytes == MakeBytes(40, 2));
        TEST_EXPECT(!FPlatformFile::IsFile(*(Root + TEXT("/B.rtas.tmp"))));
        TEST_EXPECT(FPlatformFile::IsFile(*(Root + TEXT("/Notes.txt"))));
    }

    TEST_SECTION(TEXT("RemoveEntries deletes the entries picked and their files"));
    {
        FDiskAccelerationStructureCacheBackend Backend(Root, 0);
        Backend.Store(FStringView(TEXT("Keep-1")), MakeBytes(10, 3));
        Backend.Store(FStringView(TEXT("Drop-1")), MakeBytes(10, 4));
        Backend.Store(FStringView(TEXT("Drop-2")), MakeBytes(10, 5));

        Backend.RemoveEntries([](const FString& Key)
        {
            return Key.StartsWith(TEXT("Drop-"));
        });

        TArray<uint8> Bytes;
        TEST_EXPECT(Backend.Load(FStringView(TEXT("Keep-1")), Bytes));
        TEST_EXPECT(!Backend.Load(FStringView(TEXT("Drop-1")), Bytes) && !Backend.Load(FStringView(TEXT("Drop-2")), Bytes));
        TEST_EXPECT(!FPlatformFile::IsFile(*(Root + TEXT("/Drop-1.rtas"))));
        TEST_EXPECT(Backend.GetSizeInBytes() == 50);
    }

    File::DeleteDirectoryTree(Root);

    TEST_SECTION(TEXT("Past the size limit, the least recently used entries are evicted first, a load counting as a use"));
    {
        {
            FDiskAccelerationStructureCacheBackend Backend(Root, 0);
            Backend.Store(FStringView(TEXT("Old")), MakeBytes(100, 6));
            WaitForNextFileTime();
            Backend.Store(FStringView(TEXT("Middle")), MakeBytes(100, 7));
            WaitForNextFileTime();

            TArray<uint8> Bytes;
            TEST_EXPECT(Backend.Load(FStringView(TEXT("Old")), Bytes));
        }

        FDiskAccelerationStructureCacheBackend Backend(Root, 150);

        TArray<uint8> Bytes;
        TEST_EXPECT(Backend.GetNumEntries() == 1 && Backend.GetSizeInBytes() == 100);
        TEST_EXPECT(Backend.Load(FStringView(TEXT("Old")), Bytes));
        TEST_EXPECT(!Backend.Load(FStringView(TEXT("Middle")), Bytes) && !FPlatformFile::IsFile(*(Root + TEXT("/Middle.rtas"))));

        WaitForNextFileTime();
        Backend.Store(FStringView(TEXT("New")), MakeBytes(100, 8));
        TEST_EXPECT(Backend.GetNumEntries() == 1 && Backend.Load(FStringView(TEXT("New")), Bytes));

        Backend.SetMaxSize(50);
        TEST_EXPECT(Backend.GetNumEntries() == 0 && Backend.GetSizeInBytes() == 0);
    }

    TEST_EXPECT(File::DeleteDirectoryTree(Root));

    TEST_END();
}
