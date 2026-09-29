#pragma once
#include "D3D11RHI/D3D11Core.h"

struct D3D11Debug
{
    /** Prints every DXGI and D3D11 object that is still alive to the debugger output */
    static void ReportLiveDXGIObjects();
};
