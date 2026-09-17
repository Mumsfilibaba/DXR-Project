#include "Core/Platform/PlatformLibrary.h"
#include "Core/Misc/ConsoleManager.h"
#include "CoreApplication/Platform/PlatformApplicationMisc.h"
#include "D3D12RHI/D3D12Core.h"
#include "D3D12RHI/D3D12Loader.h"

PFN_CREATE_DXGI_FACTORY_2                              D3D12::CreateDXGIFactory2                            = nullptr;
PFN_DXGI_GET_DEBUG_INTERFACE_1                         D3D12::DXGIGetDebugInterface1                        = nullptr;
PFN_D3D12_CREATE_DEVICE                                D3D12::D3D12CreateDevice                             = nullptr;
PFN_D3D12_GET_DEBUG_INTERFACE                          D3D12::D3D12GetDebugInterface                        = nullptr;
PFN_D3D12_SERIALIZE_ROOT_SIGNATURE                     D3D12::D3D12SerializeRootSignature                   = nullptr;
PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER           D3D12::D3D12CreateRootSignatureDeserializer          = nullptr;
PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE           D3D12::D3D12SerializeVersionedRootSignature          = nullptr;
PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER D3D12::D3D12CreateVersionedRootSignatureDeserializer = nullptr;
PFN_PIXBeginEventOnCommandList                         D3D12::PIXBeginEventOnCommandList                    = nullptr;
PFN_PIXEndEventOnCommandList                           D3D12::PIXEndEventOnCommandList                      = nullptr;
DxcCreateInstanceProc                                  D3D12::DxcCreateInstance                             = nullptr;
#if D3D12_ENABLE_COMPOSITION
PFN_DCOMPOSITION_CREATE_DEVICE                         D3D12::DCompositionCreateDevice                      = nullptr;
#endif

#define D3D12_LOAD_FUNCTION(Function, LibraryHandle) \
do \
{ \
    D3D12::Function = FPlatformLibrary::LoadSymbol<decltype(D3D12::Function)>(#Function, LibraryHandle); \
    if (!D3D12::Function) \
    { \
        D3D12_ERROR_CRITICAL("Failed to load '%s'", #Function); \
        return false; \
    } \
} while(false)

void* D3D12::DXGILibrary  = nullptr;
void* D3D12::D3D12Library = nullptr;
void* D3D12::PIXLibrary   = nullptr;
void* D3D12::DXCLibrary   = nullptr;
#if D3D12_ENABLE_COMPOSITION
void* D3D12::DCompLibrary = nullptr;
#endif

bool D3D12::Initialize(bool bEnablePIX)
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

#if D3D12_ENABLE_COMPOSITION
    DCompLibrary = FPlatformLibrary::LoadDynamicLib("dcomp");
    if (DCompLibrary)
    {
        D3D12_INFO("Loaded dcomp.dll");

        D3D12::DCompositionCreateDevice = FPlatformLibrary::LoadSymbol<PFN_DCOMPOSITION_CREATE_DEVICE>("DCompositionCreateDevice", DCompLibrary);
    }

    if (!D3D12::DCompositionCreateDevice)
    {
        D3D12_INFO("DirectComposition NOT found, so a transparent swap chain will present opaque");
    }
#endif

    if (bEnablePIX)
    {
        PIXLibrary = FPlatformLibrary::LoadDynamicLib("WinPixEventRuntime");
        if (PIXLibrary)
        {
            D3D12_INFO("Loaded WinPixEventRuntime.dll");

            D3D12::PIXBeginEventOnCommandList = FPlatformLibrary::LoadSymbol<PFN_PIXBeginEventOnCommandList>("PIXBeginEventOnCommandList", PIXLibrary);
            D3D12::PIXEndEventOnCommandList   = FPlatformLibrary::LoadSymbol<PFN_PIXEndEventOnCommandList>("PIXEndEventOnCommandList", PIXLibrary);
        }
        else
        {
            D3D12_INFO("PIX Runtime NOT found");
        }
    }

    return true;
}

void D3D12::Release()
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

#if D3D12_ENABLE_COMPOSITION
    if (DCompLibrary)
    {
        FPlatformLibrary::FreeDynamicLib(DCompLibrary);
        DCompLibrary = nullptr;
    }
#endif

    D3D12::CreateDXGIFactory2                            = nullptr;
    D3D12::DXGIGetDebugInterface1                        = nullptr;
    D3D12::D3D12CreateDevice                             = nullptr;
    D3D12::D3D12GetDebugInterface                        = nullptr;
    D3D12::D3D12SerializeRootSignature                   = nullptr;
    D3D12::D3D12SerializeVersionedRootSignature          = nullptr;
    D3D12::D3D12CreateRootSignatureDeserializer          = nullptr;
    D3D12::D3D12CreateVersionedRootSignatureDeserializer = nullptr;
    D3D12::PIXBeginEventOnCommandList                    = nullptr;
    D3D12::PIXEndEventOnCommandList                      = nullptr;
    D3D12::DxcCreateInstance                             = nullptr;
#if D3D12_ENABLE_COMPOSITION
    D3D12::DCompositionCreateDevice                      = nullptr;
#endif
}
