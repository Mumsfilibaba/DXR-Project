#include "D3D12Device.h"
#include "D3D12ShaderCompiler.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12RootSignature.h"
#include "D3D12CommandAllocator.h"
#include "D3D12CommandQueue.h"
#include "D3D12PipelineState.h"
#include "D3D12FunctionPointers.h"

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

void D3D12RHIDeviceRemovedHandler(CD3D12Device* Device)
{
    Check(Device != nullptr);

    String Message = "[D3D12RHI] Device Removed";
    LOG_ERROR(Message);

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

    const D3D12_AUTO_BREADCRUMB_NODE* CurrentNode = DredAutoBreadcrumbsOutput.pHeadAutoBreadcrumbNode;
    const D3D12_AUTO_BREADCRUMB_NODE* PreviousNode = nullptr;
    while (CurrentNode)
    {
        Message = "BreadCrumbs:";
        if (File)
        {
            fwrite(Message.Data(), 1, Message.Size(), File);
            fputc('\n', File);
        }

        LOG_ERROR(Message);
        for (uint32 i = 0; i < CurrentNode->BreadcrumbCount; i++)
        {
            Message = "    " + String(ToString(CurrentNode->pCommandHistory[i]));
            LOG_ERROR(Message);
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

    PlatformApplicationMisc::MessageBox("Error", " [D3D12RHI] Device Removed");
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12Device

TSharedRef<CD3D12Device> CD3D12Device::CreateDevice(CRHIInstanceD3D12* InInstance, bool bInEnableDebugLayer, bool bInEnableGPUValidation, bool bInEnableDRED)
{
    TSharedRef<CD3D12Device> NewDevice = dbg_new CD3D12Device(InInstance, bInEnableDebugLayer, bInEnableGPUValidation, bInEnableDRED);
    if (NewDevice && NewDevice->Initialize())
    {
        return NewDevice;
    }

    return nullptr;
}

CD3D12Device::CD3D12Device(CRHIInstanceD3D12* InInstance, bool bInEnableDebugLayer, bool bInEnableGPUValidation, bool bInEnableDRED)
    : Instance(InInstance)
    , Factory(nullptr)
    , Adapter(nullptr)
    , Device(nullptr)
    , DXRDevice(nullptr)
    , bEnableDebugLayer(bInEnableDebugLayer)
    , bEnableGPUValidation(bInEnableGPUValidation)
    , bEnableDRED(bInEnableDRED)
{
}

CD3D12Device::~CD3D12Device()
{
    if (bEnableDebugLayer)
    {
        TComPtr<ID3D12DebugDevice> DebugDevice;
        if (SUCCEEDED(Device.GetAs(&DebugDevice)))
        {
            DebugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
        }

        GraphicsAnalysisInterface.Reset();

        if (PIXLib)
        {
            FreeLibrary(PIXLib);
            PIXLib = 0;
        }
    }

    Factory.Reset();
    Adapter.Reset();
    Device.Reset();
    DXRDevice.Reset();

    FreeLibrary(DXGILib);
    DXGILib = 0;

    FreeLibrary(D3D12Lib);
    D3D12Lib = 0;
}

bool CD3D12Device::Initialize()
{
    DXGILib = LoadLibrary("dxgi.dll");
    if (!DXGILib)
    {
        PlatformApplicationMisc::MessageBox("ERROR", "FAILED to load dxgi.dll");
        return false;
    }
    else
    {
        D3D12_INFO("Loaded dxgi.dll");
    }

    D3D12Lib = LoadLibrary("d3d12.dll");
    if (!D3D12Lib)
    {
        PlatformApplicationMisc::MessageBox("ERROR", "FAILED to load d3d12.dll");
        return false;
    }
    else
    {
        D3D12_INFO("Loaded d3d12.dll");
    }

    // Load DXGI functions 
    NDXGIFunctions::CreateDXGIFactory2     = PlatformLibrary::LoadSymbolAddress<PFN_CREATE_DXGI_FACTORY_2>("CreateDXGIFactory2", DXGILib);
    NDXGIFunctions::DXGIGetDebugInterface1 = PlatformLibrary::LoadSymbolAddress<PFN_DXGI_GET_DEBUG_INTERFACE_1>("DXGIGetDebugInterface1", DXGILib);

    // Load D3D12 functions
    ND3D12Functions::D3D12CreateDevice                             = PlatformLibrary::LoadSymbolAddress<PFN_D3D12_CREATE_DEVICE>("D3D12CreateDevice", D3D12Lib);
    ND3D12Functions::D3D12GetDebugInterface                        = PlatformLibrary::LoadSymbolAddress<PFN_D3D12_GET_DEBUG_INTERFACE>("D3D12GetDebugInterface", D3D12Lib);
    ND3D12Functions::D3D12SerializeRootSignature                   = PlatformLibrary::LoadSymbolAddress<PFN_D3D12_SERIALIZE_ROOT_SIGNATURE>("D3D12SerializeRootSignature", D3D12Lib);
    ND3D12Functions::D3D12SerializeVersionedRootSignature          = PlatformLibrary::LoadSymbolAddress<PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE>("D3D12SerializeVersionedRootSignature", D3D12Lib);
    ND3D12Functions::D3D12CreateRootSignatureDeserializer          = PlatformLibrary::LoadSymbolAddress<PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER>("D3D12CreateRootSignatureDeserializer", D3D12Lib);
    ND3D12Functions::D3D12CreateVersionedRootSignatureDeserializer = PlatformLibrary::LoadSymbolAddress<PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER>("D3D12CreateVersionedRootSignatureDeserializer", D3D12Lib);

    if (bEnableDebugLayer)
    {
        PIXLib = LoadLibrary("WinPixEventRuntime.dll");
        if (PIXLib != NULL)
        {
            D3D12_INFO("Loaded WinPixEventRuntime.dll");
            ND3D12Functions::SetMarkerOnCommandList = PlatformLibrary::LoadSymbolAddress<PFN_SetMarkerOnCommandList>("PIXSetMarkerOnCommandList", PIXLib);
        }
        else
        {
            D3D12_INFO("PIX Runtime NOT found");
        }

        TComPtr<ID3D12Debug> DebugInterface;
        if (FAILED(ND3D12Functions::D3D12GetDebugInterface(IID_PPV_ARGS(&DebugInterface))))
        {
            D3D12_ERROR_ALWAYS("FAILED to enable DebugLayer");
            return false;
        }
        else
        {
            DebugInterface->EnableDebugLayer();
        }

        if (bEnableDRED)
        {
            TComPtr<ID3D12DeviceRemovedExtendedDataSettings> DredSettings;
            if (SUCCEEDED(ND3D12Functions::D3D12GetDebugInterface(IID_PPV_ARGS(&DredSettings))))
            {
                DredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
                DredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
            }
            else
            {
                D3D12_ERROR_ALWAYS("FAILED to enable DRED");
            }
        }

        if (bEnableGPUValidation)
        {
            TComPtr<ID3D12Debug1> DebugInterface1;
            if (FAILED(DebugInterface.GetAs(&DebugInterface1)))
            {
                D3D12_ERROR_ALWAYS("FAILED to enable GPU- Validation");
                return false;
            }
            else
            {
                DebugInterface1->SetEnableGPUBasedValidation(true);
            }
        }

        // TODO: Check for windows SDK version
#if 0
        {
            TComPtr<ID3D12Debug5> DebugInterface5;
            if (FAILED(DebugInterface.GetAs(&DebugInterface5)))
            {
                D3D12_ERROR_ALWAYS("FAILED to enable auto-naming of objects");
            }
            else
            {
                DebugInterface5->SetEnableAutoName(true);
            }
        }
#endif

        TComPtr<IDXGIInfoQueue> InfoQueue;
        if (SUCCEEDED(NDXGIFunctions::DXGIGetDebugInterface1(0, IID_PPV_ARGS(&InfoQueue))))
        {
            InfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
            InfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
        }
        else
        {
            D3D12_ERROR_ALWAYS("FAILED to retrieve InfoQueue");
        }

        TComPtr<IDXGraphicsAnalysis> TempGraphicsAnalysisInterface;
        if (SUCCEEDED(NDXGIFunctions::DXGIGetDebugInterface1(0, IID_PPV_ARGS(&TempGraphicsAnalysisInterface))))
        {
            GraphicsAnalysisInterface = TempGraphicsAnalysisInterface;
        }
        else
        {
            D3D12_INFO("PIX is not connected to the application");
        }
    }

    // Create factory
    if (FAILED(NDXGIFunctions::CreateDXGIFactory2(0, IID_PPV_ARGS(&Factory))))
    {
        D3D12_ERROR_ALWAYS("FAILED to create factory");
        return false;
    }
    else
    {
        TComPtr<IDXGIFactory5> Factory5;
        if (FAILED(Factory.GetAs(&Factory5)))
        {
            D3D12_ERROR_ALWAYS("FAILED to retrieve IDXGIFactory5");
            return false;
        }
        else
        {
            HRESULT hResult = Factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &bAllowTearing, sizeof(bAllowTearing));
            if (SUCCEEDED(hResult))
            {
                if (bAllowTearing)
                {
                    D3D12_INFO("Tearing is supported");
                }
                else
                {
                    D3D12_INFO("Tearing is NOT supported");
                }
            }
        }
    }

    // Choose adapter
    TComPtr<IDXGIAdapter1> TempAdapter;
    for (uint32 ID = 0; DXGI_ERROR_NOT_FOUND != Factory->EnumAdapters1(ID, &TempAdapter); ID++)
    {
        DXGI_ADAPTER_DESC1 Desc;
        if (FAILED(TempAdapter->GetDesc1(&Desc)))
        {
            D3D12_ERROR_ALWAYS("FAILED to retrieve DXGI_ADAPTER_DESC1");
            return false;
        }

        if (Desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            continue;
        }

        if (SUCCEEDED(ND3D12Functions::D3D12CreateDevice(TempAdapter.Get(), MinFeatureLevel, _uuidof(ID3D12Device), nullptr)))
        {
            AdapterID = ID;

            char Buff[256] = {};
            sprintf_s(Buff, "Direct3D Adapter (%u): %ls", AdapterID, Desc.Description);
            D3D12_INFO(Buff);

            break;
        }
    }

    if (!TempAdapter)
    {
        D3D12_ERROR_ALWAYS("FAILED to retrieve adapter");
        return false;
    }
    else
    {
        Adapter = TempAdapter;
    }

    // Create Device
    if (FAILED(ND3D12Functions::D3D12CreateDevice(Adapter.Get(), MinFeatureLevel, IID_PPV_ARGS(&Device))))
    {
        PlatformApplicationMisc::MessageBox("ERROR", "FAILED to create device");
        return false;
    }
    else
    {
        D3D12_INFO("Created Device");
    }

    // Configure debug device (if active).
    if (bEnableDebugLayer)
    {
        TComPtr<ID3D12InfoQueue> InfoQueue;
        if (SUCCEEDED(Device.GetAs(&InfoQueue)))
        {
            InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
            InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);

            D3D12_MESSAGE_ID Hide[] =
            {
                D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
                D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
            };

            D3D12_INFO_QUEUE_FILTER Filter;
            CMemory::Memzero(&Filter);

            Filter.DenyList.NumIDs  = ArrayCount(Hide);
            Filter.DenyList.pIDList = Hide;
            InfoQueue->AddStorageFilterEntries(&Filter);
        }
    }

    if (FAILED(Device.GetAs<ID3D12Device5>(&DXRDevice)))
    {
        D3D12_ERROR_ALWAYS("Failed to retrieve DXR-Device");
        return false;
    }

    const D3D_FEATURE_LEVEL SupportedFeatureLevels[] =
    {
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

    // Check for Ray-Tracing support
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 Features5;
        CMemory::Memzero(&Features5, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));

        Result = Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &Features5, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));
        if (SUCCEEDED(Result))
        {
            RayTracingTier = Features5.RaytracingTier;
        }
    }

    // Checking for Variable Shading Rate support
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS6 Features6;
        CMemory::Memzero(&Features6, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS6));

        Result = Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &Features6, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS6));
        if (SUCCEEDED(Result))
        {
            VariableShadingRateTier = Features6.VariableShadingRateTier;
            VariableShadingRateTileSize = Features6.ShadingRateImageTileSize;
        }
    }

    // Check for Mesh-Shaders, and SamplerFeedback support
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS7 Features7;
        CMemory::Memzero(&Features7, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS7));

        Result = Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &Features7, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS7));
        if (SUCCEEDED(Result))
        {
            MeshShaderTier = Features7.MeshShaderTier;
            SamplerFeedBackTier = Features7.SamplerFeedbackTier;
        }
    }

    return true;
}

int32 CD3D12Device::GetMultisampleQuality(DXGI_FORMAT Format, uint32 SampleCount)
{
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS Data;
    CMemory::Memzero(&Data);

    Data.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    Data.Format = Format;
    Data.SampleCount = SampleCount;

    HRESULT hr = Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &Data, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
    if (FAILED(hr))
    {
        D3D12_ERROR_ALWAYS("[CD3D12Device] CheckFeatureSupport failed");
        return 0;
    }

    return static_cast<uint32>(Data.NumQualityLevels - 1);
}

String CD3D12Device::GetAdapterName() const
{
    DXGI_ADAPTER_DESC Desc;
    Adapter->GetDesc(&Desc);

    WString WideName = Desc.Description;
    return WideToChar(WideName);
}