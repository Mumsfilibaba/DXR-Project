#include "D3D12CommandContext.h"
#include "D3D12Device.h"
#include "D3D12CommandQueue.h"
#include "D3D12CommandList.h"
#include "D3D12Helpers.h"
#include "D3D12Buffer.h"
#include "D3D12Texture.h"
#include "D3D12PipelineState.h"
#include "D3D12ShaderCompiler.h"
#include "D3D12Shader.h"

#if D3D12_ENABLE_PIX_MARKERS
    #include <pix3.h>
#endif

D3D12ShaderDescriptorTableState::D3D12ShaderDescriptorTableState()
    : CBVOfflineHandles()
    , CBVDescriptorTable()
    , SRVOfflineHandles()
    , SRVDescriptorTable()
    , UAVOfflineHandles()
    , UAVDescriptorTable()
    , SamplerOfflineHandles()
    , SamplerDescriptorTable()
    , IsResourcesDirty(true)
    , IsSamplersDirty(true)
{
    SrcRangeSizes.Resize(D3D12_DEFAULT_DESCRIPTOR_TABLE_HANDLE_COUNT, 1);
    OfflineResourceHandles.Resize(D3D12_DEFAULT_DESCRIPTOR_TABLE_HANDLE_COUNT * 3);
}

Bool D3D12ShaderDescriptorTableState::CreateResources(D3D12Device& Device)
{
    const UInt32 NumDefaultResourceDescriptors = 4;
    DefaultResourceHeap = Device.CreateDescriptorHeap(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        NumDefaultResourceDescriptors,
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
    if (!DefaultResourceHeap->Init())
    {
        return false;
    }
    else
    {
        DefaultResourceHeap->SetName("[D3D12ShaderDescriptorTableState] Default Resource DescriptorHeap");
    }

    const UInt32 NumDefaultSamplerDescriptors = 2;
    DefaultSamplerHeap = Device.CreateDescriptorHeap(
        D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
        NumDefaultSamplerDescriptors,
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
    if (!DefaultSamplerHeap->Init())
    {
        return false;
    }
    else
    {
        DefaultSamplerHeap->SetName("[D3D12ShaderDescriptorTableState] Default Sampler DescriptorHeap");
    }

    const UInt32 ResourceDescriptorHandleIncrementSize           = DefaultResourceHeap->GetDescriptorHandleIncrementSize();
    const D3D12_CPU_DESCRIPTOR_HANDLE ResourceHandleForHeapStart = DefaultResourceHeap->GetCPUDescriptorHandleForHeapStart();

    D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
    Memory::Memzero(&CBVDesc);

    CBVDesc.BufferLocation = 0;
    CBVDesc.SizeInBytes    = 0;

    DefaultCBVOfflineHandle = ResourceHandleForHeapStart;
    Device.CreateConstantBufferView(&CBVDesc, DefaultCBVOfflineHandle);

    D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc;
    Memory::Memzero(&SrvDesc);

    SrvDesc.Format                        = DXGI_FORMAT_R8G8B8A8_UNORM;
    SrvDesc.Shader4ComponentMapping       = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    SrvDesc.ViewDimension                 = D3D12_SRV_DIMENSION_TEXTURE2D;
    SrvDesc.Texture2D.MipLevels           = 1;
    SrvDesc.Texture2D.MostDetailedMip     = 0;
    SrvDesc.Texture2D.PlaneSlice          = 0;
    SrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    DefaultSRVOfflineHandle = D3D12_DESCRIPTOR_HANDLE_INCREMENT(DefaultCBVOfflineHandle, ResourceDescriptorHandleIncrementSize);
    Device.CreateShaderResourceView(nullptr, &SrvDesc, DefaultSRVOfflineHandle);

    D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc;
    Memory::Memzero(&UavDesc);

    UavDesc.Format               = DXGI_FORMAT_R8G8B8A8_UNORM;
    UavDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
    UavDesc.Texture2D.MipSlice   = 0;
    UavDesc.Texture2D.PlaneSlice = 0;

    DefaultUAVOfflineHandle = D3D12_DESCRIPTOR_HANDLE_INCREMENT(DefaultSRVOfflineHandle, ResourceDescriptorHandleIncrementSize);
    Device.CreateUnorderedAccessView(nullptr, nullptr, &UavDesc, DefaultUAVOfflineHandle);

    const D3D12_CPU_DESCRIPTOR_HANDLE SamplerHandleForHeapStart = DefaultSamplerHeap->GetCPUDescriptorHandleForHeapStart();

    D3D12_SAMPLER_DESC SamplerDesc;
    Memory::Memzero(&SamplerDesc);

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
    SamplerDesc.MaxLOD         = FLT_MAX;
    SamplerDesc.MinLOD         = -FLT_MAX;
    SamplerDesc.MipLODBias     = 0.0f;

    DefaultSamplerOfflineHandle = SamplerHandleForHeapStart;
    Device.CreateSampler(&SamplerDesc, DefaultSamplerOfflineHandle);

    CBVOfflineHandles.Resize(D3D12_DEFAULT_DESCRIPTOR_TABLE_HANDLE_COUNT, DefaultCBVOfflineHandle);
    SRVOfflineHandles.Resize(D3D12_DEFAULT_DESCRIPTOR_TABLE_HANDLE_COUNT, DefaultSRVOfflineHandle);
    UAVOfflineHandles.Resize(D3D12_DEFAULT_DESCRIPTOR_TABLE_HANDLE_COUNT, DefaultUAVOfflineHandle);
    SamplerOfflineHandles.Resize(D3D12_DEFAULT_DESCRIPTOR_TABLE_HANDLE_COUNT, DefaultSamplerOfflineHandle);

    DescriptorHeaps.Fill(nullptr);
    BoundGraphicsDescriptorTables.Fill({ 0 });
    BoundComputeDescriptorTables.Fill({ 0 });

    return true;
}

void D3D12ShaderDescriptorTableState::BindConstantBuffer(D3D12ConstantBufferView* ConstantBufferView, UInt32 Slot)
{
    if (Slot >= CBVOfflineHandles.Size())
    {
        CBVOfflineHandles.Resize(Slot + 1, DefaultCBVOfflineHandle);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE OfflineHandle = ConstantBufferView ? ConstantBufferView->GetOfflineHandle() : DefaultCBVOfflineHandle;
    if (CBVOfflineHandles[Slot] != OfflineHandle)
    {
        CBVOfflineHandles[Slot] = OfflineHandle;
        IsResourcesDirty = true;
    }
}

void D3D12ShaderDescriptorTableState::BindShaderResourceView(D3D12ShaderResourceView* ShaderResourceView, UInt32 Slot)
{
    if (Slot >= SRVOfflineHandles.Size())
    {
        SRVOfflineHandles.Resize(Slot + 1, DefaultSRVOfflineHandle);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE OfflineHandle = ShaderResourceView ? ShaderResourceView->GetOfflineHandle() : DefaultSRVOfflineHandle;
    if (SRVOfflineHandles[Slot] != OfflineHandle)
    {
        SRVOfflineHandles[Slot] = OfflineHandle;
        IsResourcesDirty = true;
    }
}

void D3D12ShaderDescriptorTableState::BindUnorderedAccessView(D3D12UnorderedAccessView* UnorderedAccessView, UInt32 Slot)
{
    if (Slot >= UAVOfflineHandles.Size())
    {
        UAVOfflineHandles.Resize(Slot + 1, DefaultUAVOfflineHandle);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE OfflineHandle = UnorderedAccessView ? UnorderedAccessView->GetOfflineHandle() : DefaultUAVOfflineHandle;
    if (UAVOfflineHandles[Slot] != OfflineHandle)
    {
        UAVOfflineHandles[Slot] = OfflineHandle;
        IsResourcesDirty = true;
    }
}

void D3D12ShaderDescriptorTableState::BindSamplerState(D3D12SamplerState* SamplerState, UInt32 Slot)
{
    if (Slot >= SamplerOfflineHandles.Size())
    {
        SamplerOfflineHandles.Resize(Slot + 1, DefaultSamplerOfflineHandle);
    }

    D3D12_CPU_DESCRIPTOR_HANDLE OfflineHandle = SamplerState ? SamplerState->GetOfflineHandle() : DefaultSamplerOfflineHandle;
    if (SamplerOfflineHandles[Slot] != OfflineHandle)
    {
        SamplerOfflineHandles[Slot] = OfflineHandle;
        IsSamplersDirty = true;
    }
}

void D3D12ShaderDescriptorTableState::CommitGraphicsDescriptorTables(
    D3D12Device& Device,
    D3D12OnlineDescriptorHeap& ResourceDescriptorHeap,
    D3D12OnlineDescriptorHeap& SamplerDescriptorHeap,
    D3D12CommandListHandle& CmdList)
{
    InternalAllocateAndCopyDescriptorHandles(Device, ResourceDescriptorHeap, SamplerDescriptorHeap);

    Bool ForceRebind = false;
    if (DescriptorHeaps[0] != ResourceDescriptorHeap.GetNativeHeap() ||
        DescriptorHeaps[1] != SamplerDescriptorHeap.GetNativeHeap())
    {
        DescriptorHeaps[0] = ResourceDescriptorHeap.GetNativeHeap();
        DescriptorHeaps[1] = SamplerDescriptorHeap.GetNativeHeap();
        CmdList.SetDescriptorHeaps(DescriptorHeaps.Data(), DescriptorHeaps.Size());

        ForceRebind = true;
    }

    if (CBVDescriptorTable.OnlineHandleStart_GPU != BoundGraphicsDescriptorTables[0] || ForceRebind)
    {
        CmdList.SetGraphicsRootDescriptorTable(CBVDescriptorTable.OnlineHandleStart_GPU, D3D12_DEFAULT_CONSTANT_BUFFER_ROOT_PARAMETER);
        BoundGraphicsDescriptorTables[0] = CBVDescriptorTable.OnlineHandleStart_GPU;
    }

    if (SRVDescriptorTable.OnlineHandleStart_GPU != BoundGraphicsDescriptorTables[1] || ForceRebind)
    {
        CmdList.SetGraphicsRootDescriptorTable(SRVDescriptorTable.OnlineHandleStart_GPU, D3D12_DEFAULT_SHADER_RESOURCE_VIEW_ROOT_PARAMETER);
        BoundGraphicsDescriptorTables[1] = SRVDescriptorTable.OnlineHandleStart_GPU;
    }

    if (UAVDescriptorTable.OnlineHandleStart_GPU != BoundGraphicsDescriptorTables[2] || ForceRebind)
    {
        CmdList.SetGraphicsRootDescriptorTable(UAVDescriptorTable.OnlineHandleStart_GPU, D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_ROOT_PARAMETER);
        BoundGraphicsDescriptorTables[2] = UAVDescriptorTable.OnlineHandleStart_GPU;
    }

    if (SamplerDescriptorTable.OnlineHandleStart_GPU != BoundGraphicsDescriptorTables[3] || ForceRebind)
    {
        CmdList.SetGraphicsRootDescriptorTable(SamplerDescriptorTable.OnlineHandleStart_GPU, D3D12_DEFAULT_SAMPLER_STATE_ROOT_PARAMETER);
        BoundGraphicsDescriptorTables[3] = SamplerDescriptorTable.OnlineHandleStart_GPU;
    }
}

void D3D12ShaderDescriptorTableState::CommitComputeDescriptorTables(
    D3D12Device& Device, 
    D3D12OnlineDescriptorHeap& ResourceDescriptorHeap,
    D3D12OnlineDescriptorHeap& SamplerDescriptorHeap,
    D3D12CommandListHandle& CmdList)
{
    InternalAllocateAndCopyDescriptorHandles(Device, ResourceDescriptorHeap, SamplerDescriptorHeap);

    Bool ForceRebind = false;
    if (DescriptorHeaps[0] != ResourceDescriptorHeap.GetNativeHeap() ||
        DescriptorHeaps[1] != SamplerDescriptorHeap.GetNativeHeap())
    {
        DescriptorHeaps[0] = ResourceDescriptorHeap.GetNativeHeap();
        DescriptorHeaps[1] = SamplerDescriptorHeap.GetNativeHeap();
        CmdList.SetDescriptorHeaps(DescriptorHeaps.Data(), DescriptorHeaps.Size());

        ForceRebind = true;
    }

    if (CBVDescriptorTable.OnlineHandleStart_GPU != BoundComputeDescriptorTables[0] || ForceRebind)
    {
        CmdList.SetComputeRootDescriptorTable(CBVDescriptorTable.OnlineHandleStart_GPU, D3D12_DEFAULT_CONSTANT_BUFFER_ROOT_PARAMETER);
        BoundComputeDescriptorTables[0] = CBVDescriptorTable.OnlineHandleStart_GPU;
    }

    if (SRVDescriptorTable.OnlineHandleStart_GPU != BoundComputeDescriptorTables[1] || ForceRebind)
    {
        CmdList.SetComputeRootDescriptorTable(SRVDescriptorTable.OnlineHandleStart_GPU, D3D12_DEFAULT_SHADER_RESOURCE_VIEW_ROOT_PARAMETER);
        BoundComputeDescriptorTables[1] = SRVDescriptorTable.OnlineHandleStart_GPU;
    }

    if (UAVDescriptorTable.OnlineHandleStart_GPU != BoundComputeDescriptorTables[2] || ForceRebind)
    {
        CmdList.SetComputeRootDescriptorTable(UAVDescriptorTable.OnlineHandleStart_GPU, D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_ROOT_PARAMETER);
        BoundComputeDescriptorTables[2] = UAVDescriptorTable.OnlineHandleStart_GPU;
    }

    if (SamplerDescriptorTable.OnlineHandleStart_GPU != BoundComputeDescriptorTables[3] || ForceRebind)
    {
        CmdList.SetComputeRootDescriptorTable(SamplerDescriptorTable.OnlineHandleStart_GPU, D3D12_DEFAULT_SAMPLER_STATE_ROOT_PARAMETER);
        BoundComputeDescriptorTables[3] = SamplerDescriptorTable.OnlineHandleStart_GPU;
    }
}

void D3D12ShaderDescriptorTableState::InternalAllocateAndCopyDescriptorHandles(
    D3D12Device& Device, 
    D3D12OnlineDescriptorHeap& ResourceDescriptorHeap,
    D3D12OnlineDescriptorHeap& SamplerDescriptorHeap)
{
    const UInt32 NumDescriptorHandles = D3D12_DEFAULT_DESCRIPTOR_TABLE_HANDLE_COUNT;
    
    Bool ForceRealloc = DescriptorHeaps[0] != ResourceDescriptorHeap.GetNativeHeap();
    if (IsResourcesDirty || ForceRealloc)
    {
        const UInt32 NumResourceDescriptorHandles = NumDescriptorHandles * 3;
        const UInt32 StartHandle = ResourceDescriptorHeap.AllocateHandles(NumResourceDescriptorHandles);

        CBVDescriptorTable.SetStart(
            ResourceDescriptorHeap.GetGPUDescriptorHandleAt(StartHandle),
            ResourceDescriptorHeap.GetCPUDescriptorHandleAt(StartHandle));
        SRVDescriptorTable.SetStart(
            ResourceDescriptorHeap.GetGPUDescriptorHandleAt(StartHandle + NumDescriptorHandles),
            ResourceDescriptorHeap.GetCPUDescriptorHandleAt(StartHandle + NumDescriptorHandles));
        UAVDescriptorTable.SetStart(
            ResourceDescriptorHeap.GetGPUDescriptorHandleAt(StartHandle + NumDescriptorHandles * 2),
            ResourceDescriptorHeap.GetCPUDescriptorHandleAt(StartHandle + NumDescriptorHandles * 2));

        if (OfflineResourceHandles.Size() < NumResourceDescriptorHandles)
        {
            OfflineResourceHandles.Resize(NumResourceDescriptorHandles);
        }

        if (SrcRangeSizes.Size() < NumResourceDescriptorHandles)
        {
            SrcRangeSizes.Resize(NumResourceDescriptorHandles, 1);
        }

        Memory::Memcpy(
            OfflineResourceHandles.Data(), 
            CBVOfflineHandles.Data(), 
            NumDescriptorHandles * sizeof(D3D12_CPU_DESCRIPTOR_HANDLE));
        
        Memory::Memcpy(
            OfflineResourceHandles.Data() + NumDescriptorHandles, 
            SRVOfflineHandles.Data(), 
            NumDescriptorHandles * sizeof(D3D12_CPU_DESCRIPTOR_HANDLE));
        
        Memory::Memcpy(
            OfflineResourceHandles.Data() + (NumDescriptorHandles * 2), 
            UAVOfflineHandles.Data(), 
            NumDescriptorHandles * sizeof(D3D12_CPU_DESCRIPTOR_HANDLE));

        Device.CopyDescriptors(
            1, 
            &CBVDescriptorTable.OnlineHandleStart_CPU, 
            &NumResourceDescriptorHandles,
            NumResourceDescriptorHandles,
            OfflineResourceHandles.Data(),
            SrcRangeSizes.Data(),
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        IsResourcesDirty = false;
    }

    ForceRealloc = DescriptorHeaps[1] != SamplerDescriptorHeap.GetNativeHeap();
    if (IsSamplersDirty || ForceRealloc)
    {
        const UInt32 StartHandle = SamplerDescriptorHeap.AllocateHandles(NumDescriptorHandles);
        SamplerDescriptorTable.SetStart(
            SamplerDescriptorHeap.GetGPUDescriptorHandleAt(StartHandle),
            SamplerDescriptorHeap.GetCPUDescriptorHandleAt(StartHandle));

        Device.CopyDescriptors(
            1,
            &SamplerDescriptorTable.OnlineHandleStart_CPU,
            &NumDescriptorHandles,
            NumDescriptorHandles,
            SamplerOfflineHandles.Data(),
            SrcRangeSizes.Data(),
            D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

        IsSamplersDirty = false;
    }
}

Bool D3D12ResourceBarrierBatcher::Init()
{
    return Bool();
}

void D3D12ResourceBarrierBatcher::AddTransitionBarrier(
    ID3D12Resource* Resource, 
    D3D12_RESOURCE_STATES BeforeState, 
    D3D12_RESOURCE_STATES AfterState)
{
    VALIDATE(Resource != nullptr);

    if (BeforeState != AfterState)
    {
        // Make sure we are not already have transition for this resource
        for (TArray<D3D12_RESOURCE_BARRIER>::Iterator It = Barriers.Begin(); It != Barriers.End(); It++)
        {
            if ((*It).Type == D3D12_RESOURCE_BARRIER_TYPE_TRANSITION)
            {
                if ((*It).Transition.pResource == Resource)
                {
                    if ((*It).Transition.StateBefore != AfterState)
                    {
                        (*It).Transition.StateAfter = AfterState;
                    }
                    else
                    {
                        Barriers.Erase(It);
                    }

                    return;
                }
            }
        }

        // Add new resource barrier
        D3D12_RESOURCE_BARRIER Barrier;
        Memory::Memzero(&Barrier);

        Barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        Barrier.Transition.pResource   = Resource;
        Barrier.Transition.StateAfter  = AfterState;
        Barrier.Transition.StateBefore = BeforeState;
        Barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        Barriers.EmplaceBack(Barrier);
    }
}

void D3D12ResourceBarrierBatcher::AddTransitionBarrier(
    D3D12Resource* Resource, 
    D3D12_RESOURCE_STATES BeforeState, 
    D3D12_RESOURCE_STATES AfterState)
{
    AddTransitionBarrier(Resource->GetNativeResource(), BeforeState, AfterState);
}

Bool D3D12GPUResourceUploader::Reserve(UInt32 InSizeInBytes)
{
    if (InSizeInBytes == SizeInBytes)
    {
        return true;
    }

    if (Resource)
    {
        Resource->Unmap(0, nullptr);
        GarbageResources.EmplaceBack(Resource);
        Resource.Reset();
    }

    D3D12_HEAP_PROPERTIES HeapProperties;
    Memory::Memzero(&HeapProperties);

    HeapProperties.Type                 = D3D12_HEAP_TYPE_UPLOAD;
    HeapProperties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

    D3D12_RESOURCE_DESC Desc;
    Memory::Memzero(&Desc);

    Desc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    Desc.Flags              = D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    Desc.Format             = DXGI_FORMAT_UNKNOWN;
    Desc.Width              = InSizeInBytes;
    Desc.Height             = 1;
    Desc.DepthOrArraySize   = 1;
    Desc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    Desc.MipLevels          = 1;
    Desc.SampleDesc.Count   = 1;
    Desc.SampleDesc.Quality = 0;

    HRESULT Result = Device->CreateCommitedResource(
        &HeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &Desc, 
        D3D12_RESOURCE_STATE_GENERIC_READ, 
        nullptr,
        IID_PPV_ARGS(&Resource));
    if (SUCCEEDED(Result))
    {
        Resource->SetName(L"[D3D12GPUResourceUploader] Buffer");
        Resource->Map(0, nullptr, reinterpret_cast<Void**>(&MappedMemory));
        
        SizeInBytes        = InSizeInBytes;
        OffsetInBytes    = 0;
        return true;
    }
    else
    {
        return false;
    }
}

void D3D12GPUResourceUploader::Reset()
{
    constexpr UInt32 MAX_RESERVED_GARBAGE_RESOURCES = 5;
    constexpr UInt32 NEW_RESERVED_GARBAGE_RESOURCES = 2;

    // Clear garbage resource, and release memory we do not need
    GarbageResources.Clear();
    if (GarbageResources.Capacity() >= MAX_RESERVED_GARBAGE_RESOURCES)
    {
        GarbageResources.Reserve(NEW_RESERVED_GARBAGE_RESOURCES);
    }

    // Reset memory offset
    OffsetInBytes = 0;
}

Byte* D3D12GPUResourceUploader::LinearAllocate(UInt32 InSizeInBytes)
{
    constexpr UInt32 EXTRA_BYTES_ALLOCATED = 1024;

    const UInt32 NeededSize = OffsetInBytes + InSizeInBytes;
    if (NeededSize > SizeInBytes)
    {
        Reserve(NeededSize + EXTRA_BYTES_ALLOCATED);
    }

    Byte* ResultPtr = MappedMemory + OffsetInBytes;
    OffsetInBytes += InSizeInBytes;
    return ResultPtr;
}

Bool D3D12CommandBatch::Init()
{
    // TODO: Do not have D3D12_COMMAND_LIST_TYPE_DIRECT directly
    if (!CmdAllocator.Init(D3D12_COMMAND_LIST_TYPE_DIRECT))
    {
        return false;
    }

    OnlineResourceDescriptorHeap = DBG_NEW D3D12OnlineDescriptorHeap(
        Device,
        D3D12_DEFAULT_ONLINE_RESOURCE_DESCRIPTOR_HEAP_COUNT,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    if (!OnlineResourceDescriptorHeap->Init())
    {
        return false;
    }

    OnlineSamplerDescriptorHeap = DBG_NEW D3D12OnlineDescriptorHeap(Device,
        D3D12_DEFAULT_ONLINE_SAMPLER_DESCRIPTOR_HEAP_COUNT,
        D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    if (!OnlineSamplerDescriptorHeap->Init())
    {
        return false;
    }

    GpuResourceUploader.Reserve(1024);
    return true;
}

D3D12CommandContext::D3D12CommandContext(D3D12Device* InDevice, const D3D12DefaultRootSignatures& InDefaultRootSignatures)
    : ICommandContext()
    , D3D12DeviceChild(InDevice)
    , CmdQueue(InDevice)
    , CmdList(InDevice)
    , Fence(InDevice)
    , CmdBatches()
    , DefaultRootSignatures(InDefaultRootSignatures)
    , VertexBufferState()
    , RenderTargetState()
    , BarrierBatcher()
{
}

D3D12CommandContext::~D3D12CommandContext()
{
    Flush();
}

Bool D3D12CommandContext::Init()
{
    if (!CmdQueue.Init(D3D12_COMMAND_LIST_TYPE_DIRECT))
    {
        return false;
    }

    // TODO: Have support for more than 4 commandbatches?
    for (UInt32 i = 0; i < 4; i++)
    {
        D3D12CommandBatch& Batch = CmdBatches.EmplaceBack(Device);
        if (!Batch.Init())
        {
            return false;
        }
    }

    if (!CmdList.Init(D3D12_COMMAND_LIST_TYPE_DIRECT, CmdBatches[0].GetCommandAllocator(), nullptr))
    {
        return false;
    }

    FenceValue = 0;
    if (!Fence.Init(FenceValue))
    {
        return false;
    }

    if (!ShaderDescriptorState.CreateResources(*Device))
    {
        return false;
    }

    TArray<UInt8> Code;
    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/GenerateMipsTex2D.hlsl",
        "Main",
        nullptr,
        EShaderStage::ShaderStage_Compute,
        EShaderModel::ShaderModel_6_0,
        Code))
    {
        LOG_ERROR("[D3D12CommandContext]: Failed to compile GenerateMipsTex2D Shader");
        Debug::DebugBreak();
    }

    TSharedRef<D3D12ComputeShader> Shader = DBG_NEW D3D12ComputeShader(Device, Code);
    Shader->CreateRootSignature();

    TSharedRef<D3D12RootSignature> RootSignature = MakeSharedRef<D3D12RootSignature>(Shader->GetRootSignature());

    GenerateMipsTex2D_PSO = DBG_NEW D3D12ComputePipelineState(Device, Shader, RootSignature);
    if (!GenerateMipsTex2D_PSO->Init())
    {
        LOG_ERROR("[D3D12CommandContext]: Failed to create GenerateMipsTex2D PipelineState");
        return false;
    }
    else
    {
        GenerateMipsTex2D_PSO->SetName("GenerateMipsTex2D Gen PSO");
    }

    if (!ShaderCompiler::CompileFromFile(
        "../DXR-Engine/Shaders/GenerateMipsTexCube.hlsl",
        "Main",
        nullptr,
        EShaderStage::ShaderStage_Compute,
        EShaderModel::ShaderModel_6_0,
        Code))
    {
        LOG_ERROR("[D3D12CommandContext]: Failed to compile GenerateMipsTexCube Shader");
        Debug::DebugBreak();
    }

    Shader = DBG_NEW D3D12ComputeShader(Device, Code);
    if (!Shader->CreateRootSignature())
    {
        return false;
    }

    RootSignature = MakeSharedRef<D3D12RootSignature>(Shader->GetRootSignature());

    GenerateMipsTexCube_PSO = DBG_NEW D3D12ComputePipelineState(Device, Shader, RootSignature);
    if (!GenerateMipsTexCube_PSO->Init())
    {
        LOG_ERROR("[D3D12CommandContext]: Failed to create GenerateMipsTexCube PipelineState");
        return false;
    }
    else
    {
        GenerateMipsTexCube_PSO->SetName("GenerateMipsTexCube Gen PSO");
    }

    return true;
}

void D3D12CommandContext::Begin()
{
    VALIDATE(IsReady == false);

    CmdBatch = &CmdBatches[NextCmdAllocator];
    NextCmdAllocator = (NextCmdAllocator + 1) % CmdBatches.Size();

    if (FenceValue >= CmdBatches.Size())
    {
        const UInt64 WaitValue = Math::Max(FenceValue - (CmdBatches.Size() - 1), 0ULL);
        Fence.WaitForValue(WaitValue);
    }

    if (!CmdBatch->Reset())
    {
        return;
    }

    ClearState();

    if (!CmdList.Reset(CmdBatch->GetCommandAllocator()))
    {
        return;
    }

    IsReady = true;
}

void D3D12CommandContext::End()
{
    VALIDATE(IsReady == true);

    BarrierBatcher.FlushBarriers(CmdList);

    if (!CmdList.Close())
    {
        return;
    }

    CmdQueue.ExecuteCommandList(&CmdList);

    const UInt64 CurrentFenceValue = ++FenceValue;
    if (!CmdQueue.SignalFence(Fence, CurrentFenceValue))
    {
        return;
    }

    // Reset state
    CmdBatch = nullptr;
    IsReady  = false;

    CurrentGraphicsPipelineState.Reset();
    CurrentGraphicsRootSignature.Reset();
    CurrentComputePipelineState.Reset();
    CurrentComputeRootSignature.Reset();
}

void D3D12CommandContext::ClearRenderTargetView(RenderTargetView* RenderTargetView, const ColorClearValue& ClearColor)
{
    VALIDATE(RenderTargetView != nullptr);

    D3D12RenderTargetView* DxRenderTargetView = static_cast<D3D12RenderTargetView*>(RenderTargetView);
    BarrierBatcher.FlushBarriers(CmdList);
    CmdList.ClearRenderTargetView(DxRenderTargetView->GetOfflineHandle(), ClearColor.RGBA, 0, nullptr);
}

void D3D12CommandContext::ClearDepthStencilView(DepthStencilView* DepthStencilView, const DepthStencilClearValue& ClearValue) 
{
    VALIDATE(DepthStencilView != nullptr);

    D3D12DepthStencilView* DxDepthStencilView = static_cast<D3D12DepthStencilView*>(DepthStencilView);
    BarrierBatcher.FlushBarriers(CmdList);
    CmdList.ClearDepthStencilView(
        DxDepthStencilView->GetOfflineHandle(),
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
        ClearValue.Depth, 
        ClearValue.Stencil);
}

void D3D12CommandContext::ClearUnorderedAccessViewFloat(UnorderedAccessView* UnorderedAccessView, const Float ClearColor[4])
{
    D3D12UnorderedAccessView* DxUnorderedAccessView = static_cast<D3D12UnorderedAccessView*>(UnorderedAccessView);
    BarrierBatcher.FlushBarriers(CmdList);

    D3D12OnlineDescriptorHeap* OnlineDescriptorHeap = CmdBatch->GetOnlineResourceDescriptorHeap();
    const UInt32 OnlineDescriptorHandleIndex = OnlineDescriptorHeap->AllocateHandles(1);

    const D3D12_CPU_DESCRIPTOR_HANDLE OfflineHandle    = DxUnorderedAccessView->GetOfflineHandle();
    const D3D12_CPU_DESCRIPTOR_HANDLE OnlineHandle_CPU = OnlineDescriptorHeap->GetCPUDescriptorHandleAt(OnlineDescriptorHandleIndex);
    Device->CopyDescriptorsSimple(1, OnlineHandle_CPU, OfflineHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    const D3D12_GPU_DESCRIPTOR_HANDLE OnlineHandle_GPU = OnlineDescriptorHeap->GetGPUDescriptorHandleAt(OnlineDescriptorHandleIndex);
    CmdList.ClearUnorderedAccessViewFloat(OnlineHandle_GPU, DxUnorderedAccessView, ClearColor);
}

void D3D12CommandContext::SetShadingRate(EShadingRate ShadingRate)
{
    D3D12_SHADING_RATE DxShadingRate = ConvertShadingRate(ShadingRate);
    CmdList.RSSetShadingRate(DxShadingRate, nullptr);
}

void D3D12CommandContext::BeginRenderPass()
{
    // Empty for now
}

void D3D12CommandContext::EndRenderPass()
{
    // Empty for now
}

void D3D12CommandContext::BindViewport(
    Float Width, 
    Float Height, 
    Float MinDepth, 
    Float MaxDepth, 
    Float x, 
    Float y)
{
    D3D12_VIEWPORT Viewport;
    Viewport.Width    = Width;
    Viewport.Height   = Height;
    Viewport.MaxDepth = MaxDepth;
    Viewport.MinDepth = MinDepth;
    Viewport.TopLeftX = x;
    Viewport.TopLeftY = y;

    CmdList.RSSetViewports(&Viewport, 1);
}

void D3D12CommandContext::BindScissorRect(
    Float Width, 
    Float Height, 
    Float x, 
    Float y)
{
    D3D12_RECT ScissorRect;
    ScissorRect.top    = LONG(y);
    ScissorRect.bottom = LONG(Height);
    ScissorRect.left   = LONG(x);
    ScissorRect.right  = LONG(Width);

    CmdList.RSSetScissorRects(&ScissorRect, 1);
}

void D3D12CommandContext::BindBlendFactor(const ColorClearValue& Color)
{
    CmdList.OMSetBlendFactor(Color.RGBA);
}

void D3D12CommandContext::BindPrimitiveTopology(EPrimitiveTopology InPrimitveTopology)
{
    const D3D12_PRIMITIVE_TOPOLOGY PrimitiveTopology = ConvertPrimitiveTopology(InPrimitveTopology);
    CmdList.IASetPrimitiveTopology(PrimitiveTopology);
}

void D3D12CommandContext::BindVertexBuffers(
    VertexBuffer* const * VertexBuffers, 
    UInt32 BufferCount, 
    UInt32 BufferSlot)
{
    for (UInt32 i = 0; i < BufferCount; i++)
    {
        D3D12VertexBuffer* DxVertexBuffer = static_cast<D3D12VertexBuffer*>(VertexBuffers[i]);
        VertexBufferState.BindVertexBuffer(DxVertexBuffer, BufferSlot + i);
    }
    
    CmdList.IASetVertexBuffers(
        0, 
        VertexBufferState.GetVertexBufferViews(),
        VertexBufferState.GetNumVertexBufferViews());
}

void D3D12CommandContext::BindIndexBuffer(IndexBuffer* IndexBuffer)
{
    if (IndexBuffer)
    {
        D3D12IndexBuffer* DxIndexBuffer = static_cast<D3D12IndexBuffer*>(IndexBuffer);
        CmdList.IASetIndexBuffer(&DxIndexBuffer->GetView());
    }
    else
    {
        CmdList.IASetIndexBuffer(nullptr);
    }
}

void D3D12CommandContext::BindRayTracingScene(RayTracingScene* RayTracingScene)
{
    UNREFERENCED_VARIABLE(RayTracingScene);
    // TODO: Implement this function
}

void D3D12CommandContext::BindRenderTargets(
    RenderTargetView* const* RenderTargetViews, 
    UInt32 RenderTargetCount, 
    DepthStencilView* DepthStencilView)
{
    RenderTargetState.Reset();

    for (UInt32 i = 0; i < RenderTargetCount; i++)
    {
        D3D12RenderTargetView* DxRenderTargetView = static_cast<D3D12RenderTargetView*>(RenderTargetViews[i]);
        RenderTargetState.BindRenderTargetView(DxRenderTargetView, i);
    }

    D3D12DepthStencilView* DxDepthStencilView = static_cast<D3D12DepthStencilView*>(DepthStencilView);
    RenderTargetState.BindDepthStencilView(DxDepthStencilView);

    CmdList.OMSetRenderTargets(
        RenderTargetState.GetRenderTargetViewHandles(), 
        RenderTargetState.GetNumRenderTargetViewHandles(),
        FALSE, 
        RenderTargetState.GetDepthStencilHandle());
}

void D3D12CommandContext::BindGraphicsPipelineState(class GraphicsPipelineState* PipelineState)
{
    VALIDATE(PipelineState != nullptr);

    D3D12GraphicsPipelineState* DxPipelineState = static_cast<D3D12GraphicsPipelineState*>(PipelineState);
    if (DxPipelineState != CurrentGraphicsPipelineState.Get())
    {
        CurrentGraphicsPipelineState = MakeSharedRef<D3D12GraphicsPipelineState>(DxPipelineState);
        CmdList.SetPipelineState(CurrentGraphicsPipelineState->GetPipeline());
    }

    D3D12RootSignature* DxRootSignature = DxPipelineState->GetRootSignature();
    if (DxRootSignature != CurrentGraphicsRootSignature.Get())
    {
        CurrentGraphicsRootSignature = MakeSharedRef<D3D12RootSignature>(DxRootSignature);
        CmdList.SetGraphicsRootSignature(CurrentGraphicsRootSignature.Get());
    }
}

void D3D12CommandContext::BindComputePipelineState(class ComputePipelineState* PipelineState)
{
    VALIDATE(PipelineState != nullptr);

    D3D12ComputePipelineState* DxPipelineState = static_cast<D3D12ComputePipelineState*>(PipelineState);
    if (DxPipelineState != CurrentComputePipelineState.Get())
    {
        CurrentComputePipelineState = MakeSharedRef<D3D12ComputePipelineState>(DxPipelineState);
        CmdList.SetPipelineState(CurrentComputePipelineState->GetPipeline());
    }

    D3D12RootSignature* DxRootSignature = DxPipelineState->GetRootSignature();
    if (DxRootSignature != CurrentComputeRootSignature.Get())
    {
        CurrentComputeRootSignature = MakeSharedRef<D3D12RootSignature>(DxRootSignature);
        CmdList.SetComputeRootSignature(CurrentComputeRootSignature.Get());
    }
}

void D3D12CommandContext::BindRayTracingPipelineState(class RayTracingPipelineState* PipelineState)
{
    UNREFERENCED_VARIABLE(PipelineState);
    // TODO: Implement this function
}

void D3D12CommandContext::Bind32BitShaderConstants(
    EShaderStage ShaderStage, 
    const Void* Shader32BitConstants, 
    UInt32 Num32BitConstants)
{
    VALIDATE(Num32BitConstants <= D3D12_DEFAULT_SHADER_32BIT_CONSTANTS_COUNT);

    if (ShaderStageIsGraphics(ShaderStage))
    {
        CmdList.SetGraphicsRoot32BitConstants(
            Shader32BitConstants,
            Num32BitConstants,
            0,
            D3D12_DEFAULT_SHADER_32BIT_CONSTANTS_ROOT_PARAMETER);
    }
    else if (ShaderStageIsCompute(ShaderStage))
    {
        CmdList.SetComputeRoot32BitConstants(
            Shader32BitConstants,
            Num32BitConstants,
            0,
            D3D12_DEFAULT_SHADER_32BIT_CONSTANTS_ROOT_PARAMETER);
    }
}

void D3D12CommandContext::BindShaderResourceViews(
    EShaderStage ShaderStage, 
    ShaderResourceView* const* ShaderResourceViews, 
    UInt32 ShaderResourceViewCount, 
    UInt32 StartSlot)
{
    UNREFERENCED_VARIABLE(ShaderStage);

    for (UInt32 i = 0; i < ShaderResourceViewCount; i++)
    {
        D3D12ShaderResourceView* DxShaderResourceView = static_cast<D3D12ShaderResourceView*>(ShaderResourceViews[i]);
        ShaderDescriptorState.BindShaderResourceView(DxShaderResourceView, StartSlot + i);
    }
}

void D3D12CommandContext::BindSamplerStates(
    EShaderStage ShaderStage, 
    SamplerState* const* SamplerStates, 
    UInt32 SamplerStateCount, 
    UInt32 StartSlot)
{
    UNREFERENCED_VARIABLE(ShaderStage);

    for (UInt32 i = 0; i < SamplerStateCount; i++)
    {
        D3D12SamplerState* DxSamplerState = static_cast<D3D12SamplerState*>(SamplerStates[i]);
        ShaderDescriptorState.BindSamplerState(DxSamplerState, StartSlot + i);
    }
}

void D3D12CommandContext::BindUnorderedAccessViews(
    EShaderStage ShaderStage, 
    UnorderedAccessView* const* UnorderedAccessViews, 
    UInt32 UnorderedAccessViewCount, 
    UInt32 StartSlot)
{
    UNREFERENCED_VARIABLE(ShaderStage);

    for (UInt32 i = 0; i < UnorderedAccessViewCount; i++)
    {
        D3D12UnorderedAccessView* DxUnorderedAccessView = static_cast<D3D12UnorderedAccessView*>(UnorderedAccessViews[i]);
        ShaderDescriptorState.BindUnorderedAccessView(DxUnorderedAccessView, StartSlot + i);
    }
}

void D3D12CommandContext::BindConstantBuffers(
    EShaderStage ShaderStage, 
    ConstantBuffer* const* ConstantBuffers, 
    UInt32 ConstantBufferCount, 
    UInt32 StartSlot)
{
    UNREFERENCED_VARIABLE(ShaderStage);

    for (UInt32 i = 0; i < ConstantBufferCount; i++)
    {
        D3D12ConstantBuffer* DxConstantBuffer = static_cast<D3D12ConstantBuffer*>(ConstantBuffers[i]);
        ShaderDescriptorState.BindConstantBuffer(DxConstantBuffer->GetView(), StartSlot + i);
    }
}

void D3D12CommandContext::ResolveTexture(Texture* Destination, Texture* Source)
{
    //TODO: For now texture must be the same format. I.e typeless does probably not work

    D3D12Texture* DxDestination = D3D12TextureCast(Destination);
    D3D12Texture* DxSource      = D3D12TextureCast(Source);
    const DXGI_FORMAT DstFormat = DxDestination->GetNativeFormat();
    const DXGI_FORMAT SrcFormat = DxSource->GetNativeFormat();
    
    VALIDATE(DstFormat == SrcFormat);

    BarrierBatcher.FlushBarriers(CmdList);
    CmdList.ResolveSubresource(DxDestination, DxSource, DstFormat);
}

void D3D12CommandContext::UpdateBuffer(
    Buffer* Destination, 
    UInt64 OffsetInBytes, 
    UInt64 SizeInBytes, 
    const Void* SourceData)
{
    if (SizeInBytes != 0)
    {
        D3D12Buffer* DxDestination = D3D12BufferCast(Destination);
        BarrierBatcher.FlushBarriers(CmdList);

        D3D12GPUResourceUploader& GpuResourceUploader = CmdBatch->GetGpuResourceUploader();
        
        //TODO: Maybe is not needed, investigate
        const UInt32 AlignedSizeInBytes = Math::AlignUp<UInt32>(UInt32(SizeInBytes), 16u);
        Byte* GpuSourceMemory = GpuResourceUploader.LinearAllocate(AlignedSizeInBytes);

        const UInt32 GpuSourceOffsetInBytes = GpuResourceUploader.GetOffsetInBytesFromPtr(GpuSourceMemory);
        Memory::Memcpy(GpuSourceMemory, SourceData, SizeInBytes);

        CmdList.CopyBufferRegion(
            DxDestination->GetNativeResource(), 
            OffsetInBytes, 
            GpuResourceUploader.GetGpuResource(),
            GpuSourceOffsetInBytes,
            SizeInBytes);
    }
}

void D3D12CommandContext::UpdateTexture2D(
    Texture2D* Destination, 
    UInt32 Width,
    UInt32 Height,
    UInt32 MipLevel,
    const Void* SourceData)
{
    if (Width > 0 && Height > 0)
    {
        D3D12Texture* DxDestination = D3D12TextureCast(Destination);
        BarrierBatcher.FlushBarriers(CmdList);

        const DXGI_FORMAT NativeFormat = DxDestination->GetNativeFormat();
        const UInt32 Stride      = GetFormatStride(NativeFormat);
        const UInt32 RowPitch    = ((Width * Stride) + (D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u)) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);
        const UInt32 SizeInBytes = Height * RowPitch;
    
        D3D12GPUResourceUploader& GpuResourceUploader = CmdBatch->GetGpuResourceUploader();
        Byte* SourceMemory = GpuResourceUploader.LinearAllocate(SizeInBytes);

        const UInt32 SourceOffsetInBytes = GpuResourceUploader.GetOffsetInBytesFromPtr(SourceMemory);
        const Byte* Source = reinterpret_cast<const Byte*>(SourceData);
        for (UInt32 y = 0; y < Height; y++)
        {
            Memory::Memcpy(SourceMemory + (y * RowPitch), Source + (y * Width * Stride), Width * Stride);
        }

        // Copy to Dest
        D3D12_TEXTURE_COPY_LOCATION SourceLocation;
        Memory::Memzero(&SourceLocation);

        SourceLocation.pResource                          = GpuResourceUploader.GetGpuResource();
        SourceLocation.Type                               = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        SourceLocation.PlacedFootprint.Footprint.Format   = NativeFormat;
        SourceLocation.PlacedFootprint.Footprint.Width    = Width;
        SourceLocation.PlacedFootprint.Footprint.Height   = Height;
        SourceLocation.PlacedFootprint.Footprint.Depth    = 1;
        SourceLocation.PlacedFootprint.Footprint.RowPitch = RowPitch;

        // TODO: Miplevel may not be the correct subresource
        D3D12_TEXTURE_COPY_LOCATION DestLocation;
        Memory::Memzero(&DestLocation);

        DestLocation.pResource        = DxDestination->GetNativeResource();
        DestLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        DestLocation.SubresourceIndex = MipLevel;

        CmdList.CopyTextureRegion(
            &DestLocation,
            0, 0, 0,
            &SourceLocation,
            nullptr);
    }
}

void D3D12CommandContext::CopyBuffer(
    Buffer* Destination, 
    Buffer* Source, 
    const CopyBufferInfo& CopyInfo)
{
    D3D12Resource* DxDestination = D3D12BufferCast(Destination);
    D3D12Resource* DxSource      = D3D12BufferCast(Source);
    
    BarrierBatcher.FlushBarriers(CmdList);
    CmdList.CopyBuffer(
        DxDestination, 
        CopyInfo.DestinationOffset, 
        DxSource,
        CopyInfo.SourceOffset,
        CopyInfo.SizeInBytes);
}

void D3D12CommandContext::CopyTexture(Texture* Destination, Texture* Source)
{
    D3D12Texture* DxDestination = D3D12TextureCast(Destination);
    D3D12Texture* DxSource      = D3D12TextureCast(Source);

    BarrierBatcher.FlushBarriers(CmdList);
    CmdList.CopyResource(DxDestination, DxSource);
}

void D3D12CommandContext::CopyTextureRegion(
    Texture* Destination, 
    Texture* Source, 
    const CopyTextureInfo& CopyInfo)
{
    D3D12Texture* DxDestination = D3D12TextureCast(Destination);
    D3D12Texture* DxSource      = D3D12TextureCast(Source);

    // Source
    D3D12_TEXTURE_COPY_LOCATION SourceLocation;
    Memory::Memzero(&SourceLocation);

    SourceLocation.pResource        = DxSource->GetNativeResource();
    SourceLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    SourceLocation.SubresourceIndex = CopyInfo.Source.SubresourceIndex;

    D3D12_BOX SourceBox;
    SourceBox.left   = CopyInfo.Source.x;
    SourceBox.right  = CopyInfo.Source.x + CopyInfo.Width;
    SourceBox.bottom = CopyInfo.Source.y;
    SourceBox.top    = CopyInfo.Source.y + CopyInfo.Height;
    SourceBox.front  = CopyInfo.Source.z;
    SourceBox.back   = CopyInfo.Source.z + CopyInfo.Depth;

    // Destination
    D3D12_TEXTURE_COPY_LOCATION DestinationLocation;
    Memory::Memzero(&DestinationLocation);

    DestinationLocation.pResource        = DxDestination->GetNativeResource();
    DestinationLocation.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    DestinationLocation.SubresourceIndex = CopyInfo.Destination.SubresourceIndex;

    BarrierBatcher.FlushBarriers(CmdList);

    CmdList.CopyTextureRegion(
        &DestinationLocation,
        CopyInfo.Destination.x,
        CopyInfo.Destination.y,
        CopyInfo.Destination.z,
        &SourceLocation,
        &SourceBox);
}

void D3D12CommandContext::DestroyResource(PipelineResource* Resource)
{
    CmdBatch->EnqueueResourceDestruction(Resource);
}

void D3D12CommandContext::BuildRayTracingGeometry(RayTracingGeometry* RayTracingGeometry)
{
    UNREFERENCED_VARIABLE(RayTracingGeometry);

    // TODO: Implement this function
    BarrierBatcher.FlushBarriers(CmdList);
}

void D3D12CommandContext::BuildRayTracingScene(RayTracingScene* RayTracingScene)
{
    UNREFERENCED_VARIABLE(RayTracingScene);

    // TODO: Implement this function
    BarrierBatcher.FlushBarriers(CmdList);
}

void D3D12CommandContext::GenerateMips(Texture* Texture)
{
    D3D12Texture* DxTexture = D3D12TextureCast(Texture);
    VALIDATE(DxTexture != nullptr);

    D3D12_RESOURCE_DESC Desc = DxTexture->GetDesc();
    Desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    
    VALIDATE(Desc.MipLevels > 1);

    D3D12_HEAP_PROPERTIES HeapProperties;
    Memory::Memzero(&HeapProperties);

    HeapProperties.Type                 = DxTexture->GetHeapType();
    HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProperties.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProperties.CreationNodeMask     = 0;
    HeapProperties.VisibleNodeMask      = 0;

    TComPtr<ID3D12Resource> StagingTexture;
    HRESULT Result = Device->CreateCommitedResource(
        &HeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &Desc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&StagingTexture));
    if (FAILED(Result))
    {
        LOG_ERROR("[D3D12CommandContext] Failed to create StagingTexture for GenerateMips");
        return;
    }

    // Check Type
    const Bool IsTextureCube = Texture->AsTextureCube();

    D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc;
    Memory::Memzero(&SrvDesc);

    SrvDesc.Format                  = Desc.Format;
    SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    if (IsTextureCube)
    {
        SrvDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURECUBE;
        SrvDesc.TextureCube.MipLevels           = Desc.MipLevels;
        SrvDesc.TextureCube.MostDetailedMip     = 0;
        SrvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
    }
    else
    {
        SrvDesc.ViewDimension             = D3D12_SRV_DIMENSION_TEXTURE2D;
        SrvDesc.Texture2D.MipLevels       = Desc.MipLevels;
        SrvDesc.Texture2D.MostDetailedMip = 0;
    }

    D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc;
    Memory::Memzero(&UavDesc);

    UavDesc.Format = Desc.Format;
    if (IsTextureCube)
    {
        UavDesc.ViewDimension                  = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        UavDesc.Texture2DArray.ArraySize       = 6;
        UavDesc.Texture2DArray.FirstArraySlice = 0;
        UavDesc.Texture2DArray.PlaneSlice      = 0;
    }
    else
    {
        UavDesc.ViewDimension        = D3D12_UAV_DIMENSION_TEXTURE2D;
        UavDesc.Texture2D.PlaneSlice = 0;
    }

    const UInt32 MipLevelsPerDispatch     = 4;
    const UInt32 UavDescriptorHandleCount = Math::AlignUp<UInt32>(Desc.MipLevels, MipLevelsPerDispatch);
    const UInt32 NumDispatches            = UavDescriptorHandleCount / MipLevelsPerDispatch;

    D3D12OnlineDescriptorHeap* ResourceHeap = CmdBatch->GetOnlineResourceDescriptorHeap();

    // Allocate an extra handle for SRV
    const UInt32 StartDescriptorHandleIndex         = ResourceHeap->AllocateHandles(UavDescriptorHandleCount + 1); 
    const D3D12_CPU_DESCRIPTOR_HANDLE SrvHandle_CPU = ResourceHeap->GetCPUDescriptorHandleAt(StartDescriptorHandleIndex);
    Device->CreateShaderResourceView(DxTexture->GetNativeResource(), &SrvDesc, SrvHandle_CPU);

    const UInt32 UavStartDescriptorHandleIndex = StartDescriptorHandleIndex + 1;
    for (UInt32 i = 0; i < Desc.MipLevels; i++)
    {
        if (IsTextureCube)
        {
            UavDesc.Texture2DArray.MipSlice = i;
        }
        else
        {
            UavDesc.Texture2D.MipSlice = i;
        }

        const D3D12_CPU_DESCRIPTOR_HANDLE UavHandle_CPU = ResourceHeap->GetCPUDescriptorHandleAt(UavStartDescriptorHandleIndex + i);
        Device->CreateUnorderedAccessView(StagingTexture.Get(), nullptr, &UavDesc, UavHandle_CPU);
    }

    for (UInt32 i = Desc.MipLevels; i < UavDescriptorHandleCount; i++)
    {
        if (IsTextureCube)
        {
            UavDesc.Texture2DArray.MipSlice = 0;
        }
        else
        {
            UavDesc.Texture2D.MipSlice = 0;
        }

        const D3D12_CPU_DESCRIPTOR_HANDLE UavHandle_CPU = ResourceHeap->GetCPUDescriptorHandleAt(UavStartDescriptorHandleIndex + i);
        Device->CreateUnorderedAccessView(nullptr, nullptr, &UavDesc, UavHandle_CPU);
    }

    // We assume the destination is in D3D12_RESOURCE_STATE_COPY_DEST
    BarrierBatcher.AddTransitionBarrier(
        DxTexture, 
        D3D12_RESOURCE_STATE_COPY_DEST, 
        D3D12_RESOURCE_STATE_COPY_SOURCE);
    
    BarrierBatcher.AddTransitionBarrier(
        StagingTexture.Get(), 
        D3D12_RESOURCE_STATE_COMMON, 
        D3D12_RESOURCE_STATE_COPY_DEST);

    BarrierBatcher.FlushBarriers(CmdList);

    CmdList.CopyNativeResource(
        StagingTexture.Get(), 
        DxTexture->GetNativeResource());

    BarrierBatcher.AddTransitionBarrier(
        DxTexture,
        D3D12_RESOURCE_STATE_COPY_SOURCE,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

    BarrierBatcher.AddTransitionBarrier(
        StagingTexture.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    BarrierBatcher.FlushBarriers(CmdList);

    if (IsTextureCube)
    {
        CmdList.SetPipelineState(GenerateMipsTexCube_PSO->GetPipeline());
        CmdList.SetComputeRootSignature(GenerateMipsTexCube_PSO->GetRootSignature());
    }
    else
    {
        CmdList.SetPipelineState(GenerateMipsTex2D_PSO->GetPipeline());
        CmdList.SetComputeRootSignature(GenerateMipsTex2D_PSO->GetRootSignature());
    }

    const D3D12_GPU_DESCRIPTOR_HANDLE SrvHandle_GPU = ResourceHeap->GetGPUDescriptorHandleAt(StartDescriptorHandleIndex);
    ID3D12DescriptorHeap* OnlineResourceHeap        = ResourceHeap->GetHeap()->GetHeap();    
    CmdList.SetDescriptorHeaps(&OnlineResourceHeap, 1);
    CmdList.SetComputeRootDescriptorTable(SrvHandle_GPU, 1);

    struct ConstantBuffer
    {
        UInt32   SrcMipLevel;
        UInt32   NumMipLevels;
        XMFLOAT2 TexelSize;
    } ConstantData;

    UInt32 DstWidth  = static_cast<UInt32>(Desc.Width);
    UInt32 DstHeight = Desc.Height;
    ConstantData.SrcMipLevel = 0;

    const UInt32 ThreadsZ     = IsTextureCube ? 6 : 1;    
    UInt32 RemainingMiplevels = Desc.MipLevels;
    for (UInt32 i = 0; i < NumDispatches; i++)
    {
        ConstantData.TexelSize    = XMFLOAT2(1.0f / static_cast<Float>(DstWidth), 1.0f / static_cast<Float>(DstHeight));
        ConstantData.NumMipLevels = Math::Min<UInt32>(4, RemainingMiplevels);

        CmdList.SetComputeRoot32BitConstants(
            &ConstantData, 4, 0, 0);

        const UInt32 GPUDescriptorHandleIndex = i * MipLevelsPerDispatch;
        const D3D12_GPU_DESCRIPTOR_HANDLE UavHandle_GPU = ResourceHeap->GetGPUDescriptorHandleAt(UavStartDescriptorHandleIndex + GPUDescriptorHandleIndex);
        CmdList.SetComputeRootDescriptorTable(UavHandle_GPU, 2);

        constexpr UInt32 ThreadCount = 8;
        const UInt32 ThreadsX = Math::DivideByMultiple(DstWidth, ThreadCount);
        const UInt32 ThreadsY = Math::DivideByMultiple(DstHeight, ThreadCount);
        CmdList.Dispatch(ThreadsX, ThreadsY, ThreadsZ);

        CmdList.UnorderedAccessBarrier(StagingTexture.Get());

        BarrierBatcher.AddTransitionBarrier(
            DxTexture,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_COPY_DEST);

        BarrierBatcher.AddTransitionBarrier(
            StagingTexture.Get(),
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            D3D12_RESOURCE_STATE_COPY_SOURCE);

        BarrierBatcher.FlushBarriers(CmdList);

        // TODO: Copy only miplevels (Maybe faster?)
        CmdList.CopyNativeResource(
            DxTexture->GetNativeResource(),
            StagingTexture.Get());

        BarrierBatcher.AddTransitionBarrier(
            DxTexture,
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

        BarrierBatcher.AddTransitionBarrier(
            StagingTexture.Get(),
            D3D12_RESOURCE_STATE_COPY_SOURCE,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        BarrierBatcher.FlushBarriers(CmdList);

        DstWidth  = DstWidth / 16;
        DstHeight = DstHeight / 16;

        ConstantData.SrcMipLevel += 3;
        RemainingMiplevels -= MipLevelsPerDispatch;
    }

    BarrierBatcher.AddTransitionBarrier(
        DxTexture,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_COPY_DEST);

    BarrierBatcher.FlushBarriers(CmdList);

    CmdBatch->EnqueueResourceDestruction(StagingTexture);
}

void D3D12CommandContext::TransitionTexture(
    Texture* Texture, 
    EResourceState BeforeState, 
    EResourceState AfterState)
{
    const D3D12_RESOURCE_STATES DxBeforeState = ConvertResourceState(BeforeState);
    const D3D12_RESOURCE_STATES DxAfterState  = ConvertResourceState(AfterState);

    D3D12Resource* Resource = D3D12TextureCast(Texture);
    BarrierBatcher.AddTransitionBarrier(Resource, DxBeforeState, DxAfterState);
}

void D3D12CommandContext::TransitionBuffer(
    Buffer* Buffer, 
    EResourceState BeforeState, 
    EResourceState AfterState)
{
    const D3D12_RESOURCE_STATES DxBeforeState = ConvertResourceState(BeforeState);
    const D3D12_RESOURCE_STATES DxAfterState  = ConvertResourceState(AfterState);
    
    D3D12Resource* Resource = D3D12BufferCast(Buffer);
    BarrierBatcher.AddTransitionBarrier(Resource, DxBeforeState, DxAfterState);
}

void D3D12CommandContext::UnorderedAccessTextureBarrier(Texture* Texture)
{
    D3D12Resource* Resource = D3D12TextureCast(Texture);
    BarrierBatcher.AddUnorderedAccessBarrier(Resource);
}

void D3D12CommandContext::Draw(UInt32 VertexCount, UInt32 StartVertexLocation)
{
    BarrierBatcher.FlushBarriers(CmdList);

    D3D12OnlineDescriptorHeap* OnlineResourceDescriptorHeap = CmdBatch->GetOnlineResourceDescriptorHeap();
    D3D12OnlineDescriptorHeap* OnlineSamplerDescriptorHeap  = CmdBatch->GetOnlineSamplerDescriptorHeap();
    ShaderDescriptorState.CommitGraphicsDescriptorTables(
        *Device,
        *OnlineResourceDescriptorHeap,
        *OnlineSamplerDescriptorHeap,
        CmdList);

    CmdList.DrawInstanced(VertexCount, 1, StartVertexLocation, 0);
}

void D3D12CommandContext::DrawIndexed(
    UInt32 IndexCount, 
    UInt32 StartIndexLocation, 
    UInt32 BaseVertexLocation)
{
    BarrierBatcher.FlushBarriers(CmdList);

    D3D12OnlineDescriptorHeap* OnlineResourceDescriptorHeap = CmdBatch->GetOnlineResourceDescriptorHeap();
    D3D12OnlineDescriptorHeap* OnlineSamplerDescriptorHeap  = CmdBatch->GetOnlineSamplerDescriptorHeap();
    ShaderDescriptorState.CommitGraphicsDescriptorTables(
        *Device,
        *OnlineResourceDescriptorHeap,
        *OnlineSamplerDescriptorHeap,
        CmdList);

    CmdList.DrawIndexedInstanced(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
}

void D3D12CommandContext::DrawInstanced(
    UInt32 VertexCountPerInstance, 
    UInt32 InstanceCount, 
    UInt32 StartVertexLocation, 
    UInt32 StartInstanceLocation)
{
    BarrierBatcher.FlushBarriers(CmdList);

    D3D12OnlineDescriptorHeap* OnlineResourceDescriptorHeap = CmdBatch->GetOnlineResourceDescriptorHeap();
    D3D12OnlineDescriptorHeap* OnlineSamplerDescriptorHeap  = CmdBatch->GetOnlineSamplerDescriptorHeap();
    ShaderDescriptorState.CommitGraphicsDescriptorTables(
        *Device,
        *OnlineResourceDescriptorHeap,
        *OnlineSamplerDescriptorHeap,
        CmdList);

    CmdList.DrawInstanced(
        VertexCountPerInstance,
        InstanceCount, 
        StartVertexLocation,
        StartInstanceLocation);
}

void D3D12CommandContext::DrawIndexedInstanced(
    UInt32 IndexCountPerInstance, 
    UInt32 InstanceCount, 
    UInt32 StartIndexLocation, 
    UInt32 BaseVertexLocation, 
    UInt32 StartInstanceLocation)
{
    BarrierBatcher.FlushBarriers(CmdList);
    
    D3D12OnlineDescriptorHeap* OnlineResourceDescriptorHeap = CmdBatch->GetOnlineResourceDescriptorHeap();
    D3D12OnlineDescriptorHeap* OnlineSamplerDescriptorHeap  = CmdBatch->GetOnlineSamplerDescriptorHeap();
    ShaderDescriptorState.CommitGraphicsDescriptorTables(
        *Device,
        *OnlineResourceDescriptorHeap,
        *OnlineSamplerDescriptorHeap,
        CmdList);

    CmdList.DrawIndexedInstanced(
        IndexCountPerInstance, 
        InstanceCount, 
        StartIndexLocation, 
        BaseVertexLocation, 
        StartInstanceLocation);
}

void D3D12CommandContext::Dispatch(
    UInt32 ThreadGroupCountX, 
    UInt32 ThreadGroupCountY,
    UInt32 ThreadGroupCountZ)
{
    BarrierBatcher.FlushBarriers(CmdList);
    
    D3D12OnlineDescriptorHeap* OnlineResourceDescriptorHeap = CmdBatch->GetOnlineResourceDescriptorHeap();
    D3D12OnlineDescriptorHeap* OnlineSamplerDescriptorHeap  = CmdBatch->GetOnlineSamplerDescriptorHeap();
    ShaderDescriptorState.CommitComputeDescriptorTables(
        *Device, 
        *OnlineResourceDescriptorHeap, 
        *OnlineSamplerDescriptorHeap,
        CmdList);

    CmdList.Dispatch(
        ThreadGroupCountX, 
        ThreadGroupCountY, 
        ThreadGroupCountZ);
}

void D3D12CommandContext::DispatchRays(UInt32 Width, UInt32 Height, UInt32 Depth)
{
    // TODO: Finish this function
    VALIDATE(false);

    D3D12_DISPATCH_RAYS_DESC RayDispatchDesc;
    Memory::Memzero(&RayDispatchDesc);

    RayDispatchDesc.Width  = Width;
    RayDispatchDesc.Height = Height;
    RayDispatchDesc.Depth  = Depth;

    BarrierBatcher.FlushBarriers(CmdList);
    CmdList.DispatchRays(&RayDispatchDesc);
}

void D3D12CommandContext::ClearState()
{
    VertexBufferState.Reset();
    RenderTargetState.Reset();
    ShaderDescriptorState.Reset();

    CurrentGraphicsPipelineState.Reset();
    CurrentGraphicsRootSignature.Reset();
    CurrentComputePipelineState.Reset();
    CurrentComputeRootSignature.Reset();
}

void D3D12CommandContext::Flush()
{
    const UInt64 NewFenceValue = ++FenceValue;
    if (!CmdQueue.SignalFence(Fence, NewFenceValue))
    {
        return;
    }

    Fence.WaitForValue(FenceValue);
}

void D3D12CommandContext::InsertMarker(const std::string& Message)
{
#if D3D12_ENABLE_PIX_MARKERS
    // TODO: Look into this since "%s" is not that nice, however safer since string can contain format, that will cuase a crash
    PIXSetMarker(CmdList.GetGraphicsCommandList(), PIX_COLOR(255, 255, 255), "%s", Message.c_str());
#else
    UNREFERENCED_VARIABLE(Message);
#endif
}
