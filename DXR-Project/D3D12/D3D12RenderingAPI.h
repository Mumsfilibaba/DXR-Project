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

	virtual bool Initialize(TSharedRef<GenericWindow> RenderWindow, bool EnableDebug) override final;

	// Resources
	virtual class Texture1D*		CreateTexture1D(const ResourceData* InitalData, EFormat InFormat, Uint32 InUsage, Uint32 InWidth, Uint32 InMipLevels, const ClearValue& InOptimizedClearValue) const override final;
	virtual class Texture2D*		CreateTexture2D(const ResourceData* InitalData, EFormat InFormat, Uint32 InUsage, Uint32 InWidth, Uint32 InHeight, Uint32 InMipLevels, Uint32 InSampleCount, const ClearValue& InOptimizedClearValue) const override final;
	virtual class Texture2DArray*	CreateTexture2DArray(const ResourceData* InitalData, EFormat InFormat, Uint32 InUsage, Uint32 InWidth, Uint32 InHeight, Uint32 InMipLevels, Uint32 InArrayCount, Uint32 InSampleCount, const ClearValue& InOptimizedClearValue)	const override final;
	virtual class TextureCube*		CreateTextureCube(const ResourceData* InitalData, EFormat InFormat, Uint32 InUsage, Uint32 InSize, Uint32 InMipLevels, Uint32 InSampleCount, const ClearValue& InOptimizedClearValue) const override final;
	virtual class Texture3D*		CreateTexture3D(const ResourceData* InitalData, EFormat InFormat, Uint32 InUsage, Uint32 InWidth, Uint32 InHeight, Uint32 InDepth, Uint32 InMipLevels, const ClearValue& InOptimizedClearValue) const override final;

	virtual class VertexBuffer*		CreateVertexBuffer(const ResourceData* InitalData, Uint32 SizeInBytes, Uint32 VertexStride, Uint32 Usage) const override final;
	virtual class IndexBuffer*		CreateIndexBuffer(const ResourceData* InitalData, Uint32 SizeInBytes, EIndexFormat IndexFormat, Uint32 Usage) const override final;
	virtual class ConstantBuffer*	CreateConstantBuffer(const ResourceData* InitalData, Uint32 SizeInBytes, Uint32 Usage) const override final;
	virtual class StructuredBuffer*	CreateStructuredBuffer(const ResourceData* InitalData, Uint32 SizeInBytes, Uint32 Stride, Uint32 Usage) const override final;

	virtual class RayTracingGeometry*	CreateRayTracingGeometry()	const override final;
	virtual class RayTracingScene*		CreateRayTracingScene()		const override final;

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
	virtual class ICommandContext*	CreateCommandContext()		const override final;
	virtual class CommandList&		GetDefaultCommandList()		const override final;
	virtual class CommandExecutor&	GetDefaultCommandExecutor() const override final;

	virtual std::string GetAdapterName() const override final
	{
		return Device->GetAdapterName();
	}

	virtual bool IsRayTracingSupported() const override final;

	virtual bool UAVSupportsFormat(DXGI_FORMAT Format) const override final;

private:
	bool AllocateBuffer(D3D12Resource& Resource, D3D12_HEAP_TYPE HeapType, D3D12_RESOURCE_FLAGS Flags, Uint32 SizeInBytes) const;
	bool UploadResource(D3D12Resource& Resource, const ResourceData* InitalData) const;
	bool AllocateTexture(D3D12Resource& Resource);

	TSharedPtr<WindowsWindow>				RenderWindow;
	TSharedPtr<D3D12SwapChain>				SwapChain;
	TSharedPtr<D3D12Device>					Device;
	TSharedPtr<D3D12CommandQueue>			Queue;
	TSharedPtr<D3D12CommandQueue>			ComputeQueue;
	TSharedPtr<D3D12ImmediateCommandList>	ImmediateCommandList;
};