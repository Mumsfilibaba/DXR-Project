#pragma once
#include "D3D11RHI/D3D11Constants.h"
#include <dxgi1_6.h>

typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY_2)(UINT Flags, REFIID riid, _COM_Outptr_ void** ppFactory);
typedef HRESULT(WINAPI* PFN_DXGI_GET_DEBUG_INTERFACE_1)(UINT Flags, REFIID riid, _COM_Outptr_ void** pDebug);
typedef HRESULT(WINAPI* PFN_D3D_REFLECT)(LPCVOID pSrcData, SIZE_T SrcDataSize, REFIID pInterface, void** ppReflector);

struct D3D11
{
    static bool Initialize();
    static void Release();

    static PFN_CREATE_DXGI_FACTORY_2      CreateDXGIFactory2;
    static PFN_DXGI_GET_DEBUG_INTERFACE_1 DXGIGetDebugInterface1;
    static PFN_D3D11_CREATE_DEVICE        D3D11CreateDevice;
    static PFN_D3D_REFLECT                D3DReflect;

private:
    static void* DXGILibrary;
    static void* D3D11Library;
    static void* D3DCompilerLibrary;
};
