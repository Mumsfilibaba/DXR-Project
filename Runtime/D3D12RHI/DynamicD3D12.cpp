#include "D3D12Core.h"
#include "DynamicD3D12.h"
#include "Core/Platform/PlatformLibrary.h"
#include "Core/Misc/ConsoleManager.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"

void* FDynamicD3D12::DXGILib  = nullptr;
void* FDynamicD3D12::D3D12Lib = nullptr;
void* FDynamicD3D12::PIXLib   = nullptr;

PFN_CREATE_DXGI_FACTORY_2                              FDynamicD3D12::CreateDXGIFactory2                            = nullptr;
PFN_DXGI_GET_DEBUG_INTERFACE_1                         FDynamicD3D12::DXGIGetDebugInterface1                        = nullptr;

PFN_D3D12_CREATE_DEVICE                                FDynamicD3D12::D3D12CreateDevice                             = nullptr;
PFN_D3D12_GET_DEBUG_INTERFACE                          FDynamicD3D12::D3D12GetDebugInterface                        = nullptr;
PFN_D3D12_SERIALIZE_ROOT_SIGNATURE                     FDynamicD3D12::D3D12SerializeRootSignature                   = nullptr;
PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER           FDynamicD3D12::D3D12CreateRootSignatureDeserializer          = nullptr;
PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE           FDynamicD3D12::D3D12SerializeVersionedRootSignature          = nullptr;
PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER FDynamicD3D12::D3D12CreateVersionedRootSignatureDeserializer = nullptr;

PFN_SetMarkerOnCommandList                             FDynamicD3D12::SetMarkerOnCommandList                        = nullptr;

#define D3D12_LOAD_FUNCTION(Function, LibraryHandle)                                                  \
    do                                                                                                \
    {                                                                                                 \
        Function = FPlatformLibrary::LoadSymbolAddress<decltype(Function)>(#Function, LibraryHandle); \
        if (!Function)                                                                                \
        {                                                                                             \
            D3D12_ERROR("Failed to load '%s'", #Function);                                            \
            return false;                                                                             \
        }                                                                                             \
    } while(false)


bool FDynamicD3D12::Initialize(bool bEnablePIX)
{
    DXGILib = FPlatformLibrary::LoadDynamicLib("dxgi");
    if (!DXGILib)
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "FAILED to load dxgi.dll");
        return false;
    }
    else
    {
        D3D12_INFO("Loaded dxgi.dll");
    }

    D3D12Lib = FPlatformLibrary::LoadDynamicLib("d3d12");
    if (!D3D12Lib)
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "FAILED to load d3d12.dll");
        return false;
    }
    else
    {
        D3D12_INFO("Loaded d3d12.dll");
    }

    if (bEnablePIX)
    {
        PIXLib = FPlatformLibrary::LoadDynamicLib("WinPixEventRuntime");
        if (PIXLib)
        {
            D3D12_INFO("Loaded WinPixEventRuntime.dll");
            SetMarkerOnCommandList = FPlatformLibrary::LoadSymbolAddress<PFN_SetMarkerOnCommandList>("PIXSetMarkerOnCommandList", PIXLib);
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

void FDynamicD3D12::Release()
{
    if (DXGILib)
    {
        FPlatformLibrary::FreeDynamicLib(DXGILib);
        DXGILib = nullptr;
    }

    if (D3D12Lib)
    {
        FPlatformLibrary::FreeDynamicLib(D3D12Lib);
        D3D12Lib = nullptr;
    }

    if (PIXLib)
    {
        FPlatformLibrary::FreeDynamicLib(PIXLib);
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
