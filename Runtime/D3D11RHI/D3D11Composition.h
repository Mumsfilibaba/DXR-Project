#pragma once
#include "D3D11RHI/D3D11Configuration.h"

#if D3D11_ENABLE_COMPOSITION
#include "Core/RefCountedBase.h"
#include "Core/Windows/Windows.h"
#include "D3D11RHI/D3D11Core.h"
#include "D3D11RHI/D3D11DeviceChild.h"

#include <dxgi1_2.h>
#include <dcomp.h>

typedef TSharedRef<class FD3D11Composition> FD3D11CompositionRef;

class FD3D11Composition : public FD3D11DeviceChild, public FRefCountedBase
{
public:
    FD3D11Composition(FD3D11Device* InDevice);
    virtual ~FD3D11Composition();

    bool Initialize(HWND InHwnd, IDXGISwapChain1* InSwapChain);

private:
    TComPtr<IDCompositionTarget> Target;
    TComPtr<IDCompositionVisual> Visual;
};

#endif
