#include "D3D12RHI/D3D12RHI.h"
#include "D3D12RHI/D3D12Allocators.h"
#include "D3D12RHI/D3D12Buffer.h"
#include "RHI/RHIStats.h"

uint64 FD3D12BufferRHI::GetBufferAlignment(const FRHIBufferDesc& Desc)
{
    uint64 Alignment;
    if (Desc.IsConstantBuffer())
    {
        Alignment = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
    }
    else if (Desc.Stride > 0)
    {
        Alignment = Math::LeastCommonMultiple<uint64>(Desc.Stride, 16);
    }
    else
    {
        Alignment = 16;
    }

    if (Desc.IsReadBack())
    {
        Alignment = Math::Max<uint64>(Alignment, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
    }

    if (Desc.IsAccelerationStructure())
    {
        Alignment = Math::Max<uint64>(Alignment, RHI::AccelerationStructureBufferAlignment); // 256
    }

    return Alignment;
}

FD3D12BufferRHI::FD3D12BufferRHI(FD3D12Device* InDevice, const FRHIBufferDesc& InBufferDesc)
    : FRHIBuffer(InBufferDesc)
    , FD3D12ResourceBase(InDevice)
{
}

void* FD3D12BufferRHI::GetRHINativeResource() const
{
    FD3D12Resource* Resource = ResourceStorage.GetResource();
    return Resource ? reinterpret_cast<void*>(Resource->GetD3D12Resource()) : nullptr;
}

FRHIDescriptorHandle FD3D12BufferRHI::GetBindlessHandle() const
{
    if (!Desc.IsConstantBuffer())
    {
        CHECK(false && "GetBindlessHandle called on a non-constant-buffer FD3D12BufferRHI");
        return FRHIDescriptorHandle();
    }

    FD3D12ConstantBufferView* LocalConstantBufferView = const_cast<FD3D12BufferRHI*>(this)->GetOrCreateConstantBufferView();
    if (!LocalConstantBufferView)
    {
        return FRHIDescriptorHandle();
    }

    return LocalConstantBufferView->GetBindlessHandle();
}

FD3D12BufferRHI::~FD3D12BufferRHI()
{
#if D3D12_ENABLE_STATS
    const int64 AllocatedSize = static_cast<int64>(ResourceStorage.GetSize());
    if (AllocatedSize > 0)
    {
        if (Desc.IsVertexBuffer())
        {
            STAT_SUBTRACT(STAT_RHI_VertexBufferMemory, AllocatedSize);
        }
        else if (Desc.IsIndexBuffer())
        {
            STAT_SUBTRACT(STAT_RHI_IndexBufferMemory, AllocatedSize);
        }
        else if (Desc.IsConstantBuffer())
        {
            STAT_SUBTRACT(STAT_RHI_ConstantBufferMemory, AllocatedSize);
        }
        else if (Desc.IsShaderResourceBuffer() || Desc.IsUnorderedAccessBuffer())
        {
            STAT_SUBTRACT(STAT_RHI_StructuredBufferMemory, AllocatedSize);
        }
        else
        {
            STAT_SUBTRACT(STAT_RHI_MiscBufferMemory, AllocatedSize);
        }

        if (Desc.IsReadBack())
        {
            STAT_SUBTRACT(STAT_RHI_ReadbackMemory, AllocatedSize);
        }
        if (Desc.IsDynamic() || Desc.IsTransient())
        {
            STAT_SUBTRACT(STAT_RHI_UploadMemory, AllocatedSize);
        }
    }
#endif
}

bool FD3D12BufferRHI::Initialize(FD3D12CommandContext* InCommandContext, ERHIResourceState InInitialAccess, const void* InInitialData)
{
    const uint64 Alignment   = GetBufferAlignment(Desc);
    const uint64 AlignedSize = Math::AlignUpToMultiple<uint64>(Desc.Size, Alignment);

    D3D12_RESOURCE_DESC ResourceDesc = {};
    ResourceDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    ResourceDesc.Flags              = ConvertBufferFlags(Desc.Flags);
    ResourceDesc.Format             = DXGI_FORMAT_UNKNOWN;
    ResourceDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ResourceDesc.Width              = AlignedSize;
    ResourceDesc.Height             = 1;
    ResourceDesc.DepthOrArraySize   = 1;
    ResourceDesc.MipLevels          = 1;
    ResourceDesc.Alignment          = 0;
    ResourceDesc.SampleDesc.Count   = 1;
    ResourceDesc.SampleDesc.Quality = 0;

    const bool bIsManual = (ConvertResourceStateMode(Desc.TrackingMode) == ED3D12ResourceStateMode::ManualState);

    ED3D12ResourceStateMode StateMode         = bIsManual ? ED3D12ResourceStateMode::ManualState : ED3D12ResourceStateMode::MultipleStates;
    D3D12_RESOURCE_STATES   D3D12InitialState = D3D12_RESOURCE_STATE_COMMON;
    D3D12_HEAP_TYPE         D3D12HeapType     = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_STATES   D3D12DefaultState = bIsManual ? D3D12_RESOURCE_STATES(0) : DetermineDefaultBufferState(Desc.Flags);

    if (Desc.IsReadBack())
    {
        D3D12HeapType      = D3D12_HEAP_TYPE_READBACK;
        D3D12InitialState  = D3D12_RESOURCE_STATE_COPY_DEST;
        D3D12DefaultState  = D3D12_RESOURCE_STATE_COPY_DEST;
        StateMode          = ED3D12ResourceStateMode::SingleState;
        ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    }
    else if (Desc.IsDynamic() || Desc.IsTransient())
    {
        D3D12HeapType     = D3D12_HEAP_TYPE_UPLOAD;
        D3D12InitialState = D3D12_RESOURCE_STATE_GENERIC_READ;
        D3D12DefaultState = D3D12_RESOURCE_STATE_GENERIC_READ;
        StateMode         = ED3D12ResourceStateMode::SingleState;
    }
    else if (D3D12DefaultState != D3D12_RESOURCE_STATES(0))
    {
        D3D12InitialState = D3D12DefaultState;
        StateMode         = ED3D12ResourceStateMode::SingleState;
    }

    bool bAllocated = false;
    if (Desc.IsTransient())
    {
        if (Desc.IsConstantBuffer())
        {
            bAllocated = GetDevice()->GetDynamicConstantsAllocator()->Allocate(AlignedSize, ResourceStorage) != nullptr;
        }
        else
        {
            bAllocated = GetDevice()->GetUploadHeapAllocator()->Allocate(AlignedSize, Alignment, ResourceStorage) != nullptr;
        }
    }
    else if (Desc.IsDynamic())
    {
        bAllocated = GetDevice()->GetUploadHeapAllocator()->Allocate(AlignedSize, Alignment, ResourceStorage) != nullptr;
    }
    else
    {
        bAllocated = GetDevice()->GetBufferAllocator()->TryAllocate(
            D3D12HeapType, 
            ResourceDesc, 
            D3D12InitialState, 
            StateMode, 
            Alignment, 
            ResourceStorage);
    }

    if (!bAllocated || ResourceStorage.GetResource() == nullptr)
    {
        return false;
    }

    FD3D12Resource* D3D12Resource = ResourceStorage.GetResource();

    Desc.TrackingMode = ConvertResourceStateMode(StateMode);

    const bool bHasDefaultState = D3D12DefaultState != D3D12_RESOURCE_STATES(0);
    const bool bPlaced          = D3D12Resource->IsPlacedResource();
    const bool bMappedUpload    = InInitialData && (Desc.IsDynamic() || Desc.IsTransient());

    if (bMappedUpload)
    {
        void* MappedAddress = ResourceStorage.GetMappedBaseAddress();
        if (!MappedAddress)
        {
            // MapRange returns the base of the whole backing resource, so the suballocation offset has to be folded in here.
            uint8* ResourceBase = reinterpret_cast<uint8*>(D3D12Resource->MapRange(0, nullptr));
            if (!ResourceBase)
            {
                D3D12_ERROR("Failed to map buffer data");
                return false;
            }

            const uint64 ResourceOffset = ResourceStorage.GetResourceOffset();
            Memory::Memcpy(ResourceBase + ResourceOffset, InInitialData, Desc.Size);

            const D3D12_RANGE WrittenRange = { ResourceOffset, ResourceOffset + Desc.Size };
            D3D12Resource->UnmapRange(0, &WrittenRange);
        }
        else
        {
            Memory::Memcpy(MappedAddress, InInitialData, Desc.Size);
        }
    }

    const bool bGpuUpload       = InInitialData && !bMappedUpload;
    const bool bNeedsTransition = !InInitialData && !bHasDefaultState && (InInitialAccess != ERHIResourceState::Common) && (D3D12HeapType == D3D12_HEAP_TYPE_DEFAULT);

    if (bPlaced || bGpuUpload || bNeedsTransition)
    {
        InCommandContext->StartContext();

        if (bPlaced)
        {
            InCommandContext->AliasingBarrier(D3D12Resource);
            InCommandContext->GetBarrierBatcher().FlushBarriers(InCommandContext->GetCommandList());
        }

        if (bGpuUpload)
        {
            const D3D12_RESOURCE_STATES BeforeState = bHasDefaultState ? D3D12DefaultState : D3D12_RESOURCE_STATE_COMMON;
            const D3D12_RESOURCE_STATES AfterState  = bHasDefaultState ? D3D12DefaultState : ConvertResourceState(InInitialAccess);

            InCommandContext->TransitionResourceState(D3D12Resource, BeforeState, D3D12_RESOURCE_STATE_COPY_DEST);
            InCommandContext->UpdateBuffer(this, FBufferRegion(0, Desc.Size), InInitialData);
            InCommandContext->TransitionResourceState(D3D12Resource, D3D12_RESOURCE_STATE_COPY_DEST, AfterState);
        }
        else if (bNeedsTransition)
        {
            InCommandContext->TransitionResourceState(D3D12Resource, D3D12_RESOURCE_STATE_COMMON, ConvertResourceState(InInitialAccess));
        }

        InCommandContext->FinishContext();
    }

    ResourceStorage.FinalizeAllocation();
    return true;
}

void* FD3D12BufferRHI::Map(uint64 Offset, uint64 Size)
{
    if (!ResourceStorage.GetResource())
    {
        return nullptr;
    }

    if (!Desc.IsDynamic() && !Desc.IsReadBack() && !Desc.IsTransient())
    {
        String DebugName;
        GetDebugName(DebugName);
        D3D12_ERROR("Attempting to map a non-mappable buffer. Name='%s'", *DebugName);
        return nullptr;
    }

    const uint64 BufferSize = ResourceStorage.GetSize();
    CHECK(Offset <= BufferSize);

    uint64 MapSize = Size;
    if (MapSize == UINT64_MAX)
    {
        MapSize = BufferSize - Offset;
    }

    if (ResourceStorage.GetMappedBaseAddress())
    {
        return static_cast<uint8*>(ResourceStorage.GetMappedBaseAddress()) + Offset;
    }

    // MapRange returns the base of the whole backing resource, so the suballocation offset has to be folded in here.
    const uint64 ResourceOffset = ResourceStorage.GetResourceOffset();

    D3D12_RANGE ReadRange = {};
    ReadRange.Begin = ResourceOffset + Offset;
    ReadRange.End   = ResourceOffset + Offset + MapSize;

    uint8* MappedData = reinterpret_cast<uint8*>(ResourceStorage.GetResource()->MapRange(0, &ReadRange));
    if (!MappedData)
    {
        return nullptr;
    }

    return MappedData + ResourceOffset + Offset;
}

void FD3D12BufferRHI::Unmap(uint64 Offset, uint64 Size)
{
    if (!ResourceStorage.GetResource())
    {
        return;
    }

    if (ResourceStorage.GetMappedBaseAddress())
    {
        return;
    }

    const uint64 BufferSize = ResourceStorage.GetSize();
    CHECK(Offset <= BufferSize);

    uint64 UnmapSize = Size;
    if (UnmapSize == UINT64_MAX)
    {
        UnmapSize = BufferSize - Offset;
    }

    const uint64      ResourceOffset = ResourceStorage.GetResourceOffset();
    const D3D12_RANGE WrittenRange   = { ResourceOffset + Offset, ResourceOffset + Offset + UnmapSize };
    ResourceStorage.GetResource()->UnmapRange(0, &WrittenRange);
}

void FD3D12BufferRHI::SetDebugName(const String& InName)
{
    if (ResourceStorage.GetResource())
    {
        ResourceStorage.GetResource()->SetDebugName(InName);
    }
}

void FD3D12BufferRHI::GetDebugName(String& OutDebugName) const
{
    if (ResourceStorage.GetResource())
    {
        ResourceStorage.GetResource()->GetDebugName(OutDebugName);
    }
    else
    {
        OutDebugName.Clear();
    }
}

void FD3D12BufferRHI::SetResource(FD3D12Resource* InResource)
{
    ResourceStorage.ReleaseResource();
    
    if (InResource)
    {
        ResourceStorage.InitStandalone(InResource);
    }

    if (Desc.IsConstantBuffer() && ConstantBufferView.IsValid())
    {
        CreateConstantBufferView();
    }
}

FD3D12ConstantBufferView* FD3D12BufferRHI::GetOrCreateConstantBufferView()
{
    if (!ConstantBufferView.IsValid())
    {
        ConstantBufferView = new FD3D12ConstantBufferView(GetDevice(), GetDevice()->GetResourceOfflineDescriptorHeap());

        if (!CreateConstantBufferView())
        {
            return nullptr;
        }
    }
    
    return ConstantBufferView.Get();
}

bool FD3D12BufferRHI::CreateConstantBufferView()
{
    CHECK(ResourceStorage.GetResource() != nullptr);

    D3D12_CONSTANT_BUFFER_VIEW_DESC ViewDesc;
    Memory::Memzero(&ViewDesc);

    ViewDesc.SizeInBytes    = Math::AlignUp<uint32>(static_cast<uint32>(Desc.Size), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    ViewDesc.BufferLocation = ResourceStorage.GetGPUVirtualAddress();

    const bool bSuccess = ConstantBufferView->IsValid()
        ? ConstantBufferView->UpdateView(ResourceStorage.GetResource(), ViewDesc)
        : ConstantBufferView->Initialize(ResourceStorage.GetResource(), ViewDesc);

    if (!bSuccess)
    {
        D3D12_ERROR_CRITICAL("Failed to Create ConstantBufferView");
        return false;
    }

    ConstantBufferView->RegisterWithResource(this);
    return true;
}
