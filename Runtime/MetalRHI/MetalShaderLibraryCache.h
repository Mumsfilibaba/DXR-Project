#pragma once
#include "Core/RefCounted.h"
#include "Core/Containers/Array.h"
#include "Core/Containers/ArrayView.h"
#include "Core/Containers/Map.h"
#include "Core/Containers/String.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Platform/CriticalSection.h"
#include "MetalRHI/MetalCore.h"

class FMetalDevice;

struct FMetalCompiledShader : public FRefCounted
{
    FMetalCompiledShader();
    ~FMetalCompiledShader();

    id<MTLLibrary>  Library      = nil;
    id<MTLFunction> Function     = nil;
    NSString*       FunctionName = nil;
};

class FMetalShaderLibraryCache
{
public:
    explicit FMetalShaderLibraryCache(FMetalDevice* InDevice);
    ~FMetalShaderLibraryCache();

    /**
     * @brief Returns the shader for Source and EntryPoint, compiling the library on a miss
     *
     * @param Source MSL source, not null-terminated
     * @param EntryPoint Function to look up in the library, or empty to take "Main" or else the first function
     * @return The cached or newly compiled shader, or nullptr when the source fails to compile or lacks the function
     */
    TSharedRef<FMetalCompiledShader> GetOrCompile(TArrayView<const uint8> Source, const CHAR* EntryPoint);
    void Prune();

private:
    struct FEntry
    {
        TArray<uint8>                    Source;
        String                           EntryPoint;
        TSharedRef<FMetalCompiledShader> Shader;
    };

    TSharedRef<FMetalCompiledShader> Find(uint64 Hash, TArrayView<const uint8> Source, const CHAR* EntryPoint) const;
    TSharedRef<FMetalCompiledShader> Compile(TArrayView<const uint8> Source, const CHAR* EntryPoint) const;

    FMetalDevice*                Device;
    TMap<uint64, TArray<FEntry>> Entries;
    FCriticalSection             EntriesCS;
};
