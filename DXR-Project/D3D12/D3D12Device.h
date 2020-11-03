#pragma once
#include <dxgi1_6.h>
#include <d3d12.h>
#include <dxcapi.h>

#include <wrl/client.h>

#include "Types.h"

class D3D12DescriptorHeap;
class D3D12OfflineDescriptorHeap;
class D3D12OnlineDescriptorHeap;
class D3D12ComputePipelineState;
class D3D12RootSignature;

/*
* Function Typedefs
*/

typedef HRESULT(WINAPI* PFN_CREATE_DXGI_FACTORY_2)(UINT Flags, REFIID riid, _COM_Outptr_ void** ppFactory);
typedef HRESULT(WINAPI* PFN_DXGI_GET_DEBUG_INTERFACE_1)(UINT Flags, REFIID riid, _COM_Outptr_ void** pDebug);

/*
* D3D12Device
*/

class D3D12Device
{
public:
	D3D12Device();
	~D3D12Device();

	bool CreateDevice(bool DebugEnable, bool GPUValidation);
	bool InitRayTracing();

	class D3D12CommandQueue* CreateCommandQueue(D3D12_COMMAND_LIST_TYPE Type);
	class D3D12CommandAllocator* CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE Type);
	class D3D12CommandList* CreateCommandList(
		D3D12_COMMAND_LIST_TYPE Type, 
		D3D12CommandAllocator* Allocator, 
		ID3D12PipelineState* InitalPipeline);

	class D3D12Fence*			CreateFence(Uint64 InitalValue);
	class D3D12RootSignature*	CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC& Desc);
	class D3D12RootSignature*	CreateRootSignature(IDxcBlob* ShaderBlob);
	class D3D12RootSignature*	CreateRootSignature(VoidPtr RootSignatureData, const Uint32 RootSignatureSize);
	class D3D12SwapChain*		CreateSwapChain(class WindowsWindow* pWindow, D3D12CommandQueue* Queue);

	Int32 GetMultisampleQuality(DXGI_FORMAT Format, Uint32 SampleCount);
	std::string GetAdapterName() const;

	FORCEINLINE HRESULT CreateCommitedResource(
		const D3D12_HEAP_PROPERTIES* pHeapProperties,
		D3D12_HEAP_FLAGS HeapFlags,
		const D3D12_RESOURCE_DESC* pDesc,
		D3D12_RESOURCE_STATES InitialResourceState,
		const D3D12_CLEAR_VALUE* pOptimizedClearValue,
		REFIID riidResource,
		void** ppResource)
	{
		return D3DDevice->CreateCommittedResource(
			pHeapProperties, 
			HeapFlags, 
			pDesc, 
			InitialResourceState, 
			pOptimizedClearValue, 
			riidResource, 
			ppResource);
	}

	FORCEINLINE HRESULT CreatePipelineState(
		const D3D12_PIPELINE_STATE_STREAM_DESC* pDesc,
		REFIID riid,
		void** ppPipelineState)
	{
		return DXRDevice->CreatePipelineState(pDesc, riid, ppPipelineState);
	}

	FORCEINLINE HRESULT CreateRootSignatureDeserializer(
		LPCVOID pSrcData,
		SIZE_T SrcDataSizeInBytes,
		REFIID pRootSignatureDeserializerInterface,
		void** ppRootSignatureDeserializer)
	{
		return _D3D12CreateRootSignatureDeserializer(
			pSrcData,
			SrcDataSizeInBytes, 
			pRootSignatureDeserializerInterface,
			ppRootSignatureDeserializer);
	}

	FORCEINLINE HRESULT CreateVersionedRootSignatureDeserializer(
		LPCVOID pSrcData,
		SIZE_T SrcDataSizeInBytes,
		REFIID pRootSignatureDeserializerInterface,
		void** ppRootSignatureDeserializer)
	{
		return _D3D12CreateVersionedRootSignatureDeserializer(
			pSrcData,
			SrcDataSizeInBytes,
			pRootSignatureDeserializerInterface,
			ppRootSignatureDeserializer);
	}

	FORCEINLINE void CreateConstantBufferView(
		const D3D12_CONSTANT_BUFFER_VIEW_DESC* pDesc, 
		D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
	{
		D3DDevice->CreateConstantBufferView(pDesc, DestDescriptor);
	}

	FORCEINLINE void CreateRenderTargetView(
		ID3D12Resource* pResource, 
		const D3D12_RENDER_TARGET_VIEW_DESC* pDesc, 
		D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
	{
		D3DDevice->CreateRenderTargetView(pResource, pDesc, DestDescriptor);
	}

	FORCEINLINE void CreateDepthStencilView(
		ID3D12Resource* pResource, 
		const D3D12_DEPTH_STENCIL_VIEW_DESC* pDesc, 
		D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
	{
		D3DDevice->CreateDepthStencilView(pResource, pDesc, DestDescriptor);
	}

	FORCEINLINE void CreateShaderResourceView(
		ID3D12Resource* pResource, 
		const D3D12_SHADER_RESOURCE_VIEW_DESC* pDesc, 
		D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
	{
		D3DDevice->CreateShaderResourceView(pResource, pDesc, DestDescriptor);
	}

	FORCEINLINE void CreateUnorderedAccessView(
		ID3D12Resource* pResource,
		ID3D12Resource* pCounterResource,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC* pDesc,
		D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
	{
		D3DDevice->CreateUnorderedAccessView(pResource, pCounterResource, pDesc, DestDescriptor);
	}

	FORCEINLINE void GetRaytracingAccelerationStructurePrebuildInfo(
		const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS* pDesc,
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO* pInfo)
	{
		DXRDevice->GetRaytracingAccelerationStructurePrebuildInfo(pDesc, pInfo);
	}

	FORCEINLINE ID3D12Device* GetDevice() const
	{
		return D3DDevice.Get();
	}

	FORCEINLINE ID3D12Device5* GetDXRDevice() const
	{
		return DXRDevice.Get();
	}

	FORCEINLINE IDXGIFactory2* GetFactory() const
	{
		return Factory.Get();
	}

	FORCEINLINE IDXGIAdapter1* GetAdapter() const
	{
		return Adapter.Get();
	}

	FORCEINLINE bool IsTearingSupported() const
	{
		return AllowTearing;
	}

	FORCEINLINE bool IsRayTracingSupported() const
	{
		return false;// RayTracingSupported;
	}

	FORCEINLINE bool IsInlineRayTracingSupported() const
	{
		return InlineRayTracingSupported;
	}

	FORCEINLINE bool IsMeshShadersSupported() const
	{
		return MeshShadersSupported;
	}

	FORCEINLINE bool IsSamplerFeedbackSupported() const
	{
		return SamplerFeedbackSupported;
	}

	FORCEINLINE D3D12OfflineDescriptorHeap* GetGlobalResourceDescriptorHeap() const
	{
		return GlobalResourceDescriptorHeap;
	}

	FORCEINLINE D3D12OfflineDescriptorHeap* GetGlobalRenderTargetDescriptorHeap() const
	{
		return GlobalRenderTargetDescriptorHeap;
	}

	FORCEINLINE D3D12OfflineDescriptorHeap* GetGlobalDepthStencilDescriptorHeap() const
	{
		return GlobalDepthStencilDescriptorHeap;
	}

	FORCEINLINE D3D12OfflineDescriptorHeap* GetGlobalSamplerDescriptorHeap() const
	{
		return GlobalSamplerDescriptorHeap;
	}

	FORCEINLINE D3D12OnlineDescriptorHeap* GetGlobalOnlineResourceHeap() const
	{
		return GlobalOnlineResourceHeap;
	}

private:
	Microsoft::WRL::ComPtr<IDXGIFactory2>	Factory;
	Microsoft::WRL::ComPtr<IDXGIAdapter1>	Adapter;
	Microsoft::WRL::ComPtr<ID3D12Device>	D3DDevice;
	Microsoft::WRL::ComPtr<ID3D12Device5>	DXRDevice;

	D3D_FEATURE_LEVEL MinFeatureLevel		= D3D_FEATURE_LEVEL_11_0;
	D3D_FEATURE_LEVEL ActiveFeatureLevel	= D3D_FEATURE_LEVEL_11_0;

	// DLL Handles
	HMODULE hDXGI	= 0;
	HMODULE hD3D12	= 0;

	// DXGI Functions loaded from DLLs
	PFN_CREATE_DXGI_FACTORY_2		_CreateDXGIFactory2		= nullptr;
	PFN_DXGI_GET_DEBUG_INTERFACE_1	_DXGIGetDebugInterface1	= nullptr;

	// D3D12 Functions loaded from DLLs
	PFN_D3D12_CREATE_DEVICE			_D3D12CreateDevice		= nullptr;
	PFN_D3D12_GET_DEBUG_INTERFACE	_D3D12GetDebugInterface	= nullptr;
	PFN_D3D12_SERIALIZE_ROOT_SIGNATURE				_D3D12SerializeRootSignature			= nullptr;
	PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE	_D3D12SerializeVersionedRootSignature	= nullptr;
	PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER	_D3D12CreateRootSignatureDeserializer	= nullptr;
	PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER	_D3D12CreateVersionedRootSignatureDeserializer = nullptr;

	D3D12OfflineDescriptorHeap* GlobalResourceDescriptorHeap		= nullptr;
	D3D12OfflineDescriptorHeap* GlobalRenderTargetDescriptorHeap	= nullptr;
	D3D12OfflineDescriptorHeap* GlobalDepthStencilDescriptorHeap	= nullptr;
	D3D12OfflineDescriptorHeap* GlobalSamplerDescriptorHeap			= nullptr;

	D3D12OnlineDescriptorHeap* GlobalOnlineResourceHeap = nullptr;

	Uint32 AdapterID = 0;

	bool MeshShadersSupported		= false;
	bool SamplerFeedbackSupported	= false;
	bool RayTracingSupported		= false;
	bool InlineRayTracingSupported	= false;
	bool AllowTearing				= false;
	bool DebugEnabled				= false;
};