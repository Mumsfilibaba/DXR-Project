#include "Core/Containers/Stream.h"
#include "Core/Filesystem/File.h"
#include "Core/Memory/Memory.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/CRC.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "Core/Misc/Paths.h"
#include "Core/Platform/PlatformFile.h"
#include "RendererCore/Shaders/ShaderBytecodeCache.h"

static TAutoConsoleVariable<bool> CVarEnableBytecodeCache(
    "Renderer.ShaderCache.EnableBytecodeCache",
    "Reuse shader bytecode compiled by earlier runs instead of invoking the shader compiler",
    true);

static TAutoConsoleVariable<String> CVarBytecodeFileName(
    "Renderer.ShaderCache.BytecodeFileName",
    "FileName for the file storing compiled shader bytecode",
    "Shaders.shaderbytecode");

static FAutoConsoleCommand CCmdDumpShaderBytecodeCacheStats(
    "Renderer.DumpShaderBytecodeCacheStats",
    "Logs how many compiled shaders the bytecode cache is holding",
    FConsoleCommandDelegate::CreateLambda([](StringView)
    {
        if (FShaderBytecodeCache* Cache = FShaderBytecodeCache::TryGet())
        {
            LOG_INFO("[FShaderBytecodeCache]: Holding %d compiled shaders", Cache->GetNumEntries());
        }
    }));

static const CHAR*      GShaderBytecodeMagic        = "DXRSHBIN";
static constexpr uint32 GShaderBytecodeVersion      = 1;
static constexpr uint32 GMaxShaderBytecodeCacheSize = 256 * 1024 * 1024; // MSL is stored as text and every permutation carries its own blob, so this is generous rather than tight.

struct FShaderBytecodeHeader
{
    CHAR   Magic[8]; // Always "DXRSHBIN"
    uint32 Version;
    uint32 DataCRC;  // CRC of everything after this header
    uint32 DataSize; // Size of everything after this header
    uint32 NumEntries;
};

static bool CanRead(const FByteInputStream& Stream, uint64 NumBytes)
{
    return static_cast<uint64>(Stream.Size() - Stream.ReadOffset()) >= NumBytes;
}

static bool ReadString(FByteInputStream& Stream, String& OutString)
{
    uint32 Length = 0;
    if (!CanRead(Stream, sizeof(Length)))
    {
        return false;
    }

    Stream.Read(Length);
    if (Length == 0 || !CanRead(Stream, Length))
    {
        return false;
    }

    TArray<CHAR> Characters;
    Characters.Resize(static_cast<int32>(Length));
    Stream.Read(Characters.Data(), Characters.Size());

    OutString = String(Characters.Data(), Characters.Size());
    return true;
}

static void WriteString(FByteOutputStream& Stream, const String& Value)
{
    const uint32 Length = static_cast<uint32>(Value.Size());

    Stream.Add(Length);
    Stream.Add(Value.Data(), static_cast<int32>(Length));
}

FShaderBytecodeCache* FShaderBytecodeCache::GBytecodeCache = nullptr;

FShaderBytecodeCache::FShaderBytecodeCache()
    : Entries()
    , FileHashes()
    , bDirty(false)
{
}

FShaderBytecodeCache::~FShaderBytecodeCache()
{
    Entries.Clear();
    FileHashes.Clear();
}

bool FShaderBytecodeCache::Initialize()
{
    GBytecodeCache = new FShaderBytecodeCache();

    if (CVarEnableBytecodeCache.GetValue())
    {
        GBytecodeCache->Load();
    }

    return true;
}

void FShaderBytecodeCache::Release()
{
    if (GBytecodeCache)
    {
        GBytecodeCache->Save();

        delete GBytecodeCache;
        GBytecodeCache = nullptr;
    }
}

String FShaderBytecodeCache::GetFilePath()
{
    return Paths::GetAssetDir() + '/' + CVarBytecodeFileName.GetValue();
}

String FShaderBytecodeCache::MakeAssetRelativePath(const String& Path)
{
    const String AssetDir = Paths::GetAssetDir() + '/';
    if (Path.StartsWith(AssetDir))
    {
        return String(Path.Data() + AssetDir.Size(), Path.Size() - AssetDir.Size());
    }

    return Path;
}

bool FShaderBytecodeCache::TryGetFileHash(const String& RelativePath, uint64& OutHash)
{
    TScopedLock Lock(FileHashesCS);

    // Zero is the memoized "could not be read", so a file that has gone missing is not read again on every entry.
    if (const uint64* Existing = FileHashes.Find(RelativePath))
    {
        OutHash = *Existing;
        return OutHash != 0;
    }

    uint64 FileHash = 0;

    // Anything the pre-processor reached outside the asset directory was stored as it came, and is opened the same way.
    const String FilePath = FPlatformFile::IsPathRelative(*RelativePath) ? (Paths::GetAssetDir() + '/' + RelativePath) : RelativePath;

    TFileRef<IPlatformFile> SourceFile = FPlatformFile::OpenForRead(FilePath);
    if (SourceFile)
    {
        TArray<uint8> Contents;
        if (File::ReadFile(SourceFile.Get(), Contents) && !Contents.IsEmpty())
        {
            FileHash = CRC32::Generate(Contents.Data(), static_cast<uint64>(Contents.Size()));
        }
    }

    if (FileHash == 0)
    {
        LOG_WARNING("[FShaderBytecodeCache]: Cannot read '%s', so nothing depending on it is cached", *FilePath);
    }

    FileHashes.Add(RelativePath, FileHash);

    OutHash = FileHash;
    return FileHash != 0;
}

bool FShaderBytecodeCache::TryComputeDependencyHash(const TArray<String>& Dependencies, uint64& OutHash)
{
    uint64 DependencyHash = 0;
    for (const String& Dependency : Dependencies)
    {
        uint64 FileHash = 0;
        if (!TryGetFileHash(Dependency, FileHash))
        {
            return false;
        }

        HashCombine(DependencyHash, FileHash);
    }

    OutHash = DependencyHash;
    return true;
}

bool FShaderBytecodeCache::Find(uint64 CompileHash, TArray<uint8>& OutByteCode)
{
    if (!CVarEnableBytecodeCache.GetValue())
    {
        return false;
    }

    TScopedLock Lock(EntriesCS);

    if (const FShaderBytecodeEntry* Existing = Entries.Find(CompileHash))
    {
        OutByteCode = Existing->ByteCode;
        return true;
    }

    return false;
}

void FShaderBytecodeCache::Add(uint64 CompileHash, const TArray<uint8>& ByteCode, const TArray<String>& Dependencies)
{
    if (ByteCode.IsEmpty() || Dependencies.IsEmpty())
    {
        return;
    }

    FShaderBytecodeEntry NewEntry;
    NewEntry.ByteCode = ByteCode;

    NewEntry.Dependencies.Reserve(Dependencies.Size());
    for (const String& Dependency : Dependencies)
    {
        NewEntry.Dependencies.Emplace(MakeAssetRelativePath(Dependency));
    }

    // Hashed before the lock is taken, since reading the dependencies off disk has nothing to do with the entry map.
    if (!TryComputeDependencyHash(NewEntry.Dependencies, NewEntry.DependencyHash))
    {
        return;
    }

    // Several worker lanes reach this at once, and the shader cache's own lock covers only its map of shaders.
    TScopedLock Lock(EntriesCS);

    if (Entries.Contains(CompileHash))
    {
        return;
    }

    Entries.Add(CompileHash, ::Move(NewEntry));
    bDirty = true;
}

int32 FShaderBytecodeCache::GetNumEntries()
{
    TScopedLock Lock(EntriesCS);
    return Entries.Size();
}

bool FShaderBytecodeCache::Load()
{
    const String FilePath = GetFilePath();
    if (!FPlatformFile::IsFile(*FilePath))
    {
        LOG_INFO("[FShaderBytecodeCache]: No cache found at '%s' (expected on first run)", *FilePath);
        return false;
    }

    TFileRef<IPlatformFile> CacheFile = FPlatformFile::OpenForRead(FilePath);
    if (!CacheFile)
    {
        LOG_WARNING("[FShaderBytecodeCache]: Failed to open '%s'", *FilePath);
        return false;
    }

    FByteInputStream Stream;
    if (!File::ReadFile(CacheFile.Get(), Stream))
    {
        LOG_WARNING("[FShaderBytecodeCache]: Failed to read '%s'", *FilePath);
        return false;
    }

    constexpr int32 HeaderSize = static_cast<int32>(sizeof(FShaderBytecodeHeader));
    if (Stream.Size() < HeaderSize)
    {
        LOG_WARNING("[FShaderBytecodeCache]: '%s' is too small to hold a header", *FilePath);
        return false;
    }

    FShaderBytecodeHeader Header;
    Stream.Read(Header);

    if (Memory::Memcmp(Header.Magic, GShaderBytecodeMagic, sizeof(Header.Magic)) != 0)
    {
        LOG_WARNING("[FShaderBytecodeCache]: '%s' has an invalid magic", *FilePath);
        return false;
    }

    if (Header.Version != GShaderBytecodeVersion)
    {
        LOG_INFO("[FShaderBytecodeCache]: '%s' is version %u, expected %u, so everything recompiles", *FilePath, Header.Version, GShaderBytecodeVersion);
        return false;
    }

    if (Header.DataSize > GMaxShaderBytecodeCacheSize)
    {
        LOG_WARNING("[FShaderBytecodeCache]: '%s' reports %u bytes, which exceeds the limit of %u", *FilePath, Header.DataSize, GMaxShaderBytecodeCacheSize);
        return false;
    }

    if (Stream.Size() != HeaderSize + static_cast<int32>(Header.DataSize))
    {
        LOG_WARNING("[FShaderBytecodeCache]: '%s' is %d bytes, but the header reports %u bytes of data", *FilePath, Stream.Size(), Header.DataSize);
        return false;
    }

    if (CRC32::Generate(Stream.Data() + HeaderSize, Header.DataSize) != Header.DataCRC)
    {
        LOG_WARNING("[FShaderBytecodeCache]: '%s' failed its checksum", *FilePath);
        return false;
    }

    int32 NumStale = 0;
    for (uint32 Index = 0; Index < Header.NumEntries; ++Index)
    {
        uint64 CompileHash     = 0;
        uint64 DependencyHash  = 0;
        uint32 NumDependencies = 0;
        
        if (!CanRead(Stream, sizeof(CompileHash) + sizeof(DependencyHash) + sizeof(NumDependencies)))
        {
            break;
        }

        Stream.Read(CompileHash);
        Stream.Read(DependencyHash);
        Stream.Read(NumDependencies);

        if (NumDependencies == 0)
        {
            break;
        }

        FShaderBytecodeEntry NewEntry;
        NewEntry.DependencyHash = DependencyHash;
        NewEntry.Dependencies.Reserve(static_cast<int32>(NumDependencies));

        bool bReadEntry = true;
        for (uint32 DependencyIndex = 0; DependencyIndex < NumDependencies; ++DependencyIndex)
        {
            String Dependency;
            if (!ReadString(Stream, Dependency))
            {
                bReadEntry = false;
                break;
            }

            NewEntry.Dependencies.Emplace(::Move(Dependency));
        }

        uint32 ByteCodeSize = 0;
        if (!bReadEntry || !CanRead(Stream, sizeof(ByteCodeSize)))
        {
            break;
        }

        Stream.Read(ByteCodeSize);
        if (ByteCodeSize == 0 || !CanRead(Stream, ByteCodeSize))
        {
            break;
        }

        NewEntry.ByteCode.Resize(static_cast<int32>(ByteCodeSize));
        Stream.Read(NewEntry.ByteCode.Data(), NewEntry.ByteCode.Size());

        // An entry whose sources moved on is dropped on its own, so editing one include costs only the shaders that read it.
        uint64 CurrentDependencyHash = 0;
        if (!TryComputeDependencyHash(NewEntry.Dependencies, CurrentDependencyHash) || CurrentDependencyHash != NewEntry.DependencyHash)
        {
            NumStale++;
            continue;
        }

        Entries.Add(CompileHash, ::Move(NewEntry));
    }

    // The stale entries are gone from memory, so the file is rewritten without them even if this run compiles nothing.
    bDirty = (NumStale > 0);

    LOG_INFO("[FShaderBytecodeCache]: Loaded %d compiled shaders from '%s' (%d stale)", Entries.Size(), *FilePath, NumStale);
    return true;
}

bool FShaderBytecodeCache::Save()
{
    TScopedLock Lock(EntriesCS);

    if (!bDirty || !CVarEnableBytecodeCache.GetValue())
    {
        return false;
    }

    FByteOutputStream Payload;

    uint32 NumEntries = 0;
    Entries.Foreach([&Payload, &NumEntries](const uint64& CompileHash, const FShaderBytecodeEntry& Entry)
    {
        if (Entry.ByteCode.IsEmpty() || Entry.Dependencies.IsEmpty())
        {
            return;
        }

        Payload.Add(CompileHash);
        Payload.Add(Entry.DependencyHash);
        Payload.Add(static_cast<uint32>(Entry.Dependencies.Size()));

        for (const String& Dependency : Entry.Dependencies)
        {
            WriteString(Payload, Dependency);
        }

        Payload.Add(static_cast<uint32>(Entry.ByteCode.Size()));
        Payload.Add(Entry.ByteCode.Data(), Entry.ByteCode.Size());

        NumEntries++;
    });

    if (NumEntries == 0)
    {
        return false;
    }

    FShaderBytecodeHeader Header;
    Memory::Memcpy(Header.Magic, GShaderBytecodeMagic, sizeof(Header.Magic));

    Header.Version    = GShaderBytecodeVersion;
    Header.DataCRC    = CRC32::Generate(Payload.Data(), static_cast<uint64>(Payload.Size()));
    Header.DataSize   = static_cast<uint32>(Payload.Size());
    Header.NumEntries = NumEntries;

    const String FilePath = GetFilePath();

    TFileRef<IPlatformFile> CacheFile = FPlatformFile::OpenForWrite(FilePath);
    if (!CacheFile)
    {
        LOG_WARNING("[FShaderBytecodeCache]: Failed to open '%s' for writing", *FilePath);
        return false;
    }

    constexpr int32 HeaderSize = static_cast<int32>(sizeof(FShaderBytecodeHeader));
    if (CacheFile->Write(reinterpret_cast<const uint8*>(&Header), HeaderSize) != HeaderSize)
    {
        LOG_WARNING("[FShaderBytecodeCache]: Failed to write the header to '%s'", *FilePath);
        return false;
    }

    if (CacheFile->Write(Payload.Data(), Payload.Size()) != Payload.Size())
    {
        LOG_WARNING("[FShaderBytecodeCache]: Failed to write %d bytes to '%s'", Payload.Size(), *FilePath);
        return false;
    }

    bDirty = false;

    LOG_INFO("[FShaderBytecodeCache]: Saved %u compiled shaders to '%s'", NumEntries, *FilePath);
    return true;
}
