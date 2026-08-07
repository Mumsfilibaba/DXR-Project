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
#include "D3D12RHI/D3D12Stats.h"
#include "D3D12RHI/D3D12CommandContext.h"
#include "D3D12RHI/D3D12Query.h"
#include "D3D12RHI/D3D12RHI.h"
#include "D3D12RHI/D3D12DeviceDebug.h"

static TAutoConsoleVariable<bool> CVarBreakOnError(
    "D3D12RHI.BreakOnError",
    "When enabled, there will be a DebugBreak when the validation layer encounters an errors",
    true);

static TAutoConsoleVariable<bool> CVarBreakOnWarning(
    "D3D12RHI.BreakOnWarning",
    "When enabled, there will be a DebugBreak when the validation layer encounters an warnings",
    false);

static TAutoConsoleVariable<bool> CVarPreferDedicatedGPU(
    "D3D12RHI.PreferDedicatedGPU",
    "When enabled, a dedicated GPU will be selected when creating a the Device", 
    true);

static TAutoConsoleVariable<bool> CVarStablePowerState(
    "D3D12RHI.StablePowerState",
    "Enables stable power state on the GPU for more consistent profiling results. Requires developer mode enabled in Windows.",
    false);

static TAutoConsoleVariable<int32> CVarResourceOnlineDescriptorBlockSize(
    "D3D12RHI.ResourceOnlineDescriptorBlockSize",
    "Number of descriptors in each Resource OnlineDescriptorHeap", 
    2048);

static TAutoConsoleVariable<int32> CVarSamplerOnlineDescriptorBlockSize(
    "D3D12RHI.SamplerOnlineDescriptorBlockSize",
    "Number of descriptors in each Sampler OnlineDescriptorHeap", 
    256);

static TAutoConsoleVariable<bool> CVarEnableBindless(
    "D3D12RHI.EnableBindless",
    "When enabled, allocates a sub-region of the global descriptor heaps for bindless resources "
    "and emits root signatures with HEAP_DIRECTLY_INDEXED flags for shaders that need it. "
    "Has no effect when GD3D12SupportsBindless is false (requires SM 6.6 + Resource Binding Tier 3).",
    true);

static TAutoConsoleVariable<int32> CVarNumBindlessResourceDescriptors(
    "D3D12RHI.NumBindlessResourceDescriptors",
    "Number of CBV/SRV/UAV slots reserved at the start of the global resource descriptor heap "
    "for bindless resources. Clamped so that at least one block remains available for legacy "
    "descriptor tables.",
    100000);

static TAutoConsoleVariable<int32> CVarNumBindlessSamplerDescriptors(
    "D3D12RHI.NumBindlessSamplerDescriptors",
    "Number of sampler slots reserved at the start of the global sampler descriptor heap for "
    "bindless samplers. Clamped so that at least one block remains available for legacy tables.",
    256);

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

static TAutoConsoleVariable<int32> CVarNumPipelineStatsQueriesPerHeap(
    "D3D12RHI.NumPipelineStatsQueriesPerHeap",
    "Number of pipeline statistics queries in each query heap",
    D3D12_DEFAULT_QUERY_COUNT);

static TAutoConsoleVariable<int32> CVarMaxDefragMovesPerFrame(
    "D3D12RHI.MaxDefragMovesPerFrame",
    "Maximum number of resource defragmentation moves per frame (0 to disable)",
    4);

FD3D12Adapter::FD3D12Adapter()
    : AdapterIndex(0)
    , bAllowTearing(false)
    , bEnableDebugLayer(false)
    , Factory(nullptr)
#if DXGI_1_6
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
    
    // DRED does not require the debug layer. Must happen before device creation.
    D3D12RHIEnableDRED();

    // Debug layer, GPU-based validation, object auto-naming and DXGI InfoQueue break settings.
    D3D12RHISetupDebugInterfaces(bEnableDebugLayer);

    if (bEnableDebugLayer)
    {
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
    #if WIN10_BUILD_20348
        D3D_FEATURE_LEVEL_12_2,
    #endif
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    TComPtr<IDXGIAdapter1> FinalAdapter;

#if !DXGI_1_6
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
    , ResourceBindlessHeap(nullptr)
    , SamplerBindlessHeap(nullptr)
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
    , FrameFence(nullptr)
    , MinFeatureLevel(D3D_FEATURE_LEVEL_12_0)
    , ActiveFeatureLevel(D3D_FEATURE_LEVEL_11_0)
    , Adapter(InAdapter)
    , D3D12Device(nullptr)
#if D3D12_USE_ID3D12DEVICE_1
    , D3D12Device1(nullptr)
#endif
#if D3D12_USE_ID3D12DEVICE_2
    , D3D12Device2(nullptr)
#endif
#if D3D12_USE_ID3D12DEVICE_3
    , D3D12Device3(nullptr)
#endif
#if D3D12_USE_ID3D12DEVICE_4
    , D3D12Device4(nullptr)
#endif
#if D3D12_USE_ID3D12DEVICE_5
    , D3D12Device5(nullptr)
#endif
#if D3D12_USE_ID3D12DEVICE_6
    , D3D12Device6(nullptr)
#endif
#if D3D12_USE_ID3D12DEVICE_7
    , D3D12Device7(nullptr)
#endif
#if D3D12_USE_ID3D12DEVICE_8
    , D3D12Device8(nullptr)
#endif
#if D3D12_USE_ID3D12DEVICE_9
    , D3D12Device9(nullptr)
#endif
#if D3D12_USE_ID3D12DEVICE_10
    , D3D12Device10(nullptr)
#endif
#if D3D12_USE_ID3D12DEVICE_11
    , D3D12Device11(nullptr)
#endif
#if D3D12_USE_ID3D12DEVICE_12
    , D3D12Device12(nullptr)
#endif
#if D3D12_USE_ID3D12DEVICE_13
    , D3D12Device13(nullptr)
#endif
#if D3D12_USE_ID3D12DEVICE_14
    , D3D12Device14(nullptr)
#endif
    , NodeMask(0)
    , NodeCount(0)
    , TimingQueryHeapManager(nullptr)
    , OcclusionQueryHeapManager(nullptr)
    , PipelineStatsQueryHeapManager(nullptr)
#if D3D12_USE_DEBUG_MESSAGE_CALLBACK
    , DebugMessageCallbackCookie(0)
#endif
    , DeviceRemovedEvent(nullptr)
    , DeviceRemovedWait(nullptr)
{
    // Create CommandAllocatorManagers
    DirectCommandAllocatorManager  = new FD3D12CommandAllocatorManager(this, ED3D12CommandQueueType::Direct);
    CopyCommandAllocatorManager    = new FD3D12CommandAllocatorManager(this, ED3D12CommandQueueType::Copy);
    ComputeCommandAllocatorManager = new FD3D12CommandAllocatorManager(this, ED3D12CommandQueueType::Compute);
}

FD3D12Device::~FD3D12Device()
{
    // Tear down the device-removed notification first so the thread-pool callback cannot fire while the device is being destroyed.
    if (DeviceRemovedWait)
    {
        ::UnregisterWaitEx(DeviceRemovedWait, INVALID_HANDLE_VALUE); // block until callback drained
        DeviceRemovedWait = nullptr;
    }

    DeviceRemovedFence.Reset();
    if (DeviceRemovedEvent)
    {
        ::CloseHandle(DeviceRemovedEvent);
        DeviceRemovedEvent = nullptr;
    }

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
    SAFE_DELETE(PipelineStatsQueryHeapManager);

    // Destroy all CommandLists
    SAFE_DELETE(DirectQueue);
    SAFE_DELETE(ComputeQueue);
    SAFE_DELETE(CopyQueue);

    // Destroy all CommandAllocators
    SAFE_DELETE(DirectCommandAllocatorManager);
    SAFE_DELETE(CopyCommandAllocatorManager);
    SAFE_DELETE(ComputeCommandAllocatorManager);

    // Drain #1: release everything deferred so far (e.g. the DefaultDescriptors views reset
    // above, plus any allocator-backed resources) while the descriptor heaps AND allocators
    // are still alive.
    FD3D12DeviceRHI::FlushDeferredDeletions();

    // Release Heaps. Bindless heaps must be released before the global heaps they alias.
    SAFE_DELETE(ResourceBindlessHeap);
    SAFE_DELETE(SamplerBindlessHeap);
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

    FD3D12DeviceRHI::FlushDeferredDeletions();

    if (ResidencyManager)
    {
        delete ResidencyManager;
        ResidencyManager = nullptr;
    }

    // Release the rest of the managers
    SAFE_DELETE(RootSignatureManager);

    // Release device-owned GPU objects before the live-object report / device reset.
    for (TComPtr<ID3D12CommandSignature>& CommandSignature : CommandSignatures)
    {
        CommandSignature.Reset();
    }

    FrameFence.Reset();

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
   
    // Unregister debug callback
    UnregisterDebugMessageCallback();

    D3D12Device.Reset();
#if D3D12_USE_ID3D12DEVICE_1
    D3D12Device1.Reset();
#endif
#if D3D12_USE_ID3D12DEVICE_2
    D3D12Device2.Reset();
#endif
#if D3D12_USE_ID3D12DEVICE_3
    D3D12Device3.Reset();
#endif
#if D3D12_USE_ID3D12DEVICE_4
    D3D12Device4.Reset();
#endif
#if D3D12_USE_ID3D12DEVICE_5
    D3D12Device5.Reset();
#endif
#if D3D12_USE_ID3D12DEVICE_6
    D3D12Device6.Reset();
#endif
#if D3D12_USE_ID3D12DEVICE_7
    D3D12Device7.Reset();
#endif
#if D3D12_USE_ID3D12DEVICE_8
    D3D12Device8.Reset();
#endif
#if D3D12_USE_ID3D12DEVICE_9
    D3D12Device9.Reset();
#endif
#if D3D12_USE_ID3D12DEVICE_10
    D3D12Device10.Reset();
#endif
#if D3D12_USE_ID3D12DEVICE_11
    D3D12Device11.Reset();
#endif
#if D3D12_USE_ID3D12DEVICE_12
    D3D12Device12.Reset();
#endif
#if D3D12_USE_ID3D12DEVICE_13
    D3D12Device13.Reset();
#endif
#if D3D12_USE_ID3D12DEVICE_14
    D3D12Device14.Reset();
#endif
}

void FD3D12Device::BeginFrame(FD3D12CommandContext* InCommandContext)
{
    UNREFERENCED_VARIABLE(InCommandContext);

    FinalizePendingDefragMoves();

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
}

void FD3D12Device::EndFrame(FD3D12CommandContext* InCommandContext)
{
    CHECK(InCommandContext != nullptr);
    CHECK(InCommandContext->IsRecording());

    const int32 MaxMovesPerFrame = CVarMaxDefragMovesPerFrame.GetValue();
    int32 NumRecordedMoves = 0;

#if D3D12_TEXTURE_ALLOCATOR_USE_POOL_ALLOCATOR
    if (TextureAllocator)
    {
        NumRecordedMoves += TextureAllocator->RecordDefragMoves(InCommandContext, MaxMovesPerFrame);
    }
#endif

#if D3D12_BUFFER_ALLOCATOR_USE_POOL_ALLOCATOR
    if (BufferAllocator)
    {
        NumRecordedMoves += BufferAllocator->RecordDefragMoves(InCommandContext, MaxMovesPerFrame);
    }
#endif

    if (NumRecordedMoves <= 0)
    {
        return;
    }

    InCommandContext->SplitCommandList(true, false);

    const uint64 CompletionFenceValue = FrameFence->Signal(DirectQueue->GetD3D12CommandQueue());

#if D3D12_TEXTURE_ALLOCATOR_USE_POOL_ALLOCATOR
    if (TextureAllocator)
    {
        TextureAllocator->SetDefragCompletionFence(CompletionFenceValue);
    }
#endif

#if D3D12_BUFFER_ALLOCATOR_USE_POOL_ALLOCATOR
    if (BufferAllocator)
    {
        BufferAllocator->SetDefragCompletionFence(CompletionFenceValue);
    }
#endif
}

void FD3D12Device::FinalizePendingDefragMoves()
{
    uint64 CompletionFenceValue = 0;

#if D3D12_TEXTURE_ALLOCATOR_USE_POOL_ALLOCATOR
    if (TextureAllocator)
    {
        CompletionFenceValue = Math::Max(CompletionFenceValue, TextureAllocator->GetDefragCompletionFence());
    }
#endif

#if D3D12_BUFFER_ALLOCATOR_USE_POOL_ALLOCATOR
    if (BufferAllocator)
    {
        CompletionFenceValue = Math::Max(CompletionFenceValue, BufferAllocator->GetDefragCompletionFence());
    }
#endif

    if (CompletionFenceValue == 0)
    {
        return;
    }

    FrameFence->WaitForValue(CompletionFenceValue);
    DirectQueue->ProcessCommandQueue();

#if D3D12_TEXTURE_ALLOCATOR_USE_POOL_ALLOCATOR
    if (TextureAllocator)
    {
        TextureAllocator->FinalizeDefragMoves();
    }
#endif

#if D3D12_BUFFER_ALLOCATOR_USE_POOL_ALLOCATOR
    if (BufferAllocator)
    {
        BufferAllocator->FinalizeDefragMoves();
    }
#endif
}

void FD3D12Device::CancelPendingDefragMoves(FD3D12ResourceBase* Owner)
{
#if D3D12_TEXTURE_ALLOCATOR_USE_POOL_ALLOCATOR
    if (TextureAllocator)
    {
        TextureAllocator->CancelPendingDefragMoves(Owner);
    }
#endif

#if D3D12_BUFFER_ALLOCATOR_USE_POOL_ALLOCATOR
    if (BufferAllocator)
    {
        BufferAllocator->CancelPendingDefragMoves(Owner);
    }
#endif
}

bool FD3D12Device::Initialize()
{
    if (!CreateDevice())
    {
        return false;
    }

#if D3D12_ENABLE_PIPELINE_STATE_STREAM
    #if D3D12_USE_ID3D12DEVICE_2
        GD3D12SupportPipelineStream = (GetD3D12Device2() != nullptr);
    #else
        GD3D12SupportPipelineStream = false;
    #endif
#else
    GD3D12SupportPipelineStream = false;
#endif

    if (GD3D12SupportPipelineStream)
    {
        D3D12_INFO("[FD3D12Device]: Pipeline State Stream creation enabled (ID3D12Device2)");
    }
    else
    {
        D3D12_INFO("[FD3D12Device]: Pipeline State Stream creation disabled, using legacy pipeline state creation");
    }

    if (!CreateCommandQueues())
    {
        return false;
    }

    FrameFence = new FD3D12Fence(this);
    if (!FrameFence->Initialize(0))
    {
        D3D12_ERROR_CRITICAL("[FD3D12Device]: Failed to create FrameFence");
        return false;
    }

    FrameFence->SetDebugName("FrameFence");

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

    TimingQueryHeapManager        = new FD3D12QueryHeapManager(this, D3D12_QUERY_HEAP_TYPE_TIMESTAMP, CVarNumTimestampQueriesPerHeap.GetValue());
    OcclusionQueryHeapManager     = new FD3D12QueryHeapManager(this, D3D12_QUERY_HEAP_TYPE_OCCLUSION, CVarNumOcclusionQueriesPerHeap.GetValue());
    PipelineStatsQueryHeapManager = new FD3D12QueryHeapManager(this, GetPipelineStatsHeapType(), CVarNumPipelineStatsQueriesPerHeap.GetValue());

    // Create RootSignatureManager
    RootSignatureManager = new FD3D12RootSignatureManager(this);

    // Create DescriptorHeaps
    const bool bBindlessEnabled = GD3D12SupportsBindless && CVarEnableBindless.GetValue();

    const uint32 NumOnlineResourceDescriptors   = Math::Min<uint32>(D3D12_MAX_RESOURCE_ONLINE_DESCRIPTOR_COUNT, GD3D12MaxResourceDescriptorHeapSize);
    const uint32 ResourceDescriptorBlockSize    = Math::Min<uint32>(CVarResourceOnlineDescriptorBlockSize.GetValue(), NumOnlineResourceDescriptors);
    const uint32 RequestedBindlessResourceCount = bBindlessEnabled ? Math::Max<int32>(0, CVarNumBindlessResourceDescriptors.GetValue()) : 0u;
    const uint32 RequestedBindlessSamplerCount  = bBindlessEnabled ? Math::Max<int32>(0, CVarNumBindlessSamplerDescriptors.GetValue())  : 0u;

    const uint32 EffectiveBindlessResourceCount = (RequestedBindlessResourceCount > 0 && NumOnlineResourceDescriptors > ResourceDescriptorBlockSize)
        ? Math::Min<uint32>(RequestedBindlessResourceCount, NumOnlineResourceDescriptors - ResourceDescriptorBlockSize)
        : 0u;

    GlobalResourceHeap = new FD3D12OnlineDescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    if (!GlobalResourceHeap->Initialize(NumOnlineResourceDescriptors, ResourceDescriptorBlockSize, EffectiveBindlessResourceCount))
    {
        D3D12_ERROR("Failed to create global resource descriptor heap");
        return false;
    }

    if (EffectiveBindlessResourceCount > 0)
    {
        ResourceBindlessHeap = new FD3D12BindlessDescriptorHeap(*GlobalResourceHeap, EffectiveBindlessResourceCount);
        D3D12_INFO("[FD3D12Device]: Bindless resource heap. Capacity=%u (Requested=%u Heap=%u BlockSize=%u)",
            EffectiveBindlessResourceCount, RequestedBindlessResourceCount, NumOnlineResourceDescriptors, ResourceDescriptorBlockSize);
    }

    const uint32 NumOnlineSamplerDescriptors = Math::Min<uint32>(D3D12_MAX_SAMPLER_ONLINE_DESCRIPTOR_COUNT, GD3D12MaxSamplerDescriptorHeapSize);
    const uint32 SamplerDescriptorBlockSize  = Math::Min<uint32>(CVarSamplerOnlineDescriptorBlockSize.GetValue(), NumOnlineSamplerDescriptors);

    const uint32 EffectiveBindlessSamplerCount = (RequestedBindlessSamplerCount > 0 && NumOnlineSamplerDescriptors > SamplerDescriptorBlockSize)
        ? Math::Min<uint32>(RequestedBindlessSamplerCount, NumOnlineSamplerDescriptors - SamplerDescriptorBlockSize)
        : 0u;

    GlobalSamplerHeap = new FD3D12OnlineDescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    if (!GlobalSamplerHeap->Initialize(NumOnlineSamplerDescriptors, SamplerDescriptorBlockSize, EffectiveBindlessSamplerCount))
    {
        D3D12_ERROR("Failed to create global sampler descriptor heap");
        return false;
    }

    if (EffectiveBindlessSamplerCount > 0)
    {
        SamplerBindlessHeap = new FD3D12BindlessDescriptorHeap(*GlobalSamplerHeap, EffectiveBindlessSamplerCount);
        D3D12_INFO("[FD3D12Device]: Bindless sampler heap. Capacity=%u (Requested=%u Heap=%u BlockSize=%u)",
            EffectiveBindlessSamplerCount, RequestedBindlessSamplerCount, NumOnlineSamplerDescriptors, SamplerDescriptorBlockSize);
    }

    RHI::bSupportsBindless = bBindlessEnabled && (ResourceBindlessHeap != nullptr);

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
        
        BufferAllocator = new FD3D12BufferAllocator(this, BufferPageSizeBytes, D3D12_MIN_BUDDY_ALLOCATOR_BLOCK_SIZE, BufferMaxSuballocBytes);
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

    // Create indirect command signatures
    if (!CreateCommandSignatures())
    {
        return false;
    }

    // Create PipelineCache
    PipelineStateManager = new FD3D12PipelineStateManager(this);
    if (!PipelineStateManager->Initialize())
    {
        SAFE_DELETE(PipelineStateManager);
        GD3D12SupportPipelineCache = false;
        D3D12_WARNING("[FD3D12Device]: Pipeline cache initialization failed, continuing without cache");
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
        const String Description = Adapter->GetDescription();
        D3D12_INFO("[FD3D12Device]: Created Device for adapter '%s'", *Description);
    }

    if (CVarStablePowerState.GetValue())
    {
        HRESULT StablePowerResult = D3D12Device->SetStablePowerState(TRUE);
        if (SUCCEEDED(StablePowerResult))
        {
            D3D12_INFO("[FD3D12Device]: Stable power state enabled");
        }
        else
        {
            D3D12_WARNING("[FD3D12Device]: Failed to enable stable power state (HRESULT: 0x%08X). Ensure Windows Developer Mode is enabled.", StablePowerResult);
        }
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
            Memory::Memzero(&Filter);

            Filter.DenyList.NumIDs = ARRAY_COUNT(Hide);
            Filter.DenyList.pIDList = Hide;
            InfoQueue->AddStorageFilterEntries(&Filter);
        }

        RegisterDebugMessageCallback();
    }

#if D3D12_USE_ID3D12DEVICE_1
    if (FAILED(D3D12Device.GetAs<ID3D12Device1>(&D3D12Device1)))
    {
        D3D12_WARNING("[FD3D12Device]: Failed to retrieve ID3D12Device1");
    }
#endif

#if D3D12_USE_ID3D12DEVICE_2
    if (FAILED(D3D12Device.GetAs<ID3D12Device2>(&D3D12Device2)))
    {
        D3D12_WARNING("[FD3D12Device]: Failed to retrieve ID3D12Device2");
    }
#endif

#if D3D12_USE_ID3D12DEVICE_3
    if (FAILED(D3D12Device.GetAs<ID3D12Device3>(&D3D12Device3)))
    {
        D3D12_WARNING("[FD3D12Device]: Failed to retrieve ID3D12Device3");
    }
#endif

#if D3D12_USE_ID3D12DEVICE_4
    if (FAILED(D3D12Device.GetAs<ID3D12Device4>(&D3D12Device4)))
    {
        D3D12_WARNING("[FD3D12Device]: Failed to retrieve ID3D12Device4");
    }
#endif

#if D3D12_USE_ID3D12DEVICE_5
    if (FAILED(D3D12Device.GetAs<ID3D12Device5>(&D3D12Device5)))
    {
        D3D12_WARNING("[FD3D12Device]: Failed to retrieve ID3D12Device5");
    }
#endif

#if D3D12_USE_ID3D12DEVICE_6
    if (FAILED(D3D12Device.GetAs<ID3D12Device6>(&D3D12Device6)))
    {
        D3D12_WARNING("[FD3D12Device]: Failed to retrieve ID3D12Device6");
    }
#endif

#if D3D12_USE_ID3D12DEVICE_7
    if (FAILED(D3D12Device.GetAs<ID3D12Device7>(&D3D12Device7)))
    {
        D3D12_WARNING("[FD3D12Device]: Failed to retrieve ID3D12Device7");
    }
#endif

#if D3D12_USE_ID3D12DEVICE_8
    if (FAILED(D3D12Device.GetAs<ID3D12Device8>(&D3D12Device8)))
    {
        D3D12_WARNING("[FD3D12Device]: Failed to retrieve ID3D12Device8");
    }
#endif

#if D3D12_USE_ID3D12DEVICE_9
    if (FAILED(D3D12Device.GetAs<ID3D12Device9>(&D3D12Device9)))
    {
        D3D12_WARNING("[FD3D12Device]: Failed to retrieve ID3D12Device9");
    }
#endif

#if D3D12_USE_ID3D12DEVICE_10
    if (FAILED(D3D12Device.GetAs<ID3D12Device10>(&D3D12Device10)))
    {
        D3D12_WARNING("[FD3D12Device]: Failed to retrieve ID3D12Device10");
    }
#endif

#if D3D12_USE_ID3D12DEVICE_11
    if (FAILED(D3D12Device.GetAs<ID3D12Device11>(&D3D12Device11)))
    {
        D3D12_WARNING("[FD3D12Device]: Failed to retrieve ID3D12Device11");
    }
#endif

#if D3D12_USE_ID3D12DEVICE_12
    if (FAILED(D3D12Device.GetAs<ID3D12Device12>(&D3D12Device12)))
    {
        D3D12_WARNING("[FD3D12Device]: Failed to retrieve ID3D12Device12");
    }
#endif

#if D3D12_USE_ID3D12DEVICE_13
    if (FAILED(D3D12Device.GetAs<ID3D12Device13>(&D3D12Device13)))
    {
        D3D12_WARNING("[FD3D12Device]: Failed to retrieve ID3D12Device13");
    }
#endif

#if D3D12_USE_ID3D12DEVICE_14
    if (FAILED(D3D12Device.GetAs<ID3D12Device14>(&D3D12Device14)))
    {
        D3D12_WARNING("[FD3D12Device]: Failed to retrieve ID3D12Device14");
    }
#endif

    // Automated device-removed notification.
    RegisterDeviceRemovedEvent();

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
    if (!DefaultDescriptors.DefaultCBV->Initialize(nullptr, CBVDesc))
    {
        return false;
    }

    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
    UAVDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
    UAVDesc.Format               = DXGI_FORMAT_R8G8B8A8_UNORM;
    UAVDesc.Texture2D.MipSlice   = 0;
    UAVDesc.Texture2D.PlaneSlice = 0;

    DefaultDescriptors.DefaultUAV = new FD3D12UnorderedAccessViewRHI(this, GetResourceOfflineDescriptorHeap(), nullptr, FRHIUnorderedAccessViewDesc::CreateTexture2D(EFormat::R8G8B8A8_Unorm, 0));
    if (!DefaultDescriptors.DefaultUAV->Initialize(nullptr, nullptr, UAVDesc))
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

    DefaultDescriptors.DefaultSRV = new FD3D12ShaderResourceViewRHI(this, GetResourceOfflineDescriptorHeap(), nullptr, FRHIShaderResourceViewDesc::CreateTexture2D(EFormat::R8G8B8A8_Unorm, 0, 1));
    if (!DefaultDescriptors.DefaultSRV->Initialize(nullptr, SRVDesc))
    {
        return false;
    }

    D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = {};
    RTVDesc.ViewDimension        = D3D12_RTV_DIMENSION_TEXTURE2D;
    RTVDesc.Format               = DXGI_FORMAT_R8G8B8A8_UNORM;
    RTVDesc.Texture2D.MipSlice   = 0;
    RTVDesc.Texture2D.PlaneSlice = 0;

    DefaultDescriptors.DefaultRTV = new FD3D12RenderTargetViewRHI(this, GetRenderTargetOfflineDescriptorHeap(), nullptr, FRHIRenderTargetViewDesc::CreateTexture2D(EFormat::R8G8B8A8_Unorm, 0));
    if (!DefaultDescriptors.DefaultRTV->Initialize(nullptr, RTVDesc))
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

    DefaultDescriptors.DefaultSampler = new FD3D12SamplerStateRHI(this, GetSamplerOfflineDescriptorHeap(), FRHISamplerStateDesc());
    if (!DefaultDescriptors.DefaultSampler->CreateSampler(SamplerDesc))
    {
        return false;
    }

    return true;
}

bool FD3D12Device::CreateCommandSignatures()
{
    const auto CreateCommandSignature = [this](ED3D12CommandSignatureType::Type SignatureType, D3D12_INDIRECT_ARGUMENT_TYPE ArgumentType, uint32 ByteStride) -> bool
    {
        D3D12_INDIRECT_ARGUMENT_DESC IndirectArgumentDesc = {};
        IndirectArgumentDesc.Type = ArgumentType;

        D3D12_COMMAND_SIGNATURE_DESC CommandSignatureDesc = {};
        CommandSignatureDesc.ByteStride       = ByteStride;
        CommandSignatureDesc.NumArgumentDescs = 1;
        CommandSignatureDesc.pArgumentDescs   = &IndirectArgumentDesc;

        TComPtr<ID3D12CommandSignature> CommandSignature;
        const HRESULT Result = GetD3D12Device()->CreateCommandSignature(&CommandSignatureDesc, nullptr, IID_PPV_ARGS(&CommandSignature));
        if (FAILED(Result))
        {
            D3D12_ERROR("[FD3D12Device]: Failed to create indirect command signature (Type=%u)", uint32(ArgumentType));
            return false;
        }

        CommandSignatures[SignatureType] = CommandSignature;
        return true;
    };

    if (!CreateCommandSignature(ED3D12CommandSignatureType::Draw, D3D12_INDIRECT_ARGUMENT_TYPE_DRAW, sizeof(FRHIDrawIndirectParameters)) ||
        !CreateCommandSignature(ED3D12CommandSignatureType::DrawIndexed, D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED, sizeof(FRHIDrawIndexedIndirectParameters)) ||
        !CreateCommandSignature(ED3D12CommandSignatureType::Dispatch, D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH, sizeof(FRHIDispatchIndirectParameters)))
    {
        return false;
    }

#if D3D12_USE_ID3D12COMMANDLIST_6
    if (GD3D12MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED &&
        !CreateCommandSignature(ED3D12CommandSignatureType::DispatchMesh, D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_MESH, sizeof(FRHIDispatchMeshIndirectParameters)))
    {
        return false;
    }
#endif

#if D3D12_USE_ID3D12COMMANDLIST_4
    if (GD3D12RayTracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
    {
        if (!CreateCommandSignature(ED3D12CommandSignatureType::DispatchRays, D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH_RAYS, sizeof(FRHIDispatchRaysIndirectParameters)))
        {
            return false;
        }
    }
#endif

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

    OutResource = new FD3D12Resource(this, NewResource.ReleaseOwnership(), HeapType, InitialState, ClearValue);

#if D3D12_ENABLE_STATS
    {
        const uint64 Size = OutResource->GetAllocationSize();
        STAT_ADD(STAT_D3D12_CommittedResourceMemory, Size);
        STAT_ADD(STAT_D3D12_CommittedResourceCount, 1);

        switch (HeapType)
        {
        case D3D12_HEAP_TYPE_DEFAULT:  STAT_ADD(STAT_D3D12_CommittedDefaultMemory,  Size); break;
        case D3D12_HEAP_TYPE_UPLOAD:   STAT_ADD(STAT_D3D12_CommittedUploadMemory,   Size); break;
        case D3D12_HEAP_TYPE_READBACK: STAT_ADD(STAT_D3D12_CommittedReadbackMemory, Size); break;
        }
    }
#endif

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

    OutResource = new FD3D12Resource(this, NewResource.ReleaseOwnership(), Heap->GetHeapType(), InitialState, ClearValue, Heap);

    return true;
}

#if D3D12_USE_RESOURCE_DESC1
bool FD3D12Device::CreateCommittedResource2(const D3D12_RESOURCE_DESC1& Desc, D3D12_HEAP_TYPE HeapType, D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* ClearValue, FD3D12ResourceRef& OutResource)
{
    if (!GetD3D12Device8())
    {
        D3D12_ERROR("[FD3D12Device] CreateCommittedResource2 requires ID3D12Device8");
        return false;
    }

    D3D12_HEAP_PROPERTIES HeapProperties = {};
    HeapProperties.Type                 = HeapType;
    HeapProperties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProperties.VisibleNodeMask      = NodeMask ? NodeMask : 1;
    HeapProperties.CreationNodeMask     = NodeMask ? NodeMask : 1;

    TComPtr<ID3D12Resource2> NewResource;
    const HRESULT Result = GetD3D12Device8()->CreateCommittedResource2(
        &HeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &Desc,
        InitialState,
        ClearValue,
        nullptr, // No protected session
        IID_PPV_ARGS(&NewResource));

    if (FAILED(Result))
    {
        D3D12_ERROR("[FD3D12Device] CreateCommittedResource2 failed");
        return false;
    }

    OutResource = new FD3D12Resource(this, NewResource.ReleaseOwnership(), HeapType, InitialState, ClearValue);

#if D3D12_ENABLE_STATS
    {
        const uint64 Size = OutResource->GetAllocationSize();
        STAT_ADD(STAT_D3D12_CommittedResourceMemory, Size);
        STAT_ADD(STAT_D3D12_CommittedResourceCount, 1);

        switch (HeapType)
        {
        case D3D12_HEAP_TYPE_DEFAULT:  STAT_ADD(STAT_D3D12_CommittedDefaultMemory,  Size); break;
        case D3D12_HEAP_TYPE_UPLOAD:   STAT_ADD(STAT_D3D12_CommittedUploadMemory,   Size); break;
        case D3D12_HEAP_TYPE_READBACK: STAT_ADD(STAT_D3D12_CommittedReadbackMemory, Size); break;
        }
    }
#endif

    if (HeapType == D3D12_HEAP_TYPE_DEFAULT)
    {
        OutResource->StartResidencyTracking();
    }

    return true;
}

bool FD3D12Device::CreatePlacedResource1(FD3D12Heap* Heap, uint64 Offset, const D3D12_RESOURCE_DESC1& Desc, D3D12_RESOURCE_STATES InitialState, const D3D12_CLEAR_VALUE* ClearValue, FD3D12ResourceRef& OutResource)
{
    if (!Heap)
    {
        D3D12_ERROR("[FD3D12Device] CreatePlacedResource1 requires a valid Heap");
        return false;
    }

    if (!GetD3D12Device8())
    {
        D3D12_ERROR("[FD3D12Device] CreatePlacedResource1 requires ID3D12Device8");
        return false;
    }

    TComPtr<ID3D12Resource2> NewResource;
    const HRESULT Result = GetD3D12Device8()->CreatePlacedResource1(
        Heap->GetD3D12Heap(),
        Offset,
        &Desc,
        InitialState,
        ClearValue,
        IID_PPV_ARGS(&NewResource));

    if (FAILED(Result))
    {
        D3D12_ERROR("[FD3D12Device] CreatePlacedResource1 failed");
        return false;
    }

    OutResource = new FD3D12Resource(this, NewResource.ReleaseOwnership(), Heap->GetHeapType(), InitialState, ClearValue, Heap);
    return true;
}
#endif

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

bool FD3D12Device::QueryMultisampleQuality(DXGI_FORMAT Format, uint32 SampleCount, uint32& OutQuality)
{
    OutQuality = D3D12_DEFAULT_MULTISAMPLE_QUALITY;

    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS Data = {};
    Data.Flags       = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    Data.Format      = Format;
    Data.SampleCount = SampleCount;

    HRESULT hr = D3D12Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &Data, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
    if (FAILED(hr))
    {
        D3D12_ERROR("[FD3D12Device] CheckFeatureSupport failed");
        return false;
    }

    // Zero quality levels means the device does not support this sample-count for this format
    if (Data.NumQualityLevels == 0)
    {
        return false;
    }

    return true;
}

bool FD3D12Device::SupportsSwapChainFormat(DXGI_FORMAT DXGIFormat, ESwapChainUsageFlags Usage) const
{
    if (DXGIFormat == DXGI_FORMAT_UNKNOWN)
    {
        return false;
    }

    D3D12_FEATURE_DATA_FORMAT_SUPPORT FormatSupport = {};
    FormatSupport.Format = DXGIFormat;
    if (FAILED(D3D12Device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &FormatSupport, sizeof(FormatSupport))))
    {
        return false;
    }

    UINT RequiredSupport1 = D3D12_FORMAT_SUPPORT1_DISPLAY;
    if (IsEnumFlagSet(Usage, ESwapChainUsageFlags::RenderTarget))
    {
        RequiredSupport1 |= D3D12_FORMAT_SUPPORT1_RENDER_TARGET;
    }

    if (IsEnumFlagSet(Usage, ESwapChainUsageFlags::UnorderedAccess))
    {
        RequiredSupport1 |= D3D12_FORMAT_SUPPORT1_TYPED_UNORDERED_ACCESS_VIEW;
    }

    return (FormatSupport.Support1 & RequiredSupport1) == RequiredSupport1;
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

void FD3D12Device::WaitForGPU()
{
    const ED3D12CommandQueueType Types[] =
    {
        ED3D12CommandQueueType::Direct,
        ED3D12CommandQueueType::Compute,
        ED3D12CommandQueueType::Copy,
    };

    for (ED3D12CommandQueueType Type : Types)
    {
        if (FD3D12Queue* Queue = GetQueue(Type))
        {
            Queue->WaitForCompletion();
            Queue->ProcessCommandQueue();
        }
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
    else if (QueryType == EQueryType::PipelineStatistics)
    {
        return PipelineStatsQueryHeapManager;
    }
    else
    {
        return nullptr;
    }
}

FD3D12QueryHeap* FD3D12Device::ObtainQueryHeap(D3D12_QUERY_HEAP_TYPE HeapType)
{
    switch (HeapType)
    {
    case D3D12_QUERY_HEAP_TYPE_TIMESTAMP:
        return TimingQueryHeapManager ? TimingQueryHeapManager->ObtainHeap() : nullptr;
    case D3D12_QUERY_HEAP_TYPE_OCCLUSION:
        return OcclusionQueryHeapManager ? OcclusionQueryHeapManager->ObtainHeap() : nullptr;
    case D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS:
#ifdef D3D12_SUPPORT_PIPELINE_STATISTICS1
    case D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS1:
#endif
        return PipelineStatsQueryHeapManager ? PipelineStatsQueryHeapManager->ObtainHeap() : nullptr;
    default:
        return nullptr;
    }
}

void FD3D12Device::RecycleQueryHeap(FD3D12QueryHeap* Heap)
{
    if (!Heap)
    {
        return;
    }

    switch (Heap->QueryHeapType)
    {
        case D3D12_QUERY_HEAP_TYPE_TIMESTAMP:
        {
            if (TimingQueryHeapManager)
            {
                TimingQueryHeapManager->RecycleHeap(Heap);
            }
            
            break;
        }

        case D3D12_QUERY_HEAP_TYPE_OCCLUSION:
        {
            if (OcclusionQueryHeapManager)
            {
                OcclusionQueryHeapManager->RecycleHeap(Heap);
            }
            
            break;
        }

        case D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS:
    #ifdef D3D12_SUPPORT_PIPELINE_STATISTICS1
        case D3D12_QUERY_HEAP_TYPE_PIPELINE_STATISTICS1:
    #endif
        {
            if (PipelineStatsQueryHeapManager)
            {
                PipelineStatsQueryHeapManager->RecycleHeap(Heap);
            }
            
            break;
        }

        default:
            break;
    }
}

bool FD3D12Device::ReallocateGlobalDescriptorHeap(ED3D12GlobalDescriptorHeapType HeapType)
{
    FD3D12OnlineDescriptorHeap*   GlobalHeap   = nullptr;
    FD3D12BindlessDescriptorHeap* BindlessHeap = nullptr;
    uint32                        Cap          = 0;

    switch (HeapType)
    {
        case ED3D12GlobalDescriptorHeapType::Resource:
            GlobalHeap   = GlobalResourceHeap;
            BindlessHeap = ResourceBindlessHeap;
            Cap          = Math::Min<uint32>(D3D12_MAX_RESOURCE_ONLINE_DESCRIPTOR_COUNT, GD3D12MaxResourceDescriptorHeapSize);
            break;
        case ED3D12GlobalDescriptorHeapType::Sampler:
            GlobalHeap   = GlobalSamplerHeap;
            BindlessHeap = SamplerBindlessHeap;
            Cap          = Math::Min<uint32>(D3D12_MAX_SAMPLER_ONLINE_DESCRIPTOR_COUNT, GD3D12MaxSamplerDescriptorHeapSize);
            break;
    }

    CHECK(GlobalHeap != nullptr);

    if (GlobalHeap->GetNumDescriptors() >= Cap)
    {
        D3D12_ERROR("[FD3D12Device]: Cannot reallocate global %s descriptor heap -- already at cap of %u", ToString(HeapType), Cap);
        return false;
    }

    const uint32 BindlessReserved = GlobalHeap->GetBindlessReservedCount();
    if (!GlobalHeap->Reallocate(Cap, BindlessReserved))
    {
        return false;
    }

    if (BindlessHeap)
    {
        BindlessHeap->Rebuild(*GlobalHeap);
    }

    return true;
}
