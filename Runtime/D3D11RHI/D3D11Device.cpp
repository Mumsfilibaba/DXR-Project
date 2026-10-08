#include "Core/Misc/ConsoleManager.h"
#include "Core/Threading/ScopedLock.h"
#include "D3D11RHI/D3D11Capabilities.h"
#include "D3D11RHI/D3D11Device.h"
#include "D3D11RHI/D3D11Loader.h"

#if D3D11_ENABLE_COMPOSITION
    #include <dcomp.h>
#endif

static TAutoConsoleVariable<bool> CVarBreakOnError(
    "D3D11RHI.BreakOnError",
    "When enabled, there will be a DebugBreak when the debug layer encounters an error",
    true);

static TAutoConsoleVariable<bool> CVarBreakOnWarning(
    "D3D11RHI.BreakOnWarning",
    "When enabled, there will be a DebugBreak when the debug layer encounters a warning",
    false);

static TAutoConsoleVariable<bool> CVarPreferDedicatedGPU(
    "D3D11RHI.PreferDedicatedGPU",
    "When enabled, a dedicated GPU will be selected when creating the Device",
    true);

static const D3D_FEATURE_LEVEL GD3D11FeatureLevels[] =
{
    D3D_FEATURE_LEVEL_11_1,
    D3D_FEATURE_LEVEL_11_0,
};

FD3D11Adapter::FD3D11Adapter()
    : Adapter(nullptr)
    , Adapter3(nullptr)
    , Factory(nullptr)
    , Factory5(nullptr)
    , Factory6(nullptr)
    , AdapterDesc()
    , AdapterIndex(0)
    , bAllowTearing(false)
    , bEnableDebugLayer(false)
{
}

FD3D11Adapter::~FD3D11Adapter() = default;

bool FD3D11Adapter::Initialize()
{
    if (IConsoleVariable* CVarEnableDebugLayer = FConsoleManager::Get().FindConsoleVariable("RHI.EnableDebugLayer"))
    {
        bEnableDebugLayer = CVarEnableDebugLayer->GetBool();
    }

    HRESULT Result = D3D11::CreateDXGIFactory2(bEnableDebugLayer ? DXGI_CREATE_FACTORY_DEBUG : 0, IID_PPV_ARGS(&Factory));
    if (FAILED(Result) && bEnableDebugLayer)
    {
        D3D11_WARNING("[FD3D11Adapter]: The DXGI debug layer is not available, creating the factory without it");
        Result = D3D11::CreateDXGIFactory2(0, IID_PPV_ARGS(&Factory));
    }

    if (FAILED(Result))
    {
        D3D11_ERROR_CRITICAL("[FD3D11Adapter]: FAILED to create factory");
        return false;
    }

    if (SUCCEEDED(Factory.GetAs(&Factory5)))
    {
        BOOL bTearingSupported = FALSE;
        if (SUCCEEDED(Factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &bTearingSupported, sizeof(bTearingSupported))))
        {
            bAllowTearing = (bTearingSupported != FALSE);
        }
    }

    D3D11_INFO("[FD3D11Adapter]: Tearing is %s", bAllowTearing ? "supported" : "NOT supported");

    Factory.GetAs(&Factory6);

    const DXGI_GPU_PREFERENCE GPUPreference = CVarPreferDedicatedGPU.GetValue() ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED;
    const auto EnumerateAdapter = [this, GPUPreference](uint32 Index, TComPtr<IDXGIAdapter1>& OutAdapter) -> HRESULT
    {
        return Factory6 ? Factory6->EnumAdapterByGpuPreference(Index, GPUPreference, IID_PPV_ARGS(&OutAdapter)) : Factory->EnumAdapters1(Index, &OutAdapter);
    };

    D3D_FEATURE_LEVEL      BestFeatureLevel = static_cast<D3D_FEATURE_LEVEL>(0);
    TComPtr<IDXGIAdapter1> FinalAdapter;

    for (uint32 Index = 0;; Index++)
    {
        TComPtr<IDXGIAdapter1> TempAdapter;
        if (EnumerateAdapter(Index, TempAdapter) == DXGI_ERROR_NOT_FOUND)
        {
            break;
        }

        DXGI_ADAPTER_DESC1 Desc;
        if (FAILED(TempAdapter->GetDesc1(&Desc)))
        {
            D3D11_ERROR("[FD3D11Adapter]: FAILED to retrieve DXGI_ADAPTER_DESC1");
            return false;
        }

        if (Desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            continue;
        }

        D3D_FEATURE_LEVEL SupportedLevel = static_cast<D3D_FEATURE_LEVEL>(0);
        Result = D3D11::D3D11CreateDevice(TempAdapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, 0, GD3D11FeatureLevels, ARRAY_COUNT(GD3D11FeatureLevels),
            D3D11_SDK_VERSION, nullptr, &SupportedLevel, nullptr);

        if (SUCCEEDED(Result))
        {
            D3D11_INFO("[FD3D11Adapter]: Suitable Direct3D Adapter (%u): %ls", Index, Desc.Description);

            // Adapters arrive in preference order, so only a higher feature level replaces an earlier pick
            if (SupportedLevel > BestFeatureLevel)
            {
                AdapterIndex     = Index;
                BestFeatureLevel = SupportedLevel;
                FinalAdapter     = TempAdapter;
            }
        }
    }

    if (!FinalAdapter)
    {
        D3D11_ERROR("[FD3D11Adapter]: FAILED to retrieve adapter");
        return false;
    }

    Adapter = FinalAdapter;
    if (FAILED(Adapter->GetDesc1(&AdapterDesc)))
    {
        D3D11_ERROR("[FD3D11Adapter]: FAILED to retrieve DXGI_ADAPTER_DESC1");
        return false;
    }

    if (FAILED(Adapter.GetAs<IDXGIAdapter3>(&Adapter3)))
    {
        D3D11_ERROR("[FD3D11Adapter]: FAILED to retrieve IDXGIAdapter3");
        return false;
    }

    return true;
}

FD3D11Device::FD3D11Device(FD3D11Adapter* InAdapter)
    : Adapter(InAdapter)
    , D3D11Device(nullptr)
    , D3D11Device1(nullptr)
    , D3D11Device5(nullptr)
    , D3D11Context(nullptr)
    , D3D11Context1(nullptr)
    , D3D11Context4(nullptr)
    , InfoQueue(nullptr)
    , FeatureLevel(D3D_FEATURE_LEVEL_11_0)
    , SamplerStateMap()
    , SamplerStateMapCS()
#if D3D11_ENABLE_COMPOSITION
    , CompositionDevice(nullptr)
#endif
{
}

FD3D11Device::~FD3D11Device()
{
    {
        TScopedLock Lock(SamplerStateMapCS);
        SamplerStateMap.Clear();
    }

    if (D3D11Context)
    {
        D3D11Context->ClearState();
        D3D11Context->Flush();
    }

    FlushDebugMessages();
}

bool FD3D11Device::Initialize()
{
    if (!CreateDevice())
    {
        return false;
    }

    if (Adapter->IsDebugLayerEnabled())
    {
        SetupDebugMessages();
    }

    return true;
}

bool FD3D11Device::CreateDevice()
{
    UINT DeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    if (Adapter->IsDebugLayerEnabled())
    {
        DeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    }

    HRESULT Result = D3D11::D3D11CreateDevice(Adapter->GetDXGIAdapter(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, DeviceFlags, GD3D11FeatureLevels,
        ARRAY_COUNT(GD3D11FeatureLevels), D3D11_SDK_VERSION, &D3D11Device, &FeatureLevel, &D3D11Context);

    if (FAILED(Result) && (DeviceFlags & D3D11_CREATE_DEVICE_DEBUG))
    {
        D3D11_WARNING("[FD3D11Device]: The D3D11 debug layer is not available, creating the device without it");

        DeviceFlags &= ~D3D11_CREATE_DEVICE_DEBUG;
        Result = D3D11::D3D11CreateDevice(Adapter->GetDXGIAdapter(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, DeviceFlags, GD3D11FeatureLevels,
            ARRAY_COUNT(GD3D11FeatureLevels), D3D11_SDK_VERSION, &D3D11Device, &FeatureLevel, &D3D11Context);
    }

    if (FAILED(Result))
    {
        D3D11_ERROR_CRITICAL("[FD3D11Device]: FAILED to create device (0x%08X)", static_cast<uint32>(Result));
        return false;
    }

    D3D11Device.GetAs(&D3D11Device1);
    D3D11Device.GetAs(&D3D11Device5);
    D3D11Context.GetAs(&D3D11Context1);
    D3D11Context.GetAs(&D3D11Context4);

    // Resource creation and FRHIBuffer::Map can happen outside of the RHI thread
    TComPtr<ID3D11Multithread> Multithread;
    if (SUCCEEDED(D3D11Context.GetAs(&Multithread)))
    {
        Multithread->SetMultithreadProtected(TRUE);
    }
    else
    {
        D3D11_WARNING("[FD3D11Device]: ID3D11Multithread is not available, the immediate context is not protected");
    }

    D3D11_INFO("[FD3D11Device]: Created device (FeatureLevel=0x%X, Debug=%s)", static_cast<uint32>(FeatureLevel), (DeviceFlags & D3D11_CREATE_DEVICE_DEBUG) ? "Yes" : "No");
    return true;
}

void FD3D11Device::SetupDebugMessages()
{
    if (FAILED(D3D11Device.GetAs(&InfoQueue)))
    {
        return;
    }

    D3D11_MESSAGE_ID DeniedMessages[] =
    {
        D3D11_MESSAGE_ID_DEVICE_UNORDEREDACCESSVIEW_RETURN_TYPE_MISMATCH,
    };

    D3D11_INFO_QUEUE_FILTER Filter = {};
    Filter.DenyList.NumIDs  = ARRAY_COUNT(DeniedMessages);
    Filter.DenyList.pIDList = DeniedMessages;
    InfoQueue->AddStorageFilterEntries(&Filter);

    InfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, CVarBreakOnError.GetValue());
    InfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, CVarBreakOnError.GetValue());
    InfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, CVarBreakOnWarning.GetValue());
}

void FD3D11Device::FlushDebugMessages()
{
    if (!InfoQueue)
    {
        return;
    }

    const uint64 NumMessages = InfoQueue->GetNumStoredMessages();
    for (uint64 Index = 0; Index < NumMessages; ++Index)
    {
        SIZE_T MessageLength = 0;
        if (FAILED(InfoQueue->GetMessage(Index, nullptr, &MessageLength)) || MessageLength == 0)
        {
            continue;
        }

        TArray<uint8> Storage;
        Storage.Resize(static_cast<int32>(MessageLength));

        D3D11_MESSAGE* Message = reinterpret_cast<D3D11_MESSAGE*>(Storage.Data());
        if (FAILED(InfoQueue->GetMessage(Index, Message, &MessageLength)))
        {
            continue;
        }

        switch (Message->Severity)
        {
            case D3D11_MESSAGE_SEVERITY_CORRUPTION:
            case D3D11_MESSAGE_SEVERITY_ERROR:
                D3D11_ERROR("[DebugLayer] %.*s", static_cast<int32>(Message->DescriptionByteLength), Message->pDescription);
                break;

            case D3D11_MESSAGE_SEVERITY_WARNING:
                D3D11_WARNING("[DebugLayer] %.*s", static_cast<int32>(Message->DescriptionByteLength), Message->pDescription);
                break;

            default:
                break;
        }
    }

    InfoQueue->ClearStoredMessages();
}

void FD3D11Device::CheckDeviceRemoved(HRESULT Result, const CHAR* Operation) const
{
    if (Result == DXGI_ERROR_DEVICE_REMOVED || Result == DXGI_ERROR_DEVICE_RESET)
    {
        D3D11_ERROR_CRITICAL("[FD3D11Device]: Device removed during %s (Reason=0x%08X)", Operation, static_cast<uint32>(D3D11Device->GetDeviceRemovedReason()));
    }
}

bool FD3D11Device::SupportsSwapChainFormat(DXGI_FORMAT DXGIFormat, ESwapChainUsageFlags Usage) const
{
    if (DXGIFormat == DXGI_FORMAT_UNKNOWN)
    {
        return false;
    }

    UINT FormatSupport = 0;
    if (FAILED(D3D11Device->CheckFormatSupport(DXGIFormat, &FormatSupport)))
    {
        return false;
    }

    UINT RequiredSupport = D3D11_FORMAT_SUPPORT_DISPLAY;
    if (IsEnumFlagSet(Usage, ESwapChainUsageFlags::RenderTarget))
    {
        RequiredSupport |= D3D11_FORMAT_SUPPORT_RENDER_TARGET;
    }

    if (IsEnumFlagSet(Usage, ESwapChainUsageFlags::UnorderedAccess))
    {
        RequiredSupport |= D3D11_FORMAT_SUPPORT_TYPED_UNORDERED_ACCESS_VIEW;
    }

    if (IsEnumFlagSet(Usage, ESwapChainUsageFlags::ShaderResource))
    {
        RequiredSupport |= D3D11_FORMAT_SUPPORT_SHADER_SAMPLE;
    }

    return (FormatSupport & RequiredSupport) == RequiredSupport;
}

bool FD3D11Device::QueryMultisampleQuality(DXGI_FORMAT Format, uint32 SampleCount, uint32& OutQuality) const
{
    OutQuality = 0;

    UINT NumQualityLevels = 0;
    if (FAILED(D3D11Device->CheckMultisampleQualityLevels(Format, SampleCount, &NumQualityLevels)))
    {
        D3D11_ERROR("[FD3D11Device] CheckMultisampleQualityLevels failed");
        return false;
    }

    return NumQualityLevels != 0;
}

bool FD3D11Device::FindOrCreateSamplerState(const FRHISamplerStateDesc& SamplerDesc, TComPtr<ID3D11SamplerState>& OutSamplerState)
{
    TScopedLock Lock(SamplerStateMapCS);

    if (TComPtr<ID3D11SamplerState>* ExistingSamplerState = SamplerStateMap.Find(SamplerDesc))
    {
        OutSamplerState = *ExistingSamplerState;
        return true;
    }

    D3D11_SAMPLER_DESC D3DSamplerDesc = {};
    D3DSamplerDesc.AddressU       = ConvertSamplerMode(SamplerDesc.AddressU);
    D3DSamplerDesc.AddressV       = ConvertSamplerMode(SamplerDesc.AddressV);
    D3DSamplerDesc.AddressW       = ConvertSamplerMode(SamplerDesc.AddressW);
    D3DSamplerDesc.Filter         = ConvertSamplerFilter(SamplerDesc.Filter);
    D3DSamplerDesc.MaxAnisotropy  = Math::Clamp<UINT>(SamplerDesc.MaxAnisotropy, 1, D3D11_REQ_MAXANISOTROPY);
    D3DSamplerDesc.MipLODBias     = SamplerDesc.MipLODBias;
    D3DSamplerDesc.MinLOD         = SamplerDesc.MinLOD;
    D3DSamplerDesc.MaxLOD         = SamplerDesc.MaxLOD;
    D3DSamplerDesc.ComparisonFunc = SamplerDesc.IsComparisonSampler() ? ConvertComparisonFunc(SamplerDesc.ComparisonFunc) : D3D11_COMPARISON_NEVER;

    Memory::Memcpy(D3DSamplerDesc.BorderColor, SamplerDesc.BorderColor.RGBA, sizeof(D3DSamplerDesc.BorderColor));

    TComPtr<ID3D11SamplerState> NewSamplerState;

    const HRESULT Result = D3D11Device->CreateSamplerState(&D3DSamplerDesc, &NewSamplerState);
    if (FAILED(Result))
    {
        D3D11_ERROR("[FD3D11Device]: FAILED to create SamplerState (0x%08X)", static_cast<uint32>(Result));
        return false;
    }

    D3D11SetDebugName(NewSamplerState.Get(), String::Printf("Sampler %d", SamplerStateMap.Size()));

    SamplerStateMap.Add(SamplerDesc, NewSamplerState);
    OutSamplerState = NewSamplerState;
    return true;
}

bool FD3D11Device::FindOrCreateSamplerState(const FRHIStaticSamplerInfo& StaticSamplerInfo, TComPtr<ID3D11SamplerState>& OutSamplerState)
{
    return FindOrCreateSamplerState(StaticSamplerInfo.GetSamplerStateDesc(), OutSamplerState);
}

#if D3D11_ENABLE_COMPOSITION
IDCompositionDevice* FD3D11Device::GetCompositionDevice()
{
    if (!CompositionDevice && GD3D11SupportsComposition)
    {
        if (FAILED(D3D11::DCompositionCreateDevice(nullptr, IID_PPV_ARGS(&CompositionDevice))))
        {
            D3D11_WARNING("[FD3D11Device]: FAILED to create a DirectComposition device, so transparent surfaces will present opaque");
        }
    }

    return CompositionDevice.Get();
}
#endif
