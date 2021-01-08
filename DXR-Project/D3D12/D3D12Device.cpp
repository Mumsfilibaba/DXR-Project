#include "Application/Platform/PlatformDialogMisc.h"

#include "D3D12Device.h"
#include "D3D12ShaderCompiler.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12PipelineState.h"
#include "D3D12RootSignature.h"
#include "D3D12CommandAllocator.h"
#include "D3D12CommandQueue.h"

#include "Windows/Windows.h"
#include "Windows/Windows.inl"

#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")

/*
* D3D12Device
*/

D3D12Device::D3D12Device(Bool InEnableDebugLayer, Bool InEnableGPUValidation)
	: Factory(nullptr)
	, Adapter(nullptr)
	, Device(nullptr)
	, DXRDevice(nullptr)
	, EnableDebugLayer(InEnableDebugLayer)
	, EnableGPUValidation(InEnableGPUValidation)
{
}

D3D12Device::~D3D12Device()
{
	SAFERELEASE(GlobalResourceDescriptorHeap);
	SAFERELEASE(GlobalRenderTargetDescriptorHeap);
	SAFERELEASE(GlobalDepthStencilDescriptorHeap);
	SAFERELEASE(GlobalSamplerDescriptorHeap);

	if (EnableDebugLayer)
	{
		TComPtr<ID3D12DebugDevice> DebugDevice;
		if (SUCCEEDED(Device.As(&DebugDevice)))
		{
			DebugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
		}
	}

	::FreeLibrary(DXGILib);
	::FreeLibrary(D3D12Lib);
}

bool D3D12Device::Init()
{
	DXGILib = ::LoadLibrary("dxgi.dll");
	if (DXGILib == NULL)
	{
		PlatformDialogMisc::MessageBox("ERROR", "FAILED to load dxgi.dll");
		return false;
	}
	else
	{
		LOG_INFO("Loaded dxgi.dll");
	}

	D3D12Lib = ::LoadLibrary("d3d12.dll");
	if (D3D12Lib == NULL)
	{
		PlatformDialogMisc::MessageBox("ERROR", "FAILED to load d3d12.dll");
		return false;
	}
	else
	{
		LOG_INFO("Loaded d3d12.dll");
	}

	CreateDXGIFactory2Func = GetTypedProcAddress<PFN_CREATE_DXGI_FACTORY_2>(
		DXGILib, 
		"CreateDXGIFactory2");
	DXGIGetDebugInterface1Func = GetTypedProcAddress<PFN_DXGI_GET_DEBUG_INTERFACE_1>(
		DXGILib, 
		"DXGIGetDebugInterface1");
	D3D12CreateDeviceFunc = GetTypedProcAddress<PFN_D3D12_CREATE_DEVICE>(
		D3D12Lib, 
		"D3D12CreateDevice");
	D3D12GetDebugInterfaceFunc = GetTypedProcAddress<PFN_D3D12_GET_DEBUG_INTERFACE>(
		D3D12Lib, 
		"D3D12GetDebugInterface");
	D3D12SerializeRootSignatureFunc = GetTypedProcAddress<PFN_D3D12_SERIALIZE_ROOT_SIGNATURE>(
		D3D12Lib, 
		"D3D12SerializeRootSignature");
	D3D12SerializeVersionedRootSignatureFunc = GetTypedProcAddress<PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE>(
		D3D12Lib, 
		"D3D12SerializeVersionedRootSignature");
	D3D12CreateRootSignatureDeserializerFunc = GetTypedProcAddress<PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER>(
		D3D12Lib, 
		"D3D12CreateRootSignatureDeserializer");
	D3D12CreateVersionedRootSignatureDeserializerFunc = GetTypedProcAddress<PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER>(
		D3D12Lib, 
		"D3D12CreateVersionedRootSignatureDeserializer");

	if (EnableDebugLayer)
	{
		TComPtr<ID3D12Debug> DebugInterface;
		if (FAILED(D3D12GetDebugInterfaceFunc(IID_PPV_ARGS(&DebugInterface))))
		{
			LOG_ERROR("[D3D12Device]: FAILED to enable DebugLayer");
			return false;
		}
		else
		{
			DebugInterface->EnableDebugLayer();
		}

		if (EnableGPUValidation)
		{
			TComPtr<ID3D12Debug1> DebugInterface1;
			if (FAILED(DebugInterface.As(&DebugInterface1)))
			{
				LOG_ERROR("[D3D12Device]: FAILED to enable GPU- Validation");
				return false;
			}
			else
			{
				DebugInterface1->SetEnableGPUBasedValidation(TRUE);
			}
		}

		TComPtr<IDXGIInfoQueue> InfoQueue;
		if (SUCCEEDED(DXGIGetDebugInterface1Func(0, IID_PPV_ARGS(&InfoQueue))))
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
	if (FAILED(CreateDXGIFactory2Func(0, IID_PPV_ARGS(&Factory))))
	{
		LOG_ERROR("[D3D12Device]: FAILED to create factory");
		return false;
	}
	else
	{
		// Retrive newer factory interface
		TComPtr<IDXGIFactory5> Factory5;
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
	TComPtr<IDXGIAdapter1> TempAdapter;
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
		if (SUCCEEDED(D3D12CreateDeviceFunc(TempAdapter.Get(), MinFeatureLevel, _uuidof(ID3D12Device), nullptr)))
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
	if (FAILED(D3D12CreateDeviceFunc(Adapter.Get(), MinFeatureLevel, IID_PPV_ARGS(&Device))))
	{
		PlatformDialogMisc::MessageBox("ERROR", "FAILED to create device");
		return false;
	}
	else
	{
		LOG_INFO("[D3D12Device]: Created Device");
	}

	// Configure debug device (if active).
	if (EnableDebugLayer)
	{
		TComPtr<ID3D12InfoQueue> InfoQueue;
		if (SUCCEEDED(Device.As(&InfoQueue)))
		{
			InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
			InfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);

			D3D12_MESSAGE_ID Hide[] =
			{
				D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
				D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
			};

			D3D12_INFO_QUEUE_FILTER Filter;
			Memory::Memzero(&Filter);

			Filter.DenyList.NumIDs	= _countof(Hide);
			Filter.DenyList.pIDList = Hide;
			InfoQueue->AddStorageFilterEntries(&Filter);
		}
	}

	// Get DXR Interfaces
	if (FAILED(Device.As<ID3D12Device5>(&DXRDevice)))
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

	HRESULT hr = Device->CheckFeatureSupport(D3D12_FEATURE_FEATURE_LEVELS, &FeatureLevels, sizeof(FeatureLevels));
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

		hr = Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &Features5, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS5));
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

		hr = Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &Features7, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS7));
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

	return true;
}

D3D12CommandAllocator* D3D12Device::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE Type)
{
	ID3D12CommandAllocator* Allocator = nullptr;

	HRESULT hResult = Device->CreateCommandAllocator(Type, IID_PPV_ARGS(&Allocator));
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

D3D12Fence* D3D12Device::CreateFence(UInt64 InitalValue)
{
	ID3D12Fence* Fence = nullptr;

	HRESULT hResult = Device->CreateFence(InitalValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence));
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

	HRESULT hResult = Device->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&Queue));
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

	HRESULT hResult = Device->CreateCommandList(0, Type, Allocator->GetAllocator(), InitalPipeline, IID_PPV_ARGS(&CmdList));
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
	TComPtr<ID3DBlob> ErrorBlob;
	TComPtr<ID3DBlob> SignatureBlob;
	
	HRESULT hResult = D3D12SerializeRootSignatureFunc(&Desc, D3D_ROOT_SIGNATURE_VERSION_1, &SignatureBlob, &ErrorBlob);
	if (FAILED(hResult))
	{
		LOG_ERROR("[D3D12Device]: FAILED to Serialize RootSignature");
		LOG_ERROR(reinterpret_cast<const Char*>(ErrorBlob->GetBufferPointer()));

		Debug::DebugBreak();
		return nullptr;
	}
	else
	{
		return CreateRootSignature(SignatureBlob->GetBufferPointer(), static_cast<UInt32>(SignatureBlob->GetBufferSize()));
	}
}

D3D12RootSignature* D3D12Device::CreateRootSignature(IDxcBlob* ShaderBlob)
{
	VALIDATE(ShaderBlob != nullptr);
	return CreateRootSignature(ShaderBlob->GetBufferPointer(), ShaderBlob->GetBufferSize());
}

D3D12RootSignature* D3D12Device::CreateRootSignature(Void* RootSignatureData, const UInt32 RootSignatureSize)
{
	ID3D12RootSignature* RootSignature = nullptr;

	HRESULT hResult = Device->CreateRootSignature(0, RootSignatureData, RootSignatureSize, IID_PPV_ARGS(&RootSignature));
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

D3D12DescriptorHeap* D3D12Device::CreateDescriptorHeap(
	D3D12_DESCRIPTOR_HEAP_TYPE Type,
	UInt32 NumDescriptors,
	D3D12_DESCRIPTOR_HEAP_FLAGS Flags)
{
	ID3D12DescriptorHeap* Heap = nullptr;

	D3D12_DESCRIPTOR_HEAP_DESC Desc;
	Memory::Memzero(&Desc);
	
	Desc.Type	= Type;
	Desc.Flags	= Flags;
	Desc.NumDescriptors = NumDescriptors;

	HRESULT hResult = Device->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&Heap));
	if (FAILED(hResult))
	{
		LOG_ERROR("[D3D12Device]: FAILED to Create DescriptorHeap");
		Debug::DebugBreak();

		return nullptr;
	}
	else
	{
		LOG_INFO("[D3D12Device]: Created DescriptorHeap");
		return new D3D12DescriptorHeap(this, Heap);
	}
}

Int32 D3D12Device::GetMultisampleQuality(DXGI_FORMAT Format, UInt32 SampleCount)
{
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS Data;
	Memory::Memzero(&Data);

	Data.Flags			= D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	Data.Format			= Format;
	Data.SampleCount	= SampleCount;
	
	HRESULT hr = Device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &Data, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
	if (FAILED(hr))
	{
		LOG_ERROR("[D3D12Device] CheckFeatureSupport failed");
		return 0;
	}

	return static_cast<UInt32>(Data.NumQualityLevels - 1);
}

std::string D3D12Device::GetAdapterName() const
{
	DXGI_ADAPTER_DESC Desc;
	Adapter->GetDesc(&Desc);

	std::wstring WideName = Desc.Description;
	return ConvertToAscii(WideName);
}