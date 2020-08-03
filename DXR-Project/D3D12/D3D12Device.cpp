#include "D3D12Device.h"
#include "D3D12ShaderCompiler.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12ComputePipelineState.h"
#include "D3D12RootSignature.h"

#include <dxgidebug.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid")

/*
* Members
*/

D3D12Device::D3D12Device()
	: Factory(nullptr)
	, Adapter(nullptr)
	, D3DDevice(nullptr)
	, DXRDevice(nullptr)
{
}

D3D12Device::~D3D12Device()
{
	using namespace Microsoft::WRL;

	SAFEDELETE(GlobalResourceDescriptorHeap);
	SAFEDELETE(GlobalRenderTargetDescriptorHeap);
	SAFEDELETE(GlobalDepthStencilDescriptorHeap);
	SAFEDELETE(GlobalSamplerDescriptorHeap);
	SAFEDELETE(GlobalOnlineResourceHeap);

	if (DebugEnabled)
	{
		ComPtr<ID3D12DebugDevice> DebugDevice;
		if (SUCCEEDED(D3DDevice.As<ID3D12DebugDevice>(&DebugDevice)))
		{
			DebugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
		}
	}
}

bool D3D12Device::Initialize(bool DebugEnable)
{
	using namespace Microsoft::WRL;

	// Enable Debug
	DebugEnabled = DebugEnable;

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
		LOG_INFO("[D3D12GraphicsDevice]: Created D3D12Device");

		// Get DXR Interfaces
		if (FAILED(D3DDevice.As<ID3D12Device5>(&DXRDevice)))
		{
			LOG_ERROR("[D3D12RayTracer]: Failed to retrive DXR-Device");
			return false;
		}
	}

	// Configure debug device (if active).
	if (DebugEnabled)
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

	// Check for Ray-Tracing support
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 Features5 = { };
	hResult = D3DDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &Features5, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));
	if (SUCCEEDED(hResult))
	{
		if (Features5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
		{
			RayTracingSupported = true;
		}
	}

	// Create Global DescriptorHeaps
	GlobalResourceDescriptorHeap		= new D3D12OfflineDescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	GlobalRenderTargetDescriptorHeap	= new D3D12OfflineDescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	GlobalDepthStencilDescriptorHeap	= new D3D12OfflineDescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	GlobalSamplerDescriptorHeap			= new D3D12OfflineDescriptorHeap(this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	// Create Global Online Heap
	GlobalOnlineResourceHeap = new D3D12OnlineDescriptorHeap(this, 2048, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	if (!GlobalOnlineResourceHeap->Initialize())
	{
		return false;
	}

	return true;
}

std::string D3D12Device::GetAdapterName() const
{
	DXGI_ADAPTER_DESC Desc;
	Adapter->GetDesc(&Desc);

	std::wstring WideName = Desc.Description;
	return ConvertToAscii(WideName);
}

/*
* Static
*/

D3D12Device* D3D12Device::Make(bool DebugEnable)
{
	D3D12Device* NewDevice = new D3D12Device();
	if (NewDevice->Initialize(DebugEnable))
	{
		return NewDevice;
	}
	else
	{
		return nullptr;
	}
}

bool D3D12Device::CreateFactory()
{
	using namespace Microsoft::WRL;

	// Enable debuglayer
	Uint32 DebugFlags = 0;
	if (DebugEnabled)
	{
		ComPtr<ID3D12Debug> DebugInterface;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&DebugInterface))))
		{
			DebugInterface->EnableDebugLayer();
		}
		else
		{
			LOG_ERROR("[D3D12GraphicsDevice]: FAILED to enable DebugLayer");
		}

		ComPtr<IDXGIInfoQueue> InfoQueue;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&InfoQueue))))
		{
			InfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
			InfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
		}
		else
		{
			LOG_ERROR("[D3D12GraphicsDevice]: FAILED to retrive InfoQueue");
		}
	}

	// Create factory
	if (FAILED(CreateDXGIFactory2(DebugFlags, IID_PPV_ARGS(&Factory))))
	{
		LOG_ERROR("[D3D12GraphicsDevice]: FAILED to create factory");
		return false;
	}
	else
	{
		// Retrive newer factory interface
		ComPtr<IDXGIFactory5> Factory5;
		if (FAILED(Factory.As(&Factory5)))
		{
			LOG_ERROR("[D3D12GraphicsDevice]: FAILED to retrive IDXGIFactory5");
			return false;
		}
		else
		{
			HRESULT hResult = Factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &AllowTearing, sizeof(AllowTearing));
			if (SUCCEEDED(hResult))
			{
				if (AllowTearing)
				{
					LOG_INFO("[D3D12GraphicsDevice]: Tearing is supported");
				}
				else
				{
					LOG_INFO("[D3D12GraphicsDevice]: Tearing is NOT supported");
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
			LOG_ERROR("[D3D12GraphicsDevice]: FAILED to retrive DXGI_ADAPTER_DESC1");
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
			sprintf_s(Buff, "[D3D12GraphicsDevice]: Direct3D Adapter (%u): %ls", AdapterID, Desc.Description);
			LOG_INFO(Buff);

			break;
		}
	}

	if (!TempAdapter)
	{
		LOG_ERROR("[D3D12GraphicsDevice]: FAILED to retrive adapter");
		return false;
	}
	else
	{
		Adapter = TempAdapter;
		return true;
	}
}
