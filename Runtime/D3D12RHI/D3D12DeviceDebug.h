#pragma once
#include "D3D12RHI/D3D12Configuration.h"

class FD3D12Device;

// -------------------------------------------------------------------------------------------
// D3D12 Debug / Diagnostics
// -------------------------------------------------------------------------------------------

struct D3D12Debug
{
    static void EnableDRED();
    static void SetupDebugInterfaces(bool bEnableDebugLayer);

    static void ReportLiveDXGIObjects();

    static void DeviceRemovedHandler(FD3D12Device* Device, const CHAR_T* Source);
    static bool CheckDeviceRemoved(FD3D12Device* Device, HRESULT Result, const CHAR_T* Source);
};

D3D12RHI_API void D3D12ResetValidationErrors();
D3D12RHI_API bool D3D12HasValidationErrors();
