#include "AssetCore/AssetFile.h"
#include "AssetCore/EnvironmentAsset.h"
#include "Core/Memory/Memory.h"

struct FEnvironmentPayloadHeader
{
    uint32 Magic;
    uint32 Version;
    uint32 FilterVersion;
    uint32 NumCubes;
};

struct FEnvironmentPayloadCube
{
    uint8  Format;
    uint8  Reserved[3];
    uint32 NumMips;
};

struct FEnvironmentPayloadMip
{
    uint32 Size;
    uint32 Reserved;
    uint64 Offset;
    uint64 DataSize;
};

static_assert(sizeof(FEnvironmentPayloadHeader) == 16, "FEnvironmentPayloadHeader is stored in files and must keep its layout");
static_assert(sizeof(FEnvironmentPayloadCube) == 8, "FEnvironmentPayloadCube is stored in files and must keep its layout");
static_assert(sizeof(FEnvironmentPayloadMip) == 24, "FEnvironmentPayloadMip is stored in files and must keep its layout");

static constexpr uint32 EnvironmentPayloadMagic   = MakeAssetChunkId("DXEV");
static constexpr uint32 EnvironmentPayloadVersion = 1;
static constexpr uint32 NumEnvironmentCubes       = 3;
static constexpr uint64 MipAlignment              = 16;
static constexpr uint32 MaxCubeMips               = 16;
static constexpr uint32 MaxCubeSize               = 16384;

static uint64 AlignMipOffset(uint64 Offset)
{
    return (Offset + MipAlignment - 1) & ~(MipAlignment - 1);
}

static uint64 GetCubeMipSize(ETextureFormat Format, int32 Size)
{
    return static_cast<uint64>(TextureAssets::GetMipSize(Format, Size, Size)) * EnvironmentAssets::NumCubeFaces;
}

void EnvironmentAssets::Write(const FEnvironmentAssetData& Environment, TArray<uint8>& OutPayload)
{
    const FEnvironmentCube* Cubes[NumEnvironmentCubes] = { &Environment.Sky, &Environment.Specular, &Environment.Diffuse };

    uint32 NumMips = 0;
    for (const FEnvironmentCube* Cube : Cubes)
    {
        NumMips += Cube->Mips.Size();
    }

    const uint64 TableSize = sizeof(FEnvironmentPayloadHeader) + NumEnvironmentCubes * sizeof(FEnvironmentPayloadCube) + NumMips * sizeof(FEnvironmentPayloadMip);

    TArray<FEnvironmentPayloadMip> Entries;
    uint64 Offset = AlignMipOffset(TableSize);

    for (const FEnvironmentCube* Cube : Cubes)
    {
        for (const FTextureMip& Mip : Cube->Mips)
        {
            FEnvironmentPayloadMip& Entry = Entries.Emplace();
            Entry.Size     = static_cast<uint32>(Mip.Width);
            Entry.Reserved = 0;
            Entry.Offset   = Offset;
            Entry.DataSize = static_cast<uint64>(Mip.Data.Size());
            Offset = AlignMipOffset(Offset + Entry.DataSize);
        }
    }

    OutPayload.Reset(static_cast<int32>(Offset), 0);

    FEnvironmentPayloadHeader Header;
    Header.Magic         = EnvironmentPayloadMagic;
    Header.Version       = EnvironmentPayloadVersion;
    Header.FilterVersion = Environment.FilterVersion;
    Header.NumCubes      = NumEnvironmentCubes;

    uint8* Cursor = OutPayload.Data();
    Memory::Memcpy(Cursor, &Header, sizeof(Header));
    Cursor += sizeof(Header);

    for (const FEnvironmentCube* Cube : Cubes)
    {
        FEnvironmentPayloadCube Entry;
        Memory::Memzero(&Entry, sizeof(Entry));
        Entry.Format  = static_cast<uint8>(Cube->Format);
        Entry.NumMips = static_cast<uint32>(Cube->Mips.Size());

        Memory::Memcpy(Cursor, &Entry, sizeof(Entry));
        Cursor += sizeof(Entry);
    }

    Memory::Memcpy(Cursor, Entries.Data(), Entries.Size() * sizeof(FEnvironmentPayloadMip));

    int32 EntryIndex = 0;
    for (const FEnvironmentCube* Cube : Cubes)
    {
        for (const FTextureMip& Mip : Cube->Mips)
        {
            Memory::Memcpy(OutPayload.Data() + Entries[EntryIndex].Offset, Mip.Data.Data(), Mip.Data.Size());
            ++EntryIndex;
        }
    }
}

bool EnvironmentAssets::Read(const TArray<uint8>& Payload, FEnvironmentAssetData& OutEnvironment, FString& OutError)
{
    const uint64 PayloadSize = static_cast<uint64>(Payload.Size());

    FEnvironmentPayloadHeader Header;
    FEnvironmentPayloadCube   CubeEntries[NumEnvironmentCubes];

    if (PayloadSize < sizeof(Header) + sizeof(CubeEntries))
    {
        OutError = FString(TEXT("The environment data is shorter than its header"));
        return false;
    }

    Memory::Memcpy(&Header, Payload.Data(), sizeof(Header));
    Memory::Memcpy(CubeEntries, Payload.Data() + sizeof(Header), sizeof(CubeEntries));

    if (Header.Magic != EnvironmentPayloadMagic)
    {
        OutError = FString(TEXT("The environment data doesn't start with 'DXEV'"));
        return false;
    }

    if (Header.Version != EnvironmentPayloadVersion || Header.NumCubes != NumEnvironmentCubes)
    {
        OutError = FString::Printf(TEXT("The environment data is version %u with %u cubes, and only version %u with %u can be read"), Header.Version, Header.NumCubes, EnvironmentPayloadVersion, NumEnvironmentCubes);
        return false;
    }

    FEnvironmentAssetData Environment;
    Environment.FilterVersion = Header.FilterVersion;

    FEnvironmentCube* Cubes[NumEnvironmentCubes] = { &Environment.Sky, &Environment.Specular, &Environment.Diffuse };
    uint64 MipTableOffset = sizeof(Header) + sizeof(CubeEntries);

    for (uint32 CubeIndex = 0; CubeIndex < NumEnvironmentCubes; ++CubeIndex)
    {
        FEnvironmentCube&              Cube  = *Cubes[CubeIndex];
        const FEnvironmentPayloadCube& Entry = CubeEntries[CubeIndex];

        Cube.Format = static_cast<ETextureFormat>(Entry.Format);
        if (TextureAssets::GetElementSize(Cube.Format) == 0 || Entry.NumMips == 0 || Entry.NumMips > MaxCubeMips)
        {
            OutError = FString::Printf(TEXT("Cube %u of the environment data has format %u and %u mips"), CubeIndex, Entry.Format, Entry.NumMips);
            return false;
        }

        for (uint32 MipIndex = 0; MipIndex < Entry.NumMips; ++MipIndex)
        {
            FEnvironmentPayloadMip Mip;
            if (MipTableOffset + sizeof(Mip) > PayloadSize)
            {
                OutError = FString(TEXT("The environment data is shorter than its mip table"));
                return false;
            }

            Memory::Memcpy(&Mip, Payload.Data() + MipTableOffset, sizeof(Mip));
            MipTableOffset += sizeof(Mip);

            if (Mip.Size == 0 || Mip.Size > MaxCubeSize || Mip.DataSize != GetCubeMipSize(Cube.Format, static_cast<int32>(Mip.Size)))
            {
                OutError = FString::Printf(TEXT("Mip %u of cube %u is %u pixels with %llu bytes, which isn't six %s faces of that size"), MipIndex, CubeIndex, Mip.Size, Mip.DataSize, TextureAssets::GetFormatName(Cube.Format));
                return false;
            }

            if (Mip.Offset > PayloadSize || Mip.DataSize > PayloadSize - Mip.Offset)
            {
                OutError = FString::Printf(TEXT("Mip %u of cube %u lies past the end of the environment data"), MipIndex, CubeIndex);
                return false;
            }

            FTextureMip& CubeMip = Cube.Mips.Emplace();
            CubeMip.Width  = static_cast<int32>(Mip.Size);
            CubeMip.Height = static_cast<int32>(Mip.Size);
            CubeMip.Data.Reset(Payload.Data() + Mip.Offset, static_cast<int32>(Mip.DataSize));
        }
    }

    OutEnvironment = ::Move(Environment);
    return true;
}
