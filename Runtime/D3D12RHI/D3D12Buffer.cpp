#include "D3D12Buffer.h"
#include "D3D12Interface.h"

FD3D12Buffer::FD3D12Buffer(FD3D12Device* InDevice, const FRHIBufferDesc& InDesc)
    : FRHIBuffer(InDesc)
    , FD3D12DeviceChild(InDevice)
    , Resource(nullptr)
{ }

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
        if (!View)
        {
            View = dbg_new FD3D12ConstantBufferView(GetDevice(), FD3D12Interface::GetRHI()->GetResourceOfflineDescriptorHeap());
        }

        if (Resource)
        {
            D3D12_CONSTANT_BUFFER_VIEW_DESC ViewDesc;
            FMemory::Memzero(&ViewDesc);

            ViewDesc.BufferLocation = Resource->GetGPUVirtualAddress();
            ViewDesc.SizeInBytes    = static_cast<uint32>(Resource->GetSize());

            if (View->GetOfflineHandle() == 0)
            {
                if (!View->AllocateHandle())
                {
                    D3D12_ERROR("Failed to allocate ConstantBuffer Descriptor");
                    return;
                }
            }

            if (!View->CreateView(Resource.Get(), ViewDesc))
            {
                D3D12_ERROR("Failed to Create ConstantBufferView");
            }
        }
    }
}
