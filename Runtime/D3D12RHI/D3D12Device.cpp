#include "D3D12Device.h"
#include "D3D12RHIShaderCompiler.h"
#include "D3D12Descriptors.h"
#include "D3D12RootSignature.h"
#include "D3D12PipelineState.h"
#include "DynamicD3D12.h"
#include "Core/Windows/Windows.h"
#include "Core/Platform/PlatformLibrary.h"
#include "Core/Platform/PlatformFile.h"
#include "Core/Misc/CoreDelegates.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Containers/String.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"

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

////////////////////////////////////////////////////
// Global variables that describe different features

D3D12RHI_API bool GD3D12ForceBinding = false;
D3D12RHI_API bool GD3D12SupportPipelineCache = false;

D3D12RHI_API D3D12_RESOURCE_BINDING_TIER GD3D12ResourceBindingTier = D3D12_RESOURCE_BINDING_TIER_1;
D3D12RHI_API D3D12_RAYTRACING_TIER GD3D12RayTracingTier = D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
D3D12RHI_API D3D12_VARIABLE_SHADING_RATE_TIER GD3D12VariableRateShadingTier = D3D12_VARIABLE_SHADING_RATE_TIER_NOT_SUPPORTED;
D3D12RHI_API D3D12_MESH_SHADER_TIER GD3D12MeshShaderTier = D3D12_MESH_SHADER_TIER_NOT_SUPPORTED;
D3D12RHI_API D3D12_SAMPLER_FEEDBACK_TIER GD3D12SamplerFeedbackTier = D3D12_SAMPLER_FEEDBACK_TIER_NOT_SUPPORTED;
D3D12RHI_API D3D12_VIEW_INSTANCING_TIER GD3D12ViewInstancingTier = D3D12_VIEW_INSTANCING_TIER_NOT_SUPPORTED;

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

static const CHAR* GDeviceRemovedDumpFile = "D3D12DeviceRemovedDump.txt";

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

    FFileHandleRef File = FPlatformFile::OpenForWrite(GDeviceRemovedDumpFile);
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
    , Factory(nullptr)
#if WIN10_BUILD_17134
    , Factory6(nullptr)
#endif
    , Adapter(nullptr)
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
        if (FAILED(FDynamicD3D12::D3D12GetDebugInterface(IID_PPV_ARGS(&DebugInterface))))
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
            if (SUCCEEDED(FDynamicD3D12::D3D12GetDebugInterface(IID_PPV_ARGS(&DredSettings))))
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
        if (SUCCEEDED(FDynamicD3D12::DXGIGetDebugInterface1(0, IID_PPV_ARGS(&InfoQueue))))
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
                if (SUCCEEDED(FDynamicD3D12::DXGIGetDebugInterface1(0, IID_PPV_ARGS(&TempGraphicsAnalysis))))
                {
                    DXGraphicsAnalysis = TempGraphicsAnalysis;
                }
                else
                {
                    D3D12_INFO("[FD3D12Adapter]: PIX is not connected to the application");
                }
            }
        }
    }

    // Create Factory
    if (FAILED(FDynamicD3D12::CreateDXGIFactory2(0, IID_PPV_ARGS(&Factory))))
    {
        D3D12_ERROR("[FD3D12Adapter]: FAILED to create factory");
        return false;
    }
    else
    {
        if (FAILED(Factory.GetAs(&Factory5)))
        {
            D3D12_ERROR("[FD3D12Adapter]: FAILED to retrieve IDXGIFactory5");
            return false;
        }
        else
        {
            HRESULT hResult = Factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &bAllowTearing, sizeof(bAllowTearing));
            if (SUCCEEDED(hResult))
            {
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

                Result = FDynamicD3D12::D3D12CreateDevice(TempAdapter.Get(), Level, __uuidof(ID3D12Device), nullptr);
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
            D3D12_ERROR("[FD3D12Adapter]: Failed to Query IDXGIFactory6");
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

                Result = FDynamicD3D12::D3D12CreateDevice(TempAdapter.Get(), Level, __uuidof(ID3D12Device), nullptr);
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
    , UploadAllocator(nullptr)
    , PipelineStateManager(nullptr)
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
    // UploadAllocator
    UploadAllocator = new FD3D12UploadHeapAllocator(this);

    // Create CommandAllocatorManagers
    DirectCommandAllocatorManager  = new FD3D12CommandAllocatorManager(this, ED3D12CommandQueueType::Direct);
    CopyCommandAllocatorManager    = new FD3D12CommandAllocatorManager(this, ED3D12CommandQueueType::Copy);
    ComputeCommandAllocatorManager = new FD3D12CommandAllocatorManager(this, ED3D12CommandQueueType::Compute);

    // Create QueryHeapManagers
    TimingQueryHeapManager    = new FD3D12QueryHeapManager(this, EQueryType::Timestamp);
    OcclusionQueryHeapManager = new FD3D12QueryHeapManager(this, EQueryType::Occlusion);
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

    // Release the rest of the managers
    SAFE_DELETE(UploadAllocator);
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

bool FD3D12Device::Initialize()
{
    if (!CreateDevice())
    {
        return false;
    }

    if (!CreateCommandManagers())
    {
        return false;
    }

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

    // Create RootSignatureManager
    RootSignatureManager = new FD3D12RootSignatureManager(this);
    if (!RootSignatureManager->Initialize())
    {
        return false;
    } 

    // Check for Resource-Binding Tier
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS Features;
        FMemory::Memzero(&Features);

        HRESULT Result = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &Features, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));
        if (SUCCEEDED(Result))
        {
            GD3D12ResourceBindingTier = Features.ResourceBindingTier;
            D3D12_INFO("[FD3D12Device] Using ResourceBinding Tier %d", GD3D12ResourceBindingTier);
        }
    }

    // Check for Ray-Tracing support
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 Features5;
        FMemory::Memzero(&Features5);

        HRESULT Result = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &Features5, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));
        if (SUCCEEDED(Result))
        {
            GD3D12RayTracingTier = Features5.RaytracingTier;
        }
    }

    // Checking for Variable Shading Rate support
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS6 Features6;
        FMemory::Memzero(&Features6);

        HRESULT Result = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &Features6, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS6));
        if (SUCCEEDED(Result))
        {
            GD3D12VariableRateShadingTier = Features6.VariableShadingRateTier;
        }
    }

    // Check for Mesh-Shaders, and SamplerFeedback support
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS7 Features7;
        FMemory::Memzero(&Features7);

        HRESULT Result = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &Features7, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS7));
        if (SUCCEEDED(Result))
        {
            GD3D12MeshShaderTier      = Features7.MeshShaderTier;
            GD3D12SamplerFeedbackTier = Features7.SamplerFeedbackTier;
        }
    }

    // Create DescriptorHeaps
    const uint32 ResourceDescriptorBlockSize = CVarResourceOnlineDescriptorBlockSize.GetValue();
    GlobalResourceHeap = new FD3D12OnlineDescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    if (!GlobalResourceHeap->Initialize(D3D12_MAX_RESOURCE_ONLINE_DESCRIPTOR_COUNT, ResourceDescriptorBlockSize))
    {
        D3D12_ERROR("Failed to create global resource descriptor heap");
        return false;
    }

    const uint32 SamplerDescriptorBlockSize = CVarSamplerOnlineDescriptorBlockSize.GetValue();
    GlobalSamplerHeap = new FD3D12OnlineDescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    if (!GlobalSamplerHeap->Initialize(D3D12_MAX_SAMPLER_ONLINE_DESCRIPTOR_COUNT, SamplerDescriptorBlockSize))
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
    if (FAILED(FDynamicD3D12::D3D12CreateDevice(Adapter->GetDXGIAdapter(), MinFeatureLevel, IID_PPV_ARGS(&D3D12Device))))
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

bool FD3D12Device::CreateCommandManagers()
{
    DirectQueue = new FD3D12Queue(this, ED3D12CommandQueueType::Direct);
    if (!DirectQueue->Initialize())
        return false;
 
    CopyQueue = new FD3D12Queue(this, ED3D12CommandQueueType::Copy);
    if (!CopyQueue->Initialize())
        return false;

    ComputeQueue = new FD3D12Queue(this, ED3D12CommandQueueType::Compute);
    if (!ComputeQueue->Initialize())
        return false;

    return true;
}

bool FD3D12Device::CreateDefaultResources()
{
    D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
    FMemory::Memzero(&CBVDesc);

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

    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc;
    FMemory::Memzero(&UAVDesc);

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

    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    FMemory::Memzero(&SRVDesc);

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

    D3D12_RENDER_TARGET_VIEW_DESC RTVDesc;
    FMemory::Memzero(&RTVDesc);

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

    D3D12_SAMPLER_DESC SamplerDesc;
    FMemory::Memzero(&SamplerDesc);

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

ID3D12CommandQueue* FD3D12Device::GetD3D12CommandQueue(ED3D12CommandQueueType QueueType)
{
    FD3D12Queue* Queue = GetQueue(QueueType);
    return Queue ? Queue->GetD3D12CommandQueue() : nullptr;
}

int32 FD3D12Device::QueryMultisampleQuality(DXGI_FORMAT Format, uint32 SampleCount)
{
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS Data;
    FMemory::Memzero(&Data);

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

FD3D12QueryHeapManager* FD3D12Device::GetQueryHeapManager(EQueryType QueryType)
{
    if (QueryType == EQueryType::Timestamp)
    {
        CHECK(TimingQueryHeapManager->GetQueryType() == EQueryType::Timestamp);
        return TimingQueryHeapManager;
    }
    else if (QueryType == EQueryType::Occlusion)
    {
        CHECK(OcclusionQueryHeapManager->GetQueryType() == EQueryType::Occlusion);
        return OcclusionQueryHeapManager;
    }
    else
    {
        return nullptr;
    }
}
