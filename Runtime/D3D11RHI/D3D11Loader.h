#pragma once
#include "D3D11RHI/D3D11Constants.h"
#include <dxgi1_6.h>

typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY_2)(UINT Flags, REFIID riid, _COM_Outptr_ void** ppFactory);
typedef HRESULT(WINAPI* PFN_DXGI_GET_DEBUG_INTERFACE_1)(UINT Flags, REFIID riid, _COM_Outptr_ void** pDebug);
#if D3D11_ENABLE_COMPOSITION
typedef HRESULT(WINAPI* PFN_DCOMPOSITION_CREATE_DEVICE)(IDXGIDevice* dxgiDevice, REFIID iid, _COM_Outptr_ void** dcompositionDevice);
#endif

struct D3D11
{
    static bool Initialize();
    static void Release();

    static PFN_CREATE_DXGI_FACTORY_2      CreateDXGIFactory2;
    static PFN_DXGI_GET_DEBUG_INTERFACE_1 DXGIGetDebugInterface1;
    static PFN_D3D11_CREATE_DEVICE        D3D11CreateDevice;
#if D3D11_ENABLE_COMPOSITION
    static PFN_DCOMPOSITION_CREATE_DEVICE DCompositionCreateDevice;
#endif

private:
    static void* DXGILibrary;
    static void* D3D11Library;
#if D3D11_ENABLE_COMPOSITION
    static void* DCompLibrary;
#endif
};
