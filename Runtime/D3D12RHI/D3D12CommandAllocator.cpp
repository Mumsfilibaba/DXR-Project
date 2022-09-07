#include "D3D12CommandAllocator.h"
#include "D3D12Device.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12CommandAllocator

FD3D12CommandAllocator::FD3D12CommandAllocator(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , Allocator(nullptr)
{ }

bool FD3D12CommandAllocator::Create(D3D12_COMMAND_LIST_TYPE Type)
{
    HRESULT Result = GetDevice()->GetD3D12Device()->CreateCommandAllocator(Type, IID_PPV_ARGS(&Allocator));
    if (SUCCEEDED(Result))
    {
        D3D12_INFO("[FD3D12CommandAllocator]: Created CommandAllocator");
        return true;
    }
    else
    {
        D3D12_ERROR("[FD3D12CommandAllocator]: FAILED to create CommandAllocator");
        return false;
    }
}

bool FD3D12CommandAllocator::Reset()
{
    HRESULT Result = Allocator->Reset();
    if (Result == DXGI_ERROR_DEVICE_REMOVED)
    {
        D3D12DeviceRemovedHandlerRHI(GetDevice());
    }

    return SUCCEEDED(Result);
}