#include "Core/Platform/PlatformLibrary.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "D3D11RHI/D3D11Core.h"
#include "D3D11RHI/D3D11Loader.h"

PFN_CREATE_DXGI_FACTORY_2      D3D11::CreateDXGIFactory2       = nullptr;
PFN_DXGI_GET_DEBUG_INTERFACE_1 D3D11::DXGIGetDebugInterface1   = nullptr;
PFN_D3D11_CREATE_DEVICE        D3D11::D3D11CreateDevice        = nullptr;
#if D3D11_ENABLE_COMPOSITION
PFN_DCOMPOSITION_CREATE_DEVICE D3D11::DCompositionCreateDevice = nullptr;
#endif

#define D3D11_LOAD_FUNCTION(Function, LibraryHandle) \
do \
{ \
    D3D11::Function = FPlatformLibrary::LoadSymbol<decltype(D3D11::Function)>(#Function, LibraryHandle); \
    if (!D3D11::Function) \
    { \
        D3D11_ERROR_CRITICAL("Failed to load '%s'", #Function); \
        return false; \
    } \
} while(false)

void* D3D11::DXGILibrary        = nullptr;
void* D3D11::D3D11Library       = nullptr;
#if D3D11_ENABLE_COMPOSITION
void* D3D11::DCompLibrary       = nullptr;
#endif

bool D3D11::Initialize()
{
    DXGILibrary = FPlatformLibrary::LoadDynamicLib("dxgi");
    if (!DXGILibrary)
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "FAILED to load dxgi.dll");
        return false;
    }
    else
    {
        D3D11_INFO("Loaded dxgi.dll");
    }

    D3D11Library = FPlatformLibrary::LoadDynamicLib("d3d11");
    if (!D3D11Library)
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "FAILED to load d3d11.dll");
        return false;
    }
    else
    {
        D3D11_INFO("Loaded d3d11.dll");
    }

    D3D11_LOAD_FUNCTION(CreateDXGIFactory2, DXGILibrary);
    D3D11_LOAD_FUNCTION(DXGIGetDebugInterface1, DXGILibrary);
    D3D11_LOAD_FUNCTION(D3D11CreateDevice, D3D11Library);

#if D3D11_ENABLE_COMPOSITION
    DCompLibrary = FPlatformLibrary::LoadDynamicLib("dcomp");
    if (DCompLibrary)
    {
        D3D11_INFO("Loaded dcomp.dll");

        D3D11::DCompositionCreateDevice = FPlatformLibrary::LoadSymbol<PFN_DCOMPOSITION_CREATE_DEVICE>("DCompositionCreateDevice", DCompLibrary);
    }

    if (!D3D11::DCompositionCreateDevice)
    {
        D3D11_INFO("DirectComposition NOT found, so a transparent swap chain will present opaque");
    }
#endif

    return true;
}

void D3D11::Release()
{
    if (DXGILibrary)
    {
        FPlatformLibrary::FreeDynamicLib(DXGILibrary);
        DXGILibrary = nullptr;
    }

    if (D3D11Library)
    {
        FPlatformLibrary::FreeDynamicLib(D3D11Library);
        D3D11Library = nullptr;
    }

#if D3D11_ENABLE_COMPOSITION
    if (DCompLibrary)
    {
        FPlatformLibrary::FreeDynamicLib(DCompLibrary);
        DCompLibrary = nullptr;
    }
#endif

    D3D11::CreateDXGIFactory2       = nullptr;
    D3D11::DXGIGetDebugInterface1   = nullptr;
    D3D11::D3D11CreateDevice        = nullptr;
#if D3D11_ENABLE_COMPOSITION
    D3D11::DCompositionCreateDevice = nullptr;
#endif
}
