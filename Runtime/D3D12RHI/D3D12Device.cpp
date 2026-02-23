#include "Core/Windows/Windows.h"
#include "Core/Platform/PlatformLibrary.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Misc/CoreDelegates.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Containers/String.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "D3D12RHI/D3D12Device.h"
#include "D3D12RHI/D3D12Descriptors.h"
#include "D3D12RHI/D3D12RootSignature.h"
#include "D3D12RHI/D3D12PipelineState.h"
#include "D3D12RHI/D3D12Loader.h"
#include "D3D12RHI/D3D12Allocators.h"
#include "D3D12RHI/D3D12ResidencyManager.h"
#include "D3D12RHI/D3D12CommandContext.h"
#include "D3D12RHI/D3D12RHI.h"

#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")

static TAutoConsoleVariable<bool> CVarEnableGPUValidation(
    "D3D12RHI.EnableGPUValidation",
    "Enables GPU Based Validation if true",
    false);

static TAutoConsoleVariable<bool> CVarBreakOnError(
    "D3D12RHI.BreakOnError",
    "When enabled, there will be a DebugBreak when the validation layer encounters an errors",
    true);

static TAutoConsoleVariable<bool> CVarBreakOnWarning(
    "D3D12RHI.BreakOnWarning",
    "When enabled, there will be a DebugBreak when the validation layer encounters an warnings",
    false);

static TAutoConsoleVariable<bool> CVarEnableDRED(
    "D3D12RHI.EnableDRED",
    "Enables Device Removed Extended Data (DRED) if the Device gets removed",
    false);

static TAutoConsoleVariable<bool> CVarPreferDedicatedGPU(
    "D3D12RHI.PreferDedicatedGPU",
    "When enabled, a dedicated GPU will be selected when creating a the Device", 
    true);

static TAutoConsoleVariable<int32> CVarResourceOnlineDescriptorBlockSize(
    "D3D12RHI.ResourceOnlineDescriptorBlockSize",
    "Number of descriptors in each Resource OnlineDescriptorHeap", 
    2048);

static TAutoConsoleVariable<int32> CVarSamplerOnlineDescriptorBlockSize(
    "D3D12RHI.SamplerOnlineDescriptorBlockSize",
    "Number of descriptors in each Sampler OnlineDescriptorHeap", 
    1024);

static TAutoConsoleVariable<int32> CVarUploadHeapSmallAllocationThreshold(
    "D3D12RHI.UploadHeapSmallAllocationThreshold",
    "Allocation size threshold for the upload small allocator path (bytes)",
    64 * 1024);

static TAutoConsoleVariable<int32> CVarUploadHeapLargeAllocationThreshold(
    "D3D12RHI.UploadHeapLargeAllocationThreshold",
    "Max suballocation size before upload allocations become standalone (bytes)",
    2 * 1024 * 1024);

static TAutoConsoleVariable<int32> CVarTextureAllocatorDefaultPageSize(
    "D3D12RHI.TextureAllocatorDefaultPageSize",
    "Default page size for pooled texture allocator pages (bytes)",
    128 * 1024 * 1024);

static TAutoConsoleVariable<int32> CVarTextureAllocatorCommittedThreshold(
    "D3D12RHI.TextureAllocatorCommittedThreshold",
    "Size threshold for committed texture allocations (bytes)",
    128 * 1024 * 1024);

static TAutoConsoleVariable<int32> CVarDynamicConstantsAllocatorPageSize(
    "D3D12RHI.DynamicConstantsAllocatorPageSize",
    "Page size for the dynamic constants linear allocator (bytes)",
    4 * 1024 * 1024);

static TAutoConsoleVariable<bool> CVarEnableResidencyTracking(
    "D3D12RHI.EnableResidencyTracking",
    "Enables GPU memory residency tracking and eviction management",
    true);

static TAutoConsoleVariable<int32> CVarResidencyTargetBudget(
    "D3D12RHI.ResidencyTargetBudget",
    "Override target budget for residency eviction in MB (0 = use adapter-reported budget)",
    0);

static TAutoConsoleVariable<int32> CVarUploadHeapPageSize(
    "D3D12RHI.UploadHeapPageSize",
    "Page size for the upload heap allocator in KB",
    8 * 1024);

static TAutoConsoleVariable<int32> CVarStagingBufferPageSize(
    "D3D12RHI.StagingBufferPageSize",
    "Page size for the staging buffer linear allocator in KB",
    16 * 1024);

static TAutoConsoleVariable<int32> CVarBufferAllocatorPageSize(
    "D3D12RHI.BufferAllocatorPageSize",
    "Page size for the buffer buddy allocator in MB",
    128);

static TAutoConsoleVariable<int32> CVarBufferAllocatorMaxSuballocationSize(
    "D3D12RHI.BufferAllocatorMaxSuballocationSize",
    "Max suballocation size before buffer allocations become committed resources in MB",
    64);

static TAutoConsoleVariable<int32> CVarNumTimestampQueriesPerHeap(
    "D3D12RHI.NumTimestampQueriesPerHeap",
    "Number of timestamp queries in each timestamp query heap",
    D3D12_DEFAULT_QUERY_COUNT);

static TAutoConsoleVariable<int32> CVarNumOcclusionQueriesPerHeap(
    "D3D12RHI.NumOcclusionQueriesPerHeap",
    "Number of occlusion queries in each occlusion query heap",
    D3D12_DEFAULT_QUERY_COUNT);

static TAutoConsoleVariable<FString> CVarDeviceRemovedDumpFilePath(
    "D3D12RHI.DeviceRemovedDumpFilePath",
    "File path for DRED device removed dump output",
    "D3D12DeviceRemovedDump.txt");

static TAutoConsoleVariable<int32> CVarMaxDefragMovesPerFrame(
    "D3D12RHI.MaxDefragMovesPerFrame",
    "Maximum number of resource defragmentation moves per frame (0 to disable)",
    4);

static TAutoConsoleVariable<bool> CVarEnableTightAlignment(
    "D3D12RHI.EnableTightAlignment",
    "Enable tight alignment if supported by the device",
    true);

// -------------------------------------------------------------------------------------------
// D3D12 Feature Support
// -------------------------------------------------------------------------------------------

// -------------------------------------------------------------------------------------------
// General Capability Flags
// -------------------------------------------------------------------------------------------

D3D12RHI_API bool GD3D12ForceBinding            = false;
D3D12RHI_API bool GD3D12SupportPipelineCache    = false;
D3D12RHI_API bool GD3D12SupportTightAlignment   = false;
D3D12RHI_API bool GD3D12SupportGPUUploadHeaps   = false;
D3D12RHI_API bool GD3D12SupportsBindless        = false;
D3D12RHI_API bool GD3D12SupportEnhancedBarriers = false;

// -------------------------------------------------------------------------------------------
// Core Feature Tiers
// -------------------------------------------------------------------------------------------

D3D12RHI_API D3D12_RESOURCE_BINDING_TIER              GD3D12ResourceBindingTier             = D3D12_RESOURCE_BINDING_TIER_1;
D3D12RHI_API D3D12_RESOURCE_HEAP_TIER                 GD3D12ResourceHeapTier                = D3D12_RESOURCE_HEAP_TIER_1;
D3D12RHI_API D3D12_RAYTRACING_TIER                    GD3D12RayTracingTier                  = D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
D3D12RHI_API D3D12_VARIABLE_SHADING_RATE_TIER         GD3D12VariableRateShadingTier         = D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED;
D3D12RHI_API D3D12_MESH_SHADER_TIER                   GD3D12MeshShaderTier                  = D3D12_MESH_SHADER_TIER_NOT_SUPPORTED;
D3D12RHI_API D3D12_SAMPLER_FEEDBACK_TIER              GD3D12SamplerFeedbackTier             = D3D12_SAMPLER_FEEDBACK_TIER_NOT_SUPPORTED;
D3D12RHI_API D3D12_VIEW_INSTANCING_TIER               GD3D12ViewInstancingTier              = D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED;
D3D12RHI_API D3D12_CONSERVATIVE_RASTERIZATION_TIER    GD3D12ConservativeRasterizationTier   = D3D12_CONSERVATIVE_RASTERIZATION_TIER_NOT_SUPPORTED;
D3D12RHI_API D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER GD3D12ProgrammableSamplePositionsTier = D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_NOT_SUPPORTED;
D3D12RHI_API D3D12_WORK_GRAPHS_TIER                   GD3D12WorkGraphsTier                  = D3D12_WORK_GRAPHS_TIER_NOT_SUPPORTED;
D3D12RHI_API D3D12_EXECUTE_INDIRECT_TIER              GD3D12ExecuteIndirectTier             = D3D12_EXECUTE_INDIRECT_TIER_1_0;
D3D12RHI_API D3D12_TILED_RESOURCES_TIER               GD3D12TiledResourcesTier              = D3D12_TILED_RESOURCES_TIER_NOT_SUPPORTED;
D3D12RHI_API D3D_ROOT_SIGNATURE_VERSION               GD3D12RootSignatureVersion            = D3D_ROOT_SIGNATURE_VERSION_1_0;
D3D12RHI_API D3D_SHADER_MODEL                         GD3D12HighestShaderModel              = D3D_SHADER_MODEL_6_0;

// -------------------------------------------------------------------------------------------
// Boolean Capability Flags
// -------------------------------------------------------------------------------------------

D3D12RHI_API bool GD3D12RasterizerOrderViewsSupported  = false;
D3D12RHI_API bool GD3D12TypedUAVLoadAdditionalFormats  = false;
D3D12RHI_API bool GD3D12DepthBoundsTestSupported       = false;
D3D12RHI_API bool GD3D12IsArchitectureUMA              = false;
D3D12RHI_API bool GD3D12IsArchitectureCacheCoherentUMA = false;

// -------------------------------------------------------------------------------------------
// Descriptor / Heap Limits
// -------------------------------------------------------------------------------------------

D3D12RHI_API uint32 GD3D12MaxSamplerDescriptorHeapSize  = 0;
D3D12RHI_API uint32 GD3D12MaxResourceDescriptorHeapSize = 0;

// -------------------------------------------------------------------------------------------
// GPU Virtual Address / Command Capabilities
// -------------------------------------------------------------------------------------------

D3D12RHI_API uint32                           GD3D12VirtualAddressBitsPerResource    = 0;
D3D12RHI_API uint32                           GD3D12VirtualAddressBitsPerProcess     = 0;
D3D12RHI_API D3D12_COMMAND_LIST_SUPPORT_FLAGS GD3D12WriteBufferImmediateSupportFlags = D3D12_COMMAND_LIST_SUPPORT_FLAG_NONE;

// -------------------------------------------------------------------------------------------
// Device Removed Handling 
// -------------------------------------------------------------------------------------------

static const CHAR* ToString(D3D12_AUTO_BREADCRUMB_OP BreadCrumbOp)
{
    switch (BreadCrumbOp)
    {
    case D3D12_AUTO_BREADCRUMB_OP_SETMARKER:                                        return "D3D12_AUTO_BREADCRUMB_OP_SETMARKER";
    case D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT:                                       return "D3D12_AUTO_BREADCRUMB_OP_BEGINEVENT";
    case D3D12_AUTO_BREADCRUMB_OP_ENDEVENT:                                         return "D3D12_AUTO_BREADCRUMB_OP_ENDEVENT";
    case D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED:                                    return "D3D12_AUTO_BREADCRUMB_OP_DRAWINSTANCED";
    case D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED:                             return "D3D12_AUTO_BREADCRUMB_OP_DRAWINDEXEDINSTANCED";
    case D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT:                                  return "D3D12_AUTO_BREADCRUMB_OP_EXECUTEINDIRECT";
    case D3D12_AUTO_BREADCRUMB_OP_DISPATCH:                                         return "D3D12_AUTO_BREADCRUMB_OP_DISPATCH";
    case D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION:                                 return "D3D12_AUTO_BREADCRUMB_OP_COPYBUFFERREGION";
    case D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION:                                return "D3D12_AUTO_BREADCRUMB_OP_COPYTEXTUREREGION";
    case D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE:                                     return "D3D12_AUTO_BREADCRUMB_OP_COPYRESOURCE";
    case D3D12_AUTO_BREADCRUMB_OP_COPYTILES:                                        return "D3D12_AUTO_BREADCRUMB_OP_COPYTILES";
    case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE:                               return "D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCE";
    case D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW:                            return "D3D12_AUTO_BREADCRUMB_OP_CLEARRENDERTARGETVIEW";
    case D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW:                         return "D3D12_AUTO_BREADCRUMB_OP_CLEARUNORDEREDACCESSVIEW";
    case D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW:                            return "D3D12_AUTO_BREADCRUMB_OP_CLEARDEPTHSTENCILVIEW";
    case D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER:                                  return "D3D12_AUTO_BREADCRUMB_OP_RESOURCEBARRIER";
    case D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE:                                    return "D3D12_AUTO_BREADCRUMB_OP_EXECUTEBUNDLE";
    case D3D12_AUTO_BREADCRUMB_OP_PRESENT:                                          return "D3D12_AUTO_BREADCRUMB_OP_PRESENT";
    case D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA:                                 return "D3D12_AUTO_BREADCRUMB_OP_RESOLVEQUERYDATA";
    case D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION:                                  return "D3D12_AUTO_BREADCRUMB_OP_BEGINSUBMISSION";
    case D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION:                                    return "D3D12_AUTO_BREADCRUMB_OP_ENDSUBMISSION";
    case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME:                                      return "D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME";
    case D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES:                                    return "D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES";
    case D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT:                             return "D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT";
    case D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT64:                           return "D3D12_AUTO_BREADCRUMB_OP_ATOMICCOPYBUFFERUINT64";
    case D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCEREGION:                         return "D3D12_AUTO_BREADCRUMB_OP_RESOLVESUBRESOURCEREGION";
    case D3D12_AUTO_BREADCRUMB_OP_WRITEBUFFERIMMEDIATE:                             return "D3D12_AUTO_BREADCRUMB_OP_WRITEBUFFERIMMEDIATE";
    case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME1:                                     return "D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME1";
    case D3D12_AUTO_BREADCRUMB_OP_SETPROTECTEDRESOURCESESSION:                      return "D3D12_AUTO_BREADCRUMB_OP_SETPROTECTEDRESOURCESESSION";
    case D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME2:                                     return "D3D12_AUTO_BREADCRUMB_OP_DECODEFRAME2";
    case D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES1:                                   return "D3D12_AUTO_BREADCRUMB_OP_PROCESSFRAMES1";
    case D3D12_AUTO_BREADCRUMB_OP_BUILDRAYTRACINGACCELERATIONSTRUCTURE:             return "D3D12_AUTO_BREADCRUMB_OP_BUILDRAYTRACINGACCELERATIONSTRUCTURE";
    case D3D12_AUTO_BREADCRUMB_OP_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO: return "D3D12_AUTO_BREADCRUMB_OP_EMITRAYTRACINGACCELERATIONSTRUCTUREPOSTBUILDINFO";
    case D3D12_AUTO_BREADCRUMB_OP_COPYRAYTRACINGACCELERATIONSTRUCTURE:              return "D3D12_AUTO_BREADCRUMB_OP_COPYRAYTRACINGACCELERATIONSTRUCTURE";
    case D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS:                                     return "D3D12_AUTO_BREADCRUMB_OP_DISPATCHRAYS";
    case D3D12_AUTO_BREADCRUMB_OP_INITIALIZEMETACOMMAND:                            return "D3D12_AUTO_BREADCRUMB_OP_INITIALIZEMETACOMMAND";
    case D3D12_AUTO_BREADCRUMB_OP_EXECUTEMETACOMMAND:                               return "D3D12_AUTO_BREADCRUMB_OP_EXECUTEMETACOMMAND";
    case D3D12_AUTO_BREADCRUMB_OP_ESTIMATEMOTION:                                   return "D3D12_AUTO_BREADCRUMB_OP_ESTIMATEMOTION";
    case D3D12_AUTO_BREADCRUMB_OP_RESOLVEMOTIONVECTORHEAP:                          return "D3D12_AUTO_BREADCRUMB_OP_RESOLVEMOTIONVECTORHEAP";
    case D3D12_AUTO_BREADCRUMB_OP_SETPIPELINESTATE1:                                return "D3D12_AUTO_BREADCRUMB_OP_SETPIPELINESTATE1";
    case D3D12_AUTO_BREADCRUMB_OP_INITIALIZEEXTENSIONCOMMAND:                       return "D3D12_AUTO_BREADCRUMB_OP_INITIALIZEEXTENSIONCOMMAND";
    case D3D12_AUTO_BREADCRUMB_OP_EXECUTEEXTENSIONCOMMAND:                          return "D3D12_AUTO_BREADCRUMB_OP_EXECUTEEXTENSIONCOMMAND";
    case D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH:                                     return "D3D12_AUTO_BREADCRUMB_OP_DISPATCHMESH";
    default:                                                                        return "UNKNOWN";
    }
}

static const CHAR* GetDeviceRemovedDumpFilePath()
{
    return *CVarDeviceRemovedDumpFilePath.GetValue();
}

void D3D12DeviceRemovedHandlerRHI(FD3D12Device* Device)
{
    CHECK(Device != nullptr);

    FString Message = "[D3D12] Device Removed";
    D3D12_ERROR("%s", *Message);

    ID3D12Device* DxDevice = Device->GetD3D12Device();

    TComPtr<ID3D12DeviceRemovedExtendedData> Dred;
    if (FAILED(DxDevice->QueryInterface(IID_PPV_ARGS(&Dred))))
    {
        return;
    }

    D3D12_DRED_AUTO_BREADCRUMBS_OUTPUT DredAutoBreadcrumbsOutput;
    D3D12_DRED_PAGE_FAULT_OUTPUT       DredPageFaultOutput;
    if (FAILED(Dred->GetAutoBreadcrumbsOutput(&DredAutoBreadcrumbsOutput)))
    {
        return;
    }

    if (FAILED(Dred->GetPageFaultAllocationOutput(&DredPageFaultOutput)))
    {
        return;
    }

    FFileHandleRef File = FPlatformFile::OpenForWrite(GetDeviceRemovedDumpFilePath());
    if (File)
    {
        Message += '\n';
        File->Write((const uint8*)*Message, Message.Size());
    }

    const D3D12_AUTO_BREADCRUMB_NODE* CurrentNode  = DredAutoBreadcrumbsOutput.pHeadAutoBreadcrumbNode;
    const D3D12_AUTO_BREADCRUMB_NODE* PreviousNode = nullptr;
    while (CurrentNode)
    {
        Message = "BreadCrumbs:";
        if (File)
        {
            Message += '\n';
            File->Write((const uint8*)*Message, Message.Size());
        }

        D3D12_ERROR("%s", *Message);
        for (uint32 i = 0; i < CurrentNode->BreadcrumbCount; i++)
        {
            Message = "    " + FString(ToString(CurrentNode->pCommandHistory[i]));
            D3D12_ERROR("%s", *Message);
            if (File)
            {
                Message += '\n';
                File->Write((const uint8*)*Message, Message.Size());
            }
        }

        PreviousNode = CurrentNode;
        CurrentNode  = CurrentNode->pNext;
    }

    // Signal other systems that the device is removed 
    CoreDelegates::DeviceRemovedDelegate.Broadcast();

    FPlatformApplicationMisc::MessageBox("Error", " [D3D12] Device Removed");
}

FD3D12Adapter::FD3D12Adapter()
    : AdapterIndex(0)
    , bAllowTearing(false)
    , bEnableDebugLayer(false)
    , Factory(nullptr)
#if WIN10_BUILD_17134
    , Factory6(nullptr)
#endif
    , Adapter(nullptr)
{
}

FD3D12Adapter::~FD3D12Adapter()
{
}

bool FD3D12Adapter::Initialize()
{
    if (IConsoleVariable* CVarEnableDebugLayer = FConsoleManager::Get().FindConsoleVariable("RHI.EnableDebugLayer"))
    {
        bEnableDebugLayer = CVarEnableDebugLayer->GetBool();
    }
    
    if (bEnableDebugLayer)
    {
        TComPtr<ID3D12Debug> DebugInterface;
        if (FAILED(D3D12Functions::D3D12GetDebugInterface(IID_PPV_ARGS(&DebugInterface))))
        {
            D3D12_ERROR("[FD3D12Adapter]: FAILED to enable DebugLayer");
            return false;
        }
        else
        {
            DebugInterface->EnableDebugLayer();
        }

        const bool bEnableDRED = CVarEnableDRED.GetValue();
        if (bEnableDRED)
        {
            TComPtr<ID3D12DeviceRemovedExtendedDataSettings> DredSettings;
            if (SUCCEEDED(D3D12Functions::D3D12GetDebugInterface(IID_PPV_ARGS(&DredSettings))))
            {
                DredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
                DredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            }
            else
            {
                D3D12_ERROR("[FD3D12Adapter]: FAILED to enable DRED");
            }
        }

        const bool bEnableGPUValidation = CVarEnableGPUValidation.GetValue();
        if (bEnableGPUValidation)
        {
            TComPtr<ID3D12Debug1> DebugInterface1;
            if (FAILED(DebugInterface.GetAs(&DebugInterface1)))
            {
                D3D12_ERROR("[FD3D12Adapter]: FAILED to enable GPU- Validation");
                return false;
            }
            else
            {
                DebugInterface1->SetEnableGPUBasedValidation(true);
            }
        }

    #if WIN10_BUILD_20348
        {
            TComPtr<ID3D12Debug5> DebugInterface5;
            if (FAILED(DebugInterface.GetAs(&DebugInterface5)))
            {
                D3D12_WARNING("[FD3D12Adapter]: FAILED to enable auto-naming of objects");
            }
            else
            {
                DebugInterface5->SetEnableAutoName(true);
            }
        }
    #endif

        TComPtr<IDXGIInfoQueue> InfoQueue;
        if (SUCCEEDED(D3D12Functions::DXGIGetDebugInterface1(0, IID_PPV_ARGS(&InfoQueue))))
        {
            const bool bBreakOnError = CVarBreakOnError.GetValue();
            InfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, bBreakOnError);
            InfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, bBreakOnError);

            const bool bBreakOnWarning = CVarBreakOnWarning.GetValue();
            InfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, bBreakOnWarning);
        }
        else
        {
            D3D12_ERROR("[FD3D12Adapter]: FAILED to retrieve InfoQueue");
        }

        if (IConsoleVariable* CVarEnablePIX = FConsoleManager::Get().FindConsoleVariable("D3D12RHI.EnablePIX"))
        {
            if (CVarEnablePIX->GetBool())
            {
                TComPtr<IDXGraphicsAnalysis> TempGraphicsAnalysis;
                if (SUCCEEDED(D3D12Functions::DXGIGetDebugInterface1(0, IID_PPV_ARGS(&TempGraphicsAnalysis))))
                {
                    GraphicsAnalysisInterface = TempGraphicsAnalysis;
                }
                else
                {
                    D3D12_INFO("[FD3D12Adapter]: PIX is not connected to the application");
                }
            }
        }
    }

    // Create Factory
    uint32 FactoryFlags = 0;
    if (bEnableDebugLayer)
    {
        FactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }

    if (FAILED(D3D12Functions::CreateDXGIFactory2(FactoryFlags, IID_PPV_ARGS(&Factory))))
    {
        D3D12_ERROR_CRITICAL("[FD3D12Adapter]: FAILED to create factory");
        return false;
    }
    else
    {
        if (FAILED(Factory.GetAs(&Factory5)))
        {
            D3D12_ERROR_CRITICAL("[FD3D12Adapter]: FAILED to retrieve IDXGIFactory5");
            return false;
        }
        else
        {
            BOOL bTearingSupported = FALSE;
            HRESULT hResult = Factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &bTearingSupported, sizeof(bTearingSupported));
            if (SUCCEEDED(hResult))
            {
                bAllowTearing = (bTearingSupported != FALSE);
                if (bAllowTearing)
                {
                    D3D12_INFO("[FD3D12Adapter]: Tearing is supported");
                }
                else
                {
                    D3D12_INFO("[FD3D12Adapter]: Tearing is NOT supported");
                }
            }
        }
    }

    // Choose Adapter
    D3D_FEATURE_LEVEL BestFeatureLevel = D3D_FEATURE_LEVEL_11_0;
    
    const D3D_FEATURE_LEVEL TestFeatureLevels[] =
    {
#if 0 /*&& WIN10_BUILD_20348*/
        D3D_FEATURE_LEVEL_12_2,
#endif
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    TComPtr<IDXGIAdapter1> FinalAdapter;

#if !WIN10_BUILD_17134
    {
        SIZE_T BestVideoMem = 0;
        
        TComPtr<IDXGIAdapter1> TempAdapter;
        for (uint32 Index = 0; DXGI_ERROR_NOT_FOUND != Factory->EnumAdapters1(Index, &TempAdapter); Index++)
        {
            DXGI_ADAPTER_DESC1 Desc;
            if (FAILED(TempAdapter->GetDesc1(&Desc)))
            {
                D3D12_ERROR("[FD3D12Adapter]: FAILED to retrieve DXGI_ADAPTER_DESC1");
                return false;
            }

            if (Desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                continue;
            }

            for (D3D_FEATURE_LEVEL Level : TestFeatureLevels)
            {
                if (Level < BestFeatureLevel)
                {
                    break;
                }

                Result = D3D12Functions::D3D12CreateDevice(TempAdapter.Get(), Level, __uuidof(ID3D12Device), nullptr);
                if (SUCCEEDED(Result))
                {
                    // Here it is probably better to have something else to find the best GPU
                    if (Level >= BestFeatureLevel && Desc.DedicatedVideoMemory > BestVideoMem)
                    {
                        D3D12_INFO("[FD3D12Adapter]: Suitable Direct3D Adapter (%u): %ls", Index, Desc.Description);

                        AdapterIndex     = Index;
                        BestFeatureLevel = Level;
                        BestVideoMem     = Desc.DedicatedVideoMemory;
                        FinalAdapter     = TempAdapter;
                    }
                    
                    break;
                }
            }
        }
    }
#else
    {
        HRESULT Result = Factory.GetAs<IDXGIFactory6>(Factory6.GetAddressOf());
        if (FAILED(Result))
        {
            D3D12_ERROR_CRITICAL("[FD3D12Adapter]: Failed to Query IDXGIFactory6");
            return false;
        }

        const DXGI_GPU_PREFERENCE GPUPreference = CVarPreferDedicatedGPU.GetValue() ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED;

        TComPtr<IDXGIAdapter1> TempAdapter;
        for (uint32 Index = 0; DXGI_ERROR_NOT_FOUND != Factory6->EnumAdapterByGpuPreference(Index, GPUPreference, IID_PPV_ARGS(&TempAdapter)); Index++)
        {
            DXGI_ADAPTER_DESC1 Desc;
            if (FAILED(TempAdapter->GetDesc1(&Desc)))
            {
                D3D12_ERROR("[FD3D12Adapter]: FAILED to retrieve DXGI_ADAPTER_DESC1");
                return false;
            }

            if (Desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                continue;
            }

            for (D3D_FEATURE_LEVEL Level : TestFeatureLevels)
            {
                if (Level < BestFeatureLevel)
                {
                    break;
                }

                Result = D3D12Functions::D3D12CreateDevice(TempAdapter.Get(), Level, __uuidof(ID3D12Device), nullptr);
                if (SUCCEEDED(Result))
                {
                    D3D12_INFO("[FD3D12Adapter]: Suitable Direct3D Adapter (%u): %ls", Index, Desc.Description);

                    // When we loop based on DXGI_GPU_PREFERENCE we get the best one first, so cannot check for equal here
                    if (Level > BestFeatureLevel)
                    {
                        AdapterIndex     = Index;
                        BestFeatureLevel = Level;
                        FinalAdapter     = TempAdapter;
                    }

                    break;
                }
            }
        }
    }
#endif

    if (!FinalAdapter)
    {
        D3D12_ERROR("[FD3D12Adapter]: FAILED to retrieve adapter");
        return false;
    }

    Adapter = FinalAdapter;
    if (FAILED(Adapter->GetDesc1(&AdapterDesc)))
    {
        D3D12_ERROR("[FD3D12Adapter]: FAILED to retrieve DXGI_ADAPTER_DESC1");
        return false;
    }

    if (FAILED(Adapter.GetAs<IDXGIAdapter3>(&Adapter3)))
    {
        D3D12_ERROR("[FD3D12Adapter]: FAILED to retrieve IDXGIAdapter3");
        return false;
    }

    return true;
}

FD3D12Device::FD3D12Device(FD3D12Adapter* InAdapter)
    : GlobalResourceHeap(nullptr)
    , GlobalSamplerHeap(nullptr)
    , ResourceOfflineDescriptorHeap(nullptr)
    , RenderTargetOfflineDescriptorHeap(nullptr)
    , DepthStencilOfflineDescriptorHeap(nullptr)
    , SamplerOfflineDescriptorHeap(nullptr)
    , RootSignatureManager(nullptr)
    , DirectQueue(nullptr)
    , CopyQueue(nullptr)
    , ComputeQueue(nullptr)
    , DirectCommandAllocatorManager(nullptr)
    , CopyCommandAllocatorManager(nullptr)
    , ComputeCommandAllocatorManager(nullptr)
    , PipelineStateManager(nullptr)
    , StagingBufferAllocator(nullptr)
    , DynamicConstantsAllocator(nullptr)
    , BufferAllocator(nullptr)
    , TextureAllocator(nullptr)
    , UploadHeapAllocator(nullptr)
    , ResidencyManager(nullptr)
    , MinFeatureLevel(D3D_FEATURE_LEVEL_12_0)
    , ActiveFeatureLevel(D3D_FEATURE_LEVEL_11_0)
    , Adapter(InAdapter)
    , D3D12Device(nullptr)
#if WIN10_BUILD_14393
    , D3D12Device1(nullptr)
#endif
#if WIN10_BUILD_15063
    , D3D12Device2(nullptr)
#endif
#if WIN10_BUILD_16299
    , D3D12Device3(nullptr)
#endif
#if WIN10_BUILD_17134
    , D3D12Device4(nullptr)
#endif
#if WIN10_BUILD_17763
    , D3D12Device5(nullptr)
#endif
#if WIN10_BUILD_18362
    , D3D12Device6(nullptr)
#endif
#if WIN10_BUILD_19041
    , D3D12Device7(nullptr)
#endif
#if WIN10_BUILD_20348
    , D3D12Device8(nullptr)
#endif
#if WIN11_BUILD_22000
    , D3D12Device9(nullptr)
#endif
    , NodeMask(0)
    , NodeCount(0)
{
    // Create CommandAllocatorManagers
    DirectCommandAllocatorManager  = new FD3D12CommandAllocatorManager(this, ED3D12CommandQueueType::Direct);
    CopyCommandAllocatorManager    = new FD3D12CommandAllocatorManager(this, ED3D12CommandQueueType::Copy);
    ComputeCommandAllocatorManager = new FD3D12CommandAllocatorManager(this, ED3D12CommandQueueType::Compute);

    // Create QueryHeapManagers
    TimingQueryHeapManager    = new FD3D12QueryHeapManager(this, EQueryType::Timestamp, CVarNumTimestampQueriesPerHeap.GetValue());
    OcclusionQueryHeapManager = new FD3D12QueryHeapManager(this, EQueryType::Occlusion, CVarNumOcclusionQueriesPerHeap.GetValue());
}

FD3D12Device::~FD3D12Device()
{
    // Flush PipelineCache
    if (PipelineStateManager)
    {
        PipelineStateManager->SaveCacheData();

        delete PipelineStateManager;
        PipelineStateManager = nullptr;
    }

    // Destroy the default resources
    DefaultDescriptors.DefaultCBV.Reset();
    DefaultDescriptors.DefaultSRV.Reset();
    DefaultDescriptors.DefaultUAV.Reset();
    DefaultDescriptors.DefaultSampler.Reset();
    DefaultDescriptors.DefaultRTV.Reset();

    // Destroy QueryHeapManagers
    SAFE_DELETE(TimingQueryHeapManager);
    SAFE_DELETE(OcclusionQueryHeapManager);

    // Destroy all CommandLists
    SAFE_DELETE(DirectQueue);
    SAFE_DELETE(ComputeQueue);
    SAFE_DELETE(CopyQueue);

    // Destroy all CommandAllocators
    SAFE_DELETE(DirectCommandAllocatorManager);
    SAFE_DELETE(CopyCommandAllocatorManager);
    SAFE_DELETE(ComputeCommandAllocatorManager);

    // Release Heaps
    SAFE_DELETE(GlobalResourceHeap);
    SAFE_DELETE(GlobalSamplerHeap);
    SAFE_DELETE(ResourceOfflineDescriptorHeap);
    SAFE_DELETE(RenderTargetOfflineDescriptorHeap);
    SAFE_DELETE(DepthStencilOfflineDescriptorHeap);
    SAFE_DELETE(SamplerOfflineDescriptorHeap);

    if (StagingBufferAllocator)
    {
        delete StagingBufferAllocator;
        StagingBufferAllocator = nullptr;
    }

    if (DynamicConstantsAllocator)
    {
        delete DynamicConstantsAllocator;
        DynamicConstantsAllocator = nullptr;
    }

    if (BufferAllocator)
    {
        delete BufferAllocator;
        BufferAllocator = nullptr;
    }

    if (TextureAllocator)
    {
        delete TextureAllocator;
        TextureAllocator = nullptr;
    }

    if (UploadHeapAllocator)
    {
        UploadHeapAllocator->Destroy();
        delete UploadHeapAllocator;
        UploadHeapAllocator = nullptr;
    }

    if (ResidencyManager)
    {
        delete ResidencyManager;
        ResidencyManager = nullptr;
    }

    // Release the rest of the managers
    SAFE_DELETE(RootSignatureManager);

    // Report any live objects still hanging around
    if (Adapter->IsDebugLayerEnabled())
    {
        // Disable filter for warnings since this triggers breakpoints when we check for alive object
        TComPtr<ID3D12InfoQueue> InfoQueue;
        if (SUCCEEDED(D3D12Device.GetAs<ID3D12InfoQueue>(&InfoQueue)))
        {
            InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, false);
        }

        TComPtr<ID3D12DebugDevice> DebugDevice;
        if (SUCCEEDED(D3D12Device.GetAs<ID3D12DebugDevice>(&DebugDevice)))
        {
            DebugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
        }
    }

    D3D12Device.Reset();
#if WIN10_BUILD_14393
    D3D12Device1.Reset();
#endif
#if WIN10_BUILD_15063
    D3D12Device2.Reset();
#endif
#if WIN10_BUILD_16299
    D3D12Device3.Reset();
#endif
#if WIN10_BUILD_17134
    D3D12Device4.Reset();
#endif
#if WIN10_BUILD_17763
    D3D12Device5.Reset();
#endif
#if WIN10_BUILD_18362
    D3D12Device6.Reset();
#endif
#if WIN10_BUILD_19041
    D3D12Device7.Reset();
#endif
#if WIN10_BUILD_20348
    D3D12Device8.Reset();
#endif
#if WIN11_BUILD_22000
    D3D12Device9.Reset();
#endif
}

void FD3D12Device::BeginFrame(FD3D12CommandContext* InCommandContext)
{
    if (StagingBufferAllocator)
    {
        StagingBufferAllocator->CleanUp();
    }

    if (DynamicConstantsAllocator)
    {
        DynamicConstantsAllocator->CleanUp();
    }

    if (UploadHeapAllocator)
    {
        UploadHeapAllocator->CleanUp();
    }

    if (BufferAllocator)
    {
        BufferAllocator->CleanUp();
    }

    if (TextureAllocator)
    {
        TextureAllocator->CleanUp();
    }

    if (TextureAllocator)
    {
        const int32 MaxMovesPerFrame = CVarMaxDefragMovesPerFrame.GetValue();
        FD3D12FenceManager& FenceManager = DirectQueue->GetFenceManager();
        TextureAllocator->DefragmentAllocations(InCommandContext, MaxMovesPerFrame, FenceManager);
    }
}

void FD3D12Device::CancelPendingDefragMoves(FD3D12GenericResource* Owner)
{
    if (TextureAllocator)
    {
        TextureAllocator->CancelPendingDefragMoves(Owner);
    }
}

bool FD3D12Device::Initialize()
{
    if (!CreateDevice())
    {
        return false;
    }

    if (!CreateCommandQueues())
    {
        return false;
    }

    // Check current feature-level
    const D3D_FEATURE_LEVEL SupportedFeatureLevels[] =
    {
    #if WIN10_BUILD_20348
        D3D_FEATURE_LEVEL_12_2,
    #endif
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    D3D12_FEATURE_DATA_FEATURE_LEVELS FeatureLevels =
    {
        ARRAY_COUNT(SupportedFeatureLevels), SupportedFeatureLevels, D3D_FEATURE_LEVEL_11_0
    };

    {
        HRESULT Result = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &FeatureLevels, sizeof(FeatureLevels));
        if (SUCCEEDED(Result))
        {
            ActiveFeatureLevel = FeatureLevels.MaxSupportedFeatureLevel;
        }
        else
        {
            ActiveFeatureLevel = MinFeatureLevel;
        }
    }

    // Check for feature support
    QueryDeviceFeatureSupport();

    // Create RootSignatureManager
    RootSignatureManager = new FD3D12RootSignatureManager(this);
    if (!RootSignatureManager->Initialize())
    {
        return false;
    } 

    // Create DescriptorHeaps
    const uint32 NumOnlineResourceDescriptors = Math::Min<uint32>(D3D12_MAX_RESOURCE_ONLINE_DESCRIPTOR_COUNT, GD3D12MaxResourceDescriptorHeapSize);
    const uint32 ResourceDescriptorBlockSize  = Math::Min<uint32>(CVarResourceOnlineDescriptorBlockSize.GetValue(), NumOnlineResourceDescriptors);

    GlobalResourceHeap = new FD3D12OnlineDescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    if (!GlobalResourceHeap->Initialize(NumOnlineResourceDescriptors, ResourceDescriptorBlockSize))
    {
        D3D12_ERROR("Failed to create global resource descriptor heap");
        return false;
    }

    const uint32 NumOnlineSamplerDescriptors = Math::Min<uint32>(D3D12_MAX_SAMPLER_ONLINE_DESCRIPTOR_COUNT, GD3D12MaxSamplerDescriptorHeapSize);
    const uint32 SamplerDescriptorBlockSize  = Math::Min<uint32>(CVarSamplerOnlineDescriptorBlockSize.GetValue(), NumOnlineSamplerDescriptors);

    GlobalSamplerHeap = new FD3D12OnlineDescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    if (!GlobalSamplerHeap->Initialize(NumOnlineSamplerDescriptors, SamplerDescriptorBlockSize))
    {
        D3D12_ERROR("Failed to create global sampler descriptor heap");
        return false;
    }

    // Initialize Offline Descriptor heaps
    ResourceOfflineDescriptorHeap = new FD3D12OfflineDescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    if (!ResourceOfflineDescriptorHeap->Initialize())
    {
        return false;
    }

    RenderTargetOfflineDescriptorHeap = new FD3D12OfflineDescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    if (!RenderTargetOfflineDescriptorHeap->Initialize())
    {
        return false;
    }

    DepthStencilOfflineDescriptorHeap = new FD3D12OfflineDescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    if (!DepthStencilOfflineDescriptorHeap->Initialize())
    {
        return false;
    }

    SamplerOfflineDescriptorHeap = new FD3D12OfflineDescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    if (!SamplerOfflineDescriptorHeap->Initialize())
    {
        return false;
    }

    {
        const uint64 ResidencyBudgetBytes = static_cast<uint64>(Math::Max<int32>(0, CVarResidencyTargetBudget.GetValue())) * 1024ull * 1024ull;
        ResidencyManager = new FD3D12ResidencyManager(this, CVarEnableResidencyTracking.GetValue(), ResidencyBudgetBytes);
    }

    {
        const uint64 UploadHeapPageSizeBytes  = Math::Max<uint64>(1ull, static_cast<uint64>(CVarUploadHeapPageSize.GetValue())) * 1024ull;
        const uint64 UploadHeapSmallThreshold = Math::Max<uint64>(1ull, static_cast<uint64>(CVarUploadHeapSmallAllocationThreshold.GetValue()));
        const uint64 UploadHeapLargeThreshold = Math::Max<uint64>(UploadHeapSmallThreshold, static_cast<uint64>(CVarUploadHeapLargeAllocationThreshold.GetValue()));

        UploadHeapAllocator = new FD3D12UploadHeapAllocator(this, UploadHeapPageSizeBytes, 256, UploadHeapSmallThreshold, UploadHeapLargeThreshold);
        if (!UploadHeapAllocator->Initialize())
        {
            return false;
        }
    }

    {
        const uint64 StagingPageSizeBytes = Math::Max<uint64>(1ull, static_cast<uint64>(CVarStagingBufferPageSize.GetValue())) * 1024ull;
        StagingBufferAllocator = new FD3D12LinearAllocator(this, StagingPageSizeBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
    }

    {
        const uint64 DynamicConstantsPageSize = Math::Max<uint64>(1ull, static_cast<uint64>(CVarDynamicConstantsAllocatorPageSize.GetValue()));
        
        FD3D12DynamicConstantsAllocator* ConstantsAllocator = new FD3D12DynamicConstantsAllocator(this, DynamicConstantsPageSize);
        if (!ConstantsAllocator)
        {
            return false;
        }

        DynamicConstantsAllocator = ConstantsAllocator;
    }

    {
        const uint64 BufferPageSizeBytes    = Math::Max<uint64>(1ull, static_cast<uint64>(CVarBufferAllocatorPageSize.GetValue())) * 1024ull * 1024ull;
        const uint64 BufferMaxSuballocBytes = Math::Max<uint64>(1ull, static_cast<uint64>(CVarBufferAllocatorMaxSuballocationSize.GetValue())) * 1024ull * 1024ull;
        
        BufferAllocator = new FD3D12BufferAllocator(this, BufferPageSizeBytes, 256, BufferMaxSuballocBytes);
        if (!BufferAllocator->Initialize())
        {
            return false;
        }
    }

    {
        const uint64 TextureAllocatorDefaultPageSize    = Math::Max<uint64>(1ull, static_cast<uint64>(CVarTextureAllocatorDefaultPageSize.GetValue()));
        const uint64 TextureAllocatorCommittedThreshold = Math::Max<uint64>(1ull, static_cast<uint64>(CVarTextureAllocatorCommittedThreshold.GetValue()));

        TextureAllocator = new FD3D12TextureAllocator(this, TextureAllocatorDefaultPageSize, TextureAllocatorCommittedThreshold);
        if (!TextureAllocator->Initialize())
        {
            return false;
        }
    }

    // Initialize default descriptors/views
    if (!CreateDefaultResources())
    {
        return false;
    }

    // Create PipelineCache
    PipelineStateManager = new FD3D12PipelineStateManager(this);
    if (!PipelineStateManager->Initialize())
    {
        SAFE_DELETE(PipelineStateManager);
        GD3D12SupportPipelineCache = false;
        return false;
    }
    else
    {
        GD3D12SupportPipelineCache = true;
    }

    return true;
}

bool FD3D12Device::CreateDevice()
{
    // Create Device
    if (FAILED(D3D12Functions::D3D12CreateDevice(Adapter->GetDXGIAdapter(), MinFeatureLevel, IID_PPV_ARGS(&D3D12Device))))
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "FAILED to create device");
        return false;
    }
    else
    {
        const FString Description = Adapter->GetDescription();
        D3D12_INFO("[FD3D12Device]: Created Device for adapter '%s'", *Description);
    }

    // NodeMask
    NodeCount = D3D12Device->GetNodeCount();
    if (NodeCount > 1)
    {
        NodeMask = 1;
    }
    else
    {
        NodeMask = 0;
    }

    // Configure debug device (if active).
    if (Adapter->IsDebugLayerEnabled())
    {
        TComPtr<ID3D12InfoQueue> InfoQueue;
        if (SUCCEEDED(D3D12Device.GetAs(&InfoQueue)))
        {
            const bool bBreakOnError = CVarBreakOnError.GetValue();
            InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, bBreakOnError);
            InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, bBreakOnError);

            const bool bBreakOnWarning = CVarBreakOnWarning.GetValue();
            InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, bBreakOnWarning);

            D3D12_MESSAGE_ID Hide[] =
            {
                D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
                D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
                D3D12_MESSAGE_ID_LOADPIPELINE_NAMENOTFOUND,
            };

            D3D12_INFO_QUEUE_FILTER Filter;
            FMemory::Memzero(&Filter);

            Filter.DenyList.NumIDs = ARRAY_COUNT(Hide);
            Filter.DenyList.pIDList = Hide;
            InfoQueue->AddStorageFilterEntries(&Filter);
        }
    }

#if WIN10_BUILD_14393
    if (FAILED(D3D12Device.GetAs<ID3D12Device1>(&D3D12Device1)))
    {
        D3D12_WARNING("[FD3D12Device]: Failed to retrieve ID3D12Device1");
    }
#endif

#if WIN10_BUILD_15063
    if (FAILED(D3D12Device.GetAs<ID3D12Device2>(&D3D12Device2)))
    {
        D3D12_WARNING("[FD3D12Device]: Failed to retrieve ID3D12Device2");
    }
#endif

#if WIN10_BUILD_16299
    if (FAILED(D3D12Device.GetAs<ID3D12Device3>(&D3D12Device3)))
    {
        D3D12_WARNING("[FD3D12Device]: Failed to retrieve ID3D12Device3");
    }
#endif

#if WIN10_BUILD_17134
    if (FAILED(D3D12Device.GetAs<ID3D12Device4>(&D3D12Device4)))
    {
        D3D12_WARNING("[FD3D12Device]: Failed to retrieve ID3D12Device4");
    }
#endif

#if WIN10_BUILD_17763
    if (FAILED(D3D12Device.GetAs<ID3D12Device5>(&D3D12Device5)))
    {
        D3D12_WARNING("[FD3D12Device]: Failed to retrieve ID3D12Device5");
    }
#endif

#if WIN10_BUILD_18362
    if (FAILED(D3D12Device.GetAs<ID3D12Device6>(&D3D12Device6)))
    {
        D3D12_WARNING("[FD3D12Device]: Failed to retrieve ID3D12Device6");
    }
#endif

#if WIN10_BUILD_19041
    if (FAILED(D3D12Device.GetAs<ID3D12Device7>(&D3D12Device7)))
    {
        D3D12_WARNING("[FD3D12Device]: Failed to retrieve ID3D12Device7");
    }
#endif

#if WIN10_BUILD_20348
    if (FAILED(D3D12Device.GetAs<ID3D12Device8>(&D3D12Device8)))
    {
        D3D12_WARNING("[FD3D12Device]: Failed to retrieve ID3D12Device8");
    }
#endif

#if WIN11_BUILD_22000
    if (FAILED(D3D12Device.GetAs<ID3D12Device9>(&D3D12Device9)))
    {
        D3D12_WARNING("[FD3D12Device]: Failed to retrieve ID3D12Device9");
    }
#endif

    return true;
}

bool FD3D12Device::CreateCommandQueues()
{
    DirectQueue = new FD3D12Queue(this, ED3D12CommandQueueType::Direct);
    if (!DirectQueue->Initialize())
    {
        return false;
    }
 
    CopyQueue = new FD3D12Queue(this, ED3D12CommandQueueType::Copy);
    if (!CopyQueue->Initialize())
    {
        return false;
    }

    ComputeQueue = new FD3D12Queue(this, ED3D12CommandQueueType::Compute);
    if (!ComputeQueue->Initialize())
    {
        return false;
    }

    return true;
}

bool FD3D12Device::CreateDefaultResources()
{
    D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc = {};
    CBVDesc.BufferLocation = 0;
    CBVDesc.SizeInBytes    = 0;

    DefaultDescriptors.DefaultCBV = new FD3D12ConstantBufferView(this, GetResourceOfflineDescriptorHeap());
    if (!DefaultDescriptors.DefaultCBV->AllocateHandle())
    {
        return false;
    }

    if (!DefaultDescriptors.DefaultCBV->CreateView(nullptr, CBVDesc))
    {
        return false;
    }

    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
    UAVDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
    UAVDesc.Format               = DXGI_FORMAT_R8G8B8A8_UNORM;
    UAVDesc.Texture2D.MipSlice   = 0;
    UAVDesc.Texture2D.PlaneSlice = 0;

    DefaultDescriptors.DefaultUAV = new FD3D12UnorderedAccessView(this, GetResourceOfflineDescriptorHeap(), nullptr);
    if (!DefaultDescriptors.DefaultUAV->AllocateHandle())
    {
        return false;
    }

    if (!DefaultDescriptors.DefaultUAV->CreateView(nullptr, nullptr, UAVDesc))
    {
        return false;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
    SRVDesc.Format                        = DXGI_FORMAT_R8G8B8A8_UNORM;
    SRVDesc.Shader4ComponentMapping       = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    SRVDesc.Texture2D.MipLevels           = 1;
    SRVDesc.Texture2D.MostDetailedMip     = 0;
    SRVDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    SRVDesc.Texture2D.PlaneSlice          = 0;

    DefaultDescriptors.DefaultSRV = new FD3D12ShaderResourceView(this, GetResourceOfflineDescriptorHeap(), nullptr);
    if (!DefaultDescriptors.DefaultSRV->AllocateHandle())
    {
        return false;
    }

    if (!DefaultDescriptors.DefaultSRV->CreateView(nullptr, SRVDesc))
    {
        return false;
    }

    D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = {};
    RTVDesc.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
    RTVDesc.Format               = DXGI_FORMAT_R8G8B8A8_UNORM;
    RTVDesc.Texture2D.MipSlice   = 0;
    RTVDesc.Texture2D.PlaneSlice = 0;

    DefaultDescriptors.DefaultRTV = new FD3D12RenderTargetView(this, GetRenderTargetOfflineDescriptorHeap());
    if (!DefaultDescriptors.DefaultRTV->AllocateHandle())
    {
        return false;
    }

    if (!DefaultDescriptors.DefaultRTV->CreateView(nullptr, RTVDesc))
    {
        return false;
    }

    D3D12_SAMPLER_DESC SamplerDesc = {};
    SamplerDesc.AddressU       = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    SamplerDesc.AddressV       = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    SamplerDesc.AddressW       = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    SamplerDesc.BorderColor[0] = 1.0f;
    SamplerDesc.BorderColor[1] = 1.0f;
    SamplerDesc.BorderColor[2] = 1.0f;
    SamplerDesc.BorderColor[3] = 1.0f;
    SamplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    SamplerDesc.Filter         = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    SamplerDesc.MaxAnisotropy  = 1;
    SamplerDesc.MaxLOD         = TNumericLimits<float>::Max();
    SamplerDesc.MinLOD         = TNumericLimits<float>::Lowest();
    SamplerDesc.MipLODBias     = 0.0f;

    DefaultDescriptors.DefaultSampler = new FD3D12SamplerState(this, GetSamplerOfflineDescriptorHeap(), FRHISamplerStateInfo());
    if (!DefaultDescriptors.DefaultSampler->CreateSampler(SamplerDesc))
    {
        return false;
    }

    return true;
}

bool FD3D12Device::CreateCommittedResource(const D3D12_RESOURCE_DESC& Desc, D3D12_HEAP_TYPE HeapType, D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* ClearValue, FD3D12ResourceRef& OutResource)
{
    D3D12_HEAP_PROPERTIES HeapProperties = {};
    HeapProperties.Type                 = HeapType;
    HeapProperties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProperties.VisibleNodeMask      = NodeMask ? NodeMask : 1;
    HeapProperties.CreationNodeMask     = NodeMask ? NodeMask : 1;

    TComPtr<ID3D12Resource> NewResource;
    const HRESULT Result = GetD3D12Device()->CreateCommittedResource(
        &HeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &Desc,
        InitialState,
        ClearValue,
        IID_PPV_ARGS(&NewResource));

    if (FAILED(Result))
    {
        D3D12_ERROR("[FD3D12Device] CreateCommittedResource failed");
        return false;
    }

    OutResource = new FD3D12Resource(this, NewResource.ReleaseOwnership(), HeapType, InitialState);
    if (ClearValue)
    {
        OutResource->SetClearValue(*ClearValue);
    }

    if (HeapType == D3D12_HEAP_TYPE_DEFAULT)
    {
        OutResource->StartResidencyTracking();
    }


    return true;
}

bool FD3D12Device::CreatePlacedResource(FD3D12Heap* Heap, uint64 Offset, const D3D12_RESOURCE_DESC& Desc, D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* ClearValue, FD3D12ResourceRef& OutResource)
{
    if (!Heap)
    {
        return false;
    }

    TComPtr<ID3D12Resource> NewResource;
    const HRESULT Result = GetD3D12Device()->CreatePlacedResource(
        Heap->GetD3D12Heap(),
        Offset,
        &Desc,
        InitialState,
        ClearValue,
        IID_PPV_ARGS(&NewResource));

    if (FAILED(Result))
    {
        D3D12_ERROR("[FD3D12Device] CreatePlacedResource failed");
        return false;
    }

    OutResource = new FD3D12Resource(this, NewResource.ReleaseOwnership(), Heap->GetHeapType(), InitialState, Heap);
    if (ClearValue)
    {
        OutResource->SetClearValue(*ClearValue);
    }

    return true;
}

bool FD3D12Device::CreateHeap(const D3D12_HEAP_DESC& Desc, FD3D12HeapRef& OutHeap)
{
    TComPtr<ID3D12Heap> NewHeap;
    const HRESULT Result = GetD3D12Device()->CreateHeap(&Desc, IID_PPV_ARGS(&NewHeap));
    if (FAILED(Result))
    {
        D3D12_ERROR("[FD3D12Device] CreateHeap failed");
        return false;
    }

    OutHeap = new FD3D12Heap(this, NewHeap.ReleaseOwnership());
    return true;
}

int32 FD3D12Device::QueryMultisampleQuality(DXGI_FORMAT Format, uint32 SampleCount)
{
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS Data = {};
    Data.Flags       = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    Data.Format      = Format;
    Data.SampleCount = SampleCount;

    HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &Data, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
    if (FAILED(hr))
    {
        D3D12_ERROR("[FD3D12Device] CheckFeatureSupport failed");
        return 0;
    }

    return static_cast<uint32>(Data.NumQualityLevels - 1);
}

ID3D12CommandQueue* FD3D12Device::GetD3D12CommandQueue(ED3D12CommandQueueType QueueType)
{
    FD3D12Queue* Queue = GetQueue(QueueType);
    return Queue ? Queue->GetD3D12CommandQueue() : nullptr;
}

FD3D12Queue* FD3D12Device::GetQueue(ED3D12CommandQueueType QueueType)
{
    if (QueueType == ED3D12CommandQueueType::Direct)
    {
        CHECK(DirectQueue->GetQueueType() == ED3D12CommandQueueType::Direct);
        return DirectQueue;
    }
    else if (QueueType == ED3D12CommandQueueType::Copy)
    {
        CHECK(CopyQueue->GetQueueType() == ED3D12CommandQueueType::Copy);
        return CopyQueue;
    }
    else if (QueueType == ED3D12CommandQueueType::Compute)
    {
        CHECK(ComputeQueue->GetQueueType() == ED3D12CommandQueueType::Compute);
        return ComputeQueue;
    }
    else
    {
        return nullptr;
    }
}

FD3D12CommandAllocatorManager* FD3D12Device::GetCommandAllocatorManager(ED3D12CommandQueueType QueueType)
{
    if (QueueType == ED3D12CommandQueueType::Direct)
    {
        CHECK(DirectCommandAllocatorManager->GetQueueType() == ED3D12CommandQueueType::Direct);
        return DirectCommandAllocatorManager;
    }
    else if (QueueType == ED3D12CommandQueueType::Copy)
    {
        CHECK(CopyCommandAllocatorManager->GetQueueType() == ED3D12CommandQueueType::Copy);
        return CopyCommandAllocatorManager;
    }
    else if (QueueType == ED3D12CommandQueueType::Compute)
    {
        CHECK(ComputeCommandAllocatorManager->GetQueueType() == ED3D12CommandQueueType::Compute);
        return ComputeCommandAllocatorManager;
    }
    else
    {
        return nullptr;
    }
}

FD3D12QueryHeapManager* FD3D12Device::GetQueryHeapManager(EQueryType QueryType)
{
    if (QueryType == EQueryType::Timestamp)
    {
        return TimingQueryHeapManager;
    }
    else if (QueryType == EQueryType::Occlusion)
    {
        return OcclusionQueryHeapManager;
    }
    else
    {
        return nullptr;
    }
}

void FD3D12Device::QueryDeviceFeatureSupport()
{
    // -------------------------------------------------------------------------------------------
    // Baseline defaults
    // -------------------------------------------------------------------------------------------

    GD3D12SupportTightAlignment            = false;
    GD3D12SupportGPUUploadHeaps            = false;
    GD3D12SupportsBindless                 = false;
    GD3D12SupportEnhancedBarriers          = false;

    GD3D12ResourceBindingTier              = D3D12_RESOURCE_BINDING_TIER_1;
    GD3D12ResourceHeapTier                 = D3D12_RESOURCE_HEAP_TIER_1;
    GD3D12RayTracingTier                   = D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
    GD3D12VariableRateShadingTier          = D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED;
    GD3D12MeshShaderTier                   = D3D12_MESH_SHADER_TIER_NOT_SUPPORTED;
    GD3D12SamplerFeedbackTier              = D3D12_SAMPLER_FEEDBACK_TIER_NOT_SUPPORTED;
    GD3D12ViewInstancingTier               = D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED;
    GD3D12ConservativeRasterizationTier    = D3D12_CONSERVATIVE_RASTERIZATION_TIER_NOT_SUPPORTED;
    GD3D12ProgrammableSamplePositionsTier  = D3D12_PROGRAMMABLE_SAMPLE_POSITIONS_TIER_NOT_SUPPORTED;
    GD3D12WorkGraphsTier                   = D3D12_WORK_GRAPHS_TIER_NOT_SUPPORTED;
    GD3D12ExecuteIndirectTier              = D3D12_EXECUTE_INDIRECT_TIER_1_0;
    GD3D12TiledResourcesTier               = D3D12_TILED_RESOURCES_TIER_NOT_SUPPORTED;
    GD3D12RootSignatureVersion             = D3D_ROOT_SIGNATURE_VERSION_1_0;
    GD3D12HighestShaderModel               = D3D_SHADER_MODEL_6_0;

    GD3D12RasterizerOrderViewsSupported    = false;
    GD3D12TypedUAVLoadAdditionalFormats    = false;
    GD3D12DepthBoundsTestSupported         = false;
    GD3D12IsArchitectureUMA                = false;
    GD3D12IsArchitectureCacheCoherentUMA   = false;

    GD3D12MaxSamplerDescriptorHeapSize     = 0;
    GD3D12MaxResourceDescriptorHeapSize    = 0;

    GD3D12VirtualAddressBitsPerProcess     = 0;
    GD3D12VirtualAddressBitsPerResource    = 0;
    GD3D12WriteBufferImmediateSupportFlags = D3D12_COMMAND_LIST_SUPPORT_FLAG_NONE;

    // -------------------------------------------------------------------------------------------
    // Heap Tier, Binding Tier, Conservative Rasterization, Typed UAV loads, ROVs, Tiled Resources
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS Features = {};
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &Features, sizeof(Features));
        if (SUCCEEDED(hr))
        {
            GD3D12ResourceBindingTier           = Features.ResourceBindingTier;
            GD3D12ResourceHeapTier              = Features.ResourceHeapTier;
            GD3D12ConservativeRasterizationTier = Features.ConservativeRasterizationTier;
            GD3D12RasterizerOrderViewsSupported = !!Features.ROVsSupported;
            GD3D12TypedUAVLoadAdditionalFormats = !!Features.TypedUAVLoadAdditionalFormats;
            GD3D12TiledResourcesTier            = Features.TiledResourcesTier;

            D3D12_INFO("[FD3D12Device] ResourceBinding Tier: %d", GD3D12ResourceBindingTier);
            D3D12_INFO("[FD3D12Device] ResourceHeap Tier: %d", GD3D12ResourceHeapTier);
            D3D12_INFO("[FD3D12Device] ConservativeRasterization Tier: %d", GD3D12ConservativeRasterizationTier);
            D3D12_INFO("[FD3D12Device] TypedUAVLoadAdditionalFormats: %s", GD3D12TypedUAVLoadAdditionalFormats ? "true" : "false");
            D3D12_INFO("[FD3D12Device] ROVsSupported: %s", GD3D12RasterizerOrderViewsSupported ? "true" : "false");
            D3D12_INFO("[FD3D12Device] TiledResources Tier: %d", GD3D12TiledResourcesTier);
        }
        else
        {
            D3D12_WARNING("[FD3D12Device] D3D12_FEATURE_DATA_D3D12_OPTIONS query failed (hr=0x%08X)", hr);
        }
    }

    // -------------------------------------------------------------------------------------------
    // Depth bounds test & programmable sample positions
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS2 Features2 = {};
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS2, &Features2, sizeof(Features2));
        if (SUCCEEDED(hr))
        {
            GD3D12DepthBoundsTestSupported        = !!Features2.DepthBoundsTestSupported;
            GD3D12ProgrammableSamplePositionsTier = Features2.ProgrammableSamplePositionsTier;

            D3D12_INFO("[FD3D12Device] DepthBoundsTestSupported: %s", GD3D12DepthBoundsTestSupported ? "true" : "false");
            D3D12_INFO("[FD3D12Device] ProgrammableSamplePositions Tier: %d", GD3D12ProgrammableSamplePositionsTier);
        }
        else
        {
            D3D12_WARNING("[FD3D12Device] D3D12_FEATURE_DATA_D3D12_OPTIONS2 query failed (hr=0x%08X). Using defaults.", hr);
        }
    }

    // -------------------------------------------------------------------------------------------
    // View Instancing & Write buffer immediate flags
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS3 Features3 = {};
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &Features3, sizeof(Features3));
        if (SUCCEEDED(hr))
        {
            GD3D12ViewInstancingTier               = Features3.ViewInstancingTier;
            GD3D12WriteBufferImmediateSupportFlags = Features3.WriteBufferImmediateSupportFlags;

            D3D12_INFO("[FD3D12Device] ViewInstancing Tier: %d", GD3D12ViewInstancingTier);
            D3D12_INFO("[FD3D12Device] WriteBufferImmediate SupportFlags: 0x%X", static_cast<uint32>(GD3D12WriteBufferImmediateSupportFlags));
        }
        else
        {
            D3D12_WARNING("[FD3D12Device] D3D12_OPTIONS3 query failed (hr=0x%08X)", hr);
        }
    }

    // -------------------------------------------------------------------------------------------
    // Ray Tracing (DXR)
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 Features5 = {};
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &Features5, sizeof(Features5));
        if (SUCCEEDED(hr))
        {
            GD3D12RayTracingTier = Features5.RaytracingTier;
            D3D12_INFO("[FD3D12Device] RayTracing Tier: %d", GD3D12RayTracingTier);
        }
        else
        {
            D3D12_WARNING("[FD3D12Device] D3D12_OPTIONS5 query failed (hr=0x%08X)", hr);
        }
    }

    // -------------------------------------------------------------------------------------------
    // Variable Rate Shading
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS6 Features6 = {};
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &Features6, sizeof(Features6));
        if (SUCCEEDED(hr))
        {
            GD3D12VariableRateShadingTier = Features6.VariableShadingRateTier;
            D3D12_INFO("[FD3D12Device] VariableRateShading Tier: %d", GD3D12VariableRateShadingTier);
        }
        else
        {
            D3D12_WARNING("[FD3D12Device] D3D12_FEATURE_DATA_D3D12_OPTIONS6 query failed (hr=0x%08X)", hr);
        }
    }

    // -------------------------------------------------------------------------------------------
    // Mesh Shaders & Sampler Feedback
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS7 Features7 = {};
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &Features7, sizeof(Features7));
        if (SUCCEEDED(hr))
        {
            GD3D12MeshShaderTier      = Features7.MeshShaderTier;
            GD3D12SamplerFeedbackTier = Features7.SamplerFeedbackTier;

            D3D12_INFO("[FD3D12Device] MeshShader Tier: %d", GD3D12MeshShaderTier);
            D3D12_INFO("[FD3D12Device] SamplerFeedback Tier: %d", GD3D12SamplerFeedbackTier);
        }
        else
        {
            D3D12_WARNING("[FD3D12Device] D3D12_FEATURE_DATA_D3D12_OPTIONS7 query failed (hr=0x%08X)", hr);
        }
    }

    // -------------------------------------------------------------------------------------------
    // Enhanced Barriers
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS12 Features12 = {};
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS12, &Features12, sizeof(Features12));
        if (SUCCEEDED(hr))
        {
            GD3D12SupportEnhancedBarriers = !!Features12.EnhancedBarriersSupported;
            D3D12_INFO("[FD3D12Device] Enhanced Barriers Supported: %s", GD3D12SupportEnhancedBarriers ? "true" : "false");
        }
        else
        {
            D3D12_WARNING("[FD3D12Device] D3D12_FEATURE_DATA_D3D12_OPTIONS12 query failed (hr=0x%08X)", hr);
        }
    }

    // -------------------------------------------------------------------------------------------
    // GPU Upload Heaps
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS16 Features16 = {};
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS16, &Features16, sizeof(Features16));
        if (SUCCEEDED(hr))
        {
            GD3D12SupportGPUUploadHeaps = !!Features16.GPUUploadHeapSupported;
            D3D12_INFO("[FD3D12Device] GPU Upload Heaps Supported: %s", GD3D12SupportGPUUploadHeaps ? "true" : "false");
        }
        else
        {
            D3D12_WARNING("[FD3D12Device] D3D12_FEATURE_DATA_D3D12_OPTIONS16 query failed (hr=0x%08X)", hr);
        }
    }

    // -------------------------------------------------------------------------------------------
    // Descriptor heap sizes
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS19 Features19 = {};
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS19, &Features19, sizeof(Features19));
        if (SUCCEEDED(hr))
        {
            GD3D12MaxSamplerDescriptorHeapSize  = Features19.MaxSamplerDescriptorHeapSizeWithStaticSamplers;
            GD3D12MaxResourceDescriptorHeapSize = Features19.MaxViewDescriptorHeapSize;

            D3D12_INFO("[FD3D12Device] Max Sampler Descriptor-Heap Size: %u", GD3D12MaxSamplerDescriptorHeapSize);
            D3D12_INFO("[FD3D12Device] Max Resource Descriptor-Heap Size: %u", GD3D12MaxResourceDescriptorHeapSize);
        }
        else
        {
            D3D12_WARNING("[FD3D12Device] D3D12_FEATURE_DATA_D3D12_OPTIONS19 query failed (hr=0x%08X)", hr);
        }
    }

    // -------------------------------------------------------------------------------------------
    // Tight Alignment Support
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_TIGHT_ALIGNMENT TightAlignment = {};
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_TIGHT_ALIGNMENT, &TightAlignment, sizeof(TightAlignment));
        if (SUCCEEDED(hr))
        {
            const bool bDeviceSupports = (TightAlignment.SupportTier >= D3D12_TIGHT_ALIGNMENT_TIER_1);
            GD3D12SupportTightAlignment = bDeviceSupports && CVarEnableTightAlignment.GetValue();
            if (bDeviceSupports && !GD3D12SupportTightAlignment)
            {
                D3D12_INFO("[FD3D12Device] Tight Alignment Support: disabled by CVar (device supports it)");
            }
            else
            {
                D3D12_INFO("[FD3D12Device] Tight Alignment Support: %s", GD3D12SupportTightAlignment ? "true" : "false");
            }
        }
        else
        {
            D3D12_WARNING("[FD3D12Device] D3D12_FEATURE_DATA_TIGHT_ALIGNMENT query failed (hr=0x%08X)", hr);
        }
    }

    // -------------------------------------------------------------------------------------------
    // Root Signature Version
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_ROOT_SIGNATURE RootSignature = { D3D_ROOT_SIGNATURE_VERSION_1_1 };
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &RootSignature, sizeof(RootSignature));
        if (SUCCEEDED(hr))
        {
            GD3D12RootSignatureVersion = RootSignature.HighestVersion;
            D3D12_INFO("[FD3D12Device] RootSignature Version Supported: %s", (GD3D12RootSignatureVersion == D3D_ROOT_SIGNATURE_VERSION_1_1) ? "1.1" : "1.0");
        }
        else
        {
            GD3D12RootSignatureVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
            D3D12_WARNING("[FD3D12Device] D3D12_FEATURE_DATA_ROOT_SIGNATURE query failed (hr=0x%08X)", hr);
        }
    }

    // -------------------------------------------------------------------------------------------
    // Highest Shader Model
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_SHADER_MODEL ShaderModel = {};
        ShaderModel.HighestShaderModel = D3D_HIGHEST_SHADER_MODEL;
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &ShaderModel, sizeof(ShaderModel));
        if (SUCCEEDED(hr))
        {
            GD3D12HighestShaderModel = ShaderModel.HighestShaderModel;
            D3D12_INFO("[FD3D12Device] Highest Shader Model Supported: 0x%X", GD3D12HighestShaderModel);
        }
        else
        {
            GD3D12HighestShaderModel = D3D_SHADER_MODEL_6_0;
            D3D12_WARNING("[FD3D12Device] D3D12_FEATURE_DATA_SHADER_MODEL query failed (hr=0x%08X). Using SM 6.0 baseline.", hr);
        }

        GD3D12SupportsBindless = (GD3D12HighestShaderModel >= D3D_SHADER_MODEL_6_6) && (GD3D12ResourceBindingTier >= D3D12_RESOURCE_BINDING_TIER_3);
        D3D12_INFO("[FD3D12Device] Supports Bindless: %s", GD3D12SupportsBindless ? "true" : "false");
    }

    // -------------------------------------------------------------------------------------------
    // VirtualAddress
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT VirtualAddress = {};
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_GPU_VIRTUAL_ADDRESS_SUPPORT, &VirtualAddress, sizeof(VirtualAddress));
        if (SUCCEEDED(hr))
        {
            GD3D12VirtualAddressBitsPerProcess  = VirtualAddress.MaxGPUVirtualAddressBitsPerProcess;
            GD3D12VirtualAddressBitsPerResource = VirtualAddress.MaxGPUVirtualAddressBitsPerResource;

            D3D12_INFO("[FD3D12Device] VirtualAddress Bits/Process Supported: %u", GD3D12VirtualAddressBitsPerProcess);
            D3D12_INFO("[FD3D12Device] VirtualAddress Bits/Resource Supported: %u", GD3D12VirtualAddressBitsPerResource);
        }
        else
        {
            D3D12_WARNING("[FD3D12Device] D3D12_FEATURE_DATA_GPU_VIRTUAL_ADDRESS_SUPPORT query failed (hr=0x%08X)", hr);
        }
    }

    // -------------------------------------------------------------------------------------------
    // Architecture (UMA / CacheCoherentUMA)
    // -------------------------------------------------------------------------------------------

    {
        D3D12_FEATURE_DATA_ARCHITECTURE1 Architecture = {};
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE1, &Architecture, sizeof(Architecture));
        if (SUCCEEDED(hr))
        {
            GD3D12IsArchitectureUMA              = !!Architecture.UMA;
            GD3D12IsArchitectureCacheCoherentUMA = !!Architecture.CacheCoherentUMA;

            D3D12_INFO("[FD3D12Device] UMA Architecture: %s", GD3D12IsArchitectureUMA ? "true" : "false");
            D3D12_INFO("[FD3D12Device] CacheCoherentUMA Architecture: %s", GD3D12IsArchitectureCacheCoherentUMA ? "true" : "false");
        }
        else
        {
            D3D12_WARNING("[FD3D12Device] D3D12_FEATURE_DATA_ARCHITECTURE1 query failed (hr=0x%08X)", hr);
        }
    }

    // -------------------------------------------------------------------------------------------
    // WorkGraphs & ExecuteIndirect
    // -------------------------------------------------------------------------------------------
    
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS21 Features21 = {};
        HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS21, &Features21, sizeof(Features21));
        if (SUCCEEDED(hr))
        {
            GD3D12WorkGraphsTier      = Features21.WorkGraphsTier;
            GD3D12ExecuteIndirectTier = Features21.ExecuteIndirectTier;

            D3D12_INFO("[FD3D12Device] WorkGraphs Tier: %d", GD3D12WorkGraphsTier);
            D3D12_INFO("[FD3D12Device] ExecuteIndirect Tier: %d", GD3D12ExecuteIndirectTier);
        }
        else
        {
            D3D12_WARNING("[FD3D12Device] D3D12_FEATURE_DATA_D3D12_OPTIONS19 query failed (hr=0x%08X)", hr);
        }
    }

    return;
}
