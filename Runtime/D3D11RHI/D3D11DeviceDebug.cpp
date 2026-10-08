#include "D3D11RHI/D3D11DeviceDebug.h"
#include "D3D11RHI/D3D11Loader.h"

#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")

void D3D11Debug::ReportLiveDXGIObjects()
{
    if (!D3D11::DXGIGetDebugInterface1)
    {
        return;
    }

    TComPtr<IDXGIDebug1> DXGIDebug;
    if (SUCCEEDED(D3D11::DXGIGetDebugInterface1(0, IID_PPV_ARGS(&DXGIDebug))))
    {
        TComPtr<IDXGIInfoQueue> InfoQueue;
        if (SUCCEEDED(D3D11::DXGIGetDebugInterface1(0, IID_PPV_ARGS(&InfoQueue))))
        {
            InfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, false);
            InfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, false);
        }

        DXGIDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
    }
    else
    {
        D3D11_WARNING("[FD3D11DeviceRHI]: FAILED to retrieve IDXGIDebug1, live DXGI objects will not be reported");
    }
}
