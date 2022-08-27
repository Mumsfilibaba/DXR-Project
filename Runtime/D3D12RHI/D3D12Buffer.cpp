#include "D3D12Buffer.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12Buffer

FD3D12Buffer::FD3D12Buffer(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
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