#include "D3D12RHI/D3D12RHI.h"
#include "D3D12RHI/D3D12Buffer.h"
#include "D3D12RHI/D3D12CommandContext.h"

FD3D12BufferRHI::FD3D12BufferRHI(FD3D12Device* InDevice, const FRHIBufferDesc& InBufferDesc)
    : FRHIBuffer(InBufferDesc)
    , FD3D12DeviceChild(InDevice)
    , Resource(nullptr)
{
}

FD3D12BufferRHI::~FD3D12BufferRHI()
{
}

void FD3D12BufferRHI::EnableStateTracking(EResourceAccess InitialState)
{
    if (!ResourceState)
    {
        ResourceState = MakeUniquePtr<FD3D12ResourceState>(ConvertResourceState(InitialState), 1, 1);
    }
}

void FD3D12BufferRHI::DisableStateTracking(FD3D12CommandContext* CommandContext)
{
    if (!ResourceState)
    {
        return;
    }

    if (CommandContext)
    {
        CommandContext->RequireBufferState(this, EResourceAccess::Common);
    }

    ResourceState.Reset();
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

    D3D12_HEAP_TYPE       D3D12HeapType     = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_STATES D3D12InitialState = D3D12_RESOURCE_STATE_COMMON;

    if (Desc.IsReadBack())
    {
        // Readback resources must be placed in a READBACK heap and are only valid as copy destinations.
        D3D12HeapType      = D3D12_HEAP_TYPE_READBACK;
        D3D12InitialState  = D3D12_RESOURCE_STATE_COPY_DEST;
        ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    }
    else if (Desc.IsDynamic())
    {
        D3D12HeapType     = D3D12_HEAP_TYPE_UPLOAD;
        D3D12InitialState = D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    // Support tight alignment if the device supports it
    if (GD3D12SupportTightAlignment)
    {
        ResourceDesc.Flags |= D3D12_RESOURCE_FLAG_USE_TIGHT_ALIGNMENT;
    }

    // Limit the scope of the new resource
    {
        FD3D12ResourceRef NewResource = new FD3D12Resource(GetDevice(), ResourceDesc, D3D12HeapType);
        if (NewResource->Initialize(D3D12InitialState, nullptr))
        {
            Resource = NewResource;

            if (Desc.IsConstantBuffer())
            {
                ConstantBufferView = new FD3D12ConstantBufferView(GetDevice(), GetDevice()->GetResourceOfflineDescriptorHeap());
                if (!CreateCBV())
                {
                    return false;
                }
            }
        }
        else
        {
            return false;
        }
    }

    if (InInitialData)
    {
        if (Desc.IsDynamic())
        {
            FD3D12Resource* D3D12Resource = GetResource();

            void* BufferData = D3D12Resource->MapRange(0, nullptr);
            if (!BufferData)
            {
                D3D12_ERROR("Failed to map buffer data");
                return false;
            }

            FMemory::Memcpy(BufferData, InInitialData, Desc.Size);
            D3D12Resource->UnmapRange(0, nullptr);
        }
        else
        {
            InCommandContext->StartContext();

            InCommandContext->TransitionBuffer(this, EResourceAccess::Common, EResourceAccess::CopyDest);
            InCommandContext->UpdateBuffer(this, FBufferRegion(0, Desc.Size), InInitialData);

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
        if (InInitialAccess != EResourceAccess::Common && Desc.IsDynamic())
        {
            InCommandContext->StartContext();
            InCommandContext->TransitionBuffer(this, EResourceAccess::Common, InInitialAccess);
            InCommandContext->FinishContext();
        }
    }

    return true;
}

void* FD3D12BufferRHI::Map(uint64 Offset, uint64 Size)
{
    if (!Resource)
    {
        return nullptr;
    }

    if (!Desc.IsDynamic() && !Desc.IsReadBack())
    {
        D3D12_ERROR("Attempting to map a non-mappable buffer. Name='%s'", *GetDebugName());
        return nullptr;
    }

    const uint64 BufferSize = Resource->GetSize();
    CHECK(Offset <= BufferSize);

    uint64 MapSize = Size;
    if (MapSize == UINT64_MAX)
    {
        MapSize = BufferSize - Offset;
    }

    D3D12_RANGE ReadRange = {};
    ReadRange.Begin = Offset;
    ReadRange.End   = Offset + MapSize;

    uint8* MappedData = reinterpret_cast<uint8*>(Resource->MapRange(0, &ReadRange));
    if (!MappedData)
    {
        return nullptr;
    }

    return MappedData + Offset;
}

void FD3D12BufferRHI::Unmap(uint64 /* Offset */, uint64 /* Size */)
{
    if (!Resource)
    {
        return;
    }

    // We generally use these mappings for readback or full-buffer writes; keep it simple here.
    Resource->UnmapRange(0, nullptr);
}

void FD3D12BufferRHI::SetDebugName(const FString& InName)
{
    if (Resource)
    {
        Resource->SetDebugName(InName);
    }
}

FString FD3D12BufferRHI::GetDebugName() const
{
    if (Resource)
    {
        return Resource->GetDebugName();
    }

    return "";
}

void FD3D12BufferRHI::SetResource(FD3D12Resource* InResource)
{
    Resource = InResource;

    if (Desc.IsConstantBuffer())
    {
        CreateCBV();
    }
}

bool FD3D12BufferRHI::CreateCBV()
{
    CHECK(Resource != nullptr);

    D3D12_CONSTANT_BUFFER_VIEW_DESC ViewDesc;
    FMemory::Memzero(&ViewDesc);

    ViewDesc.SizeInBytes = Math::AlignUp<uint32>(static_cast<uint32>(Resource->GetSize()), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    ViewDesc.BufferLocation = Resource->GetGPUVirtualAddress();

    if (FD3D12_CPU_DESCRIPTOR_HANDLE(0) == ConstantBufferView->GetOfflineHandle())
    {
        if (!ConstantBufferView->AllocateHandle())
        {
            D3D12_ERROR_CRITICAL("Failed to allocate ConstantBuffer Descriptor");
            return false;
        }
    }

    if (!ConstantBufferView->CreateView(Resource.Get(), ViewDesc))
    {
        D3D12_ERROR_CRITICAL("Failed to Create ConstantBufferView");
        return false;
    }

    return true;
}
