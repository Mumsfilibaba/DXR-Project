#pragma once
#include "D3D12Constants.h"

#include <dxgi1_6.h>
#include <d3d12.h>

typedef HRESULT( WINAPI* PFN_CREATE_DXGI_FACTORY_2 )(UINT Flags, REFIID riid, _COM_Outptr_ void** ppFactory);
typedef HRESULT( WINAPI* PFN_DXGI_GET_DEBUG_INTERFACE_1 )(UINT Flags, REFIID riid, _COM_Outptr_ void** pDebug);
typedef HRESULT( WINAPI* PFN_SetMarkerOnCommandList )(ID3D12GraphicsCommandList* commandList, UINT64 color, _In_ PCSTR formatString);

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace NDXGIFunctions
{
    extern PFN_CREATE_DXGI_FACTORY_2      CreateDXGIFactory2;
    extern PFN_DXGI_GET_DEBUG_INTERFACE_1 DXGIGetDebugInterface1;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

namespace ND3D12Functions
{
    extern PFN_D3D12_CREATE_DEVICE                                D3D12CreateDevice;
    extern PFN_D3D12_GET_DEBUG_INTERFACE                          D3D12GetDebugInterface;
    extern PFN_D3D12_SERIALIZE_ROOT_SIGNATURE                     D3D12SerializeRootSignature;
    extern PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER           D3D12CreateRootSignatureDeserializer;
    extern PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE           D3D12SerializeVersionedRootSignature;
    extern PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER D3D12CreateVersionedRootSignatureDeserializer;
    extern PFN_SetMarkerOnCommandList                             SetMarkerOnCommandList;
}