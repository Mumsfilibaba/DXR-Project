#include "D3D12Buffer.h"
#include "D3D12CommandContext.h"
#include "RHIInstanceD3D12.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12Buffer

CD3D12Buffer::CD3D12Buffer(CD3D12Device* InDevice, const CRHIBufferDesc& InBufferDesc)
    : CRHIBuffer(InBufferDesc)
    , CD3D12DeviceObject(InDevice)
    , Resource(nullptr)
{
}

bool CD3D12Buffer::Initialize(CD3D12CommandContext* CommandContext, EResourceAccess RHIInitalState, const SRHIResourceData* InitialData)
{
    D3D12_ERROR(CommandContext != nullptr, "CommandContext cannot be nullptr");

    D3D12_RESOURCE_DESC ResourceDesc;
    CMemory::Memzero(&ResourceDesc);

    ResourceDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    ResourceDesc.Flags              = ConvertBufferFlags(BufferDesc.Flags);
    ResourceDesc.Format             = DXGI_FORMAT_UNKNOWN;
    ResourceDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ResourceDesc.Height             = 1;
    ResourceDesc.DepthOrArraySize   = 1;
    ResourceDesc.MipLevels          = 1;
    ResourceDesc.Alignment          = 0;
    ResourceDesc.SampleDesc.Count   = 1;
    ResourceDesc.SampleDesc.Quality = 0;

    if (BufferDesc.IsConstantBuffer())
    {
        ResourceDesc.Width = NMath::AlignUp<uint32>(BufferDesc.SizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    }
    else
    {
        ResourceDesc.Width = BufferDesc.SizeInBytes;
    }

    D3D12_HEAP_TYPE       HeapType     = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_STATES InitialState = D3D12_RESOURCE_STATE_COMMON;
    if (BufferDesc.IsDynamic())
    {
        HeapType     = D3D12_HEAP_TYPE_UPLOAD;
        InitialState = D3D12_RESOURCE_STATE_GENERIC_READ;
    }
    else if (BufferDesc.IsReadBack())
    {
        HeapType     = D3D12_HEAP_TYPE_READBACK;
        InitialState = D3D12_RESOURCE_STATE_COPY_DEST;
    }

    Resource = CD3D12Resource::CreateResource(GetDevice(), ResourceDesc, HeapType, InitialState, nullptr);
    if (!Resource)
    {
        D3D12_ERROR_ALWAYS("Failed to create Resource");
        return false;
    }

    if (InitialData)
    {
        D3D12_ERROR(InitialData->GetSizeInBytes() <= BufferDesc.SizeInBytes, "Size of InitialData is larger than the allocated memory");

        if (BufferDesc.IsDynamic())
        {
            void* HostData = nullptr;
            
            const bool Result = Resource->MapResource(0, 0, &HostData);
            if (!Result || !HostData)
            {
                return false;
            }

            // Copy over relevant data
            const uint32 InitialDataSize = InitialData->GetSizeInBytes();
            CMemory::Memcpy(HostData, InitialData->GetData(), InitialDataSize);

            // Set the remaining, unused memory to zero
            CMemory::Memzero(reinterpret_cast<uint8*>(HostData) + InitialDataSize, BufferDesc.SizeInBytes - InitialDataSize);

            Resource->UnmapResource(0, 0);
        }
        else
        {
            CommandContext->Begin();

            CommandContext->TransitionBuffer(this, EResourceAccess::Common, EResourceAccess::CopyDest);
            CommandContext->UpdateBuffer(this, 0, InitialData->GetSizeInBytes(), InitialData->GetData());
            CommandContext->TransitionBuffer(this, EResourceAccess::CopyDest, RHIInitalState);

            CommandContext->End();
        }
    }
    else
    {
        if (RHIInitalState != EResourceAccess::Common && !BufferDesc.IsDynamic())
        {
            CommandContext->Begin();
            CommandContext->TransitionBuffer(this, EResourceAccess::Common, RHIInitalState);
            CommandContext->End();
        }
    }

    if (BufferDesc.IsConstantBuffer())
    {
        ConstantBufferView = dbg_new CD3D12ConstantBufferView(GetDevice(), GetDevice()->GetInstance()->GetResourceOfflineDescriptorHeap());
        if (!CreateConstantBufferView())
        {
            return false;
        }
    }

    return true;
}

bool CD3D12Buffer::CreateConstantBufferView()
{
    D3D12_CONSTANT_BUFFER_VIEW_DESC ViewDesc;
    CMemory::Memzero(&ViewDesc);

    ViewDesc.BufferLocation = Resource->GetGPUVirtualAddress();
    ViewDesc.SizeInBytes    = static_cast<UINT>(Resource->GetSizeInBytes());

    if (ConstantBufferView->GetOfflineHandle() == 0)
    {
        if (!ConstantBufferView->AllocateHandle())
        {
            return false;
        }
    }

    ConstantBufferView->CreateView(Resource.Get(), ViewDesc);
    return true;
}

void CD3D12Buffer::SetName(const String& InName)
{
    CRHIResource::SetName(InName);

    if (Resource)
    {
        Resource->SetName(InName);
    }
}

void* CD3D12Buffer::GetNativeResource() const
{
    return Resource ? reinterpret_cast<void*>(Resource->GetD3D12Resource()) : nullptr;
}

bool CD3D12Buffer::IsValid() const
{
    return Resource ? (Resource->GetD3D12Resource() != nullptr) : false;
}

void CD3D12Buffer::SetResource(const CD3D12ResourceRef& InResource) 
{ 
    Resource = InResource;

    if (BufferDesc.IsConstantBuffer())
    {
        CreateConstantBufferView();
    }
}