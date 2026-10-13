#include "AssetFileTests.h"

#include <AssetCore/AssetFile.h>
#include <Core/Filesystem/File.h>
#include <Core/Json/Json.h>
#include <Core/Memory/Memory.h>
#include <Core/Platform/PlatformFile.h>

#include "TestCommon/TestMacros.h"

static constexpr FGuid TestGuid(0x01234567u, 0x89ABCDEFu, 0xFEDCBA98u, 0x76543210u);

static TArray<uint8> MakePayload(int32 Size, uint8 Seed)
{
    TArray<uint8> Payload;
    for (int32 Index = 0; Index < Size; ++Index)
    {
        Payload.Add(static_cast<uint8>(Seed + Index * 7));
    }

    return Payload;
}

static FJsonValue MakeMetadata()
{
    FJsonValue Metadata = FJsonValue::MakeObject();
    Metadata.AddMember(TEXT("Name"), FJsonValue(TEXT("Brick Wall åäö")));
    Metadata.AddMember(TEXT("SourceFile"), FJsonValue(TEXT("Project:Textures/BrickWall.png")));
    return Metadata;
}

static FAssetFileWriter MakeWriter()
{
    FAssetFileWriter Writer(TestGuid, EAssetType::Texture);
    Writer.SetChunk(AssetChunk::Data, MakePayload(1000, 3));
    Writer.SetChunk(AssetChunk::Thumbnail, MakePayload(37, 11));
    Writer.SetMetadata(MakeMetadata());
    return Writer;
}

static TArray<uint8> ReadBytes(const FString& Filename)
{
    TArray<uint8> Bytes;
    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForRead(Filename);
    if (FileHandle)
    {
        File::ReadFile(FileHandle.Get(), Bytes);
    }

    return Bytes;
}

static bool WriteBytes(const FString& Filename, const TArray<uint8>& Bytes)
{
    TFileRef<IPlatformFile> FileHandle = FPlatformFile::OpenForWrite(Filename);
    return FileHandle && FileHandle->Write(Bytes.Data(), static_cast<uint32>(Bytes.Size())) == Bytes.Size();
}

bool AssetFile_Test()
{
    TEST_BEGIN();

    const FString Root(TEXT("AssetFileTests.Scratch"));
    File::DeleteDirectoryTree(Root);
    TEST_EXPECT(File::CreateDirectoryTree(Root));

    const FString Filename = Root + TEXT("/BrickWall.dxrasset");

    TEST_SECTION(TEXT("An asset round trips through a file"));
    {
        FString Error;
        TEST_EXPECT(MakeWriter().SaveToFile(Filename, Error));
        TEST_EXPECT(!FPlatformFile::IsFile(*(Filename + TEXT(".tmp"))));

        FAssetFileReader Reader;
        TEST_EXPECT(Reader.Open(Filename, Error));
        TEST_EXPECT(Reader.GetGuid() == TestGuid);
        TEST_EXPECT(Reader.GetType() == EAssetType::Texture);
        TEST_EXPECT(Reader.GetChunks().Size() == 3);
        TEST_EXPECT(Reader.HasChunk(AssetChunk::Metadata) && Reader.HasChunk(AssetChunk::Thumbnail) && Reader.HasChunk(AssetChunk::Data));

        TArray<uint8> Data;
        TEST_EXPECT(Reader.ReadChunk(AssetChunk::Data, Data, Error));
        TEST_EXPECT(Data == MakePayload(1000, 3));
        TEST_EXPECT(Reader.ReadChunk(AssetChunk::Thumbnail, Data, Error));
        TEST_EXPECT(Data == MakePayload(37, 11));

        FJsonValue Metadata;
        TEST_EXPECT(Reader.ReadMetadata(Metadata, Error));
        TEST_EXPECT(Metadata.Equals(MakeMetadata()));
    }

    TEST_SECTION(TEXT("META comes first, every chunk is aligned, and the same chunks give the same bytes"));
    {
        FAssetFileReader Reader;
        FString Error;
        TEST_EXPECT(Reader.Open(Filename, Error));
        TEST_EXPECT(Reader.GetChunks()[0].Id == AssetChunk::Metadata);
        TEST_EXPECT(Reader.GetChunks()[1].Id == AssetChunk::Data);
        TEST_EXPECT(Reader.GetChunks()[2].Id == AssetChunk::Thumbnail);

        for (const FAssetChunkEntry& Entry : Reader.GetChunks())
        {
            TEST_EXPECT(Entry.Offset % FAssetFile::ChunkAlignment == 0);
        }

        TEST_EXPECT(MakeWriter().SaveToFile(Root + TEXT("/Again.dxrasset"), Error));
        TEST_EXPECT(!ReadBytes(Filename).IsEmpty() && ReadBytes(Filename) == ReadBytes(Root + TEXT("/Again.dxrasset")));
    }

    TEST_SECTION(TEXT("Setting a chunk again replaces it, an empty chunk is allowed, and a view is written like a copy"));
    {
        const TArray<uint8> Viewed = MakePayload(5, 99);

        FAssetFileWriter Writer = MakeWriter();
        Writer.SetChunkView(AssetChunk::Data, Viewed.Data(), static_cast<uint64>(Viewed.Size()));
        Writer.SetChunk(AssetChunk::Thumbnail, TArray<uint8>());

        FString Error;
        TEST_EXPECT(Writer.SaveToFile(Root + TEXT("/Replaced.dxrasset"), Error));

        FAssetFileReader Reader;
        TEST_EXPECT(Reader.Open(Root + TEXT("/Replaced.dxrasset"), Error));
        TEST_EXPECT(Reader.GetChunks().Size() == 3);

        TArray<uint8> Data;
        TEST_EXPECT(Reader.ReadChunk(AssetChunk::Data, Data, Error) && Data == Viewed);
        TEST_EXPECT(Reader.ReadChunk(AssetChunk::Thumbnail, Data, Error) && Data.IsEmpty());

        FAssetFileWriter Copied = MakeWriter();
        Copied.SetChunk(AssetChunk::Data, MakePayload(5, 99));
        Copied.SetChunk(AssetChunk::Thumbnail, TArray<uint8>());
        TEST_EXPECT(Copied.SaveToFile(Root + TEXT("/Copied.dxrasset"), Error));
        TEST_EXPECT(ReadBytes(Root + TEXT("/Replaced.dxrasset")) == ReadBytes(Root + TEXT("/Copied.dxrasset")));
    }

    TEST_SECTION(TEXT("A chunk is read into a buffer, or in part"));
    {
        FAssetFileReader Reader;
        FString Error;
        TEST_EXPECT(Reader.Open(Filename, Error));

        const TArray<uint8> Payload = MakePayload(1000, 3);

        uint8 Buffer[1024] = {};
        TEST_EXPECT(Reader.ReadChunk(AssetChunk::Data, Buffer, sizeof(Buffer), Error));
        TEST_EXPECT(Memory::Memcmp(Buffer, Payload.Data(), 1000) == 0);

        Error.Clear();
        TEST_EXPECT(!Reader.ReadChunk(AssetChunk::Data, Buffer, 999, Error) && Error.Contains(TEXT("1000 bytes")));

        uint8 Part[100] = {};
        TEST_EXPECT(Reader.ReadChunkRange(AssetChunk::Data, 900, Part, sizeof(Part), Error));
        TEST_EXPECT(Memory::Memcmp(Part, Payload.Data() + 900, sizeof(Part)) == 0);
        TEST_EXPECT(Reader.ReadChunkRange(AssetChunk::Data, 1000, Part, 0, Error));

        Error.Clear();
        TEST_EXPECT(!Reader.ReadChunkRange(AssetChunk::Data, 901, Part, sizeof(Part), Error) && Error.Contains(TEXT("past the end")));
    }

    TEST_SECTION(TEXT("Opening checks only the header and chunk table, and a chunk is checked when it is read whole"));
    {
        FAssetFileReader Probe;
        FString Error;
        TEST_EXPECT(Probe.Open(Filename, Error));
        const FAssetChunkEntry* DataEntry = Probe.FindChunk(AssetChunk::Data);
        TEST_EXPECT(DataEntry != nullptr);
        const uint64 DataOffset = DataEntry ? DataEntry->Offset : 0;
        Probe.Close();

        TArray<uint8> Bytes = ReadBytes(Filename);
        Bytes[static_cast<int32>(DataOffset + 10)] ^= 0xFF;
        TEST_EXPECT(WriteBytes(Root + TEXT("/CorruptPayload.dxrasset"), Bytes));

        FAssetFileReader Reader;
        TEST_EXPECT(Reader.Open(Root + TEXT("/CorruptPayload.dxrasset"), Error));

        FJsonValue Metadata;
        TEST_EXPECT(Reader.ReadMetadata(Metadata, Error));

        uint8 Part[8] = {};
        TEST_EXPECT(Reader.ReadChunkRange(AssetChunk::Data, 0, Part, sizeof(Part), Error));

        TArray<uint8> Data;
        Error.Clear();
        TEST_EXPECT(!Reader.ReadChunk(AssetChunk::Data, Data, Error));
        TEST_EXPECT(Error.Contains(TEXT("DATA")) && Error.Contains(TEXT("checksum")));
    }

    TEST_SECTION(TEXT("A damaged header, a truncated file, another file type or a newer version fails to open"));
    {
        const TArray<uint8> Bytes = ReadBytes(Filename);

        TArray<uint8> Header = Bytes;
        Header[24] ^= 0x01;
        TEST_EXPECT(WriteBytes(Root + TEXT("/CorruptHeader.dxrasset"), Header));

        TArray<uint8> Truncated(Bytes.Data(), Bytes.Size() - 100);
        TEST_EXPECT(WriteBytes(Root + TEXT("/Truncated.dxrasset"), Truncated));

        TArray<uint8> Magic = Bytes;
        Magic[0] = 'X';
        TEST_EXPECT(WriteBytes(Root + TEXT("/Magic.dxrasset"), Magic));

        TArray<uint8> Newer = Bytes;
        Newer[8] = static_cast<uint8>(FAssetFile::CurrentVersion + 1);
        TEST_EXPECT(WriteBytes(Root + TEXT("/Newer.dxrasset"), Newer));

        TArray<uint8> Tiny(Bytes.Data(), 12);
        TEST_EXPECT(WriteBytes(Root + TEXT("/Tiny.dxrasset"), Tiny));

        FAssetFileReader Reader;
        FString Error;
        TEST_EXPECT(!Reader.Open(Root + TEXT("/CorruptHeader.dxrasset"), Error) && Error.Contains(TEXT("header checksum")));
        TEST_EXPECT(!Reader.Open(Root + TEXT("/Truncated.dxrasset"), Error) && Error.Contains(TEXT("truncated")));
        TEST_EXPECT(!Reader.Open(Root + TEXT("/Magic.dxrasset"), Error) && Error.Contains(TEXT("not an asset file")));
        TEST_EXPECT(!Reader.Open(Root + TEXT("/Newer.dxrasset"), Error) && Error.Contains(TEXT("newer engine")));
        TEST_EXPECT(!Reader.Open(Root + TEXT("/Tiny.dxrasset"), Error) && Error.Contains(TEXT("too small")));
        TEST_EXPECT(!Reader.Open(Root + TEXT("/Missing.dxrasset"), Error) && Error.Contains(TEXT("Missing.dxrasset")));
        TEST_EXPECT(!Reader.IsOpen());
    }

    TEST_SECTION(TEXT("Reading a chunk the asset doesn't have names it"));
    {
        FAssetFileWriter Writer(TestGuid, EAssetType::Mesh);
        Writer.SetChunk(AssetChunk::Data, MakePayload(16, 1));

        FString Error;
        TEST_EXPECT(Writer.SaveToFile(Root + TEXT("/NoMetadata.dxrasset"), Error));

        FAssetFileReader Reader;
        TEST_EXPECT(Reader.Open(Root + TEXT("/NoMetadata.dxrasset"), Error));
        TEST_EXPECT(!Reader.HasChunk(AssetChunk::Metadata));

        FJsonValue Metadata;
        TEST_EXPECT(!Reader.ReadMetadata(Metadata, Error) && Error.Contains(TEXT("no 'META' chunk")));
    }

    TEST_EXPECT(File::DeleteDirectoryTree(Root));

    TEST_END();
}

bool AssetFileLarge_Test()
{
    TEST_BEGIN();

    // Past 2^31, so the offsets and sizes only fit in 64 bits, and with a byte pattern that differs between blocks
    static constexpr uint64 ChunkSize = 2ull * 1024ull * 1024ull * 1024ull + 64ull * 1024ull * 1024ull + 3ull;
    static constexpr uint64 BlockSize = 1024ull * 1024ull;

    const FString Root(TEXT("AssetFileLargeTests.Scratch"));
    const FString Filename = Root + TEXT("/Large.dxrasset");
    File::DeleteDirectoryTree(Root);
    TEST_EXPECT(File::CreateDirectoryTree(Root));

    uint8* Buffer = static_cast<uint8*>(Memory::Malloc(ChunkSize));
    TEST_EXPECT(Buffer != nullptr);

    const auto ExpectedByte = [](uint64 Position)
    {
        return static_cast<uint8>((Position / BlockSize) * 31 + Position);
    };

    for (uint64 Position = 0; Position < ChunkSize; ++Position)
    {
        Buffer[Position] = ExpectedByte(Position);
    }

    TEST_SECTION(TEXT("A chunk over 2 GB is written from a view and read back whole and in part"));
    {
        FAssetFileWriter Writer(TestGuid, EAssetType::Texture);
        Writer.SetMetadata(MakeMetadata());
        Writer.SetChunkView(AssetChunk::Data, Buffer, ChunkSize);
        Writer.SetChunk(AssetChunk::Thumbnail, MakePayload(37, 11));

        FString Error;
        TEST_EXPECT(Writer.SaveToFile(Filename, Error));

        FAssetFileReader Reader;
        TEST_EXPECT(Reader.Open(Filename, Error));

        const FAssetChunkEntry* DataEntry = Reader.FindChunk(AssetChunk::Data);
        TEST_EXPECT(DataEntry && DataEntry->Size == ChunkSize);

        TArray<uint8> Thumbnail;
        TEST_EXPECT(Reader.ReadChunk(AssetChunk::Thumbnail, Thumbnail, Error) && Thumbnail == MakePayload(37, 11));
        TEST_EXPECT(Reader.FindChunk(AssetChunk::Thumbnail) && Reader.FindChunk(AssetChunk::Thumbnail)->Offset > ChunkSize);

        uint8 Part[64] = {};
        const uint64 PartOffset = ChunkSize - sizeof(Part);
        TEST_EXPECT(Reader.ReadChunkRange(AssetChunk::Data, PartOffset, Part, sizeof(Part), Error));

        bool bPartMatches = true;
        for (uint64 Index = 0; Index < sizeof(Part); ++Index)
        {
            bPartMatches &= Part[Index] == ExpectedByte(PartOffset + Index);
        }

        TEST_EXPECT(bPartMatches);

        TArray<uint8> TooLarge;
        Error.Clear();
        TEST_EXPECT(!Reader.ReadChunk(AssetChunk::Data, TooLarge, Error) && Error.Contains(TEXT("too large for an array")));

        Memory::Memzero(Buffer, ChunkSize);
        TEST_EXPECT(Reader.ReadChunk(AssetChunk::Data, Buffer, ChunkSize, Error));

        bool bWholeMatches = true;
        for (uint64 Position = 0; Position < ChunkSize; Position += 4093)
        {
            bWholeMatches &= Buffer[Position] == ExpectedByte(Position);
        }

        TEST_EXPECT(bWholeMatches && Buffer[ChunkSize - 1] == ExpectedByte(ChunkSize - 1));
    }

    Memory::Free(Buffer);
    TEST_EXPECT(File::DeleteDirectoryTree(Root));

    TEST_END();
}
