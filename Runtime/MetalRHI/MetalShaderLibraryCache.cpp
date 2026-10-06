#include "MetalRHI/MetalShaderLibraryCache.h"
#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalStats.h"
#include "Core/Memory/Memory.h"
#include "Core/Misc/CRC.h"
#include "Core/Threading/ScopedLock.h"

static NSString* ResolveMetalFunctionName(id<MTLLibrary> Library)
{
    NSArray<NSString*>* FunctionNames = [Library functionNames];

    if (FunctionNames.count == 0)
    {
        return nil;
    }

    if ([FunctionNames containsObject:@"Main"])
    {
        return @"Main";
    }

    return FunctionNames.firstObject;
}

static uint64 HashSource(TArrayView<const uint8> Source)
{
    uint64 Hash = static_cast<uint64>(Source.Size());
    HashCombine(Hash, CRC32::Generate(Source.Data(), Source.Size()));
    return Hash;
}

FMetalCompiledShader::FMetalCompiledShader() = default;

FMetalCompiledShader::~FMetalCompiledShader()
{
    [Function release];
    [FunctionName release];
    [Library release];
}

FMetalShaderLibraryCache::FMetalShaderLibraryCache(FMetalDevice* InDevice)
    : Device(InDevice)
    , Entries()
    , EntriesCS()
{
}

FMetalShaderLibraryCache::~FMetalShaderLibraryCache() = default;

TSharedRef<FMetalCompiledShader> FMetalShaderLibraryCache::GetOrCompile(TArrayView<const uint8> Source)
{
    const uint64 Hash = HashSource(Source);
    {
        TScopedLock Lock(EntriesCS);
        if (TSharedRef<FMetalCompiledShader> Existing = Find(Hash, Source))
        {
            STAT_ADD(STAT_Metal_LibraryCacheHits, 1);
            return Existing;
        }
    }

    TSharedRef<FMetalCompiledShader> NewShader = Compile(Source);
    if (!NewShader)
    {
        return nullptr;
    }

    TScopedLock Lock(EntriesCS);
    if (TSharedRef<FMetalCompiledShader> Existing = Find(Hash, Source))
    {
        return Existing;
    }

    STAT_ADD(STAT_Metal_LibraryCacheMisses, 1);

    FEntry& NewEntry = Entries.FindOrAdd(Hash).Emplace();
    NewEntry.Source = TArray<uint8>(Source.Data(), Source.Size());
    NewEntry.Shader = NewShader;
    return NewShader;
}

void FMetalShaderLibraryCache::Prune()
{
    TScopedLock Lock(EntriesCS);

    TArray<uint64> EmptyBuckets;
    Entries.Foreach([&EmptyBuckets](const uint64& Hash, TArray<FEntry>& Bucket)
    {
        for (int32 Index = Bucket.Size() - 1; Index >= 0; --Index)
        {
            if (Bucket[Index].Shader->GetRefCount() == 1)
            {
                Bucket.RemoveAtSwap(Index);
            }
        }

        if (Bucket.IsEmpty())
        {
            EmptyBuckets.Add(Hash);
        }
    });

    for (uint64 Hash : EmptyBuckets)
    {
        Entries.Remove(Hash);
    }
}

TSharedRef<FMetalCompiledShader> FMetalShaderLibraryCache::Find(uint64 Hash, TArrayView<const uint8> Source) const
{
    const TArray<FEntry>* Bucket = Entries.Find(Hash);
    if (!Bucket)
    {
        return nullptr;
    }

    for (const FEntry& Entry : *Bucket)
    {
        if (Entry.Source.Size() == Source.Size() && Memory::Memcmp(Entry.Source.Data(), Source.Data(), Source.Size()) == 0)
        {
            return Entry.Shader;
        }
    }

    return nullptr;
}

TSharedRef<FMetalCompiledShader> FMetalShaderLibraryCache::Compile(TArrayView<const uint8> Source) const
{
    SCOPED_AUTORELEASE_POOL();

    const String SourceString(reinterpret_cast<const CHAR*>(Source.Data()), Source.Size());

    NSString* SourceText = SourceString.GetNSString();
    CHECK(SourceText != nil);

    id<MTLDevice> MTLDevice = Device->GetMTLDevice();
    CHECK(MTLDevice != nil);

    NSError*       Error   = nil;
    id<MTLLibrary> Library = [MTLDevice newLibraryWithSource:SourceText options:nil error:&Error];

    if (!Library)
    {
        const String ErrorString([Error localizedDescription]);
        LOG_ERROR("Failed to compile shader. Error: %s", *ErrorString);
        return nullptr;
    }

    TSharedRef<FMetalCompiledShader> NewShader = new FMetalCompiledShader();
    NewShader->Library      = Library;
    NewShader->FunctionName = [ResolveMetalFunctionName(Library) retain];

    if (!NewShader->FunctionName)
    {
        LOG_ERROR("Compiled Library does not contain an entry-point");
        return nullptr;
    }

    NewShader->Function = [Library newFunctionWithName:NewShader->FunctionName];

    if (!NewShader->Function)
    {
        const String NameString(NewShader->FunctionName);
        LOG_ERROR("Failed to retrieve function '%s' from Library", *NameString);
        return nullptr;
    }

    return NewShader;
}
