#include "D3D12Core.h"
#include "D3D12Library.h"

#include "Core/Modules/Platform/PlatformLibrary.h"

#include "CoreApplication/Platform/PlatformApplicationMisc.h"

void* FD3D12Library::DXGILib  = nullptr;
void* FD3D12Library::D3D12Lib = nullptr;
void* FD3D12Library::PIXLib   = nullptr;

PFN_CREATE_DXGI_FACTORY_2                              FD3D12Library::CreateDXGIFactory2                            = nullptr;
PFN_DXGI_GET_DEBUG_INTERFACE_1                         FD3D12Library::DXGIGetDebugInterface1                        = nullptr;

PFN_D3D12_CREATE_DEVICE                                FD3D12Library::D3D12CreateDevice                             = nullptr;
PFN_D3D12_GET_DEBUG_INTERFACE                          FD3D12Library::D3D12GetDebugInterface                        = nullptr;
PFN_D3D12_SERIALIZE_ROOT_SIGNATURE                     FD3D12Library::D3D12SerializeRootSignature                   = nullptr;
PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER           FD3D12Library::D3D12CreateRootSignatureDeserializer          = nullptr;
PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE           FD3D12Library::D3D12SerializeVersionedRootSignature          = nullptr;
PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER FD3D12Library::D3D12CreateVersionedRootSignatureDeserializer = nullptr;

PFN_SetMarkerOnCommandList                             FD3D12Library::SetMarkerOnCommandList                        = nullptr;

#define D3D12_LOAD_FUNCTION(Function, LibraryHandle)                                                 \
    do                                                                                               \
    {                                                                                                \
        Function = PlatformLibrary::LoadSymbolAddress<decltype(Function)>(#Function, LibraryHandle); \
        if (!Function)                                                                               \
        {                                                                                            \
            D3D12_ERROR("Failed to load '%s'", #Function);                                           \
            return false;                                                                            \
        }                                                                                            \
    } while(false)

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FD3D12Library

bool FD3D12Library::Initialize(bool bEnablePIX)
{
    DXGILib = PlatformLibrary::LoadDynamicLib("dxgi");
    if (!DXGILib)
    {
        PlatformApplicationMisc::MessageBox("ERROR", "FAILED to load dxgi.dll");
        return false;
    }
    else
    {
        D3D12_INFO("Loaded dxgi.dll");
    }

    D3D12Lib = PlatformLibrary::LoadDynamicLib("d3d12");
    if (!D3D12Lib)
    {
        PlatformApplicationMisc::MessageBox("ERROR", "FAILED to load d3d12.dll");
        return false;
    }
    else
    {
        D3D12_INFO("Loaded d3d12.dll");
    }

    if (bEnablePIX)
    {
        PIXLib = PlatformLibrary::LoadDynamicLib("WinPixEventRuntime");
        if (PIXLib)
        {
            D3D12_INFO("Loaded WinPixEventRuntime.dll");
            SetMarkerOnCommandList = PlatformLibrary::LoadSymbolAddress<PFN_SetMarkerOnCommandList>("PIXSetMarkerOnCommandList", PIXLib);
        }
        else
        {
            D3D12_INFO("PIX Runtime NOT found");
        }
    }

    D3D12_LOAD_FUNCTION(CreateDXGIFactory2    , DXGILib);
    D3D12_LOAD_FUNCTION(DXGIGetDebugInterface1, DXGILib);

    D3D12_LOAD_FUNCTION(D3D12CreateDevice                            , D3D12Lib);
    D3D12_LOAD_FUNCTION(D3D12GetDebugInterface                       , D3D12Lib);
    D3D12_LOAD_FUNCTION(D3D12SerializeRootSignature                  , D3D12Lib);
    D3D12_LOAD_FUNCTION(D3D12SerializeVersionedRootSignature         , D3D12Lib);
    D3D12_LOAD_FUNCTION(D3D12CreateRootSignatureDeserializer         , D3D12Lib);
    D3D12_LOAD_FUNCTION(D3D12CreateVersionedRootSignatureDeserializer, D3D12Lib);

    return true;
}

void FD3D12Library::Release()
{
    if (DXGILib)
    {
        PlatformLibrary::FreeDynamicLib(DXGILib);
        DXGILib = nullptr;
    }

    if (D3D12Lib)
    {
        PlatformLibrary::FreeDynamicLib(D3D12Lib);
        D3D12Lib = nullptr;
    }

    if (PIXLib)
    {
        PlatformLibrary::FreeDynamicLib(PIXLib);
        PIXLib = nullptr;
    }

    CreateDXGIFactory2                            = nullptr;
    DXGIGetDebugInterface1                        = nullptr;
    D3D12CreateDevice                             = nullptr;
    D3D12GetDebugInterface                        = nullptr;
    D3D12SerializeRootSignature                   = nullptr;
    D3D12SerializeVersionedRootSignature          = nullptr;
    D3D12CreateRootSignatureDeserializer          = nullptr;
    D3D12CreateVersionedRootSignatureDeserializer = nullptr;
}
