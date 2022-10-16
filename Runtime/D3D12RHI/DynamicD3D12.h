#pragma once
#include "D3D12Constants.h"

#if WIN10_BUILD_17134
    #include <dxgi1_6.h>
#else
    #include <dxgi1_4.h>
#endif

typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY_2)(UINT Flags, REFIID riid, _COM_Outptr_ void** ppFactory);
typedef HRESULT(WINAPI* PFN_DXGI_GET_DEBUG_INTERFACE_1)(UINT Flags, REFIID riid, _COM_Outptr_ void** pDebug);
typedef HRESULT(WINAPI* PFN_SetMarkerOnCommandList)(ID3D12GraphicsCommandList* commandList, UINT64 color, _In_ PCSTR formatString);


class FDynamicD3D12
{
public:
    static bool Initialize(bool bEnablePIX);
    static void Release();

public:
    static PFN_CREATE_DXGI_FACTORY_2                              CreateDXGIFactory2;
    static PFN_DXGI_GET_DEBUG_INTERFACE_1                         DXGIGetDebugInterface1;

    static PFN_D3D12_CREATE_DEVICE                                D3D12CreateDevice;
    static PFN_D3D12_GET_DEBUG_INTERFACE                          D3D12GetDebugInterface;
    static PFN_D3D12_SERIALIZE_ROOT_SIGNATURE                     D3D12SerializeRootSignature;
    static PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER           D3D12CreateRootSignatureDeserializer;
    static PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE           D3D12SerializeVersionedRootSignature;
    static PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER D3D12CreateVersionedRootSignatureDeserializer;

    static PFN_SetMarkerOnCommandList                             SetMarkerOnCommandList;

private:
    static void* DXGILib;
    static void* D3D12Lib;
    static void* PIXLib;
};