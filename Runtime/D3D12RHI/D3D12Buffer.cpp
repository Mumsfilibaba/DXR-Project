#include "D3D12RHI/D3D12RHI.h"
#include "D3D12RHI/D3D12Allocators.h"
#include "D3D12RHI/D3D12Buffer.h"
#include "RHI/RHIStats.h"

FD3D12BufferRHI::FD3D12BufferRHI(FD3D12Device* InDevice, const FRHIBufferDesc& InBufferDesc)
    : FRHIBuffer(InBufferDesc)
    , FD3D12GenericResource(InDevice)
{
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

bool FD3D12BufferRHI::Initialize(FD3D12CommandContext* InCommandContext, EResourceAccess InInitialAccess, const void* InInitialData)
{
    const uint64 Alignment   = GetBufferAlignment(Desc.Flags);
    const uint64 AlignedSize = Math::AlignUp(Desc.Size, Alignment);

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

    ED3D12ResourceStateMode StateMode         = ED3D12ResourceStateMode::MultipleStates;
    D3D12_RESOURCE_STATES   D3D12InitialState = D3D12_RESOURCE_STATE_COMMON;
    D3D12_HEAP_TYPE         D3D12HeapType     = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_STATES   D3D12DefaultState = DetermineDefaultBufferState(Desc.Flags);

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
        bAllocated = GetDevice()->GetBufferAllocator()->TryAllocate(D3D12HeapType, ResourceDesc, D3D12InitialState, StateMode, Alignment, ResourceStorage);
    }

    if (!bAllocated || ResourceStorage.GetResource() == nullptr)
    {
        return false;
    }

    FD3D12Resource* D3D12Resource = ResourceStorage.GetResource();
    D3D12Resource->SetResourceStateMode(StateMode);

    const bool bHasDefaultState = D3D12DefaultState != D3D12_RESOURCE_STATES(0);
    if (bHasDefaultState)
    {
        D3D12Resource->SetDefaultState(D3D12DefaultState);
    }

    if (InInitialData)
    {
        if (Desc.IsDynamic() || Desc.IsTransient())
        {
            void* MappedAddress = ResourceStorage.GetMappedBaseAddress();
            if (!MappedAddress)
            {
                MappedAddress = D3D12Resource->MapRange(0, nullptr);
                if (!MappedAddress)
                {
                    D3D12_ERROR("Failed to map buffer data");
                    return false;
                }

                FMemory::Memcpy(MappedAddress, InInitialData, Desc.Size);
                D3D12Resource->UnmapRange(0, nullptr);
            }
            else
            {
                FMemory::Memcpy(MappedAddress, InInitialData, Desc.Size);
            }
        }
        else if (bHasDefaultState)
        {
            InCommandContext->StartContext();

            InCommandContext->TransitionResourceState(D3D12Resource, D3D12DefaultState, D3D12_RESOURCE_STATE_COPY_DEST);
            InCommandContext->UpdateBuffer(this, FBufferRegion(0, Desc.Size), InInitialData);
            InCommandContext->TransitionResourceState(D3D12Resource, D3D12_RESOURCE_STATE_COPY_DEST, D3D12DefaultState);

            InCommandContext->FinishContext();
        }
        else
        {
            InCommandContext->StartContext();

            InCommandContext->TransitionBufferState(this, EResourceAccess::Common, EResourceAccess::CopyDest);
            InCommandContext->UpdateBuffer(this, FBufferRegion(0, Desc.Size), InInitialData);
            InCommandContext->TransitionBufferState(this, EResourceAccess::CopyDest, InInitialAccess);

            InCommandContext->FinishContext();
        }
    }
    else if (!bHasDefaultState && InInitialAccess != EResourceAccess::Common && D3D12HeapType == D3D12_HEAP_TYPE_DEFAULT)
    {
        InCommandContext->StartContext();
        InCommandContext->TransitionBufferState(this, EResourceAccess::Common, InInitialAccess);
        InCommandContext->FinishContext();
    }

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
        D3D12_ERROR("Attempting to map a non-mappable buffer. Name='%s'", *GetDebugName());
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

    D3D12_RANGE ReadRange = {};
    ReadRange.Begin = Offset;
    ReadRange.End   = Offset + MapSize;

    uint8* MappedData = reinterpret_cast<uint8*>(ResourceStorage.GetResource()->MapRange(0, &ReadRange));
    if (!MappedData)
    {
        return nullptr;
    }

    return MappedData + Offset;
}

void FD3D12BufferRHI::Unmap(uint64 /* Offset */, uint64 /* Size */)
{
    if (!ResourceStorage.GetResource())
    {
        return;
    }

    if (!ResourceStorage.GetMappedBaseAddress())
    {
        // We generally use these mappings for readback or full-buffer writes; keep it simple here.
        ResourceStorage.GetResource()->UnmapRange(0, nullptr);
    }
}

void FD3D12BufferRHI::SetDebugName(const FString& InName)
{
    if (ResourceStorage.GetResource())
    {
        ResourceStorage.GetResource()->SetDebugName(InName);
    }
}

FString FD3D12BufferRHI::GetDebugName() const
{
    FString DebugName;
    if (ResourceStorage.GetResource())
    {
        ResourceStorage.GetResource()->GetDebugName(DebugName);
    }

    return DebugName;
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
    FMemory::Memzero(&ViewDesc);

    ViewDesc.SizeInBytes = Math::AlignUp<uint32>(static_cast<uint32>(Desc.Size), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    ViewDesc.BufferLocation = ResourceStorage.GetGPUVirtualAddress();

    if (FD3D12_CPU_DESCRIPTOR_HANDLE(0) == ConstantBufferView->GetOfflineHandle())
    {
        if (!ConstantBufferView->AllocateHandle())
        {
            D3D12_ERROR_CRITICAL("Failed to allocate ConstantBuffer Descriptor");
            return false;
        }
    }

    if (!ConstantBufferView->CreateView(ResourceStorage.GetResource(), ViewDesc))
    {
        D3D12_ERROR_CRITICAL("Failed to Create ConstantBufferView");
        return false;
    }

    ConstantBufferView->RegisterWithResource(this);
    return true;
}
