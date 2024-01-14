#include "D3D12Buffer.h"
#include "D3D12RHI.h"

FD3D12Buffer::FD3D12Buffer(FD3D12Device* InDevice, const FRHIBufferDesc& InDesc)
    : FRHIBuffer(InDesc)
    , FD3D12DeviceChild(InDevice)
    , Resource(nullptr)
{
}

FD3D12Buffer::~FD3D12Buffer()
{
    // NOTE: Empty for now
}

bool FD3D12Buffer::Initialize(EResourceAccess InInitialAccess, const void* InInitialData)
{
    const uint64 Alignment   = GetBufferAlignment(Desc.UsageFlags);
    const uint64 AlignedSize = FMath::AlignUp(Desc.Size, Alignment);

    D3D12_RESOURCE_DESC ResourceDesc;
    FMemory::Memzero(&ResourceDesc);

    ResourceDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    ResourceDesc.Flags              = ConvertBufferFlags(Desc.UsageFlags);
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
    if (Desc.IsDynamic())
    {
        D3D12HeapType     = D3D12_HEAP_TYPE_UPLOAD;
        D3D12InitialState = D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    // Limit the scope of the new resource
    {
        FD3D12ResourceRef NewResource = new FD3D12Resource(GetDevice(), ResourceDesc, D3D12HeapType);
        if (NewResource->Initialize(D3D12InitialState, nullptr))
        {
            Resource = NewResource;

            if (Desc.IsConstantBuffer())
            {
                View = new FD3D12ConstantBufferView(GetDevice(), FD3D12RHI::GetRHI()->GetResourceOfflineDescriptorHeap());
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
            FD3D12Resource* D3D12Resource = GetD3D12Resource();

            // Map buffer memory
            void* BufferData = D3D12Resource->MapRange(0, nullptr);
            if (!BufferData)
            {
                D3D12_ERROR("Failed to map buffer data");
                return false;
            }

            // Copy over relevant data
            FMemory::Memcpy(BufferData, InInitialData, Desc.Size);

            // Unmap buffer memory
            D3D12Resource->UnmapRange(0, nullptr);
        }
        else
        {
            FD3D12CommandContext* Context = FD3D12RHI::GetRHI()->ObtainCommandContext();
            Context->RHIStartContext();

            Context->RHITransitionBuffer(this, EResourceAccess::Common, EResourceAccess::CopyDest);
            Context->RHIUpdateBuffer(this, FBufferRegion(0, Desc.Size), InInitialData);

            // NOTE: Transfer to the initial state
            if (InInitialAccess != EResourceAccess::CopyDest)
            {
                Context->RHITransitionBuffer(this, EResourceAccess::CopyDest, InInitialAccess);
            }

            Context->RHIFinishContext();
        }
    }
    else
    {
        if (InInitialAccess != EResourceAccess::Common && Desc.IsDynamic())
        {
            FD3D12CommandContext* Context = FD3D12RHI::GetRHI()->ObtainCommandContext();
            Context->RHIStartContext();
            Context->RHITransitionBuffer(this, EResourceAccess::Common, InInitialAccess);
            Context->RHIFinishContext();
        }
    }

    return true;
}

void FD3D12Buffer::SetName(const FString& InName)
{
    if (Resource)
    {
        Resource->SetName(InName);
    }
}

FString FD3D12Buffer::GetName() const
{
    if (Resource)
    {
        return Resource->GetName();
    }

    return "";
}

void FD3D12Buffer::SetResource(FD3D12Resource* InResource)
{
    Resource = InResource;

    if (Desc.IsConstantBuffer())
    {
        CreateCBV();
    }
}

bool FD3D12Buffer::CreateCBV()
{
    CHECK(Resource != nullptr);

    D3D12_CONSTANT_BUFFER_VIEW_DESC ViewDesc;
    FMemory::Memzero(&ViewDesc);

    ViewDesc.BufferLocation = Resource->GetGPUVirtualAddress();
    ViewDesc.SizeInBytes    = static_cast<uint32>(Resource->GetSize());

    if (FD3D12_CPU_DESCRIPTOR_HANDLE(0) == View->GetOfflineHandle())
    {
        if (!View->AllocateHandle())
        {
            D3D12_ERROR("Failed to allocate ConstantBuffer Descriptor");
            return false;
        }
    }

    if (!View->CreateView(Resource.Get(), ViewDesc))
    {
        D3D12_ERROR("Failed to Create ConstantBufferView");
        return false;
    }

    return true;
}
