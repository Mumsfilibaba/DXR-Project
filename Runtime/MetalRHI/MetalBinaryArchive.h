#pragma once
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/String.h"
#include "Core/Platform/CriticalSection.h"
#include "Core/Tasks/TaskHandle.h"
#include "Core/Threading/Atomic/AtomicBool.h"
#include "MetalRHI/MetalPipelineState.h"

class FMetalDevice;

enum class EMetalBinaryArchiveMode : uint8
{
    Ignore = 0,
    Use,
    Append,
    Create,
};

struct FMetalBinaryArchiveHeader
{
    static constexpr uint32 ExpectedMagic   = 0x4D544C41;
    static constexpr uint32 ExpectedVersion = 1;

    uint32 Magic;
    uint32 Version;
    uint32 ShaderFormatVersion;
    uint32 PipelineFormatVersion;
    uint64 RegistryID;
    uint64 ArchiveSize;
    int64  HighestFamily;
    int32  OSMajor;
    int32  OSMinor;
    int32  OSPatch;
    CHAR   OSBuild[32];
    CHAR   DeviceName[128];
};

class FMetalBinaryArchive
{
public:
    static EMetalBinaryArchiveMode ParseMode(const String& ModeName);
    static const CHAR* GetModeName(EMetalBinaryArchiveMode Mode);

    explicit FMetalBinaryArchive(FMetalDevice* InDevice);
    ~FMetalBinaryArchive();

    bool Initialize(EMetalBinaryArchiveMode InMode, const String& InFilePath);

    TSharedRef<FMetalCachedRenderPipeline>  CreateRenderPipeline(MTLRenderPipelineDescriptor* Descriptor);
    TSharedRef<FMetalCachedRenderPipeline>  CreateMeshRenderPipeline(MTLMeshRenderPipelineDescriptor* Descriptor);
    TSharedRef<FMetalCachedComputePipeline> CreateComputePipeline(MTLComputePipelineDescriptor* Descriptor);

    bool Save();
    void SaveAsync();
    void WaitForSave();

    EMetalBinaryArchiveMode GetMode()            const { return Mode; }
    const String&           GetFilePath()        const { return FilePath; }
    const String&           GetRejectionReason() const { return RejectionReason; }

private:
    bool LoadFromFile();
    bool CreateWritableArchive();
    void Harvest(bool bHit, id Descriptor);
    FMetalBinaryArchiveHeader MakeExpectedHeader() const;

    FMetalDevice*           Device;
    EMetalBinaryArchiveMode Mode;
    String                  FilePath;
    String                  RejectionReason;
    id<MTLBinaryArchive>    LookupArchive;
    id<MTLBinaryArchive>    WritableArchive;
    FCriticalSection        ArchiveCS;
    FTaskHandle             SaveTask;
    bool                    bDirty;
    bool                    bAddedToLookupArchive;
    AtomicBool              bSaveInFlight;
    uint64                  LastSaveTimestamp;
};
