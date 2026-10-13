#include "AssetCore/AssetFile.h"
#include "Core/Containers/StringConversion.h"
#include "Core/Filesystem/File.h"
#include "Core/Json/Json.h"
#include "Core/Memory/Memory.h"
#include "Core/Misc/CRC.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Templates/CString.h"
#include "Core/Templates/NumericLimits.h"

// IPlatformFile reads and writes at most an int32 of bytes at a time
static constexpr uint64 MaxFileTransferSize = 1024ull * 1024ull * 1024ull;

static uint64 AlignChunkOffset(uint64 Offset)
{
    return (Offset + FAssetFile::ChunkAlignment - 1) & ~static_cast<uint64>(FAssetFile::ChunkAlignment - 1);
}

static bool ReadBytes(IPlatformFile* File, uint8* Destination, uint64 Size)
{
    while (Size > 0)
    {
        const uint32 BytesToRead = static_cast<uint32>(Size < MaxFileTransferSize ? Size : MaxFileTransferSize);
        if (File->Read(Destination, BytesToRead) != static_cast<int32>(BytesToRead))
        {
            return false;
        }

        Destination += BytesToRead;
        Size        -= BytesToRead;
    }

    return true;
}

static bool WriteBytes(IPlatformFile* File, const uint8* Source, uint64 Size)
{
    while (Size > 0)
    {
        const uint32 BytesToWrite = static_cast<uint32>(Size < MaxFileTransferSize ? Size : MaxFileTransferSize);
        if (File->Write(Source, BytesToWrite) != static_cast<int32>(BytesToWrite))
        {
            return false;
        }

        Source += BytesToWrite;
        Size   -= BytesToWrite;
    }

    return true;
}

static uint32 ComputeHeaderCRC(const FAssetFileHeader& Header, const FAssetChunkEntry* Chunks, uint32 NumChunks)
{
    TArray<uint8> Bytes;
    Bytes.Resize(static_cast<int32>(sizeof(FAssetFileHeader) + NumChunks * sizeof(FAssetChunkEntry)));

    FAssetFileHeader HeaderWithoutCRC = Header;
    HeaderWithoutCRC.HeaderCRC = 0;

    Memory::Memcpy(Bytes.Data(), &HeaderWithoutCRC, sizeof(FAssetFileHeader));
    if (NumChunks > 0)
    {
        Memory::Memcpy(Bytes.Data() + sizeof(FAssetFileHeader), Chunks, NumChunks * sizeof(FAssetChunkEntry));
    }

    return CRC32::Generate(Bytes.Data(), static_cast<uint64>(Bytes.Size()));
}

FString FAssetFile::ChunkIdToString(uint32 Id)
{
    const ANSICHAR Characters[5] =
    {
        static_cast<ANSICHAR>(Id & 0xFF),
        static_cast<ANSICHAR>((Id >> 8) & 0xFF),
        static_cast<ANSICHAR>((Id >> 16) & 0xFF),
        static_cast<ANSICHAR>((Id >> 24) & 0xFF),
        '\0'
    };

    for (int32 Index = 0; Index < 4; ++Index)
    {
        if (Characters[Index] < 0x20 || Characters[Index] > 0x7E)
        {
            return FString::Printf(TEXT("0x%08X"), Id);
        }
    }

    return StringCast<CHAR_T>(Characters).ToString();
}

FAssetFileWriter::FAssetFileWriter(const FGuid& InGuid, EAssetType InType)
    : Guid(InGuid)
    , Type(InType)
    , Chunks()
{
}

FAssetFileWriter::~FAssetFileWriter() = default;

FAssetFileWriter::FChunk& FAssetFileWriter::FindOrAddChunk(uint32 Id)
{
    for (FChunk& Chunk : Chunks)
    {
        if (Chunk.Id == Id)
        {
            return Chunk;
        }
    }

    FChunk& NewChunk = Chunks.Emplace();
    NewChunk.Id = Id;
    return NewChunk;
}

void FAssetFileWriter::SetChunk(uint32 Id, TArray<uint8>&& Data)
{
    FChunk& Chunk = FindOrAddChunk(Id);
    Chunk.OwnedData = ::Move(Data);
    Chunk.ViewData  = nullptr;
    Chunk.ViewSize  = 0;
    Chunk.bIsView   = false;
}

void FAssetFileWriter::SetChunkView(uint32 Id, const void* Data, uint64 Size)
{
    FChunk& Chunk = FindOrAddChunk(Id);
    Chunk.OwnedData.Clear();
    Chunk.ViewData = static_cast<const uint8*>(Data);
    Chunk.ViewSize = Size;
    Chunk.bIsView  = true;
}

void FAssetFileWriter::SetMetadata(const FJsonValue& Metadata)
{
    const FString                             Text = Json::ToString(Metadata, EJsonWriteFlags::Compact);
    const TStringConversion<ANSICHAR, CHAR_T> Utf8 = StringCast<ANSICHAR>(Text);
    SetChunk(AssetChunk::Metadata, TArray<uint8>(reinterpret_cast<const uint8*>(Utf8.Get()), Utf8.Length()));
}

bool FAssetFileWriter::SaveToFile(const FString& Filename, FString& OutError) const
{
    // META first, so a reader that only wants it reads from the start of the file
    TArray<const FChunk*> Ordered;
    for (const FChunk& Chunk : Chunks)
    {
        if (Chunk.Id == AssetChunk::Metadata)
        {
            Ordered.Add(&Chunk);
        }
    }

    for (const FChunk& Chunk : Chunks)
    {
        if (Chunk.Id != AssetChunk::Metadata)
        {
            Ordered.Add(&Chunk);
        }
    }

    TArray<const uint8*> ChunkData;
    TArray<uint64>       ChunkSizes;

    for (const FChunk* Chunk : Ordered)
    {
        ChunkData.Add(Chunk->bIsView ? Chunk->ViewData : Chunk->OwnedData.Data());
        ChunkSizes.Add(Chunk->bIsView ? Chunk->ViewSize : static_cast<uint64>(Chunk->OwnedData.Size()));
    }

    FAssetFileHeader Header{};
    Memory::Memcpy(Header.Magic, FAssetFile::Magic, sizeof(Header.Magic));
    Header.FileVersion = FAssetFile::CurrentVersion;
    Header.Type        = Type;
    Header.Guid        = Guid;
    Header.NumChunks   = static_cast<uint32>(Ordered.Size());

    TArray<FAssetChunkEntry> Entries;
    uint64 Offset = sizeof(FAssetFileHeader) + Ordered.Size() * sizeof(FAssetChunkEntry);
    for (int32 Index = 0; Index < Ordered.Size(); ++Index)
    {
        Offset = AlignChunkOffset(Offset);

        FAssetChunkEntry& Entry = Entries.Emplace();
        Entry.Id     = Ordered[Index]->Id;
        Entry.CRC    = CRC32::Generate(ChunkData[Index], ChunkSizes[Index]);
        Entry.Offset = Offset;
        Entry.Size   = ChunkSizes[Index];

        Offset += Entry.Size;
    }

    Header.HeaderCRC = ComputeHeaderCRC(Header, Entries.Data(), Header.NumChunks);

    const FString Directory = File::GetDirectoryOf(Filename);
    if (!Directory.IsEmpty() && !File::CreateDirectoryTree(Directory))
    {
        OutError = FString::Printf(TEXT("Could not create the folder '%s'"), *Directory);
        return false;
    }

    const FString TempFilename = Filename + TEXT(".tmp");

    {
        TFileRef<IPlatformFile> TempFile = FPlatformFile::OpenForWrite(TempFilename);
        if (!TempFile.IsValid())
        {
            OutError = FString::Printf(TEXT("Could not open '%s' for writing"), *TempFilename);
            return false;
        }

        // Padding is zeroed, so the same chunks always give the same bytes
        static constexpr uint8 Padding[FAssetFile::ChunkAlignment] = {};

        bool   bWritten = WriteBytes(TempFile.Get(), reinterpret_cast<const uint8*>(&Header), sizeof(FAssetFileHeader));
        uint64 Position = sizeof(FAssetFileHeader);
        if (bWritten && !Entries.IsEmpty())
        {
            bWritten  = WriteBytes(TempFile.Get(), reinterpret_cast<const uint8*>(Entries.Data()), Entries.Size() * sizeof(FAssetChunkEntry));
            Position += Entries.Size() * sizeof(FAssetChunkEntry);
        }

        for (int32 Index = 0; bWritten && Index < Entries.Size(); ++Index)
        {
            bWritten = WriteBytes(TempFile.Get(), Padding, Entries[Index].Offset - Position) && WriteBytes(TempFile.Get(), ChunkData[Index], Entries[Index].Size);
            Position = Entries[Index].Offset + Entries[Index].Size;
        }

        if (!bWritten)
        {
            TempFile.Close();
            FPlatformFile::DeleteFile(*TempFilename);
            OutError = FString::Printf(TEXT("Could not write '%s'"), *TempFilename);
            return false;
        }
    }

    if (!FPlatformFile::MoveFile(*TempFilename, *Filename))
    {
        FPlatformFile::DeleteFile(*TempFilename);
        OutError = FString::Printf(TEXT("Could not replace '%s'"), *Filename);
        return false;
    }

    return true;
}

bool FAssetFileReader::Open(const FString& InFilename, FString& OutError)
{
    Close();

    TFileRef<IPlatformFile> NewFile = FPlatformFile::OpenForRead(InFilename);
    if (!NewFile.IsValid())
    {
        OutError = FString::Printf(TEXT("Could not open '%s'"), *InFilename);
        return false;
    }

    const uint64 FileSize = static_cast<uint64>(NewFile->Size());

    FAssetFileHeader NewHeader{};
    if (FileSize < sizeof(FAssetFileHeader) || !ReadBytes(NewFile.Get(), reinterpret_cast<uint8*>(&NewHeader), sizeof(FAssetFileHeader)))
    {
        OutError = FString::Printf(TEXT("'%s' is too small to be an asset file"), *InFilename);
        return false;
    }

    if (Memory::Memcmp(NewHeader.Magic, FAssetFile::Magic, sizeof(NewHeader.Magic)) != 0)
    {
        OutError = FString::Printf(TEXT("'%s' is not an asset file"), *InFilename);
        return false;
    }

    if (NewHeader.FileVersion > FAssetFile::CurrentVersion)
    {
        OutError = FString::Printf(TEXT("'%s' is asset file version %u, which a newer engine wrote; this one reads up to %u"),
            *InFilename, NewHeader.FileVersion, FAssetFile::CurrentVersion);
        return false;
    }

    if (NewHeader.NumChunks > FAssetFile::MaxChunks)
    {
        OutError = FString::Printf(TEXT("'%s' has %u chunks, more than the %u an asset file can have"), *InFilename, NewHeader.NumChunks, FAssetFile::MaxChunks);
        return false;
    }

    const uint64 TableEnd = sizeof(FAssetFileHeader) + NewHeader.NumChunks * sizeof(FAssetChunkEntry);

    TArray<FAssetChunkEntry> NewChunks;
    NewChunks.Resize(static_cast<int32>(NewHeader.NumChunks));

    if (FileSize < TableEnd || (NewHeader.NumChunks > 0 && !ReadBytes(NewFile.Get(), reinterpret_cast<uint8*>(NewChunks.Data()), NewHeader.NumChunks * sizeof(FAssetChunkEntry))))
    {
        OutError = FString::Printf(TEXT("'%s' is too small to hold its chunk table"), *InFilename);
        return false;
    }

    if (ComputeHeaderCRC(NewHeader, NewChunks.Data(), NewHeader.NumChunks) != NewHeader.HeaderCRC)
    {
        OutError = FString::Printf(TEXT("'%s' failed its header checksum"), *InFilename);
        return false;
    }

    for (int32 Index = 0; Index < NewChunks.Size(); ++Index)
    {
        const FAssetChunkEntry& Entry = NewChunks[Index];
        if (Entry.Offset < TableEnd || Entry.Offset > FileSize || Entry.Size > FileSize - Entry.Offset)
        {
            OutError = FString::Printf(TEXT("'%s' is truncated: chunk '%s' runs past its end"), *InFilename, *FAssetFile::ChunkIdToString(Entry.Id));
            return false;
        }

        for (int32 Previous = 0; Previous < Index; ++Previous)
        {
            if (NewChunks[Previous].Id == Entry.Id)
            {
                OutError = FString::Printf(TEXT("'%s' has the chunk '%s' twice"), *InFilename, *FAssetFile::ChunkIdToString(Entry.Id));
                return false;
            }
        }
    }

    Filename = InFilename;
    File     = ::Move(NewFile);
    Header   = NewHeader;
    Chunks   = ::Move(NewChunks);
    return true;
}

void FAssetFileReader::Close()
{
    File.Close();
    Filename.Clear();
    Header = FAssetFileHeader{};
    Chunks.Clear();
}

const FAssetChunkEntry* FAssetFileReader::FindChunk(uint32 Id) const
{
    for (const FAssetChunkEntry& Entry : Chunks)
    {
        if (Entry.Id == Id)
        {
            return &Entry;
        }
    }

    return nullptr;
}

const FAssetChunkEntry* FAssetFileReader::FindChunkForRead(uint32 Id, FString& OutError) const
{
    if (!IsOpen())
    {
        OutError = FString(TEXT("No asset file is open"));
        return nullptr;
    }

    const FAssetChunkEntry* Entry = FindChunk(Id);
    if (!Entry)
    {
        OutError = FString::Printf(TEXT("'%s' has no '%s' chunk"), *Filename, *FAssetFile::ChunkIdToString(Id));
    }

    return Entry;
}

bool FAssetFileReader::ReadBytesAt(uint64 Offset, void* Destination, uint64 Size)
{
    return File->SeekFromStart(static_cast<int64>(Offset)) && ReadBytes(File.Get(), static_cast<uint8*>(Destination), Size);
}

bool FAssetFileReader::ReadChunk(uint32 Id, TArray<uint8>& OutData, FString& OutError)
{
    const FAssetChunkEntry* Entry = FindChunkForRead(Id, OutError);
    if (!Entry)
    {
        return false;
    }

    if (Entry->Size > static_cast<uint64>(TNumericLimits<int32>::Max()))
    {
        OutError = FString::Printf(TEXT("Chunk '%s' of '%s' is %llu bytes, too large for an array; read it into a buffer instead"),
            *FAssetFile::ChunkIdToString(Id), *Filename, Entry->Size);
        return false;
    }

    TArray<uint8> Data;
    Data.Resize(static_cast<int32>(Entry->Size));
    if (!ReadChunk(Id, Data.Data(), Entry->Size, OutError))
    {
        return false;
    }

    OutData = ::Move(Data);
    return true;
}

bool FAssetFileReader::ReadChunk(uint32 Id, void* Destination, uint64 DestinationSize, FString& OutError)
{
    const FAssetChunkEntry* Entry = FindChunkForRead(Id, OutError);
    if (!Entry)
    {
        return false;
    }

    if (DestinationSize < Entry->Size)
    {
        OutError = FString::Printf(TEXT("Chunk '%s' of '%s' is %llu bytes, more than the %llu bytes it was to be read into"),
            *FAssetFile::ChunkIdToString(Id), *Filename, Entry->Size, DestinationSize);
        return false;
    }

    if (!ReadBytesAt(Entry->Offset, Destination, Entry->Size))
    {
        OutError = FString::Printf(TEXT("Could not read chunk '%s' of '%s'"), *FAssetFile::ChunkIdToString(Id), *Filename);
        return false;
    }

    if (CRC32::Generate(Destination, Entry->Size) != Entry->CRC)
    {
        OutError = FString::Printf(TEXT("Chunk '%s' of '%s' failed its checksum"), *FAssetFile::ChunkIdToString(Id), *Filename);
        return false;
    }

    return true;
}

bool FAssetFileReader::ReadChunkRange(uint32 Id, uint64 Offset, void* Destination, uint64 Size, FString& OutError)
{
    const FAssetChunkEntry* Entry = FindChunkForRead(Id, OutError);
    if (!Entry)
    {
        return false;
    }

    if (Offset > Entry->Size || Size > Entry->Size - Offset)
    {
        OutError = FString::Printf(TEXT("Bytes %llu to %llu are past the end of chunk '%s' of '%s', which is %llu bytes"),
            Offset, Offset + Size, *FAssetFile::ChunkIdToString(Id), *Filename, Entry->Size);
        return false;
    }

    if (!ReadBytesAt(Entry->Offset + Offset, Destination, Size))
    {
        OutError = FString::Printf(TEXT("Could not read chunk '%s' of '%s'"), *FAssetFile::ChunkIdToString(Id), *Filename);
        return false;
    }

    return true;
}

bool FAssetFileReader::ReadMetadata(FJsonValue& OutMetadata, FString& OutError)
{
    TArray<uint8> Bytes;
    if (!ReadChunk(AssetChunk::Metadata, Bytes, OutError))
    {
        return false;
    }

    const FString Text = StringCast<CHAR_T>(reinterpret_cast<const ANSICHAR*>(Bytes.Data()), Bytes.Size()).ToString();

    FJsonError JsonError;
    FJsonValue Metadata;

    if (!Json::Parse(FStringView(*Text, Text.Length()), Metadata, &JsonError))
    {
        OutError = FString::Printf(TEXT("The metadata of '%s' is not valid JSON: %s"), *Filename, *JsonError.ToString());
        return false;
    }

    OutMetadata = ::Move(Metadata);
    return true;
}
