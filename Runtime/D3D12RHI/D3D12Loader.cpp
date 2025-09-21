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

DxcCreateInstanceProc D3D12Functions::DxcCreateInstance = nullptr;

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

void* D3D12Loader::DXGILibrary  = nullptr;
void* D3D12Loader::D3D12Library = nullptr;
void* D3D12Loader::PIXLibrary   = nullptr;
void* D3D12Loader::DXCLibrary   = nullptr;

bool D3D12Loader::Initialize(bool bEnablePIX)
{
    DXGILibrary = FPlatformLibrary::LoadDynamicLib("dxgi");
    if (!DXGILibrary)
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "FAILED to load dxgi.dll");
        return false;
    }
    else
    {
        D3D12_INFO("Loaded dxgi.dll");
    }

    D3D12Library = FPlatformLibrary::LoadDynamicLib("d3d12");
    if (!D3D12Library)
    {
        FPlatformApplicationMisc::MessageBox("ERROR", "FAILED to load d3d12.dll");
        return false;
    }
    else
    {
        D3D12_INFO("Loaded d3d12.dll");
    }

	DXCLibrary = FPlatformLibrary::LoadDynamicLib("dxcompiler");
	if (!DXCLibrary)
	{
		FPlatformApplicationMisc::MessageBox("ERROR", "FAILED to load dxcompiler.dll");
		return false;
	}

    D3D12_LOAD_FUNCTION(CreateDXGIFactory2, DXGILibrary);
    D3D12_LOAD_FUNCTION(DXGIGetDebugInterface1, DXGILibrary);

    D3D12_LOAD_FUNCTION(D3D12CreateDevice, D3D12Library);
    D3D12_LOAD_FUNCTION(D3D12GetDebugInterface, D3D12Library);
    D3D12_LOAD_FUNCTION(D3D12SerializeRootSignature, D3D12Library);
    D3D12_LOAD_FUNCTION(D3D12SerializeVersionedRootSignature, D3D12Library);
    D3D12_LOAD_FUNCTION(D3D12CreateRootSignatureDeserializer, D3D12Library);
    D3D12_LOAD_FUNCTION(D3D12CreateVersionedRootSignatureDeserializer, D3D12Library);

    D3D12_LOAD_FUNCTION(DxcCreateInstance, DXCLibrary);

    if (bEnablePIX)
    {
        PIXLibrary = FPlatformLibrary::LoadDynamicLib("WinPixEventRuntime");
        if (PIXLibrary)
        {
            D3D12_INFO("Loaded WinPixEventRuntime.dll");
            D3D12Functions::SetMarkerOnCommandList = FPlatformLibrary::LoadSymbol<PFN_SetMarkerOnCommandList>("PIXSetMarkerOnCommandList", PIXLibrary);
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
    if (DXGILibrary)
    {
        FPlatformLibrary::FreeDynamicLib(DXGILibrary);
        DXGILibrary = nullptr;
    }

    if (D3D12Library)
    {
        FPlatformLibrary::FreeDynamicLib(D3D12Library);
        D3D12Library = nullptr;
    }

    if (PIXLibrary)
    {
        FPlatformLibrary::FreeDynamicLib(PIXLibrary);
        PIXLibrary = nullptr;
    }

	if (DXCLibrary)
	{
		FPlatformLibrary::FreeDynamicLib(DXCLibrary);
        DXCLibrary = nullptr;
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

    D3D12Functions::DxcCreateInstance = nullptr;
}
