#include "D3D12GraphicsDevice.h"

#include <dxgidebug.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid")

/*
* Members
*/

D3D12GraphicsDevice::D3D12GraphicsDevice()
{
}

D3D12GraphicsDevice::~D3D12GraphicsDevice()
{
}

bool D3D12GraphicsDevice::Init(bool DebugEnable)
{
	// Enable Debug
	IsDebugEnabled = DebugEnable;

	// Start Initialization of D3D12-Device
	if (!CreateFactory())
	{
		return false;
	}

	if (!ChooseAdapter())
	{
		return false;
	}

	// Create Device

	return true;
}

/*
* Static
*/

std::unique_ptr<D3D12GraphicsDevice> D3D12GraphicsDevice::D3D12Device = nullptr;

D3D12GraphicsDevice* D3D12GraphicsDevice::Create(bool DebugEnable)
{
	D3D12Device.reset(new D3D12GraphicsDevice());
	if (D3D12Device->Init(DebugEnable))
	{
		return D3D12Device.get();
	}
	else
	{
		return nullptr;
	}
}

D3D12GraphicsDevice* D3D12GraphicsDevice::Get()
{
	return D3D12Device.get();
}

bool D3D12GraphicsDevice::CreateFactory()
{
	using namespace Microsoft::WRL;

	// Enable debuglayer
	Uint32 DebugFlags = 0;
	if (IsDebugEnabled)
	{
		ComPtr<ID3D12Debug> DebugInterface;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&DebugInterface))))
		{
			DebugInterface->EnableDebugLayer();
		}
		else
		{
			::OutputDebugString("[D3D12GraphicsDevice]: Failed to enable DebugLayer\n");
		}

		ComPtr<IDXGIInfoQueue> InfoQueue;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&InfoQueue))))
		{
			InfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
			InfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
		}
		else
		{
			::OutputDebugString("[D3D12GraphicsDevice]: Failed to retrive InfoQueue\n");
		}
	}

	// Create factory
	if (FAILED(CreateDXGIFactory2(DebugFlags, IID_PPV_ARGS(&Factory))))
	{
		::OutputDebugString("[D3D12GraphicsDevice]: Failed to create factory\n");
		return false;
	}
	else
	{
		// Retrive newer factory interface
		ComPtr<IDXGIFactory5> Factory5;
		if (FAILED(Factory.As(&Factory5)))
		{
			::OutputDebugString("[D3D12GraphicsDevice]: Failed to retrive IDXGIFactory5\n");
			return false;
		}
		else
		{
			BOOL AllowTearing = FALSE;
			HRESULT hResult = Factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &AllowTearing, sizeof(AllowTearing));
			if (SUCCEEDED(hResult))
			{
				if (AllowTearing)
				{
					IsTearingSupported = true;

					::OutputDebugString("[D3D12GraphicsDevice]: Tearing is supported\n");
				}
				else
				{
					::OutputDebugString("[D3D12GraphicsDevice]: Tearing is NOT supported\n");
				}
			}
		}
	}

	return true;
}

bool D3D12GraphicsDevice::ChooseAdapter()
{
	using namespace Microsoft::WRL;

	ComPtr<IDXGIAdapter1> TempAdapter;
	for (UINT ID = 0; DXGI_ERROR_NOT_FOUND != Factory->EnumAdapters1(ID, &TempAdapter); ID++)
	{
		DXGI_ADAPTER_DESC1 Desc;
		if (FAILED(TempAdapter->GetDesc1(&Desc)))
		{
			::OutputDebugString("[D3D12GraphicsDevice]: Failed to retrive DXGI_ADAPTER_DESC1");
			return false;
		}

		if (Desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			// Don't select the Basic Render Driver adapter.
			continue;
		}

		// Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
		if (SUCCEEDED(D3D12CreateDevice(TempAdapter.Get(), MinFeatureLevel, _uuidof(ID3D12Device), nullptr)))
		{
			AdapterID = ID;
#ifdef _DEBUG
			wchar_t Buff[256] = {};
			swprintf_s(Buff, L"Direct3D Adapter (%u): VID:%04X, PID:%04X - %ls\n", AdapterID, Desc.VendorId, Desc.DeviceId, Desc.Description);
			OutputDebugStringW(Buff);
#endif
			break;
		}
	}

	if (!TempAdapter)
	{
		::OutputDebugString("[D3D12GraphicsDevice]: Failed to retrive adapter\n");
		return false;
	}
	else
	{
		Adapter = TempAdapter;
		return true;
	}
}
