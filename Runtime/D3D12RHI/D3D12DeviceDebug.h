#pragma once
#include "D3D12RHI/D3D12Configuration.h"

class FD3D12Device;

// -------------------------------------------------------------------------------------------
// D3D12 Debug / Diagnostics
// -------------------------------------------------------------------------------------------

void D3D12RHIEnableDRED();
void D3D12RHISetupDebugInterfaces(bool bEnableDebugLayer);

void D3D12RHIDeviceRemovedHandler(FD3D12Device* Device, const char* Source);
bool D3D12RHICheckDeviceRemoved(FD3D12Device* Device, HRESULT Result, const char* Source);
