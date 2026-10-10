#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Map.h"
#include "Core/Containers/Set.h"
#include "Core/Containers/String.h"
#include "Core/Platform/CriticalSection.h"
#include "ShaderCompiler/ShaderCompileJob.h"

struct FShaderCompileInfo;

enum class EShaderJobRecording : uint8
{
    /** Typed shaders, which FShaderCache records by type and permutation instead */
    Skip,

    /** Compiles outside the type registry, recorded with their compile info for the job file */
    Record,
};

struct FShaderBytecodeEntry
{
    /** An FShaderCode container: header, reflection and native code */
    TArray<uint8>  ShaderCode;
    TArray<String> Dependencies;
    uint64         DependencyHash = 0;
};

class SHADERCOMPILER_API FShaderBytecodeCache
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
    static bool CompileFromFile(const String& Filename, const FShaderCompileInfo& CompileInfo, TArray<uint8>& OutShaderCode, EShaderJobRecording Recording = EShaderJobRecording::Record);

    /** @brief The tool writes to the path given with -output. Must be called before Initialize. */
    static void SetFilePathOverride(const String& InFilePath);

public:
    NODISCARD bool Find(uint64 CompileHash, TArray<uint8>& OutShaderCode);
    NODISCARD bool Contains(uint64 CompileHash);

    /** @brief Ignores anything that is not a valid FShaderCode container */
    void Add(uint64 CompileHash, const TArray<uint8>& ShaderCode, const TArray<String>& Dependencies);

    /** @return The standalone compiles recorded this run, empty outside EDITOR_BUILD */
    NODISCARD TArray<FShaderCompileJob> GetRecordedJobs();

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

    void RecordJob(const String& Filename, const FShaderCompileInfo& CompileInfo);

    TMap<uint64, FShaderBytecodeEntry> Entries;
    FCriticalSection                   EntriesCS;
    TMap<String, uint64>               FileHashes;
    FCriticalSection                   FileHashesCS;
    bool                               bDirty;
    TArray<FShaderCompileJob>          RecordedJobs;
    TSet<uint64>                       RecordedJobKeys;
    FCriticalSection                   RecordedJobsCS;

    static String                      FilePathOverride;
    static FShaderBytecodeCache*       BytecodeCache;
};
