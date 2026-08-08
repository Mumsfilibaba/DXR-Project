#include "Core/Containers/Stream.h"
#include "Core/Filesystem/File.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/CRC.h"
#include "Core/Memory/Memory.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/Paths.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Templates/CString.h"
#include "RendererCore/Shaders/ShaderManifest.h"

static TAutoConsoleVariable<String> CVarManifestFileName(
    "Renderer.ShaderCache.ManifestFileName",
    "FileName for the file recording which shader permutations to pre-warm",
    "Shaders.shadermanifest");
    
static const CHAR*      GShaderManifestMagic   = "DXRSHMAN";
static constexpr uint32 GShaderManifestVersion = 1;
static constexpr uint32 GMaxShaderManifestSize = 16 * 1024 * 1024; // A manifest is a few kilobytes of names and integers, so anything near this is a corrupt header rather than a big project.

struct FShaderManifestHeader
{
    CHAR   Magic[8]; // Always "DXRSHMAN"
    uint32 Version;
    uint32 DataCRC;  // CRC of everything after this header
    uint32 DataSize; // Size of everything after this header
    uint32 NumEntries;
};

static bool CanRead(const FByteInputStream& Stream, uint64 NumBytes)
{
    return static_cast<uint64>(Stream.Size() - Stream.ReadOffset()) >= NumBytes;
}

String FShaderManifest::GetFilePath()
{
    return Paths::GetAssetDir() + '/' + CVarManifestFileName.GetValue();
}

bool FShaderManifest::Load()
{
    Entries.Clear();

    const String FilePath = GetFilePath();
    if (!FPlatformFile::IsFile(*FilePath))
    {
        LOG_INFO("[FShaderManifest]: No manifest found at '%s' (expected on first run)", *FilePath);
        return false;
    }

    TFileRef<IPlatformFile> ManifestFile = FPlatformFile::OpenForRead(FilePath);
    if (!ManifestFile)
    {
        LOG_WARNING("[FShaderManifest]: Failed to open '%s'", *FilePath);
        return false;
    }

    FByteInputStream Stream;
    if (!File::ReadFile(ManifestFile.Get(), Stream))
    {
        LOG_WARNING("[FShaderManifest]: Failed to read '%s'", *FilePath);
        return false;
    }

    constexpr int32 HeaderSize = static_cast<int32>(sizeof(FShaderManifestHeader));
    if (Stream.Size() < HeaderSize)
    {
        LOG_WARNING("[FShaderManifest]: '%s' is too small to hold a header", *FilePath);
        return false;
    }

    FShaderManifestHeader Header;
    Stream.Read(Header);

    if (Memory::Memcmp(Header.Magic, GShaderManifestMagic, sizeof(Header.Magic)) != 0)
    {
        LOG_WARNING("[FShaderManifest]: '%s' has an invalid magic", *FilePath);
        return false;
    }

    if (Header.Version != GShaderManifestVersion)
    {
        LOG_INFO("[FShaderManifest]: '%s' is version %u, expected %u, so nothing is warmed this run", *FilePath, Header.Version, GShaderManifestVersion);
        return false;
    }

    if (Header.DataSize > GMaxShaderManifestSize)
    {
        LOG_WARNING("[FShaderManifest]: '%s' reports %u bytes, which exceeds the limit of %u", *FilePath, Header.DataSize, GMaxShaderManifestSize);
        return false;
    }

    if (Stream.Size() != HeaderSize + static_cast<int32>(Header.DataSize))
    {
        LOG_WARNING("[FShaderManifest]: '%s' is %d bytes, but the header reports %u bytes of data", *FilePath, Stream.Size(), Header.DataSize);
        return false;
    }

    if (CRC32::Generate(Stream.Data() + HeaderSize, Header.DataSize) != Header.DataCRC)
    {
        LOG_WARNING("[FShaderManifest]: '%s' failed its checksum", *FilePath);
        return false;
    }

    int32 NumDropped = 0;
    for (uint32 Index = 0; Index < Header.NumEntries; ++Index)
    {
        uint32 NameLength = 0;
        if (!CanRead(Stream, sizeof(NameLength)))
        {
            break;
        }

        Stream.Read(NameLength);
        if (NameLength == 0 || !CanRead(Stream, NameLength))
        {
            break;
        }

        TArray<CHAR> NameBuffer;
        NameBuffer.Resize(static_cast<int32>(NameLength));
        Stream.Read(NameBuffer.Data(), NameBuffer.Size());

        const String TypeName(NameBuffer.Data(), NameBuffer.Size());

        uint64 SpaceSignature  = 0;
        uint32 NumPermutations = 0;
        if (!CanRead(Stream, sizeof(SpaceSignature) + sizeof(NumPermutations)))
        {
            break;
        }

        Stream.Read(SpaceSignature);
        Stream.Read(NumPermutations);

        if (NumPermutations == 0 || !CanRead(Stream, static_cast<uint64>(NumPermutations) * sizeof(int32)))
        {
            break;
        }

        TArray<int32> PermutationIDs;
        PermutationIDs.Resize(static_cast<int32>(NumPermutations));
        Stream.Read(PermutationIDs.Data(), PermutationIDs.Size());

        // A renamed type or a changed permutation space renumbers the IDs, so the entry is dropped and this run records the new shape.
        FShaderType* Type = FShaderType::FindByName(*TypeName);
        if (!Type || Type->GetPermutationSpaceSignature() != SpaceSignature)
        {
            NumDropped++;
            continue;
        }

        FShaderManifestEntry& NewEntry = Entries.Emplace();
        NewEntry.Type           = Type;
        NewEntry.PermutationIDs = ::Move(PermutationIDs);
    }

    LOG_INFO("[FShaderManifest]: Loaded %d shader types from '%s' (%d dropped)", Entries.Size(), *FilePath, NumDropped);
    return true;
}

bool FShaderManifest::Save() const
{
    FByteOutputStream Payload;

    uint32 NumEntries = 0;
    for (const FShaderManifestEntry& Entry : Entries)
    {
        if (!Entry.Type || Entry.PermutationIDs.IsEmpty())
        {
            continue;
        }

        const CHAR*  TypeName        = Entry.Type->GetName();
        const uint32 NameLength      = static_cast<uint32>(CString::Strlen(TypeName));
        const uint64 SpaceSignature  = Entry.Type->GetPermutationSpaceSignature();
        const uint32 NumPermutations = static_cast<uint32>(Entry.PermutationIDs.Size());

        Payload.Add(NameLength);
        Payload.Add(TypeName, static_cast<int32>(NameLength));
        Payload.Add(SpaceSignature);
        Payload.Add(NumPermutations);
        Payload.Add(Entry.PermutationIDs.Data(), Entry.PermutationIDs.Size());

        NumEntries++;
    }

    if (NumEntries == 0)
    {
        return false;
    }

    FShaderManifestHeader Header;
    Memory::Memcpy(Header.Magic, GShaderManifestMagic, sizeof(Header.Magic));

    Header.Version    = GShaderManifestVersion;
    Header.DataCRC    = CRC32::Generate(Payload.Data(), static_cast<uint64>(Payload.Size()));
    Header.DataSize   = static_cast<uint32>(Payload.Size());
    Header.NumEntries = NumEntries;

    const String FilePath = GetFilePath();

    TFileRef<IPlatformFile> ManifestFile = FPlatformFile::OpenForWrite(FilePath);
    if (!ManifestFile)
    {
        LOG_WARNING("[FShaderManifest]: Failed to open '%s' for writing", *FilePath);
        return false;
    }

    constexpr int32 HeaderSize = static_cast<int32>(sizeof(FShaderManifestHeader));
    if (ManifestFile->Write(reinterpret_cast<const uint8*>(&Header), HeaderSize) != HeaderSize)
    {
        LOG_WARNING("[FShaderManifest]: Failed to write the header to '%s'", *FilePath);
        return false;
    }

    if (ManifestFile->Write(Payload.Data(), Payload.Size()) != Payload.Size())
    {
        LOG_WARNING("[FShaderManifest]: Failed to write %d bytes to '%s'", Payload.Size(), *FilePath);
        return false;
    }

    LOG_INFO("[FShaderManifest]: Saved %u shader types to '%s'", NumEntries, *FilePath);
    return true;
}
