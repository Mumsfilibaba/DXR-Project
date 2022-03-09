#include "D3D12Buffer.h"
#include "D3D12CommandContext.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CD3D12Buffer

CD3D12Buffer::CD3D12Buffer(CD3D12Device* InDevice, const CRHIBufferDesc& InBufferDesc)
    : CRHIBuffer(InBufferDesc)
    , CD3D12DeviceObject(InDevice)
    , Resource(nullptr)
{
}

bool CD3D12Buffer::Initialize(CD3D12CommandContext* CommandContext, ERHIResourceState InitalState, const SRHIResourceData* InitialData)
{
    D3D12_ERROR(CommandContext != nullptr, "CommandContext cannot be nullptr");

    D3D12_RESOURCE_DESC ResourceDesc;
    CMemory::Memzero(&ResourceDesc);

    ResourceDesc.Dimension          = D3D12_RESOURCE_DIMENSION_BUFFER;
    ResourceDesc.Flags              = ConvertBufferFlags(BufferDesc.Flags);
    ResourceDesc.Format             = DXGI_FORMAT_UNKNOWN;
    ResourceDesc.Layout             = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ResourceDesc.Width              = BufferDesc.SizeInBytes;
    ResourceDesc.Height             = 1;
    ResourceDesc.DepthOrArraySize   = 1;
    ResourceDesc.MipLevels          = 1;
    ResourceDesc.Alignment          = 0;
    ResourceDesc.SampleDesc.Count   = 1;
    ResourceDesc.SampleDesc.Quality = 0;

    D3D12_HEAP_TYPE       HeapType     = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_STATES InitialState = D3D12_RESOURCE_STATE_COMMON;
    if (BufferDesc.IsDynamic())
    {
        HeapType     = D3D12_HEAP_TYPE_UPLOAD;
        InitialState = D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    Resource = dbg_new CD3D12Resource(GetDevice(), ResourceDesc, HeapType);
    if (!Resource->Init(InitialState, nullptr))
    {
        return false;
    }

    if (InitialData)
    {
        D3D12_ERROR(InitialData->GetSizeInBytes() <= BufferDesc.SizeInBytes, "Size of InitialData is larger than the allocated memory");

        if (BufferDesc.IsDynamic())
        {
            void* HostData = Resource->Map(0, 0);
            if (!HostData)
            {
                return false;
            }

            // Copy over relevant data
            const uint32 InitialDataSize = InitialData->GetSizeInBytes();
            CMemory::Memcpy(HostData, InitialData->GetData(), InitialDataSize);

            // Set the remaining, unused memory to zero
            CMemory::Memzero(reinterpret_cast<uint8*>(HostData) + InitialDataSize, BufferDesc.SizeInBytes - InitialDataSize);

            Resource->Unmap(0, 0);
        }
        else
        {
            CommandContext->Begin();

            CommandContext->TransitionBuffer(this, ERHIResourceState::Common, ERHIResourceState::CopyDest);
            CommandContext->UpdateBuffer(this, 0, InitialData->GetSizeInBytes(), InitialData->GetData());
            CommandContext->TransitionBuffer(this, ERHIResourceState::CopyDest, InitialState);

            CommandContext->End();
        }
    }
    else
    {
        if (InitialState != ERHIResourceState::Common && !BufferDesc.IsDynamic())
        {
            CommandContext->Begin();
            CommandContext->TransitionBuffer(this, ERHIResourceState::Common, InitialState);
            CommandContext->End();
        }
    }

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
    return Resource ? reinterpret_cast<void*>(Resource->GetResource()) : nullptr;
}

virtual bool CD3D12Buffer::IsValid() const override
{
    return Resource ? (Resource->GetResource() != nullptr) : false;
}

void CD3D12Buffer::SetResource(const CD3D12ResourceRef& InResource) 
{ 
    Resource = InResource;
}