#pragma once
#include "RenderingCore/GenericRenderingAPI.h"

#include "Windows/WindowsWindow.h"

#include "D3D12Device.h"
#include "D3D12SwapChain.h"
#include "D3D12CommandContext.h"

class D3D12CommandContext;
class D3D12Buffer;

/*
* D3D12RenderingAPI
*/

class D3D12RenderingAPI : public GenericRenderingAPI
{
public:
	D3D12RenderingAPI();
	~D3D12RenderingAPI();

	virtual bool Init(TSharedRef<GenericWindow> RenderWindow, bool EnableDebug) override final;

	/*
	* Textures
	*/

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

	/*
	* Buffers
	*/

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

	virtual ConstantBuffer* CreateConstantBuffer(
		const ResourceData* InitalData, 
		Uint32 SizeInBytes, 
		Uint32 Usage) const override final;

	virtual StructuredBuffer* CreateStructuredBuffer(
		const ResourceData* InitalData,
		Uint32 SizeInBytes,
		Uint32 Stride,
		Uint32 Usage) const override final;

	/*
	* RayTracing
	*/

	virtual class RayTracingGeometry* CreateRayTracingGeometry() const override final;
	virtual class RayTracingScene* CreateRayTracingScene() const override final;

	/*
	* ShaderResourceView
	*/

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

	/*
	* UnorderedAccessView
	*/

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

	virtual UnorderedAccessView* CreateUnorderedAccessView(
		const Texture1D* Texture, 
		EFormat Format, 
		Uint32 MipSlice) const override final;

	virtual UnorderedAccessView* CreateUnorderedAccessView(
		const Texture1DArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstArraySlice,
		Uint32 ArraySize) const override final;

	virtual UnorderedAccessView* CreateUnorderedAccessView(
		const Texture2D* Texture, 
		EFormat Format, 
		Uint32 MipSlice) const override final;

	virtual UnorderedAccessView* CreateUnorderedAccessView(
		const Texture2DArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstArraySlice,
		Uint32 ArraySize) const override final;

	virtual UnorderedAccessView* CreateUnorderedAccessView(
		const TextureCube* Texture, 
		EFormat Format, 
		Uint32 MipSlice) const override final;

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

	/*
	* RenderTargetView
	*/

	virtual RenderTargetView* CreateRenderTargetView(
		const Texture1D* Texture, 
		EFormat Format, 
		Uint32 MipSlice) const override final;

	virtual RenderTargetView* CreateRenderTargetView(
		const Texture1DArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstArraySlice,
		Uint32 ArraySize) const override final;

	virtual RenderTargetView* CreateRenderTargetView(
		const Texture2D* Texture, 
		EFormat Format, 
		Uint32 MipSlice) const override final;

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

	/*
	* DepthStencilView
	*/
	
	virtual DepthStencilView* CreateDepthStencilView(
		const Texture1D* Texture, 
		EFormat Format, 
		Uint32 MipSlice) const override final;

	virtual DepthStencilView* CreateDepthStencilView(
		const Texture1DArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstArraySlice,
		Uint32 ArraySize) const override final;

	virtual DepthStencilView* CreateDepthStencilView(
		const Texture2D* Texture, 
		EFormat Format, 
		Uint32 MipSlice) const override final;

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

	/*
	* Pipeline
	*/

	virtual class Shader* CreateShader() const override final;

	virtual class DepthStencilState*	CreateDepthStencilState()	const override final;
	virtual class RasterizerState*		CreateRasterizerState()		const override final;
	virtual class BlendState*			CreateBlendState()	const override final;
	virtual class InputLayout*			CreateInputLayout() const override final;

	virtual class GraphicsPipelineState*	CreateGraphicsPipelineState()	const override final;
	virtual class ComputePipelineState*		CreateComputePipelineState()	const override final;
	virtual class RayTracingPipelineState*	CreateRayTracingPipelineState() const override final;

	/*
	* Supported features
	*/

	virtual bool IsRayTracingSupported() const override final;
	virtual bool UAVSupportsFormat(EFormat Format) const override final;
	
	virtual class ICommandContext* GetDefaultCommandContext() const override final
	{
		return DirectCmdContext.Get();
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
	
	bool UploadBuffer(D3D12Buffer& Buffer, Uint32 SizeInBytes, const ResourceData* InitalData) const;
	bool UploadTexture(D3D12Resource& Resource, const ResourceData* InitalData) const;

	template<typename TD3D12Texture>
	TD3D12Texture* CreateTextureResource(
		TD3D12Texture* Texture, 
		Uint32 Usage, 
		const D3D12_RESOURCE_DESC& Desc, 
		const ResourceData* InitalData) const
	{
		D3D12_HEAP_TYPE HeapType = D3D12_HEAP_TYPE_DEFAULT;
		if (Usage & TextureUsage_Dynamic)
		{
			HeapType = D3D12_HEAP_TYPE_UPLOAD;
		}

		if (!AllocateTexture(*Texture, HeapType, D3D12_RESOURCE_STATE_COMMON, Desc))
		{
			LOG_ERROR("[D3D12RenderingAPI]: Failed to allocate texture");
			return nullptr;
		}

		if (InitalData)
		{
			UploadTexture(*Texture, InitalData);
		}

		return Texture;
	}

	template<typename TD3D12Buffer, typename... TArgs>
	TD3D12Buffer* CreateBufferResource(
		const ResourceData* InitalData,
		TArgs&&... Args) const
	{
		// Create buffer object and get size to allocate
		TD3D12Buffer* NewBuffer = new TD3D12Buffer(Device.Get(), Forward<TArgs>(Args)...);
		const Uint64 Alignment		= NewBuffer->GetRequiredAlignment();
		const Uint64 SizeInBytes	= NewBuffer->GetSizeInBytes();
		const Uint64 AlignedSize	= AlignUp<Uint64>(SizeInBytes, Alignment);

		// Get properties based on Usage
		const Uint32 Usage = NewBuffer->GetUsage();
		const D3D12_RESOURCE_FLAGS Flags = ConvertBufferUsage(Usage);

		D3D12_HEAP_TYPE HeapType = D3D12_HEAP_TYPE_DEFAULT;
		D3D12_RESOURCE_STATES InitalState = D3D12_RESOURCE_STATE_COMMON;
		if (Usage & BufferUsage_Dynamic)
		{
			InitalState	= D3D12_RESOURCE_STATE_GENERIC_READ;
			HeapType	= D3D12_HEAP_TYPE_UPLOAD;
		}

		// Allocate
		if (!AllocateBuffer(*NewBuffer, HeapType, InitalState, Flags, AlignedSize))
		{
			LOG_ERROR("[D3D12RenderingAPI]: Failed to allocate buffer");
			return nullptr;
		}

		// Upload initial
		if (InitalData)
		{
			UploadBuffer(*NewBuffer, SizeInBytes, InitalData);
		}

		return NewBuffer;
	}

private:
	TSharedPtr<D3D12SwapChain>		SwapChain;
	TSharedPtr<D3D12Device>			Device;
	TSharedPtr<D3D12CommandQueue>	DirectCmdQueue;
	TSharedPtr<D3D12CommandContext>	DirectCmdContext;
	D3D12DefaultRootSignatures		DefaultRootSignatures;
};