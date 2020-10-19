#pragma once
#include "Windows/WindowsWindow.h"

#include "RenderingCore/RenderingAPI.h"

#include "D3D12Device.h"
#include "D3D12ImmediateCommandList.h"
#include "D3D12SwapChain.h"

/*
* D3D12RenderingAPI
*/

class D3D12RenderingAPI : public RenderingAPI
{
public:
	D3D12RenderingAPI();
	~D3D12RenderingAPI();

	virtual bool Init(TSharedRef<GenericWindow> RenderWindow, bool EnableDebug) override final;

	/*
	* Resources
	*/

	// Textures
	virtual Texture1D* CreateTexture1D(
		const ResourceData* InitalData,
		EFormat Format,
		Uint32 Usage,
		Uint32 Width,
		Uint32 MipLevels,
		const ClearValue& OptimizedClearValue) const override final;

	virtual Texture1DArray* CreateTexture1DArray(
		const ResourceData* InitalData,
		EFormat Format,
		Uint32 Usage,
		Uint32 Width,
		Uint32 MipLevels,
		Uint32 ArrayCount,
		const ClearValue& OptimizedClearValue) const override final;

	virtual Texture2D* CreateTexture2D(
		const ResourceData* InitalData,
		EFormat Format,
		Uint32 Usage,
		Uint32 Width,
		Uint32 Height,
		Uint32 MipLevels,
		Uint32 SampleCount,
		const ClearValue& OptimizedClearValue) const override final;

	virtual Texture2DArray* CreateTexture2DArray(
		const ResourceData* InitalData,
		EFormat Format,
		Uint32 Usage,
		Uint32 Width,
		Uint32 Height,
		Uint32 MipLevels,
		Uint32 ArrayCount,
		Uint32 SampleCount,
		const ClearValue& OptimizedClearValue) const override final;

	virtual TextureCube* CreateTextureCube(
		const ResourceData* InitalData,
		EFormat Format,
		Uint32 Usage,
		Uint32 Size,
		Uint32 MipLevels,
		Uint32 SampleCount,
		const ClearValue& OptimizedClearValue) const override final;

	virtual TextureCubeArray* CreateTextureCubeArray(
		const ResourceData* InitalData,
		EFormat Format,
		Uint32 Usage,
		Uint32 Size,
		Uint32 MipLevels,
		Uint32 ArrayCount,
		Uint32 SampleCount,
		const ClearValue& OptimizedClearValue) const override final;

	virtual Texture3D* CreateTexture3D(
		const ResourceData* InitalData,
		EFormat Format,
		Uint32 Usage,
		Uint32 Width,
		Uint32 Height,
		Uint32 Depth,
		Uint32 MipLevels,
		const ClearValue& OptimizedClearValue) const override final;

	// Buffers
	virtual VertexBuffer* CreateVertexBuffer(
		const ResourceData* InitalData,
		Uint32 SizeInBytes,
		Uint32 VertexStride,
		Uint32 Usage) const override final;

	virtual IndexBuffer* CreateIndexBuffer(
		const ResourceData* InitalData,
		Uint32 SizeInBytes,
		EIndexFormat IndexFormat,
		Uint32 Usage) const override final;

	virtual ConstantBuffer* CreateConstantBuffer(const ResourceData* InitalData, Uint32 SizeInBytes, Uint32 Usage) const override final;

	virtual StructuredBuffer* CreateStructuredBuffer(
		const ResourceData* InitalData,
		Uint32 SizeInBytes,
		Uint32 Stride,
		Uint32 Usage) const override final;

	// Ray Tracing
	virtual class RayTracingGeometry* CreateRayTracingGeometry() const override final;
	virtual class RayTracingScene* CreateRayTracingScene() const override final;

	/*
	* Resource Views
	*/

	// ShaderResourceView
	virtual ShaderResourceView* CreateShaderResourceView(
		const Buffer* Buffer,
		Uint32 FirstElement,
		Uint32 ElementCount,
		EFormat Format) const override final;

	virtual ShaderResourceView* CreateShaderResourceView(
		const Buffer* Buffer,
		Uint32 FirstElement,
		Uint32 ElementCount,
		Uint32 Stride) const override final;

	virtual ShaderResourceView* CreateShaderResourceView(
		const Texture1D* Texture,
		EFormat Format,
		Uint32 MostDetailedMip,
		Uint32 MipLevels) const override final;

	virtual ShaderResourceView* CreateShaderResourceView(
		const Texture1DArray* Texture,
		EFormat Format,
		Uint32 MostDetailedMip,
		Uint32 MipLevels,
		Uint32 FirstArraySlice,
		Uint32 ArraySize) const override final;

	virtual ShaderResourceView* CreateShaderResourceView(
		const Texture2D* Texture,
		EFormat Format,
		Uint32 MostDetailedMip,
		Uint32 MipLevels) const override final;

	virtual ShaderResourceView* CreateShaderResourceView(
		const Texture2DArray* Texture,
		EFormat Format,
		Uint32 MostDetailedMip,
		Uint32 MipLevels,
		Uint32 FirstArraySlice,
		Uint32 ArraySize) const override final;

	virtual ShaderResourceView* CreateShaderResourceView(
		const TextureCube* Texture,
		EFormat Format,
		Uint32 MostDetailedMip,
		Uint32 MipLevels) const override final;

	virtual ShaderResourceView* CreateShaderResourceView(
		const TextureCubeArray* Texture,
		EFormat Format,
		Uint32 MostDetailedMip,
		Uint32 MipLevels,
		Uint32 FirstArraySlice,
		Uint32 ArraySize) const override final;

	virtual ShaderResourceView* CreateShaderResourceView(
		const Texture3D* Texture,
		EFormat Format,
		Uint32 MostDetailedMip,
		Uint32 MipLevels) const override final;

	// UnorderedAccessView
	virtual UnorderedAccessView* CreateUnorderedAccessView(
		const Buffer* Buffer,
		Uint64 FirstElement,
		Uint32 NumElements,
		EFormat Format,
		Uint64 CounterOffsetInBytes) const override final;

	virtual UnorderedAccessView* CreateUnorderedAccessView(
		const Buffer* Buffer,
		Uint64 FirstElement,
		Uint32 NumElements,
		Uint32 StructureByteStride,
		Uint64 CounterOffsetInBytes) const override final;

	virtual UnorderedAccessView* CreateUnorderedAccessView(const Texture1D* Texture, EFormat Format, Uint32 MipSlice) const override final;

	virtual UnorderedAccessView* CreateUnorderedAccessView(
		const Texture1DArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstArraySlice,
		Uint32 ArraySize) const override final;

	virtual UnorderedAccessView* CreateUnorderedAccessView(const Texture2D* Texture, EFormat Format, Uint32 MipSlice) const override final;

	virtual UnorderedAccessView* CreateUnorderedAccessView(
		const Texture2DArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstArraySlice,
		Uint32 ArraySize) const override final;

	virtual UnorderedAccessView* CreateUnorderedAccessView(const TextureCube* Texture, EFormat Format, Uint32 MipSlice) const override final;

	virtual UnorderedAccessView* CreateUnorderedAccessView(
		const TextureCubeArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 ArraySlice) const override final;

	virtual UnorderedAccessView* CreateUnorderedAccessView(
		const Texture3D* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstDepthSlice,
		Uint32 DepthSlices) const override final;

	// RenderTargetView
	virtual RenderTargetView* CreateRenderTargetView(const Texture1D* Texture, EFormat Format, Uint32 MipSlice) const override final;

	virtual RenderTargetView* CreateRenderTargetView(
		const Texture1DArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstArraySlice,
		Uint32 ArraySize) const override final;

	virtual RenderTargetView* CreateRenderTargetView(const Texture2D* Texture, EFormat Format, Uint32 MipSlice) const override final;

	virtual RenderTargetView* CreateRenderTargetView(
		const Texture2DArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstArraySlice,
		Uint32 ArraySize) const override final;

	virtual RenderTargetView* CreateRenderTargetView(
		const TextureCube* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstFace,
		Uint32 FaceCount) const override final;

	virtual RenderTargetView* CreateRenderTargetView(
		const TextureCubeArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 ArraySlice,
		Uint32 FirstFace,
		Uint32 FaceCount) const override final;

	virtual RenderTargetView* CreateRenderTargetView(
		const Texture3D* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstDepthSlice,
		Uint32 DepthSlices) const override final;

	// DepthStencilView
	virtual DepthStencilView* CreateDepthStencilView(const Texture1D* Texture, EFormat Format, Uint32 MipSlice) const override final;

	virtual DepthStencilView* CreateDepthStencilView(
		const Texture1DArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstArraySlice,
		Uint32 ArraySize) const override final;

	virtual DepthStencilView* CreateDepthStencilView(const Texture2D* Texture, EFormat Format, Uint32 MipSlice) const override final;

	virtual DepthStencilView* CreateDepthStencilView(
		const Texture2DArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstArraySlice,
		Uint32 ArraySize) const override final;

	virtual DepthStencilView* CreateDepthStencilView(
		const TextureCube* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstFace,
		Uint32 FaceCount) const override final;

	virtual DepthStencilView* CreateDepthStencilView(
		const TextureCubeArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 ArraySlice,
		Uint32 FaceIndex,
		Uint32 FaceCount) const override final;

	// PipelineState
	virtual class Shader* CreateShader() const override final;

	virtual class DepthStencilState*	CreateDepthStencilState()	const override final;
	virtual class RasterizerState*		CreateRasterizerState()		const override final;
	virtual class BlendState*			CreateBlendState()	const override final;
	virtual class InputLayout*			CreateInputLayout() const override final;

	virtual class GraphicsPipelineState*	CreateGraphicsPipelineState()	const override final;
	virtual class ComputePipelineState*		CreateComputePipelineState()	const override final;
	virtual class RayTracingPipelineState*	CreateRayTracingPipelineState() const override final;

	// Commands
	virtual bool IsRayTracingSupported() const override final;
	virtual bool UAVSupportsFormat(EFormat Format) const override final;
	
	virtual class ICommandContext* GetCommandContext() const override final
	{
		return nullptr;
	}

	virtual std::string GetAdapterName() const override final
	{
		return Device->GetAdapterName();
	}

private:
	bool AllocateBuffer(
		D3D12Resource& Resource, 
		D3D12_HEAP_TYPE HeapType, 
		D3D12_RESOURCE_STATES InitalState, 
		D3D12_RESOURCE_FLAGS Flags, 
		Uint32 SizeInBytes) const;

	bool AllocateTexture(
		D3D12Resource& Resource, 
		D3D12_HEAP_TYPE HeapType,
		D3D12_RESOURCE_STATES InitalState, 
		const D3D12_RESOURCE_DESC& Desc) const;
	
	bool UploadResource(D3D12Resource& Resource, const ResourceData* InitalData) const;

	TSharedPtr<D3D12SwapChain>		SwapChain;
	TSharedPtr<D3D12Device>			Device;
	TSharedPtr<D3D12CommandQueue>	Queue;
	TSharedPtr<D3D12CommandQueue>	ComputeQueue;
};