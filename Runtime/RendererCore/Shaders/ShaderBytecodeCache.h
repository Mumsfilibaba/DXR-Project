#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Map.h"
#include "Core/Containers/String.h"
#include "Core/Platform/CriticalSection.h"

struct FShaderCompileInfo;

struct FShaderBytecodeEntry
{
    /** An FShaderCode container: header, reflection and native code */
    TArray<uint8>  ShaderCode;
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
        return *BytecodeCache;
    }

    static FORCEINLINE FShaderBytecodeCache* TryGet()
    {
        return BytecodeCache;
    }

    /** @brief Returns the cached container for CompileInfo, or compiles and caches it. Without a cache (tests, Playground) it only compiles. */
    static bool CompileFromFile(const String& Filename, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutShaderCode);

public:
    NODISCARD bool Find(uint64 CompileHash, TArray<uint8>& OutShaderCode);

    /** @brief Ignores anything that is not a valid FShaderCode container */
    void Add(uint64 CompileHash, const TArray<uint8>& ShaderCode, const TArray<String>& Dependencies);

    NODISCARD int32 GetNumEntries();

    void LogStats();

    /** Public so Shaders.CompileAll can write a baked cache without waiting for shutdown */
    bool Save();

private:
    NODISCARD static String GetFilePath();
    NODISCARD static String CreateAssetRelativePath(const String& Path);

    FShaderBytecodeCache();
    ~FShaderBytecodeCache();

    NODISCARD bool TryGetFileHash(const String& RelativePath, uint64& OutHash);
    NODISCARD bool TryComputeDependencyHash(const TArray<String>& Dependencies, uint64& OutHash);

    bool Load();

    TMap<uint64, FShaderBytecodeEntry> Entries;
    FCriticalSection                   EntriesCS;
    TMap<String, uint64>               FileHashes;
    FCriticalSection                   FileHashesCS;
    bool                               bDirty;

    static FShaderBytecodeCache* BytecodeCache;
};
