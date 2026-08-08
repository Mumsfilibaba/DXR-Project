#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Map.h"
#include "Core/Containers/String.h"
#include "Core/Platform/CriticalSection.h"

struct FShaderBytecodeEntry
{
    TArray<uint8>  ByteCode;
    TArray<String> Dependencies;
    uint64         DependencyHash = 0;
};

class RENDERERCORE_API FShaderBytecodeCache
{
public:
    static bool Initialize();
    static void Release();

    static FORCEINLINE FShaderBytecodeCache& Get()
    {
        return *GBytecodeCache;
    }

    static FORCEINLINE FShaderBytecodeCache* TryGet()
    {
        return GBytecodeCache;
    }

public:
    NODISCARD bool Find(uint64 CompileHash, TArray<uint8>& OutByteCode);

    void Add(uint64 CompileHash, const TArray<uint8>& ByteCode, const TArray<String>& Dependencies);

    NODISCARD int32 GetNumEntries();

private:
    NODISCARD static String GetFilePath();
    NODISCARD static String MakeAssetRelativePath(const String& Path);

    FShaderBytecodeCache();
    ~FShaderBytecodeCache();

    NODISCARD bool TryGetFileHash(const String& RelativePath, uint64& OutHash);
    NODISCARD bool TryComputeDependencyHash(const TArray<String>& Dependencies, uint64& OutHash);

    bool Load();
    bool Save();

    TMap<uint64, FShaderBytecodeEntry> Entries;
    FCriticalSection                   EntriesCS;
    TMap<String, uint64>               FileHashes;
    FCriticalSection                   FileHashesCS;
    bool                               bDirty;

    static FShaderBytecodeCache* GBytecodeCache;
};
