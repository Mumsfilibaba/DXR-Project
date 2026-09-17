#include "D3D12RHI/D3D12Composition.h"

#if D3D12_ENABLE_COMPOSITION
#include "D3D12RHI/D3D12Device.h"

FD3D12Composition::FD3D12Composition(FD3D12Device* InDevice)
    : FD3D12DeviceChild(InDevice)
    , Target(nullptr)
    , Visual(nullptr)
{
}

FD3D12Composition::~FD3D12Composition() = default;

bool FD3D12Composition::Initialize(HWND InHwnd, IDXGISwapChain1* InSwapChain)
{
    IDCompositionDevice* CompositionDevice = GetDevice()->GetCompositionDevice();
    if (!CompositionDevice)
    {
        return false;
    }

    if (FAILED(CompositionDevice->CreateTargetForHwnd(InHwnd, TRUE, &Target)))
    {
        return false;
    }

    if (FAILED(CompositionDevice->CreateVisual(&Visual)))
    {
        return false;
    }

    return SUCCEEDED(Visual->SetContent(InSwapChain))
        && SUCCEEDED(Target->SetRoot(Visual.Get()))
        && SUCCEEDED(CompositionDevice->Commit());
}

#endif
