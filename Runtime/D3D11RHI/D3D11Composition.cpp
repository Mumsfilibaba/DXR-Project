#include "D3D11RHI/D3D11Composition.h"

#if D3D11_ENABLE_COMPOSITION
#include "D3D11RHI/D3D11Device.h"

FD3D11Composition::FD3D11Composition(FD3D11Device* InDevice)
    : FD3D11DeviceChild(InDevice)
    , Target(nullptr)
    , Visual(nullptr)
{
}

FD3D11Composition::~FD3D11Composition() = default;

bool FD3D11Composition::Initialize(HWND InHwnd, IDXGISwapChain1* InSwapChain)
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
