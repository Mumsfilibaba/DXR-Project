#include "Core/Platform/PlatformLibrary.h"
#include "Core/Misc/ConsoleManager.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "D3D12RHI/D3D12Core.h"
#include "D3D12RHI/D3D12Loader.h"

PFN_CREATE_DXGI_FACTORY_2 D3D12Functions::CreateDXGIFactory2 = nullptr;
PFN_DXGI_GET_DEBUG_INTERFACE_1 D3D12Functions::DXGIGetDebugInterface1 = nullptr;

PFN_D3D12_CREATE_DEVICE D3D12Functions::D3D12CreateDevice = nullptr;
PFN_D3D12_GET_DEBUG_INTERFACE D3D12Functions::D3D12GetDebugInterface = nullptr;
PFN_D3D12_SERIALIZE_ROOT_SIGNATURE D3D12Functions::D3D12SerializeRootSignature = nullptr;
PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER D3D12Functions::D3D12CreateRootSignatureDeserializer = nullptr;
PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE D3D12Functions::D3D12SerializeVersionedRootSignature = nullptr;
PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER D3D12Functions::D3D12CreateVersionedRootSignatureDeserializer = nullptr;

PFN_SetMarkerOnCommandList D3D12Functions::SetMarkerOnCommandList = nullptr;

#define D3D12_LOAD_FUNCTION(Function, LibraryHandle) \
do \
{ \
    D3D12Functions::Function = FPlatformLibrary::LoadSymbol<decltype(D3D12Functions::Function)>(#Function, LibraryHandle); \
    if (!D3D12Functions::Function) \
    { \
        D3D12_ERROR_CRITICAL("Failed to load '%s'", #Function); \
        return false; \
    } \
} while(false)

void* D3D12Loader::DXGILib  = nullptr;
void* D3D12Loader::D3D12Lib = nullptr;
void* D3D12Loader::PIXLib   = nullptr;

bool D3D12Loader::Initialize(bool bEnablePIX)
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

    D3D12_LOAD_FUNCTION(CreateDXGIFactory2, DXGILib);
    D3D12_LOAD_FUNCTION(DXGIGetDebugInterface1, DXGILib);

    D3D12_LOAD_FUNCTION(D3D12CreateDevice, D3D12Lib);
    D3D12_LOAD_FUNCTION(D3D12GetDebugInterface, D3D12Lib);
    D3D12_LOAD_FUNCTION(D3D12SerializeRootSignature, D3D12Lib);
    D3D12_LOAD_FUNCTION(D3D12SerializeVersionedRootSignature, D3D12Lib);
    D3D12_LOAD_FUNCTION(D3D12CreateRootSignatureDeserializer, D3D12Lib);
    D3D12_LOAD_FUNCTION(D3D12CreateVersionedRootSignatureDeserializer, D3D12Lib);

    if (bEnablePIX)
    {
        PIXLib = FPlatformLibrary::LoadDynamicLib("WinPixEventRuntime");
        if (PIXLib)
        {
            D3D12_INFO("Loaded WinPixEventRuntime.dll");
            D3D12Functions::SetMarkerOnCommandList = FPlatformLibrary::LoadSymbol<PFN_SetMarkerOnCommandList>("PIXSetMarkerOnCommandList", PIXLib);
        }
        else
        {
            D3D12_INFO("PIX Runtime NOT found");
        }
    }

    return true;
}

void D3D12Loader::Release()
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

    D3D12Functions::CreateDXGIFactory2 = nullptr;
    D3D12Functions::DXGIGetDebugInterface1 = nullptr;

    D3D12Functions::D3D12CreateDevice = nullptr;
    D3D12Functions::D3D12GetDebugInterface = nullptr;
    D3D12Functions::D3D12SerializeRootSignature = nullptr;
    D3D12Functions::D3D12SerializeVersionedRootSignature = nullptr;
    D3D12Functions::D3D12CreateRootSignatureDeserializer = nullptr;
    D3D12Functions::D3D12CreateVersionedRootSignatureDeserializer = nullptr;

    D3D12Functions::SetMarkerOnCommandList = nullptr;
}
