#include "D3D12Device.h"
#include "D3D12RHIShaderCompiler.h"
#include "D3D12Descriptors.h"
#include "D3D12RootSignature.h"
#include "D3D12CommandAllocator.h"
#include "D3D12CommandQueue.h"
#include "D3D12PipelineState.h"
#include "D3D12Library.h"

#include "Core/Windows/Windows.h"
#include "Core/Modules/Platform/PlatformLibrary.h"

#include "CoreApplication/Platform/PlatformApplicationMisc.h"

#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Helpers

static const char* ToString(D3D12_AUTO_BREADCRUMB_OP BreadCrumbOp)
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

static const char* GDeviceRemovedDumpFile = "D3D12DeviceRemovedDump.txt";

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// DeviceRemovedHandler

void D3D12DeviceRemovedHandlerRHI(FD3D12Device* Device)
{
    Check(Device != nullptr);

    FString Message = "[D3D12] Device Removed";
    D3D12_ERROR("%s", Message.CStr());

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

    FILE* File = fopen(GDeviceRemovedDumpFile, "w");
    if (File)
    {
        fwrite(Message.Data(), 1, Message.Size(), File);
        fputc('\n', File);
    }

    const D3D12_AUTO_BREADCRUMB_NODE* CurrentNode  = DredAutoBreadcrumbsOutput.pHeadAutoBreadcrumbNode;
    const D3D12_AUTO_BREADCRUMB_NODE* PreviousNode = nullptr;
    while (CurrentNode)
    {
        Message = "BreadCrumbs:";
        if (File)
        {
            fwrite(Message.Data(), 1, Message.Size(), File);
            fputc('\n', File);
        }

        D3D12_ERROR("%s", Message.CStr());
        for (uint32 i = 0; i < CurrentNode->BreadcrumbCount; i++)
        {
            Message = "    " + FString(ToString(CurrentNode->pCommandHistory[i]));
            D3D12_ERROR("%s", Message.CStr());
            if (File)
            {
                fwrite(Message.Data(), 1, Message.Size(), File);
                fputc('\n', File);
            }
        }

        PreviousNode = CurrentNode;
        CurrentNode  = CurrentNode->pNext;
    }

    if (File)
    {
        fclose(File);
    }

    FPlatformApplicationMisc::MessageBox("Error", " [D3D12] Device Removed");
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12Adapter

bool FD3D12Adapter::Initialize()
{
    if (Initializer.bEnableDebugLayer)
    {
        TComPtr<ID3D12Debug> DebugInterface;
        if (FAILED(FD3D12Library::D3D12GetDebugInterface(IID_PPV_ARGS(&DebugInterface))))
        {
            D3D12_ERROR("[FD3D12Adapter]: FAILED to enable DebugLayer");
            return false;
        }
        else
        {
            DebugInterface->EnableDebugLayer();
        }

        if (Initializer.bEnableDRED)
        {
            TComPtr<ID3D12DeviceRemovedExtendedDataSettings> DredSettings;
            if (SUCCEEDED(FD3D12Library::D3D12GetDebugInterface(IID_PPV_ARGS(&DredSettings))))
            {
                DredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
                DredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            }
            else
            {
                D3D12_ERROR("[FD3D12Adapter]: FAILED to enable DRED");
            }
        }

        if (Initializer.bEnableGPUValidation)
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
        if (SUCCEEDED(FD3D12Library::DXGIGetDebugInterface1(0, IID_PPV_ARGS(&InfoQueue))))
        {
            InfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
            InfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
        }
        else
        {
            D3D12_ERROR("[FD3D12Adapter]: FAILED to retrieve InfoQueue");
        }

        if (Initializer.bEnablePIX)
        {
            TComPtr<IDXGraphicsAnalysis> TempGraphicsAnalysis;
            if (SUCCEEDED(FD3D12Library::DXGIGetDebugInterface1(0, IID_PPV_ARGS(&TempGraphicsAnalysis))))
            {
                DXGraphicsAnalysis = TempGraphicsAnalysis;
            }
            else
            {
                D3D12_INFO("[FD3D12Adapter]: PIX is not connected to the application");
            }
        }
    }

    // Create factory
    if (FAILED(FD3D12Library::CreateDXGIFactory2(0, IID_PPV_ARGS(&Factory))))
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

    // Choose adapter
    D3D_FEATURE_LEVEL BestFeatureLevel = D3D_FEATURE_LEVEL_11_0;
    
    const D3D_FEATURE_LEVEL TestFeatureLevels[] =
    {
#if WIN10_BUILD_20348
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

                Result = FD3D12Library::D3D12CreateDevice(TempAdapter.Get(), Level, __uuidof(ID3D12Device), nullptr);
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
        D3D12_ERROR_COND(SUCCEEDED(Result), "[FD3D12Adapter]: Failed to Query IDXGIFactory6");

        const DXGI_GPU_PREFERENCE GPUPreference = Initializer.bPreferDGPU ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED;

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

                Result = FD3D12Library::D3D12CreateDevice(TempAdapter.Get(), Level, __uuidof(ID3D12Device), nullptr);
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
        D3D12_ERROR("[CD3D12Device]: FAILED to retrieve adapter");
        return false;
    }
    else
    {
        Adapter = FinalAdapter;
        if (FAILED(Adapter->GetDesc1(&AdapterDesc)))
        {
            D3D12_ERROR("[CD3D12Device]: FAILED to retrieve DXGI_ADAPTER_DESC1");
            return false;
        }
    }

    return true;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12Device

FD3D12Device::~FD3D12Device()
{
    if (Adapter->IsDebugLayerEnabled())
    {
        TComPtr<ID3D12DebugDevice> DebugDevice;
        if (SUCCEEDED(Device.GetAs(&DebugDevice)))
        {
            DebugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
        }
    }

    SafeDelete(RootSignatureCache);

    Device.Reset();
#if WIN10_BUILD_14393
    Device1.Reset();
#endif
#if WIN10_BUILD_15063
    Device2.Reset();
#endif
#if WIN10_BUILD_16299
    Device3.Reset();
#endif
#if WIN10_BUILD_17134
    Device4.Reset();
#endif
#if WIN10_BUILD_17763
    Device5.Reset();
#endif
#if WIN10_BUILD_18362
    Device6.Reset();
#endif
#if WIN10_BUILD_19041
    Device7.Reset();
#endif
#if WIN10_BUILD_20348
    Device8.Reset();
#endif
#if WIN11_BUILD_22000
    Device9.Reset();
#endif
}

bool FD3D12Device::Initialize()
{
    // Create Device
    if (FAILED(FD3D12Library::D3D12CreateDevice(Adapter->GetDXGIAdapter(), MinFeatureLevel, IID_PPV_ARGS(&Device))))
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "FAILED to create device");
        return false;
    }
    else
    {
        const FString Description = Adapter->GetDescription();
        D3D12_INFO("[FD3D12Device]: Created Device for adapter '%s'", Description.CStr());
    }

    // Configure debug device (if active).
    if (Adapter->IsDebugLayerEnabled())
    {
        TComPtr<ID3D12InfoQueue> InfoQueue;
        if (SUCCEEDED(Device.GetAs(&InfoQueue)))
        {
            InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
            InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR     , true);

            D3D12_MESSAGE_ID Hide[] =
            {
                D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
                D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
            };

            D3D12_INFO_QUEUE_FILTER Filter;
            FMemory::Memzero(&Filter);

            Filter.DenyList.NumIDs  = ArrayCount(Hide);
            Filter.DenyList.pIDList = Hide;
            InfoQueue->AddStorageFilterEntries(&Filter);
        }
    }

#if WIN10_BUILD_14393
    if (FAILED(Device.GetAs<ID3D12Device1>(&Device1)))
    {
        D3D12_ERROR("[FD3D12Device]: Failed to retrieve ID3D12Device1");
        return false;
    }
#endif

#if WIN10_BUILD_15063
    if (FAILED(Device.GetAs<ID3D12Device2>(&Device2)))
    {
        D3D12_ERROR("[FD3D12Device]: Failed to retrieve ID3D12Device2");
        return false;
    }
#endif

#if WIN10_BUILD_16299
    if (FAILED(Device.GetAs<ID3D12Device3>(&Device3)))
    {
        D3D12_ERROR("[FD3D12Device]: Failed to retrieve ID3D12Device3");
        return false;
    }
#endif

#if WIN10_BUILD_17134
    if (FAILED(Device.GetAs<ID3D12Device4>(&Device4)))
    {
        D3D12_ERROR("[FD3D12Device]: Failed to retrieve ID3D12Device4");
        return false;
    }
#endif

#if WIN10_BUILD_17763
    if (FAILED(Device.GetAs<ID3D12Device5>(&Device5)))
    {
        D3D12_ERROR("[FD3D12Device]: Failed to retrieve ID3D12Device5");
        return false;
    }
#endif

#if WIN10_BUILD_18362
    if (FAILED(Device.GetAs<ID3D12Device6>(&Device6)))
    {
        D3D12_ERROR("[FD3D12Device]: Failed to retrieve ID3D12Device6");
        return false;
    }
#endif

#if WIN10_BUILD_19041
    if (FAILED(Device.GetAs<ID3D12Device7>(&Device7)))
    {
        D3D12_ERROR("[FD3D12Device]: Failed to retrieve ID3D12Device7");
        return false;
    }
#endif

#if WIN10_BUILD_20348
    if (FAILED(Device.GetAs<ID3D12Device8>(&Device8)))
    {
        D3D12_ERROR("[FD3D12Device]: Failed to retrieve ID3D12Device8");
        return false;
    }
#endif

#if 0 && WIN11_BUILD_22000
    if (FAILED(Device.GetAs<ID3D12Device9>(&Device9)))
    {
        D3D12_ERROR("[FD3D12Device]: Failed to retrieve ID3D12Device9");
        return false;
    }
#endif

    // TODO: Remove feature levels from unsupported SDKs
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
        ArrayCount(SupportedFeatureLevels), SupportedFeatureLevels, D3D_FEATURE_LEVEL_11_0
    };

    HRESULT Result = Device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &FeatureLevels, sizeof(FeatureLevels));
    if (SUCCEEDED(Result))
    {
        ActiveFeatureLevel = FeatureLevels.MaxSupportedFeatureLevel;
    }
    else
    {
        ActiveFeatureLevel = MinFeatureLevel;
    }

    // Create RootSignature cache
    RootSignatureCache = dbg_new FD3D12RootSignatureCache(this);
    if (!RootSignatureCache->Initialize())
    {
        return false;
    } 

    // Create DescriptorHeaps
    GlobalResourceHeap = dbg_new FD3D12DescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_MAX_RESOURCE_ONLINE_DESCRIPTOR_COUNT, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
    if (!GlobalResourceHeap->Initialize())
    {
        D3D12_ERROR("Failed to create global resource descriptor heap");
        return false;
    }
    else
    {
        GlobalResourceHeap->SetName("Global Resource Descriptor Heap");
    }

    GlobalSamplerHeap = dbg_new FD3D12DescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_MAX_SAMPLER_ONLINE_DESCRIPTOR_COUNT, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
    if (!GlobalSamplerHeap->Initialize())
    {
        D3D12_ERROR("Failed to create global sampler descriptor heap");
        return false;
    }
    else
    {
        GlobalSamplerHeap->SetName("Global Sampler Descriptor Heap");
    }

    return true;
}

int32 FD3D12Device::GetMultisampleQuality(DXGI_FORMAT Format, uint32 SampleCount)
{
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS Data;
    FMemory::Memzero(&Data);

    Data.Flags       = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    Data.Format      = Format;
    Data.SampleCount = SampleCount;

    HRESULT hr = Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &Data, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
    if (FAILED(hr))
    {
        D3D12_ERROR("[CD3D12Device] CheckFeatureSupport failed");
        return 0;
    }

    return static_cast<uint32>(Data.NumQualityLevels - 1);
}
