#include "MetalRHI/MetalBinaryArchive.h"
#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalStats.h"
#include "Core/Memory/Memory.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Platform/PlatformTime.h"
#include "Core/Tasks/Tasks.h"
#include "Core/Threading/ScopedLock.h"
#include <sys/stat.h>
#include <sys/sysctl.h>

static TAutoConsoleVariable<int32> CVarBinaryArchiveSaveInterval(
    "MetalRHI.BinaryArchiveSaveInterval",
    "Minimum interval in seconds between automatic archive saves",
    30);

// Bump whenever the pipeline descriptors change in a way an archive on disk cannot detect, which discards every archive
static constexpr uint32 GMetalPipelineFormatVersion = 1;

static int64 GetFileSize(const String& Path)
{
    struct stat FileStat;
    return ::stat(*Path, &FileStat) == 0 ? static_cast<int64>(FileStat.st_size) : -1;
}

static String GetErrorString(NSError* Error)
{
    return Error ? String([Error localizedDescription]) : String("unknown error");
}

static void CopyToFixedString(CHAR* Dest, uint32 DestSize, const String& Source)
{
    const uint32 Length = Math::Min<uint32>(static_cast<uint32>(Source.Length()), DestSize - 1);
    Memory::Memcpy(Dest, *Source, Length);
    Dest[Length] = '\0';
}

static bool ReadSidecar(const String& Path, FMetalBinaryArchiveHeader& OutHeader)
{
    TFileRef<IPlatformFile> File = FPlatformFile::OpenForRead(Path);
    if (!File)
    {
        return false;
    }

    if (File->Read(reinterpret_cast<uint8*>(&OutHeader), sizeof(FMetalBinaryArchiveHeader)) != static_cast<int32>(sizeof(FMetalBinaryArchiveHeader)))
    {
        return false;
    }

    OutHeader.OSBuild[sizeof(OutHeader.OSBuild) - 1]       = '\0';
    OutHeader.DeviceName[sizeof(OutHeader.DeviceName) - 1] = '\0';
    return true;
}

static bool WriteSidecar(const String& Path, const FMetalBinaryArchiveHeader& Header)
{
    const String TempPath = Path + ".tmp";
    {
        TFileRef<IPlatformFile> File = FPlatformFile::OpenForWrite(TempPath);
        if (!File)
        {
            return false;
        }

        if (File->Write(reinterpret_cast<const uint8*>(&Header), sizeof(FMetalBinaryArchiveHeader)) != static_cast<int32>(sizeof(FMetalBinaryArchiveHeader)))
        {
            return false;
        }
    }

    return FPlatformFile::MoveFile(*TempPath, *Path);
}

static bool ValidateHeader(const FMetalBinaryArchiveHeader& Found, const FMetalBinaryArchiveHeader& Expected, int64 ArchiveSize, String& OutReason)
{
    if (Found.Magic != Expected.Magic)
    {
        OutReason = "the sidecar is not a Metal archive header";
        return false;
    }

    if (Found.Version != Expected.Version)
    {
        OutReason = String::Printf("sidecar version %u, expected %u", Found.Version, Expected.Version);
        return false;
    }

    if (Found.ShaderFormatVersion != Expected.ShaderFormatVersion)
    {
        OutReason = String::Printf("shader format %u, expected %u", Found.ShaderFormatVersion, Expected.ShaderFormatVersion);
        return false;
    }

    if (Found.PipelineFormatVersion != Expected.PipelineFormatVersion)
    {
        OutReason = String::Printf("pipeline format %u, expected %u", Found.PipelineFormatVersion, Expected.PipelineFormatVersion);
        return false;
    }

    if (CString::Strcmp(Found.DeviceName, Expected.DeviceName) != 0)
    {
        OutReason = String::Printf("GPU '%s', expected '%s'", Found.DeviceName, Expected.DeviceName);
        return false;
    }

    if (Found.RegistryID != Expected.RegistryID)
    {
        OutReason = String::Printf("GPU registry ID 0x%llx, expected 0x%llx", static_cast<unsigned long long>(Found.RegistryID), static_cast<unsigned long long>(Expected.RegistryID));
        return false;
    }

    if (Found.HighestFamily != Expected.HighestFamily)
    {
        OutReason = String::Printf("GPU family %lld, expected %lld", static_cast<long long>(Found.HighestFamily), static_cast<long long>(Expected.HighestFamily));
        return false;
    }

    if (Found.OSMajor != Expected.OSMajor || Found.OSMinor != Expected.OSMinor || Found.OSPatch != Expected.OSPatch)
    {
        OutReason = String::Printf("OS %d.%d.%d, expected %d.%d.%d", Found.OSMajor, Found.OSMinor, Found.OSPatch, Expected.OSMajor, Expected.OSMinor, Expected.OSPatch);
        return false;
    }

    if (CString::Strcmp(Found.OSBuild, Expected.OSBuild) != 0)
    {
        OutReason = String::Printf("OS build %s, expected %s", Found.OSBuild, Expected.OSBuild);
        return false;
    }

    if (ArchiveSize < 0 || Found.ArchiveSize != static_cast<uint64>(ArchiveSize))
    {
        OutReason = String::Printf("archive size %lld, expected %llu", static_cast<long long>(ArchiveSize), static_cast<unsigned long long>(Found.ArchiveSize));
        return false;
    }

    return true;
}

EMetalBinaryArchiveMode FMetalBinaryArchive::ParseMode(const String& ModeName)
{
    if (ModeName.Equals("Ignore", EStringCaseType::NoCase))
    {
        return EMetalBinaryArchiveMode::Ignore;
    }

    if (ModeName.Equals("Use", EStringCaseType::NoCase))
    {
        return EMetalBinaryArchiveMode::Use;
    }

    if (ModeName.Equals("Create", EStringCaseType::NoCase))
    {
        return EMetalBinaryArchiveMode::Create;
    }

    if (!ModeName.Equals("Append", EStringCaseType::NoCase))
    {
        METAL_WARNING("[FMetalBinaryArchive] Unknown MetalRHI.BinaryArchiveMode '%s', using Append", *ModeName);
    }

    return EMetalBinaryArchiveMode::Append;
}

const CHAR* FMetalBinaryArchive::GetModeName(EMetalBinaryArchiveMode Mode)
{
    switch (Mode)
    {
        case EMetalBinaryArchiveMode::Ignore: return "Ignore";
        case EMetalBinaryArchiveMode::Use:    return "Use";
        case EMetalBinaryArchiveMode::Append: return "Append";
        case EMetalBinaryArchiveMode::Create: return "Create";
        default:                              return "Unknown";
    }
}

FMetalBinaryArchive::FMetalBinaryArchive(FMetalDevice* InDevice)
    : Device(InDevice)
    , Mode(EMetalBinaryArchiveMode::Ignore)
    , FilePath()
    , RejectionReason()
    , LookupArchive(nil)
    , WritableArchive(nil)
    , ArchiveCS()
    , SaveTask()
    , bDirty(false)
    , bAddedToLookupArchive(false)
    , bSaveInFlight(false)
    , LastSaveTimestamp(0)
{
}

FMetalBinaryArchive::~FMetalBinaryArchive()
{
    WaitForSave();

    if (WritableArchive != LookupArchive)
    {
        [WritableArchive release];
    }

    [LookupArchive release];
}

bool FMetalBinaryArchive::Initialize(EMetalBinaryArchiveMode InMode, const String& InFilePath)
{
    Mode              = InMode;
    FilePath          = InFilePath;
    LastSaveTimestamp = FPlatformTime::QueryPerformanceCounter();

    if (Mode == EMetalBinaryArchiveMode::Ignore)
    {
        METAL_INFO("[FMetalBinaryArchive] Binary archive disabled (Ignore)");
        return true;
    }

    if (Mode == EMetalBinaryArchiveMode::Use || Mode == EMetalBinaryArchiveMode::Append)
    {
        if (LoadFromFile() && Mode == EMetalBinaryArchiveMode::Append)
        {
            WritableArchive = LookupArchive;
        }
    }

    if ((Mode == EMetalBinaryArchiveMode::Append || Mode == EMetalBinaryArchiveMode::Create) && !WritableArchive)
    {
        if (!CreateWritableArchive())
        {
            return false;
        }
    }

    METAL_INFO("[FMetalBinaryArchive] Mode %s, file '%s'", GetModeName(Mode), *FilePath);
    return true;
}

bool FMetalBinaryArchive::LoadFromFile()
{
    SCOPED_AUTORELEASE_POOL();

    const String SidecarPath = FilePath + ".version";
    if (!FPlatformFile::IsFile(*FilePath) || !FPlatformFile::IsFile(*SidecarPath))
    {
        RejectionReason = "no archive on disk yet";
        METAL_INFO("[FMetalBinaryArchive] '%s' not used: %s", *FilePath, *RejectionReason);
        return false;
    }

    FMetalBinaryArchiveHeader Found;
    if (!ReadSidecar(SidecarPath, Found))
    {
        RejectionReason = "the sidecar could not be read";
        METAL_WARNING("[FMetalBinaryArchive] '%s' rejected: %s. Pipelines compile normally", *FilePath, *RejectionReason);
        return false;
    }

    if (!ValidateHeader(Found, MakeExpectedHeader(), GetFileSize(FilePath), RejectionReason))
    {
        METAL_WARNING("[FMetalBinaryArchive] '%s' rejected: %s. Pipelines compile normally", *FilePath, *RejectionReason);
        return false;
    }

    MTLBinaryArchiveDescriptor* Descriptor = [[MTLBinaryArchiveDescriptor new] autorelease];
    Descriptor.url = [NSURL fileURLWithPath:FilePath.GetNSString()];

    NSError* Error = nil;
    LookupArchive = [Device->GetMTLDevice() newBinaryArchiveWithDescriptor:Descriptor error:&Error];
    if (!LookupArchive)
    {
        RejectionReason = String::Printf("Metal refused the file (error %ld: %s)", static_cast<long>(Error ? Error.code : 0), *GetErrorString(Error));
        METAL_WARNING("[FMetalBinaryArchive] '%s' rejected: %s. Pipelines compile normally", *FilePath, *RejectionReason);
        return false;
    }

    METAL_INFO("[FMetalBinaryArchive] Loaded '%s'", *FilePath);
    return true;
}

bool FMetalBinaryArchive::CreateWritableArchive()
{
    SCOPED_AUTORELEASE_POOL();

    MTLBinaryArchiveDescriptor* Descriptor = [[MTLBinaryArchiveDescriptor new] autorelease];

    NSError* Error = nil;
    id<MTLBinaryArchive> NewArchive = [Device->GetMTLDevice() newBinaryArchiveWithDescriptor:Descriptor error:&Error];
    if (!NewArchive)
    {
        METAL_ERROR("[FMetalBinaryArchive] Failed to create a writable archive: %s", *GetErrorString(Error));
        return false;
    }

    WritableArchive = NewArchive;
    return true;
}

TSharedRef<FMetalCachedRenderPipeline> FMetalBinaryArchive::CreateRenderPipeline(MTLRenderPipelineDescriptor* Descriptor)
{
    SCOPED_AUTORELEASE_POOL();

    id<MTLDevice> MTLDevice = Device->GetMTLDevice();

    id<MTLRenderPipelineState>   PipelineState = nil;
    MTLRenderPipelineReflection* Reflection    = nil;

    if (LookupArchive)
    {
        Descriptor.binaryArchives = @[LookupArchive];
        PipelineState = [MTLDevice newRenderPipelineStateWithDescriptor:Descriptor
                                                                options:MTLPipelineOptionBindingInfo | MTLPipelineOptionFailOnBinaryArchiveMiss
                                                             reflection:&Reflection
                                                                  error:nil];
        STAT_ADD(PipelineState ? STAT_Metal_BinaryArchiveHits : STAT_Metal_BinaryArchiveMisses, 1);
    }

    const bool bHit = PipelineState != nil;
    if (!bHit)
    {
        NSError* Error = nil;
        PipelineState = [MTLDevice newRenderPipelineStateWithDescriptor:Descriptor
                                                                options:MTLPipelineOptionBindingInfo
                                                             reflection:&Reflection
                                                                  error:&Error];
        if (!PipelineState)
        {
            METAL_ERROR("Failed to create pipeline state, error %s", *GetErrorString(Error));
            return nullptr;
        }
    }

    TSharedRef<FMetalCachedRenderPipeline> NewPipeline = new FMetalCachedRenderPipeline();
    NewPipeline->PipelineState = PipelineState;
    NewPipeline->Reflection    = [Reflection retain];

    Harvest(bHit, Descriptor);

    STAT_ADD(STAT_Metal_PSOCreateCount, 1);
    return NewPipeline;
}

TSharedRef<FMetalCachedRenderPipeline> FMetalBinaryArchive::CreateMeshRenderPipeline(MTLMeshRenderPipelineDescriptor* Descriptor)
{
    SCOPED_AUTORELEASE_POOL();

    id<MTLDevice> MTLDevice = Device->GetMTLDevice();

    id<MTLRenderPipelineState>   PipelineState = nil;
    MTLRenderPipelineReflection* Reflection    = nil;

    if (@available(macOS 15.0, *))
    {
        if (LookupArchive)
        {
            Descriptor.binaryArchives = @[LookupArchive];
            PipelineState = [MTLDevice newRenderPipelineStateWithMeshDescriptor:Descriptor
                                                                        options:MTLPipelineOptionBindingInfo | MTLPipelineOptionFailOnBinaryArchiveMiss
                                                                     reflection:&Reflection
                                                                          error:nil];
            STAT_ADD(PipelineState ? STAT_Metal_BinaryArchiveHits : STAT_Metal_BinaryArchiveMisses, 1);
        }
    }

    const bool bHit = PipelineState != nil;
    if (!bHit)
    {
        NSError* Error = nil;
        PipelineState = [MTLDevice newRenderPipelineStateWithMeshDescriptor:Descriptor
                                                                    options:MTLPipelineOptionBindingInfo
                                                                 reflection:&Reflection
                                                                      error:&Error];
        if (!PipelineState)
        {
            METAL_ERROR("Failed to create meshlet pipeline state, error %s", *GetErrorString(Error));
            return nullptr;
        }
    }

    TSharedRef<FMetalCachedRenderPipeline> NewPipeline = new FMetalCachedRenderPipeline();
    NewPipeline->PipelineState = PipelineState;
    NewPipeline->Reflection    = [Reflection retain];

    if (@available(macOS 15.0, *))
    {
        Harvest(bHit, Descriptor);
    }

    STAT_ADD(STAT_Metal_PSOCreateCount, 1);
    return NewPipeline;
}

TSharedRef<FMetalCachedComputePipeline> FMetalBinaryArchive::CreateComputePipeline(MTLComputePipelineDescriptor* Descriptor)
{
    SCOPED_AUTORELEASE_POOL();

    id<MTLDevice> MTLDevice = Device->GetMTLDevice();

    id<MTLComputePipelineState>   PipelineState = nil;
    MTLComputePipelineReflection* Reflection    = nil;

    if (LookupArchive)
    {
        Descriptor.binaryArchives = @[LookupArchive];
        PipelineState = [MTLDevice newComputePipelineStateWithDescriptor:Descriptor
                                                                 options:MTLPipelineOptionBindingInfo | MTLPipelineOptionFailOnBinaryArchiveMiss
                                                              reflection:&Reflection
                                                                   error:nil];
        STAT_ADD(PipelineState ? STAT_Metal_BinaryArchiveHits : STAT_Metal_BinaryArchiveMisses, 1);
    }

    const bool bHit = PipelineState != nil;
    if (!bHit)
    {
        NSError* Error = nil;
        PipelineState = [MTLDevice newComputePipelineStateWithDescriptor:Descriptor
                                                                 options:MTLPipelineOptionBindingInfo
                                                              reflection:&Reflection
                                                                   error:&Error];
        if (!PipelineState)
        {
            METAL_ERROR("Failed to create compute pipeline state, error %s", *GetErrorString(Error));
            return nullptr;
        }
    }

    TSharedRef<FMetalCachedComputePipeline> NewPipeline = new FMetalCachedComputePipeline();
    NewPipeline->PipelineState = PipelineState;
    NewPipeline->Reflection    = [Reflection retain];

    Harvest(bHit, Descriptor);

    STAT_ADD(STAT_Metal_PSOCreateCount, 1);
    return NewPipeline;
}

static bool AddPipelineFunctions(id<MTLBinaryArchive> Archive, id Descriptor, NSError** OutError)
{
    if ([Descriptor isKindOfClass:[MTLComputePipelineDescriptor class]])
    {
        return [Archive addComputePipelineFunctionsWithDescriptor:static_cast<MTLComputePipelineDescriptor*>(Descriptor) error:OutError];
    }

    if ([Descriptor isKindOfClass:[MTLMeshRenderPipelineDescriptor class]])
    {
        if (@available(macOS 15.0, *))
        {
            return [Archive addMeshRenderPipelineFunctionsWithDescriptor:static_cast<MTLMeshRenderPipelineDescriptor*>(Descriptor) error:OutError];
        }

        return false;
    }

    return [Archive addRenderPipelineFunctionsWithDescriptor:static_cast<MTLRenderPipelineDescriptor*>(Descriptor) error:OutError];
}

void FMetalBinaryArchive::Harvest(bool bHit, id Descriptor)
{
    TScopedLock Lock(ArchiveCS);

    if (!WritableArchive || (bHit && WritableArchive == LookupArchive))
    {
        return;
    }

    NSError* Error = nil;
    bool bAdded = AddPipelineFunctions(WritableArchive, Descriptor, &Error);

    if (!bAdded && WritableArchive == LookupArchive && !bAddedToLookupArchive)
    {
        METAL_WARNING("[FMetalBinaryArchive] Metal refused additions to '%s' (%s). This session's pipelines go into a fresh archive", *FilePath, *GetErrorString(Error));

        if (!CreateWritableArchive())
        {
            return;
        }

        Error  = nil;
        bAdded = AddPipelineFunctions(WritableArchive, Descriptor, &Error);
    }

    if (!bAdded)
    {
        METAL_WARNING("[FMetalBinaryArchive] Failed to add a pipeline to '%s': %s", *FilePath, *GetErrorString(Error));
        return;
    }

    bAddedToLookupArchive |= WritableArchive == LookupArchive;
    bDirty = true;
    STAT_ADD(STAT_Metal_BinaryArchiveAdds, 1);
}

bool FMetalBinaryArchive::Save()
{
    SCOPED_AUTORELEASE_POOL();

    TScopedLock Lock(ArchiveCS);
    if (!WritableArchive || !bDirty)
    {
        return true;
    }

    const String TempPath = FilePath + ".tmp";
    NSError* Error = nil;
    if (![WritableArchive serializeToURL:[NSURL fileURLWithPath:TempPath.GetNSString()] error:&Error])
    {
        METAL_WARNING("[FMetalBinaryArchive] Failed to serialize '%s': %s", *TempPath, *GetErrorString(Error));
        return false;
    }

    if (!FPlatformFile::MoveFile(*TempPath, *FilePath))
    {
        METAL_WARNING("[FMetalBinaryArchive] Failed to replace '%s'", *FilePath);
        return false;
    }

    const int64 ArchiveSize = GetFileSize(FilePath);

    FMetalBinaryArchiveHeader Header = MakeExpectedHeader();
    Header.ArchiveSize = static_cast<uint64>(Math::Max<int64>(ArchiveSize, 0));

    if (!WriteSidecar(FilePath + ".version", Header))
    {
        METAL_WARNING("[FMetalBinaryArchive] Failed to write the sidecar for '%s'", *FilePath);
        return false;
    }

    bDirty = false;
    STAT_SET(STAT_Metal_BinaryArchiveSize, ArchiveSize);
    METAL_INFO("[FMetalBinaryArchive] Saved '%s' (%lld bytes)", *FilePath, static_cast<long long>(ArchiveSize));
    return true;
}

void FMetalBinaryArchive::SaveAsync()
{
    if (bSaveInFlight.Load())
    {
        return;
    }

    {
        TScopedLock Lock(ArchiveCS);
        if (!WritableArchive || !bDirty)
        {
            return;
        }
    }

    const uint64 CurrentTime    = FPlatformTime::QueryPerformanceCounter();
    const uint64 Frequency      = FPlatformTime::QueryPerformanceFrequency();
    const double ElapsedSeconds = static_cast<double>(CurrentTime - LastSaveTimestamp) / static_cast<double>(Frequency);

    if (ElapsedSeconds < static_cast<double>(CVarBinaryArchiveSaveInterval.GetValue()))
    {
        return;
    }

    LastSaveTimestamp = CurrentTime;
    bSaveInFlight.Store(true);

    SaveTask = Tasks::Async([this]()
    {
        Save();
        bSaveInFlight.Store(false);
    });
}

void FMetalBinaryArchive::WaitForSave()
{
    if (SaveTask.IsValid())
    {
        SaveTask.Wait();
        SaveTask = FTaskHandle();
    }
}

FMetalBinaryArchiveHeader FMetalBinaryArchive::MakeExpectedHeader() const
{
    FMetalBinaryArchiveHeader Header;
    Memory::Memzero(&Header, sizeof(FMetalBinaryArchiveHeader));

    const FMetalDeviceProperties& Properties = Device->GetProperties();
    Header.Magic                 = FMetalBinaryArchiveHeader::ExpectedMagic;
    Header.Version               = FMetalBinaryArchiveHeader::ExpectedVersion;
    Header.ShaderFormatVersion   = FMSLShaderHeader::ExpectedVersion;
    Header.PipelineFormatVersion = GMetalPipelineFormatVersion;
    Header.RegistryID            = Properties.RegistryID;
    Header.HighestFamily         = static_cast<int64>(Properties.HighestSupportedFamily);

    const NSOperatingSystemVersion OSVersion = [[NSProcessInfo processInfo] operatingSystemVersion];
    Header.OSMajor = static_cast<int32>(OSVersion.majorVersion);
    Header.OSMinor = static_cast<int32>(OSVersion.minorVersion);
    Header.OSPatch = static_cast<int32>(OSVersion.patchVersion);

    size_t BuildLength = sizeof(Header.OSBuild) - 1;
    if (::sysctlbyname("kern.osversion", Header.OSBuild, &BuildLength, nullptr, 0) != 0)
    {
        Header.OSBuild[0] = '\0';
    }

    CopyToFixedString(Header.DeviceName, sizeof(Header.DeviceName), Properties.Name);
    return Header;
}
