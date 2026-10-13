#include "AssetCore/AssetFile.h"
#include "Core/Containers/StringConversion.h"
#include "Core/Memory/Memory.h"
#include "Core/Templates/NumericLimits.h"
#include "Engine/Assets/MeshAsset.h"

struct FMeshPayloadHeader
{
    uint32 Magic;
    uint32 Version;
    uint32 AttributeFlags;
    uint32 NumVertices;
    uint32 NumIndices;
    uint32 NumSubMeshes;
    uint32 NumMaterialSlots;
    uint32 Reserved;
};

struct FMeshPayloadSection
{
    uint64 Offset;
    uint64 Size;
};

struct FMeshPayloadSubMesh
{
    uint32 BaseVertex;
    uint32 VertexCount;
    uint32 StartIndex;
    uint32 IndexCount;
    int32  MaterialIndex;
};

// Followed by the name and the path, in UTF-8 without terminators
struct FMeshPayloadSlot
{
    FGuid  Guid;
    uint32 NameSize;
    uint32 PathSize;
};

static_assert(sizeof(FMeshPayloadHeader) == 32, "FMeshPayloadHeader is stored in files and must keep its layout");
static_assert(sizeof(FMeshPayloadSection) == 16, "FMeshPayloadSection is stored in files and must keep its layout");
static_assert(sizeof(FMeshPayloadSubMesh) == 20, "FMeshPayloadSubMesh is stored in files and must keep its layout");
static_assert(sizeof(FMeshPayloadSlot) == 24, "FMeshPayloadSlot is stored in files and must keep its layout");

static constexpr uint32 MeshPayloadMagic   = MakeAssetChunkId("DXMS");
static constexpr uint32 MeshPayloadVersion = 1;
static constexpr uint64 SectionAlignment   = 16;

// The vertex streams come first, one section each, then these
static constexpr int32 IndexSection        = VERTEX_MAX_STREAMS;
static constexpr int32 SubMeshSection      = VERTEX_MAX_STREAMS + 1;
static constexpr int32 MaterialSlotSection = VERTEX_MAX_STREAMS + 2;
static constexpr int32 NumSections         = VERTEX_MAX_STREAMS + 3;

static uint64 AlignSectionOffset(uint64 Offset)
{
    return (Offset + SectionAlignment - 1) & ~(SectionAlignment - 1);
}

static void AppendBytes(TArray<uint8>& Bytes, const void* Source, int32 Size)
{
    const int32 Offset = Bytes.Size();
    Bytes.AppendUninitialized(Size);
    Memory::Memcpy(Bytes.Data() + Offset, Source, Size);
}

static bool ValidateMesh(const FMeshData& Mesh, int32 NumVertices, int32 NumMaterialSlots, FString& OutError)
{
    for (const FSubMesh& SubMesh : Mesh.SubMeshes)
    {
        if (uint64(SubMesh.StartIndex) + SubMesh.IndexCount > uint64(Mesh.Indices.Size()) || uint64(SubMesh.BaseVertex) + SubMesh.VertexCount > uint64(NumVertices))
        {
            OutError = FString::Printf(TEXT("a submesh covers indices %u-%u and vertices %u-%u of %d and %d"),
                SubMesh.StartIndex, SubMesh.StartIndex + SubMesh.IndexCount, SubMesh.BaseVertex, SubMesh.BaseVertex + SubMesh.VertexCount, Mesh.Indices.Size(), NumVertices);
            return false;
        }

        if (SubMesh.MaterialIndex < -1 || SubMesh.MaterialIndex >= NumMaterialSlots)
        {
            OutError = FString::Printf(TEXT("a submesh uses material %d of %d"), SubMesh.MaterialIndex, NumMaterialSlots);
            return false;
        }

        for (uint32 Index = SubMesh.StartIndex; Index < SubMesh.StartIndex + SubMesh.IndexCount; ++Index)
        {
            if (uint64(Mesh.Indices[Index]) + SubMesh.BaseVertex >= uint64(NumVertices))
            {
                OutError = FString::Printf(TEXT("index %u names vertex %llu of %d"), Index, uint64(Mesh.Indices[Index]) + SubMesh.BaseVertex, NumVertices);
                return false;
            }
        }
    }

    if (Mesh.SubMeshes.IsEmpty())
    {
        for (int32 Index = 0; Index < Mesh.Indices.Size(); ++Index)
        {
            if (Mesh.Indices[Index] >= uint32(NumVertices))
            {
                OutError = FString::Printf(TEXT("index %d names vertex %u of %d"), Index, Mesh.Indices[Index], NumVertices);
                return false;
            }
        }
    }

    return true;
}

bool MeshAssets::Write(const FMeshAssetData& Mesh, TArray<uint8>& OutPayload, FString& OutError)
{
    const FMeshData&     Data = Mesh.Mesh;
    TArray<uint8>        PackedStreams[VERTEX_MAX_STREAMS];
    const TArray<uint8>* Streams     = Data.PackedStreams;
    int32                NumVertices = Data.PackedVertexCount;

    if (NumVertices <= 0)
    {
        if (!Data.PackVertexStreams(PackedStreams))
        {
            OutError = FString::Printf(TEXT("The vertices of '%s' couldn't be packed for its vertex declaration"), *Data.Name);
            return false;
        }

        Streams     = PackedStreams;
        NumVertices = Data.Vertices.Size();
    }

    if (NumVertices <= 0)
    {
        OutError = FString::Printf(TEXT("'%s' has no vertices"), *Data.Name);
        return false;
    }

    for (uint8 StreamIndex = 0; StreamIndex < VERTEX_MAX_STREAMS; ++StreamIndex)
    {
        const int64 ExpectedSize = int64(NumVertices) * Data.Declaration.GetStreamStride(StreamIndex);
        if (Streams[StreamIndex].Size() != ExpectedSize)
        {
            OutError = FString::Printf(TEXT("Stream %u of '%s' has %d bytes, not the %lld its %d vertices take"), StreamIndex, *Data.Name, Streams[StreamIndex].Size(), ExpectedSize, NumVertices);
            return false;
        }
    }

    if (!ValidateMesh(Data, NumVertices, Mesh.MaterialSlots.Size(), OutError))
    {
        OutError = FString::Printf(TEXT("'%s' can't be written: %s"), *Data.Name, *OutError);
        return false;
    }

    TArray<FMeshPayloadSubMesh> SubMeshes;
    for (const FSubMesh& SubMesh : Data.SubMeshes)
    {
        SubMeshes.Add({ SubMesh.BaseVertex, SubMesh.VertexCount, SubMesh.StartIndex, SubMesh.IndexCount, SubMesh.MaterialIndex });
    }

    TArray<uint8> SlotBytes;
    for (const FMeshMaterialSlot& Slot : Mesh.MaterialSlots)
    {
        const TStringConversion<ANSICHAR, CHAR_T> Name = StringCast<ANSICHAR>(Slot.Name);
        const TStringConversion<ANSICHAR, CHAR_T> Path = StringCast<ANSICHAR>(Slot.Material.GetPath());

        FMeshPayloadSlot Entry;
        Entry.Guid     = Slot.Material.GetGuid();
        Entry.NameSize = static_cast<uint32>(Name.Length());
        Entry.PathSize = static_cast<uint32>(Path.Length());

        AppendBytes(SlotBytes, &Entry, sizeof(Entry));
        AppendBytes(SlotBytes, Name.Get(), Name.Length());
        AppendBytes(SlotBytes, Path.Get(), Path.Length());
    }

    const void* SectionData[NumSections] = {};
    uint64      SectionSizes[NumSections] = {};

    for (int32 StreamIndex = 0; StreamIndex < VERTEX_MAX_STREAMS; ++StreamIndex)
    {
        SectionData[StreamIndex]  = Streams[StreamIndex].Data();
        SectionSizes[StreamIndex] = static_cast<uint64>(Streams[StreamIndex].Size());
    }

    SectionData[IndexSection]         = Data.Indices.Data();
    SectionSizes[IndexSection]        = static_cast<uint64>(Data.Indices.Size()) * sizeof(uint32);
    SectionData[SubMeshSection]       = SubMeshes.Data();
    SectionSizes[SubMeshSection]      = static_cast<uint64>(SubMeshes.Size()) * sizeof(FMeshPayloadSubMesh);
    SectionData[MaterialSlotSection]  = SlotBytes.Data();
    SectionSizes[MaterialSlotSection] = static_cast<uint64>(SlotBytes.Size());

    FMeshPayloadSection Sections[NumSections];
    uint64 Offset = AlignSectionOffset(sizeof(FMeshPayloadHeader) + sizeof(Sections));

    for (int32 Section = 0; Section < NumSections; ++Section)
    {
        Sections[Section].Offset = SectionSizes[Section] > 0 ? Offset : 0;
        Sections[Section].Size   = SectionSizes[Section];
        Offset = AlignSectionOffset(Offset + SectionSizes[Section]);
    }

    FMeshPayloadHeader Header;
    Header.Magic            = MeshPayloadMagic;
    Header.Version          = MeshPayloadVersion;
    Header.AttributeFlags   = static_cast<uint32>(Data.Declaration.GetAttributeFlags());
    Header.NumVertices      = static_cast<uint32>(NumVertices);
    Header.NumIndices       = static_cast<uint32>(Data.Indices.Size());
    Header.NumSubMeshes     = static_cast<uint32>(SubMeshes.Size());
    Header.NumMaterialSlots = static_cast<uint32>(Mesh.MaterialSlots.Size());
    Header.Reserved         = 0;

    OutPayload.Reset(static_cast<int32>(Offset), 0);
    Memory::Memcpy(OutPayload.Data(), &Header, sizeof(Header));
    Memory::Memcpy(OutPayload.Data() + sizeof(Header), Sections, sizeof(Sections));

    for (int32 Section = 0; Section < NumSections; ++Section)
    {
        if (SectionSizes[Section] > 0)
        {
            Memory::Memcpy(OutPayload.Data() + Sections[Section].Offset, SectionData[Section], SectionSizes[Section]);
        }
    }

    return true;
}

bool MeshAssets::Read(const TArray<uint8>& Payload, FMeshAssetData& OutMesh, FString& OutError)
{
    const uint64 PayloadSize = static_cast<uint64>(Payload.Size());

    FMeshPayloadHeader  Header;
    FMeshPayloadSection Sections[NumSections];

    if (PayloadSize < sizeof(Header) + sizeof(Sections))
    {
        OutError = FString(TEXT("The mesh data is shorter than its header"));
        return false;
    }

    Memory::Memcpy(&Header, Payload.Data(), sizeof(Header));
    Memory::Memcpy(Sections, Payload.Data() + sizeof(Header), sizeof(Sections));

    if (Header.Magic != MeshPayloadMagic)
    {
        OutError = FString(TEXT("The mesh data doesn't start with 'DXMS'"));
        return false;
    }

    if (Header.Version != MeshPayloadVersion)
    {
        OutError = FString::Printf(TEXT("The mesh data is version %u, and only version %u can be read"), Header.Version, MeshPayloadVersion);
        return false;
    }

    if (Header.AttributeFlags == 0 || Header.AttributeFlags > static_cast<uint32>(VERTEX_ATTRIBUTES_ALL))
    {
        OutError = FString::Printf(TEXT("The mesh data has unknown vertex attributes (0x%x)"), Header.AttributeFlags);
        return false;
    }

    if (Header.NumVertices == 0 || Header.NumVertices > uint32(TNumericLimits<int32>::Max()) || Header.NumIndices > uint32(TNumericLimits<int32>::Max()))
    {
        OutError = FString::Printf(TEXT("The mesh data has %u vertices and %u indices"), Header.NumVertices, Header.NumIndices);
        return false;
    }

    for (int32 Section = 0; Section < NumSections; ++Section)
    {
        if (Sections[Section].Offset > PayloadSize || Sections[Section].Size > PayloadSize - Sections[Section].Offset)
        {
            OutError = FString::Printf(TEXT("Section %d of the mesh data lies past its end"), Section);
            return false;
        }
    }

    FMeshAssetData Mesh;
    FMeshData&     Data = Mesh.Mesh;
    Data.Declaration       = FVertexDeclaration::GetOrCreate(static_cast<EVertexAttributeFlags>(Header.AttributeFlags));
    Data.PackedVertexCount = static_cast<int32>(Header.NumVertices);

    for (uint8 StreamIndex = 0; StreamIndex < VERTEX_MAX_STREAMS; ++StreamIndex)
    {
        const uint64 ExpectedSize = uint64(Header.NumVertices) * Data.Declaration.GetStreamStride(StreamIndex);
        if (Sections[StreamIndex].Size != ExpectedSize)
        {
            OutError = FString::Printf(TEXT("Stream %u of the mesh data has %llu bytes, not the %llu its %u vertices take"), StreamIndex, Sections[StreamIndex].Size, ExpectedSize, Header.NumVertices);
            return false;
        }

        Data.PackedStreams[StreamIndex].Reset(Payload.Data() + Sections[StreamIndex].Offset, static_cast<int32>(ExpectedSize));
    }

    if (Sections[IndexSection].Size != uint64(Header.NumIndices) * sizeof(uint32) || Sections[SubMeshSection].Size != uint64(Header.NumSubMeshes) * sizeof(FMeshPayloadSubMesh))
    {
        OutError = FString(TEXT("The mesh data's indices or submeshes don't fill their sections"));
        return false;
    }

    Data.Indices.Reset(reinterpret_cast<const uint32*>(Payload.Data() + Sections[IndexSection].Offset), static_cast<int32>(Header.NumIndices));

    for (uint32 Index = 0; Index < Header.NumSubMeshes; ++Index)
    {
        FMeshPayloadSubMesh Entry;
        Memory::Memcpy(&Entry, Payload.Data() + Sections[SubMeshSection].Offset + Index * sizeof(FMeshPayloadSubMesh), sizeof(Entry));

        FSubMesh& SubMesh = Data.SubMeshes.Emplace();
        SubMesh.BaseVertex    = Entry.BaseVertex;
        SubMesh.VertexCount   = Entry.VertexCount;
        SubMesh.StartIndex    = Entry.StartIndex;
        SubMesh.IndexCount    = Entry.IndexCount;
        SubMesh.MaterialIndex = Entry.MaterialIndex;
    }

    const uint8* SlotData = Payload.Data() + Sections[MaterialSlotSection].Offset;
    uint64       SlotSize = Sections[MaterialSlotSection].Size;

    for (uint32 Index = 0; Index < Header.NumMaterialSlots; ++Index)
    {
        FMeshPayloadSlot Entry;
        if (SlotSize < sizeof(Entry))
        {
            OutError = FString::Printf(TEXT("Material slot %u lies past the end of the mesh data"), Index);
            return false;
        }

        Memory::Memcpy(&Entry, SlotData, sizeof(Entry));
        SlotData += sizeof(Entry);
        SlotSize -= sizeof(Entry);

        if (uint64(Entry.NameSize) + Entry.PathSize > SlotSize)
        {
            OutError = FString::Printf(TEXT("Material slot %u lies past the end of the mesh data"), Index);
            return false;
        }

        const FString Name = StringCast<CHAR_T>(reinterpret_cast<const ANSICHAR*>(SlotData), static_cast<int32>(Entry.NameSize)).ToString();
        const FString Path = StringCast<CHAR_T>(reinterpret_cast<const ANSICHAR*>(SlotData + Entry.NameSize), static_cast<int32>(Entry.PathSize)).ToString();
        SlotData += Entry.NameSize + Entry.PathSize;
        SlotSize -= Entry.NameSize + Entry.PathSize;

        FMeshMaterialSlot& Slot = Mesh.MaterialSlots.Emplace();
        Slot.Name     = Name;
        Slot.Material = Entry.Guid.IsValid() ? TAssetRef<FMaterial>(Entry.Guid, Path) : TAssetRef<FMaterial>();
    }

    if (!ValidateMesh(Data, Data.PackedVertexCount, Mesh.MaterialSlots.Size(), OutError))
    {
        OutError = FString::Printf(TEXT("The mesh data isn't valid: %s"), *OutError);
        return false;
    }

    OutMesh = ::Move(Mesh);
    return true;
}

TArray<FGuid> MeshAssets::GetDependencies(const FMeshAssetData& Mesh)
{
    TArray<FGuid> Dependencies;
    for (const FMeshMaterialSlot& Slot : Mesh.MaterialSlots)
    {
        if (Slot.Material.IsSet())
        {
            Dependencies.AddUnique(Slot.Material.GetGuid());
        }
    }

    return Dependencies;
}

TSharedPtr<FMesh> MeshAssets::Load(const FAssetData& Asset, const FString& DiskPath, FString& OutError)
{
    FAssetFileReader Reader;
    TArray<uint8>    Payload;
    FMeshAssetData   Data;

    if (!Reader.Open(DiskPath, OutError) || !Reader.ReadChunk(AssetChunk::Data, Payload, OutError))
    {
        return nullptr;
    }

    if (!Read(Payload, Data, OutError))
    {
        OutError = FString::Printf(TEXT("The mesh data of '%s' can't be read: %s"), *Asset.Path, *OutError);
        return nullptr;
    }

    Payload.Clear(true);
    Data.Mesh.Name = Asset.Name;

    TSharedPtr<FMesh> Mesh = FMesh::Create(Data.Mesh);
    if (!Mesh)
    {
        OutError = FString::Printf(TEXT("The GPU buffers for '%s' couldn't be created"), *Asset.Path);
        return nullptr;
    }

    Mesh->SetMaterialSlots(Data.MaterialSlots);
    return Mesh;
}
