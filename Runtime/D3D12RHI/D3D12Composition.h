#pragma once
#include "D3D12RHI/D3D12Configuration.h"

#if D3D12_ENABLE_COMPOSITION
#include "Core/RefCountedBase.h"
#include "Core/Windows/Windows.h"
#include "D3D12RHI/D3D12Core.h"
#include "D3D12RHI/D3D12DeviceChild.h"

#include <dxgi1_2.h>
#include <dcomp.h>

typedef TSharedRef<class FD3D12Composition> FD3D12CompositionRef;

class FD3D12Composition : public FD3D12DeviceChild, public FRefCountedBase
{
public:
    FD3D12Composition(FD3D12Device* InDevice);
    virtual ~FD3D12Composition();

    /**
     * @brief Binds a composition swap chain to a window through a DirectComposition visual.
     *
     * @param InHwnd      Window the composition target is created for.
     * @param InSwapChain Composition swap chain the visual presents.
     * @return            True if the visual was created, bound and committed.
     */
    bool Initialize(HWND InHwnd, IDXGISwapChain1* InSwapChain);

private:
    TComPtr<IDCompositionTarget> Target;
    TComPtr<IDCompositionVisual> Visual;
};

#endif
