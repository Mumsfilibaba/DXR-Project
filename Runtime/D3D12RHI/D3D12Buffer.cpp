#include "D3D12RHI/D3D12RHI.h"
#include "D3D12RHI/D3D12Allocators.h"
#include "D3D12RHI/D3D12Buffer.h"

FD3D12Buffer::FD3D12Buffer(FD3D12Device* InDevice, const FRHIBufferInfo& InBufferInfo)
    : FRHIBuffer(InBufferInfo)
    , FD3D12BaseResource(InDevice)
{
}

FD3D12Buffer::~FD3D12Buffer()
{
}

bool FD3D12Buffer::Initialize(FD3D12CommandContext* InCommandContext, EResourceAccess InInitialAccess, const void* InInitialData)
{
    const uint64 Alignment   = GetBufferAlignment(Info.Flags);
    const uint64 AlignedSize = Math::AlignUp(Info.Size, Alignment);

    D3D12_RESOURCE_DESC ResourceDesc = {};
    ResourceDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    ResourceDesc.Flags              = ConvertBufferFlags(Info.Flags);
    ResourceDesc.Format             = DXGI_FORMAT_UNKNOWN;
    ResourceDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ResourceDesc.Width              = AlignedSize;
    ResourceDesc.Height             = 1;
    ResourceDesc.DepthOrArraySize   = 1;
    ResourceDesc.MipLevels          = 1;
    ResourceDesc.Alignment          = 0;
    ResourceDesc.SampleDesc.Count   = 1;
    ResourceDesc.SampleDesc.Quality = 0;

    D3D12_HEAP_TYPE       D3D12HeapType     = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_STATES D3D12InitialState = D3D12_RESOURCE_STATE_COMMON;

    if (Info.IsReadBack())
    {
        // Readback resources must be placed in a READBACK heap and are only valid as copy destinations.
        D3D12HeapType      = D3D12_HEAP_TYPE_READBACK;
        D3D12InitialState  = D3D12_RESOURCE_STATE_COPY_DEST;
        ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    }
    else if (Info.IsDynamic())
    {
        D3D12HeapType     = D3D12_HEAP_TYPE_UPLOAD;
        D3D12InitialState = D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    // Support tight alignment if the device supports it
    if (GD3D12SupportTightAlignment)
    {
        ResourceDesc.Flags |= D3D12_RESOURCE_FLAG_USE_TIGHT_ALIGNMENT;
    }

    FD3D12ResourceAllocationRequest Request{};
    Request.Size = AlignedSize;
    Request.Alignment = Alignment;
    Request.ResourceType = ED3D12ResourceType::Buffer;
    Request.HeapType = D3D12HeapType;
    Request.InitialState = D3D12InitialState;
    Request.ResourceFlags = ResourceDesc.Flags;
    Request.bHasResourceDesc = true;
    Request.ResourceDesc = ResourceDesc;
    Request.bPersistent = Info.IsReadBack();
    Request.FencePoint.QueueType = InCommandContext ? InCommandContext->GetQueueType() : ED3D12CommandQueueType::Direct;

    bool bAllocated = false;
    if (Info.IsDynamic())
    {
        bAllocated = GetDevice()->GetUploadHeapAllocator()->TryAllocate(Request, ResourceStorage);
    }
    else
    {
        bAllocated = GetDevice()->GetBufferAllocator()->TryAllocate(Request, ResourceStorage);
    }

    if (!bAllocated || ResourceStorage.GetResource() == nullptr)
    {
        return false;
    }

    if (Info.IsConstantBuffer())
    {
        ConstantBufferView = new FD3D12ConstantBufferView(GetDevice(), GetDevice()->GetResourceOfflineDescriptorHeap());
        if (!CreateConstantBufferView())
        {
            return false;
        }
    }

    if (InInitialData)
    {
        if (Info.IsDynamic())
        {
            FD3D12Resource* D3D12Resource = ResourceStorage.GetResource();

            void* BufferData = D3D12Resource->MapRange(0, nullptr);
            if (!BufferData)
            {
                D3D12_ERROR("Failed to map buffer data");
                return false;
            }

            FMemory::Memcpy(BufferData, InInitialData, Info.Size);
            D3D12Resource->UnmapRange(0, nullptr);
        }
        else
        {
            InCommandContext->StartContext();

            InCommandContext->TransitionBuffer(this, EResourceAccess::Common, EResourceAccess::CopyDest);
            InCommandContext->UpdateBuffer(this, FBufferRegion(0, Info.Size), InInitialData);

            // NOTE: Transfer to the initial state
            if (InInitialAccess != EResourceAccess::CopyDest)
            {
                InCommandContext->TransitionBuffer(this, EResourceAccess::CopyDest, InInitialAccess);
            }

            InCommandContext->FinishContext();
        }
    }
    else
    {
        if (InInitialAccess != EResourceAccess::Common && Info.IsDynamic())
        {
            InCommandContext->StartContext();
            InCommandContext->TransitionBuffer(this, EResourceAccess::Common, InInitialAccess);
            InCommandContext->FinishContext();
        }
    }

    return true;
}

void* FD3D12Buffer::Map(uint64 Offset, uint64 Size)
{
    if (!ResourceStorage.GetResource())
    {
        return nullptr;
    }

    if (!Info.IsDynamic() && !Info.IsReadBack())
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

void FD3D12Buffer::Unmap(uint64 /* Offset */, uint64 /* Size */)
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

void FD3D12Buffer::SetDebugName(const FString& InName)
{
    if (ResourceStorage.GetResource())
    {
        ResourceStorage.GetResource()->SetDebugName(InName);
    }
}

FString FD3D12Buffer::GetDebugName() const
{
    if (ResourceStorage.GetResource())
    {
        return ResourceStorage.GetResource()->GetDebugName();
    }

    return "";
}

void FD3D12Buffer::SetResource(FD3D12Resource* InResource)
{
    ResourceStorage.ReleaseResource();
    
    if (InResource)
    {
        FD3D12ResourceRef ResourceRef = InResource;
        ResourceStorage.SetResource(ResourceRef);
    }

    if (Info.IsConstantBuffer())
    {
        CreateConstantBufferView();
    }
}

bool FD3D12Buffer::CreateConstantBufferView()
{
    CHECK(ResourceStorage.GetResource() != nullptr);

    D3D12_CONSTANT_BUFFER_VIEW_DESC ViewDesc;
    FMemory::Memzero(&ViewDesc);

    ViewDesc.SizeInBytes = Math::AlignUp<uint32>(static_cast<uint32>(Info.Size), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    ViewDesc.BufferLocation = ResourceStorage.GetGpuVirtualAddress();

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

    return true;
}
