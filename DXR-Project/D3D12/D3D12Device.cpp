#include "D3D12Device.h"
#include "D3D12ShaderCompiler.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12PipelineState.h"
#include "D3D12RootSignature.h"
#include "D3D12CommandAllocator.h"
#include "D3D12CommandQueue.h"
#include "D3D12SwapChain.h"

#include "Windows/Windows.h"
#include "Windows/Windows.inl"

#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")

/*
* D3D12Device
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

	// Release
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

	// Close DLLs
	::CloseHandle(hDXGI);
	::CloseHandle(hD3D12);
}

bool D3D12Device::CreateDevice(bool InDebugEnable, bool GPUValidation)
{
	using namespace Microsoft::WRL;

	// Enable Debug
	DebugEnabled = InDebugEnable;

	// Load DLLs
	hDXGI = ::LoadLibrary("dxgi.dll");
	if (hDXGI == NULL)
	{
		::MessageBox(0, "FAILED to load dxgi.dll", "Error", MB_ICONERROR | MB_OK);
		return -1;
	}
	else
	{
		LOG_INFO("Loaded dxgi.dll");
	}

	hD3D12 = ::LoadLibrary("d3d12.dll");
	if (hD3D12 == NULL)
	{
		::MessageBox(0, "FAILED to load d3d12.dll", "Error", MB_ICONERROR | MB_OK);
		return -1;
	}
	else
	{
		LOG_INFO("Loaded d3d12.dll");
	}

	// Load DXGI Functions
	_CreateDXGIFactory2		= GetTypedProcAddress<PFN_CREATE_DXGI_FACTORY_2>(hDXGI, "CreateDXGIFactory2");
	_DXGIGetDebugInterface1	= GetTypedProcAddress<PFN_DXGI_GET_DEBUG_INTERFACE_1>(hDXGI, "DXGIGetDebugInterface1");

	// Load D3D12 Functions
	_D3D12CreateDevice		= GetTypedProcAddress<PFN_D3D12_CREATE_DEVICE>(hD3D12, "D3D12CreateDevice");
	_D3D12GetDebugInterface	= GetTypedProcAddress<PFN_D3D12_GET_DEBUG_INTERFACE>(hD3D12, "D3D12GetDebugInterface");
	_D3D12SerializeRootSignature			= GetTypedProcAddress<PFN_D3D12_SERIALIZE_ROOT_SIGNATURE>(hD3D12, "D3D12SerializeRootSignature");
	_D3D12SerializeVersionedRootSignature	= GetTypedProcAddress<PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE>(hD3D12, "D3D12SerializeVersionedRootSignature");
	_D3D12CreateRootSignatureDeserializer	= GetTypedProcAddress<PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER>(hD3D12, "D3D12CreateRootSignatureDeserializer");
	_D3D12CreateVersionedRootSignatureDeserializer	= GetTypedProcAddress<PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER>(hD3D12, "D3D12CreateVersionedRootSignatureDeserializer");

	// Enable debuglayer
	if (DebugEnabled)
	{
		ComPtr<ID3D12Debug> DebugInterface;
		if (FAILED(_D3D12GetDebugInterface(IID_PPV_ARGS(&DebugInterface))))
		{
			LOG_ERROR("[D3D12Device]: FAILED to enable DebugLayer");
			return false;
		}
		else
		{
			DebugInterface->EnableDebugLayer();
		}

		if (GPUValidation)
		{
			ComPtr<ID3D12Debug1> DebugInterface1;
			if (FAILED(DebugInterface.As<ID3D12Debug1>(&DebugInterface1)))
			{
				LOG_ERROR("[D3D12Device]: FAILED to enable GPU- Validation");
				return false;
			}
			else
			{
				DebugInterface1->SetEnableGPUBasedValidation(TRUE);
			}
		}

		ComPtr<IDXGIInfoQueue> InfoQueue;
		if (SUCCEEDED(_DXGIGetDebugInterface1(0, IID_PPV_ARGS(&InfoQueue))))
		{
			InfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
			InfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
		}
		else
		{
			LOG_ERROR("[D3D12Device]: FAILED to retrive InfoQueue");
		}
	}

	// Create factory
	if (FAILED(_CreateDXGIFactory2(0, IID_PPV_ARGS(&Factory))))
	{
		LOG_ERROR("[D3D12Device]: FAILED to create factory");
		return false;
	}
	else
	{
		// Retrive newer factory interface
		ComPtr<IDXGIFactory5> Factory5;
		if (FAILED(Factory.As(&Factory5)))
		{
			LOG_ERROR("[D3D12Device]: FAILED to retrive IDXGIFactory5");
			return false;
		}
		else
		{
			HRESULT hResult = Factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &AllowTearing, sizeof(AllowTearing));
			if (SUCCEEDED(hResult))
			{
				if (AllowTearing)
				{
					LOG_INFO("[D3D12Device]: Tearing is supported");
				}
				else
				{
					LOG_INFO("[D3D12Device]: Tearing is NOT supported");
				}
			}
		}
	}

	// Choose adapter
	ComPtr<IDXGIAdapter1> TempAdapter;
	for (UINT ID = 0; DXGI_ERROR_NOT_FOUND != Factory->EnumAdapters1(ID, &TempAdapter); ID++)
	{
		DXGI_ADAPTER_DESC1 Desc;
		if (FAILED(TempAdapter->GetDesc1(&Desc)))
		{
			LOG_ERROR("[D3D12Device]: FAILED to retrive DXGI_ADAPTER_DESC1");
			return false;
		}

		// Don't select the Basic Render Driver adapter.
		if (Desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			continue;
		}

		// Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
		if (SUCCEEDED(_D3D12CreateDevice(TempAdapter.Get(), MinFeatureLevel, _uuidof(ID3D12Device), nullptr)))
		{
			AdapterID = ID;

			char Buff[256] = {};
			sprintf_s(Buff, "[D3D12Device]: Direct3D Adapter (%u): %ls", AdapterID, Desc.Description);
			LOG_INFO(Buff);

			break;
		}
	}

	if (!TempAdapter)
	{
		LOG_ERROR("[D3D12Device]: FAILED to retrive adapter");
		return false;
	}
	else
	{
		Adapter = TempAdapter;
	}

	// Create Device
	if (FAILED(_D3D12CreateDevice(Adapter.Get(), MinFeatureLevel, IID_PPV_ARGS(&D3DDevice))))
	{
		::MessageBox(0, "Failed to create Device", "ERROR", MB_OK | MB_ICONERROR);
		return false;
	}
	else
	{
		LOG_INFO("[D3D12Device]: Created Device");
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

			D3D12_INFO_QUEUE_FILTER Filter;
			Memory::Memzero(&Filter, sizeof(D3D12_INFO_QUEUE_FILTER));

			Filter.DenyList.NumIDs	= _countof(Hide);
			Filter.DenyList.pIDList = Hide;
			InfoQueue->AddStorageFilterEntries(&Filter);
		}
	}

	// Get DXR Interfaces
	if (FAILED(D3DDevice.As<ID3D12Device5>(&DXRDevice)))
	{
		LOG_ERROR("[D3D12Device]: Failed to retrive DXR-Device");
		return false;
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

	HRESULT hr = D3DDevice->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &FeatureLevels, sizeof(FeatureLevels));
	if (SUCCEEDED(hr))
	{
		ActiveFeatureLevel = FeatureLevels.MaxSupportedFeatureLevel;
	}
	else
	{
		ActiveFeatureLevel = MinFeatureLevel;
	}

	// Check for Ray-Tracing support
	{
		D3D12_FEATURE_DATA_D3D12_OPTIONS5 Features5;
		Memory::Memzero(&Features5, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));

		hr = D3DDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &Features5, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));
		if (SUCCEEDED(hr))
		{
			if (Features5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
			{
				RayTracingSupported = true;
				if (Features5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1)
				{
					InlineRayTracingSupported = true;
				}
			}
		}
	}

	// Check for Mesh-Shaders support
	{
		D3D12_FEATURE_DATA_D3D12_OPTIONS7 Features7;
		Memory::Memzero(&Features7, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS7));

		hr = D3DDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &Features7, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS7));
		if (SUCCEEDED(hr))
		{
			if (Features7.MeshShaderTier != D3D12_MESH_SHADER_TIER_NOT_SUPPORTED)
			{
				MeshShadersSupported = true;
			}

			if (Features7.SamplerFeedbackTier != D3D12_SAMPLER_FEEDBACK_TIER_NOT_SUPPORTED)
			{
				SamplerFeedbackSupported = true;
			}
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

D3D12CommandAllocator* D3D12Device::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE Type)
{
	ID3D12CommandAllocator* Allocator = nullptr;

	HRESULT hResult = D3DDevice->CreateCommandAllocator(Type, IID_PPV_ARGS(&Allocator));
	if (SUCCEEDED(hResult))
	{
		LOG_INFO("[D3D12Device]: Created CommandAllocator");
		return new D3D12CommandAllocator(this, Allocator);
	}
	else
	{
		LOG_ERROR("[D3D12Device]: FAILED to create CommandAllocator");
		return nullptr;
	}
}

D3D12Fence* D3D12Device::CreateFence(Uint64 InitalValue)
{
	ID3D12Fence* Fence = nullptr;

	HRESULT hResult = D3DDevice->CreateFence(InitalValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence));
	if (SUCCEEDED(hResult))
	{
		HANDLE hEvent = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (hEvent == NULL)
		{
			LOG_ERROR("[D3D12Device]: FAILED to create Event for Fence");
			return nullptr;
		}
		else
		{
			LOG_INFO("[D3D12Device]: Created Fence");
			return new D3D12Fence(this, Fence, hEvent);
		}
	}
	else
	{
		LOG_INFO("[D3D12Device]: FAILED to create Fence");
		return nullptr;
	}
}

D3D12CommandQueue* D3D12Device::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE Type)
{
	// Create the command queue.
	D3D12_COMMAND_QUEUE_DESC QueueDesc;
	Memory::Memzero(&QueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));

	QueueDesc.Flags		= D3D12_COMMAND_QUEUE_FLAG_NONE;
	QueueDesc.NodeMask	= 0;
	QueueDesc.Priority	= D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	QueueDesc.Type		= Type;

	ID3D12CommandQueue* Queue = nullptr;

	HRESULT hResult = D3DDevice->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&Queue));
	if (SUCCEEDED(hResult))
	{
		LOG_INFO("[D3D12Device]: Created CommandQueue");
		return new D3D12CommandQueue(this, Queue);
	}
	else
	{
		LOG_ERROR("[D3D12Device]: FAILED to create CommandQueue");
		return nullptr;
	}

}

D3D12CommandList* D3D12Device::CreateCommandList(D3D12_COMMAND_LIST_TYPE Type, D3D12CommandAllocator* Allocator, ID3D12PipelineState* InitalPipeline)
{
	ID3D12GraphicsCommandList* CmdList = nullptr;

	HRESULT hResult = D3DDevice->CreateCommandList(0, Type, Allocator->GetAllocator(), InitalPipeline, IID_PPV_ARGS(&CmdList));
	if (SUCCEEDED(hResult))
	{
		CmdList->Close();
		LOG_INFO("[D3D12Device]: Created CommandList");

		D3D12CommandList* CreatedCmdList = new D3D12CommandList(this, CmdList);
		if (IsRayTracingSupported())
		{
			if (!CreatedCmdList->InitRayTracing())
			{
				return nullptr;
			}
		}

		return CreatedCmdList;
	}
	else
	{
		LOG_ERROR("[D3D12CommandList]: FAILED to create CommandList");
		return nullptr;
	}
}

D3D12RootSignature* D3D12Device::CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC& Desc)
{
	using namespace Microsoft::WRL;

	ComPtr<ID3DBlob> ErrorBlob;
	ComPtr<ID3DBlob> SignatureBlob;
	
	HRESULT hResult = _D3D12SerializeRootSignature(&Desc, D3D_ROOT_SIGNATURE_VERSION_1, &SignatureBlob, &ErrorBlob);
	if (FAILED(hResult))
	{
		LOG_ERROR("[D3D12Device]: FAILED to Serialize RootSignature");
		LOG_ERROR(reinterpret_cast<const Char*>(ErrorBlob->GetBufferPointer()));

		Debug::DebugBreak();
		return nullptr;
	}
	else
	{
		return CreateRootSignature(SignatureBlob->GetBufferPointer(), static_cast<Uint32>(SignatureBlob->GetBufferSize()));
	}
}

D3D12RootSignature* D3D12Device::CreateRootSignature(IDxcBlob* ShaderBlob)
{
	VALIDATE(ShaderBlob != nullptr);
	return CreateRootSignature(ShaderBlob->GetBufferPointer(), ShaderBlob->GetBufferSize());
}

D3D12RootSignature* D3D12Device::CreateRootSignature(VoidPtr RootSignatureData, const Uint32 RootSignatureSize)
{
	ID3D12RootSignature* RootSignature = nullptr;

	HRESULT hResult = D3DDevice->CreateRootSignature(0, RootSignatureData, RootSignatureSize, IID_PPV_ARGS(&RootSignature));
	if (FAILED(hResult))
	{
		LOG_ERROR("[D3D12Device]: FAILED to Create RootSignature");
		Debug::DebugBreak();

		return nullptr;
	}
	else
	{
		LOG_INFO("[D3D12Device]: Created RootSignature");
		return new D3D12RootSignature(this, RootSignature);
	}
}

D3D12SwapChain* D3D12Device::CreateSwapChain(WindowsWindow* pWindow, D3D12CommandQueue* Queue)
{
	D3D12SwapChain* SwapChain = new D3D12SwapChain(this);
	if (!SwapChain->CreateSwapChain(Factory.Get(), pWindow, Queue))
	{
		return nullptr;
	}
	else
	{
		return SwapChain;
	}
}

Int32 D3D12Device::GetMultisampleQuality(DXGI_FORMAT Format, Uint32 SampleCount)
{
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS Data = { };
	Data.Flags			= D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	Data.Format			= Format;
	Data.SampleCount	= SampleCount;
	
	HRESULT hr = D3DDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &Data, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
	if (FAILED(hr))
	{
		LOG_ERROR("[D3D12Device] CheckFeatureSupport failed");
		return 0;
	}

	return static_cast<Uint32>(Data.NumQualityLevels - 1);
}

std::string D3D12Device::GetAdapterName() const
{
	DXGI_ADAPTER_DESC Desc;
	Adapter->GetDesc(&Desc);

	std::wstring WideName = Desc.Description;
	return ConvertToAscii(WideName);
}