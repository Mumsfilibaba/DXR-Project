#include "D3D12Device.h"
#include "D3D12ShaderCompiler.h"

#include <dxgidebug.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid")

/*
* Members
*/

D3D12Device::D3D12Device()
{
}

D3D12Device::~D3D12Device()
{
	using namespace Microsoft::WRL;

	if (IsDebugEnabled)
	{
		ComPtr<ID3D12DebugDevice> DebugDevice;
		if (SUCCEEDED(D3DDevice.As<ID3D12DebugDevice>(&DebugDevice)))
		{
			DebugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
		}
	}
}

bool D3D12Device::Init(bool DebugEnable)
{
	using namespace Microsoft::WRL;

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
	if (FAILED(D3D12CreateDevice(Adapter.Get(), MinFeatureLevel, IID_PPV_ARGS(&D3DDevice))))
	{
		::MessageBox(0, "Failed to create D3D12Device", "ERROR", MB_OK | MB_ICONERROR);
		return false;
	}
	else
	{
		::OutputDebugString("[D3D12GraphicsDevice]: Created D3D12Device\n");

		// Get DXR Interfaces
		if (FAILED(D3DDevice.As<ID3D12Device5>(&DXRDevice)))
		{
			::OutputDebugString("[D3D12RayTracer]: Failed to retrive DXR-Device\n");
			return false;
		}
	}

	// Configure debug device (if active).
	if (IsDebugEnabled)
	{
		ComPtr<ID3D12InfoQueue> InfoQueue;
		if (SUCCEEDED(D3DDevice.As(&InfoQueue)))
		{
			InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);

			D3D12_MESSAGE_ID Hide[] =
			{
				D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
				D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
			};

			D3D12_INFO_QUEUE_FILTER Filter = { };
			Filter.DenyList.NumIDs	= _countof(Hide);
			Filter.DenyList.pIDList = Hide;
			InfoQueue->AddStorageFilterEntries(&Filter);
		}
	}

	// Determine maximum supported feature level for this device
	static const D3D_FEATURE_LEVEL SupportedFeatureLevels[] =
	{
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
	};

	D3D12_FEATURE_DATA_FEATURE_LEVELS FeatureLevels =
	{
		_countof(SupportedFeatureLevels), SupportedFeatureLevels, D3D_FEATURE_LEVEL_11_0
	};

	HRESULT hResult = D3DDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &FeatureLevels, sizeof(FeatureLevels));
	if (SUCCEEDED(hResult))
	{
		ActiveFeatureLevel = FeatureLevels.MaxSupportedFeatureLevel;
	}
	else
	{
		ActiveFeatureLevel = MinFeatureLevel;
	}


	// Create ShaderCompiler
	D3D12ShaderCompiler* ShaderCompiler = D3D12ShaderCompiler::Create();
	if (!ShaderCompiler)
	{
		return false;
	}

	return true;
}

/*
* Static
*/

std::unique_ptr<D3D12Device> D3D12Device::Device = nullptr;

D3D12Device* D3D12Device::Create(bool DebugEnable)
{
	Device.reset(new D3D12Device());
	if (Device->Init(DebugEnable))
	{
		return Device.get();
	}
	else
	{
		return nullptr;
	}
}

D3D12Device* D3D12Device::Get()
{
	return Device.get();
}

bool D3D12Device::CreateFactory()
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
			HRESULT hResult = Factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &AllowTearing, sizeof(AllowTearing));
			if (SUCCEEDED(hResult))
			{
				if (AllowTearing)
				{
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

bool D3D12Device::ChooseAdapter()
{
	using namespace Microsoft::WRL;

	ComPtr<IDXGIAdapter1> TempAdapter;
	for (UINT ID = 0; DXGI_ERROR_NOT_FOUND != Factory->EnumAdapters1(ID, &TempAdapter); ID++)
	{
		DXGI_ADAPTER_DESC1 Desc;
		if (FAILED(TempAdapter->GetDesc1(&Desc)))
		{
			::OutputDebugString("[D3D12GraphicsDevice]: Failed to retrive DXGI_ADAPTER_DESC1\n");
			return false;
		}

		// Don't select the Basic Render Driver adapter.
		if (Desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			continue;
		}

		// Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
		if (SUCCEEDED(D3D12CreateDevice(TempAdapter.Get(), MinFeatureLevel, _uuidof(ID3D12Device), nullptr)))
		{
			AdapterID = ID;

			char Buff[256] = {};
			sprintf_s(Buff, "Direct3D Adapter (%u): %ls\n", AdapterID, Desc.Description);
			::OutputDebugString(Buff);

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
