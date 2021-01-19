#pragma once
#include <dxgi1_6.h>
#include <d3d12.h>
#include <dxcapi.h>

#include "D3D12Helpers.h"

#include "Core/RefCountedObject.h"

class D3D12OfflineDescriptorHeap;
class D3D12OnlineDescriptorHeap;
class D3D12ComputePipelineState;
class D3D12RootSignature;

#define D3D12_PIPELINE_STATE_STREAM_ALIGNMENT	(sizeof(void*))
#define D3D12_ENABLE_PIX_MARKERS				 0

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
	D3D12Device(Bool InEnableDebugLayer, Bool InEnableGPUValidation);
	~D3D12Device();

	Bool Init();

	class D3D12CommandQueue*		CreateCommandQueue(D3D12_COMMAND_LIST_TYPE Type);
	class D3D12CommandAllocator*	CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE Type);
	
	class D3D12CommandList* CreateCommandList(
		D3D12_COMMAND_LIST_TYPE Type, 
		D3D12CommandAllocator* Allocator, 
		ID3D12PipelineState* InitalPipeline);

	class D3D12Fence*	CreateFence(UInt64 InitalValue);
	D3D12RootSignature* CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC& Desc);
	D3D12RootSignature* CreateRootSignature(IDxcBlob* ShaderBlob);
	D3D12RootSignature* CreateRootSignature(Void* RootSignatureData, const UInt32 RootSignatureSize);
	
	class D3D12DescriptorHeap* CreateDescriptorHeap(
		D3D12_DESCRIPTOR_HEAP_TYPE Type,
		UInt32 NumDescriptors,
		D3D12_DESCRIPTOR_HEAP_FLAGS Flags);

	Int32 GetMultisampleQuality(DXGI_FORMAT Format, UInt32 SampleCount);
	std::string GetAdapterName() const;

	FORCEINLINE HRESULT CreateCommitedResource(
		const D3D12_HEAP_PROPERTIES* HeapProperties,
		D3D12_HEAP_FLAGS HeapFlags,
		const D3D12_RESOURCE_DESC* Desc,
		D3D12_RESOURCE_STATES InitialResourceState,
		const D3D12_CLEAR_VALUE* OptimizedClearValue,
		REFIID RiidResource,
		void** Resource)
	{
		return Device->CreateCommittedResource(
			HeapProperties, 
			HeapFlags, 
			Desc, 
			InitialResourceState, 
			OptimizedClearValue, 
			RiidResource, 
			Resource);
	}

	FORCEINLINE HRESULT CreatePipelineState(
		const D3D12_PIPELINE_STATE_STREAM_DESC* Desc,
		REFIID Riid,
		void** PipelineState)
	{
		return DXRDevice->CreatePipelineState(Desc, Riid, PipelineState);
	}

	FORCEINLINE HRESULT CreateRootSignatureDeserializer(
		LPCVOID SrcData,
		SIZE_T SrcDataSizeInBytes,
		REFIID RootSignatureDeserializerInterface,
		void** RootSignatureDeserializer)
	{
		return D3D12CreateRootSignatureDeserializerFunc(
			SrcData,
			SrcDataSizeInBytes, 
			RootSignatureDeserializerInterface,
			RootSignatureDeserializer);
	}

	FORCEINLINE HRESULT CreateVersionedRootSignatureDeserializer(
		LPCVOID SrcData,
		SIZE_T SrcDataSizeInBytes,
		REFIID RootSignatureDeserializerInterface,
		void** RootSignatureDeserializer)
	{
		return D3D12CreateVersionedRootSignatureDeserializerFunc(
			SrcData,
			SrcDataSizeInBytes,
			RootSignatureDeserializerInterface,
			RootSignatureDeserializer);
	}

	FORCEINLINE void CreateConstantBufferView(
		const D3D12_CONSTANT_BUFFER_VIEW_DESC* Desc, 
		D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
	{
		Device->CreateConstantBufferView(Desc, DestDescriptor);
	}

	FORCEINLINE void CreateRenderTargetView(
		ID3D12Resource* Resource, 
		const D3D12_RENDER_TARGET_VIEW_DESC* Desc, 
		D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
	{
		Device->CreateRenderTargetView(Resource, Desc, DestDescriptor);
	}

	FORCEINLINE void CreateDepthStencilView(
		ID3D12Resource* Resource, 
		const D3D12_DEPTH_STENCIL_VIEW_DESC* Desc, 
		D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
	{
		Device->CreateDepthStencilView(Resource, Desc, DestDescriptor);
	}

	FORCEINLINE void CreateShaderResourceView(
		ID3D12Resource* Resource, 
		const D3D12_SHADER_RESOURCE_VIEW_DESC* Desc, 
		D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
	{
		Device->CreateShaderResourceView(Resource, Desc, DestDescriptor);
	}

	FORCEINLINE void CreateUnorderedAccessView(
		ID3D12Resource* Resource,
		ID3D12Resource* CounterResource,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC* Desc,
		D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
	{
		Device->CreateUnorderedAccessView(Resource, CounterResource, Desc, DestDescriptor);
	}

	FORCEINLINE void CreateSampler(
		const D3D12_SAMPLER_DESC* Desc,
		D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptor)
	{
		Device->CreateSampler(Desc, DestDescriptor);
	}

	FORCEINLINE void CopyDescriptors(
		UInt32 NumDestDescriptorRanges,
		const D3D12_CPU_DESCRIPTOR_HANDLE* DestDescriptorRangeStarts,
		const UInt32* DestDescriptorRangeSizes,
		UInt32 NumSrcDescriptorRanges,
		const D3D12_CPU_DESCRIPTOR_HANDLE* SrcDescriptorRangeStarts,
		const UInt32* SrcDescriptorRangeSizes,
		D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType)
	{
		Device->CopyDescriptors(
			NumDestDescriptorRanges,
			DestDescriptorRangeStarts,
			DestDescriptorRangeSizes,
			NumSrcDescriptorRanges,
			SrcDescriptorRangeStarts,
			SrcDescriptorRangeSizes,
			DescriptorHeapsType);
	}

	FORCEINLINE void CopyDescriptorsSimple(
		UInt32 NumDescriptors,
		D3D12_CPU_DESCRIPTOR_HANDLE DestDescriptorRangeStart,
		D3D12_CPU_DESCRIPTOR_HANDLE SrcDescriptorRangeStart,
		D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapsType)
	{
		Device->CopyDescriptorsSimple(
			NumDescriptors,
			DestDescriptorRangeStart,
			SrcDescriptorRangeStart,
			DescriptorHeapsType);
	}

	FORCEINLINE void GetRaytracingAccelerationStructurePrebuildInfo(
		const D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS* Desc,
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO* Info)
	{
		DXRDevice->GetRaytracingAccelerationStructurePrebuildInfo(Desc, Info);
	}

	FORCEINLINE UInt32 GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeapType)
	{
		return Device->GetDescriptorHandleIncrementSize(DescriptorHeapType);
	}

	FORCEINLINE ID3D12Device* GetDevice() const
	{
		return Device.Get();
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

	FORCEINLINE Bool IsTearingSupported() const
	{
		return AllowTearing;
	}

	FORCEINLINE Bool IsRayTracingSupported() const
	{
		return RayTracingSupported;
	}

	FORCEINLINE Bool IsInlineRayTracingSupported() const
	{
		return InlineRayTracingSupported;
	}

	FORCEINLINE Bool IsMeshShadersSupported() const
	{
		return MeshShadersSupported;
	}

	FORCEINLINE Bool IsSamplerFeedbackSupported() const
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

	PFN_CREATE_DXGI_FACTORY_2								CreateDXGIFactory2Func								= nullptr;
	PFN_DXGI_GET_DEBUG_INTERFACE_1							DXGIGetDebugInterface1Func							= nullptr;
	PFN_D3D12_CREATE_DEVICE									D3D12CreateDeviceFunc								= nullptr;
	PFN_D3D12_GET_DEBUG_INTERFACE							D3D12GetDebugInterfaceFunc							= nullptr;
	PFN_D3D12_SERIALIZE_ROOT_SIGNATURE						D3D12SerializeRootSignatureFunc						= nullptr;
	PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER			D3D12CreateRootSignatureDeserializerFunc			= nullptr;
	PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE			D3D12SerializeVersionedRootSignatureFunc			= nullptr;
	PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER	D3D12CreateVersionedRootSignatureDeserializerFunc	= nullptr;

private:
	TComPtr<IDXGIFactory2>	Factory;
	TComPtr<IDXGIAdapter1>	Adapter;
	TComPtr<ID3D12Device>	Device;
	TComPtr<ID3D12Device5>	DXRDevice;

	D3D_FEATURE_LEVEL MinFeatureLevel		= D3D_FEATURE_LEVEL_11_0;
	D3D_FEATURE_LEVEL ActiveFeatureLevel	= D3D_FEATURE_LEVEL_11_0;

	HMODULE DXGILib		= 0;
	HMODULE D3D12Lib	= 0;

	D3D12OfflineDescriptorHeap* GlobalResourceDescriptorHeap		= nullptr;
	D3D12OfflineDescriptorHeap* GlobalRenderTargetDescriptorHeap	= nullptr;
	D3D12OfflineDescriptorHeap* GlobalDepthStencilDescriptorHeap	= nullptr;
	D3D12OfflineDescriptorHeap* GlobalSamplerDescriptorHeap			= nullptr;

	UInt32 AdapterID = 0;

	Bool MeshShadersSupported		= false;
	Bool SamplerFeedbackSupported	= false;
	Bool RayTracingSupported		= false;
	Bool InlineRayTracingSupported	= false;
	Bool AllowTearing				= false;
	Bool EnableDebugLayer			= false;
	Bool EnableGPUValidation		= false;
};