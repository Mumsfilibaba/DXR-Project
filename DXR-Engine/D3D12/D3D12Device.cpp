#include "Application/Platform/PlatformDialogMisc.h"

#include "D3D12Device.h"
#include "D3D12ShaderCompiler.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12PipelineState.h"
#include "D3D12RootSignature.h"
#include "D3D12CommandAllocator.h"
#include "D3D12CommandQueue.h"

#include "Windows/Windows.h"
#include "Windows/Windows.inl"

#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")

PFN_CREATE_DXGI_FACTORY_2                              CreateDXGIFactory2Func                            = nullptr;
PFN_DXGI_GET_DEBUG_INTERFACE_1                         DXGIGetDebugInterface1Func                        = nullptr;
PFN_D3D12_CREATE_DEVICE                                D3D12CreateDeviceFunc                             = nullptr;
PFN_D3D12_GET_DEBUG_INTERFACE                          D3D12GetDebugInterfaceFunc                        = nullptr;
PFN_D3D12_SERIALIZE_ROOT_SIGNATURE                     D3D12SerializeRootSignatureFunc                   = nullptr;
PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER           D3D12CreateRootSignatureDeserializerFunc          = nullptr;
PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE           D3D12SerializeVersionedRootSignatureFunc          = nullptr;
PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER D3D12CreateVersionedRootSignatureDeserializerFunc = nullptr;
PFN_SetMarkerOnCommandList                             SetMarkerOnCommandListFunc                        = nullptr;

D3D12Device::D3D12Device(Bool InEnableDebugLayer, Bool InEnableGPUValidation)
    : Factory(nullptr)
    , Adapter(nullptr)
    , Device(nullptr)
    , DXRDevice(nullptr)
    , EnableDebugLayer(InEnableDebugLayer)
    , EnableGPUValidation(InEnableGPUValidation)
{
}

D3D12Device::~D3D12Device()
{
    if (EnableDebugLayer)
    {
        TComPtr<ID3D12DebugDevice> DebugDevice;
        if (SUCCEEDED(Device.As(&DebugDevice)))
        {
            DebugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
        }

        PIXCaptureInterface.Reset();

        if (PIXLib)
        {
            ::FreeLibrary(PIXLib);
            PIXLib = 0;
        }
    }

    Factory.Reset();
    Adapter.Reset();
    Device.Reset();
    DXRDevice.Reset();

    ::FreeLibrary(DXGILib);
    DXGILib = 0;

    ::FreeLibrary(D3D12Lib);
    D3D12Lib = 0;
}

Bool D3D12Device::Init()
{
    DXGILib = ::LoadLibrary("dxgi.dll");
    if (DXGILib == NULL)
    {
        PlatformDialogMisc::MessageBox("ERROR", "FAILED to load dxgi.dll");
        return false;
    }
    else
    {
        LOG_INFO("Loaded dxgi.dll");
    }

    D3D12Lib = ::LoadLibrary("d3d12.dll");
    if (D3D12Lib == NULL)
    {
        PlatformDialogMisc::MessageBox("ERROR", "FAILED to load d3d12.dll");
        return false;
    }
    else
    {
        LOG_INFO("Loaded d3d12.dll");
    }

    CreateDXGIFactory2Func = GetTypedProcAddress<PFN_CREATE_DXGI_FACTORY_2>(
        DXGILib, 
        "CreateDXGIFactory2");
    DXGIGetDebugInterface1Func = GetTypedProcAddress<PFN_DXGI_GET_DEBUG_INTERFACE_1>(
        DXGILib, 
        "DXGIGetDebugInterface1");
    D3D12CreateDeviceFunc = GetTypedProcAddress<PFN_D3D12_CREATE_DEVICE>(
        D3D12Lib, 
        "D3D12CreateDevice");
    D3D12GetDebugInterfaceFunc = GetTypedProcAddress<PFN_D3D12_GET_DEBUG_INTERFACE>(
        D3D12Lib, 
        "D3D12GetDebugInterface");
    D3D12SerializeRootSignatureFunc = GetTypedProcAddress<PFN_D3D12_SERIALIZE_ROOT_SIGNATURE>(
        D3D12Lib, 
        "D3D12SerializeRootSignature");
    D3D12SerializeVersionedRootSignatureFunc = GetTypedProcAddress<PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE>(
        D3D12Lib, 
        "D3D12SerializeVersionedRootSignature");
    D3D12CreateRootSignatureDeserializerFunc = GetTypedProcAddress<PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER>(
        D3D12Lib, 
        "D3D12CreateRootSignatureDeserializer");
    D3D12CreateVersionedRootSignatureDeserializerFunc = GetTypedProcAddress<PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER>(
        D3D12Lib, 
        "D3D12CreateVersionedRootSignatureDeserializer");

    if (EnableDebugLayer)
    {
        PIXLib = LoadLibrary("WinPixEventRuntime.dll");
        if (PIXLib != NULL)
        {
            LOG_INFO("Loaded WinPixEventRuntime.dll");

            SetMarkerOnCommandListFunc = GetTypedProcAddress<PFN_SetMarkerOnCommandList>(
                PIXLib,
                "PIXSetMarkerOnCommandList");
        }
        else
        {
            LOG_INFO("PIX Runtime NOT found");
        }

        TComPtr<ID3D12Debug> DebugInterface;
        if (FAILED(D3D12GetDebugInterfaceFunc(IID_PPV_ARGS(&DebugInterface))))
        {
            LOG_ERROR("[D3D12Device]: FAILED to enable DebugLayer");
            return false;
        }
        else
        {
            DebugInterface->EnableDebugLayer();
        }

        if (EnableGPUValidation)
        {
            TComPtr<ID3D12Debug1> DebugInterface1;
            if (FAILED(DebugInterface.As(&DebugInterface1)))
            {
                LOG_ERROR("[D3D12Device]: FAILED to enable GPU- Validation");
                return false;
            }
            else
            {
                DebugInterface1->SetEnableGPUBasedValidation(TRUE);
            }
        }

        TComPtr<IDXGIInfoQueue> InfoQueue;
        if (SUCCEEDED(DXGIGetDebugInterface1Func(0, IID_PPV_ARGS(&InfoQueue))))
        {
            InfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
            InfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
        }
        else
        {
            LOG_ERROR("[D3D12Device]: FAILED to retrive InfoQueue");
        }

        TComPtr<IDXGraphicsAnalysis> TempPIXCaptureInterface;
        if (SUCCEEDED(DXGIGetDebugInterface1Func(0, IID_PPV_ARGS(&TempPIXCaptureInterface))))
        {
            PIXCaptureInterface = TempPIXCaptureInterface;
        }
        else
        {
            LOG_INFO("[D3D12Device]: PIX is not connected to the application");
        }
    }

    // Create factory
    if (FAILED(CreateDXGIFactory2Func(0, IID_PPV_ARGS(&Factory))))
    {
        LOG_ERROR("[D3D12Device]: FAILED to create factory");
        return false;
    }
    else
    {
        // Retrive newer factory interface
        TComPtr<IDXGIFactory5> Factory5;
        if (FAILED(Factory.As(&Factory5)))
        {
            LOG_ERROR("[D3D12Device]: FAILED to retrive IDXGIFactory5");
            return false;
        }
        else
        {
            HRESULT hResult = Factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &AllowTearing, sizeof(AllowTearing));
            if (SUCCEEDED(hResult))
            {
                if (AllowTearing)
                {
                    LOG_INFO("[D3D12Device]: Tearing is supported");
                }
                else
                {
                    LOG_INFO("[D3D12Device]: Tearing is NOT supported");
                }
            }
        }
    }

    // Choose adapter
    TComPtr<IDXGIAdapter1> TempAdapter;
    for (UINT ID = 0; DXGI_ERROR_NOT_FOUND != Factory->EnumAdapters1(ID, &TempAdapter); ID++)
    {
        DXGI_ADAPTER_DESC1 Desc;
        if (FAILED(TempAdapter->GetDesc1(&Desc)))
        {
            LOG_ERROR("[D3D12Device]: FAILED to retrive DXGI_ADAPTER_DESC1");
            return false;
        }

        // Don't select the Basic Render Driver adapter.
        if (Desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            continue;
        }

        // Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
        if (SUCCEEDED(D3D12CreateDeviceFunc(TempAdapter.Get(), MinFeatureLevel, _uuidof(ID3D12Device), nullptr)))
        {
            AdapterID = ID;

            char Buff[256] = {};
            sprintf_s(Buff, "[D3D12Device]: Direct3D Adapter (%u): %ls", AdapterID, Desc.Description);
            LOG_INFO(Buff);

            break;
        }
    }

    if (!TempAdapter)
    {
        LOG_ERROR("[D3D12Device]: FAILED to retrive adapter");
        return false;
    }
    else
    {
        Adapter = TempAdapter;
    }

    // Create Device
    if (FAILED(D3D12CreateDeviceFunc(Adapter.Get(), MinFeatureLevel, IID_PPV_ARGS(&Device))))
    {
        PlatformDialogMisc::MessageBox("ERROR", "FAILED to create device");
        return false;
    }
    else
    {
        LOG_INFO("[D3D12Device]: Created Device");
    }

    // Configure debug device (if active).
    if (EnableDebugLayer)
    {
        TComPtr<ID3D12InfoQueue> InfoQueue;
        if (SUCCEEDED(Device.As(&InfoQueue)))
        {
            InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
            InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);

            D3D12_MESSAGE_ID Hide[] =
            {
                D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
                D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
            };

            D3D12_INFO_QUEUE_FILTER Filter;
            Memory::Memzero(&Filter);

            Filter.DenyList.NumIDs    = _countof(Hide);
            Filter.DenyList.pIDList = Hide;
            InfoQueue->AddStorageFilterEntries(&Filter);
        }
    }

    // Get DXR Interfaces
    if (FAILED(Device.As<ID3D12Device5>(&DXRDevice)))
    {
        LOG_ERROR("[D3D12Device]: Failed to retrive DXR-Device");
        return false;
    }

    // Determine maximum supported feature level for this device
    static const D3D_FEATURE_LEVEL SupportedFeatureLevels[] =
    {
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    D3D12_FEATURE_DATA_FEATURE_LEVELS FeatureLevels =
    {
        _countof(SupportedFeatureLevels), SupportedFeatureLevels, D3D_FEATURE_LEVEL_11_0
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
        Memory::Memzero(&Features5, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));

        Result = Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &Features5, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));
        if (SUCCEEDED(Result))
        {
            RayTracingTier = Features5.RaytracingTier;
        }
    }

    // Checking for Variable Shading Rate
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS6 Features6;
        Memory::Memzero(&Features6, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS6));

        Result = Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS6, &Features6, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS6));
        if (SUCCEEDED(Result))
        {
            VariableShadingRateTier     = Features6.VariableShadingRateTier;
            VariableShadingRateTileSize = Features6.ShadingRateImageTileSize;
        }
    }

    // Check for Mesh-Shaders support
    {
        D3D12_FEATURE_DATA_D3D12_OPTIONS7 Features7;
        Memory::Memzero(&Features7, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS7));

        Result = Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &Features7, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS7));
        if (SUCCEEDED(Result))
        {
            MeshShaderTier      = Features7.MeshShaderTier;
            SamplerFeedBackTier = Features7.SamplerFeedbackTier;
        }
    }

    return true;
}

Int32 D3D12Device::GetMultisampleQuality(DXGI_FORMAT Format, UInt32 SampleCount)
{
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS Data;
    Memory::Memzero(&Data);

    Data.Flags       = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    Data.Format      = Format;
    Data.SampleCount = SampleCount;
    
    HRESULT hr = Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &Data, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
    if (FAILED(hr))
    {
        LOG_ERROR("[D3D12Device] CheckFeatureSupport failed");
        return 0;
    }

    return static_cast<UInt32>(Data.NumQualityLevels - 1);
}

std::string D3D12Device::GetAdapterName() const
{
    DXGI_ADAPTER_DESC Desc;
    Adapter->GetDesc(&Desc);

    std::wstring WideName = Desc.Description;
    return ConvertToAscii(WideName);
}