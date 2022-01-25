#include "D3D12FunctionPointers.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// NDXGIFunctions

namespace NDXGIFunctions
{
    PFN_CREATE_DXGI_FACTORY_2      CreateDXGIFactory2 = nullptr;
    PFN_DXGI_GET_DEBUG_INTERFACE_1 DXGIGetDebugInterface1 = nullptr;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ND3D12Functions

namespace ND3D12Functions
{
    PFN_D3D12_CREATE_DEVICE                                D3D12CreateDevice = nullptr;
    PFN_D3D12_GET_DEBUG_INTERFACE                          D3D12GetDebugInterface = nullptr;
    PFN_D3D12_SERIALIZE_ROOT_SIGNATURE                     D3D12SerializeRootSignature = nullptr;
    PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER           D3D12CreateRootSignatureDeserializer = nullptr;
    PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE           D3D12SerializeVersionedRootSignature = nullptr;
    PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER D3D12CreateVersionedRootSignatureDeserializer = nullptr;
    PFN_SetMarkerOnCommandList                             SetMarkerOnCommandList = nullptr;
}