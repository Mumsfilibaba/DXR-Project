#include "D3D12RHI/D3D12RHI.h"
#include "D3D12RHI/D3D12Allocators.h"
#include "D3D12RHI/D3D12Buffer.h"

FD3D12Buffer::FD3D12Buffer(FD3D12Device* InDevice, const FRHIBufferInfo& InBufferInfo)
    : FRHIBuffer(InBufferInfo)
    , FD3D12GenericResource(InDevice)
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
        D3D12HeapType      = D3D12_HEAP_TYPE_READBACK;
        D3D12InitialState  = D3D12_RESOURCE_STATE_COPY_DEST;
        ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    }
    else if (Info.IsDynamic() || Info.IsTransient())
    {
        D3D12HeapType     = D3D12_HEAP_TYPE_UPLOAD;
        D3D12InitialState = D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    bool bAllocated = false;
    if (Info.IsTransient())
    {
        if (Info.IsConstantBuffer())
        {
            bAllocated = GetDevice()->GetDynamicConstantsAllocator()->Allocate(AlignedSize, ResourceStorage) != nullptr;
        }
        else
        {
            bAllocated = GetDevice()->GetUploadHeapAllocator()->Allocate(AlignedSize, Alignment, ResourceStorage) != nullptr;
        }
    }
    else if (Info.IsDynamic())
    {
        bAllocated = GetDevice()->GetUploadHeapAllocator()->Allocate(AlignedSize, Alignment, ResourceStorage) != nullptr;
    }
    else
    {
        bAllocated = GetDevice()->GetBufferAllocator()->TryAllocate(D3D12HeapType, ResourceDesc, D3D12InitialState, Alignment, ResourceStorage);
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
        if (Info.IsDynamic() || Info.IsTransient())
        {
            void* MappedAddress = ResourceStorage.GetMappedBaseAddress();
            if (!MappedAddress)
            {
                FD3D12Resource* D3D12Resource = ResourceStorage.GetResource();
                MappedAddress = D3D12Resource->MapRange(0, nullptr);
                if (!MappedAddress)
                {
                    D3D12_ERROR("Failed to map buffer data");
                    return false;
                }

                FMemory::Memcpy(MappedAddress, InInitialData, Info.Size);
                D3D12Resource->UnmapRange(0, nullptr);
            }
            else
            {
                FMemory::Memcpy(MappedAddress, InInitialData, Info.Size);
            }
        }
        else
        {
            InCommandContext->StartContext();

            InCommandContext->TransitionBufferState(this, EResourceAccess::Common, EResourceAccess::CopyDest);
            InCommandContext->UpdateBuffer(this, FBufferRegion(0, Info.Size), InInitialData);

            if (InInitialAccess != EResourceAccess::CopyDest)
            {
                InCommandContext->TransitionBufferState(this, EResourceAccess::CopyDest, InInitialAccess);
            }

            InCommandContext->FinishContext();
        }
    }
    else
    {
        if (InInitialAccess != EResourceAccess::Common && D3D12HeapType == D3D12_HEAP_TYPE_DEFAULT)
        {
            InCommandContext->StartContext();
            InCommandContext->TransitionBufferState(this, EResourceAccess::Common, InInitialAccess);
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

    if (!Info.IsDynamic() && !Info.IsReadBack() && !Info.IsTransient())
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
    FString DebugName;
    if (ResourceStorage.GetResource())
    {
        ResourceStorage.GetResource()->GetDebugName(DebugName);
    }
    return DebugName;
}

void FD3D12Buffer::SetResource(FD3D12Resource* InResource)
{
    ResourceStorage.ReleaseResource();
    
    if (InResource)
    {
        ResourceStorage.InitStandalone(InResource);
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

    ConstantBufferView->RegisterWithResource(this);
    return true;
}
