#pragma once
#include "Windows/WindowsWindow.h"

#include "Containers/TUniquePtr.h"
#include "Containers/TSharedPtr.h"

#include "D3D12/D3D12RayTracingScene.h"
#include "D3D12/D3D12ImmediateCommandList.h"

/*
* ERenderingAPI
*/

enum class ERenderingAPI : uint32
{
	RENDERING_API_UNKNOWN	= 0,
	RENDERING_API_D3D12		= 1,
};

class D3D12RootSignature;

/*
* RenderingAPI
*/

class RenderingAPI
{
public:
	virtual ~RenderingAPI() = default;

	virtual bool Initialize(TSharedRef<GenericWindow> RenderWindow, bool EnableDebug) = 0;

	virtual class D3D12Texture* CreateTexture(const struct TextureProperties& Properties) const = 0;
	virtual class D3D12Buffer* CreateBuffer(const struct BufferProperties& Properties) const = 0;
	virtual class D3D12RayTracingScene* CreateRayTracingScene(class D3D12RayTracingPipelineState* PipelineState) const = 0;
	virtual class D3D12RayTracingGeometry* CreateRayTracingGeometry() const = 0;
	virtual class D3D12DescriptorTable* CreateDescriptorTable(uint32 DescriptorCount) const = 0;

	virtual class D3D12ShaderResourceView* CreateShaderResourceView(ID3D12Resource* InResource, const D3D12_SHADER_RESOURCE_VIEW_DESC* InDesc) const = 0;
	virtual class D3D12UnorderedAccessView* CreateUnorderedAccessView(ID3D12Resource* InCounterResource, ID3D12Resource* InResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* InDesc) const = 0;
	virtual class D3D12RenderTargetView* CreateRenderTargetView(ID3D12Resource* InResource, const D3D12_RENDER_TARGET_VIEW_DESC* InDesc) const = 0;
	virtual class D3D12DepthStencilView* CreateDepthStencilView(ID3D12Resource* InResource, const D3D12_DEPTH_STENCIL_VIEW_DESC* InDesc) const = 0;
	virtual class D3D12ConstantBufferView* CreateConstantBufferView(ID3D12Resource* InResource, const D3D12_CONSTANT_BUFFER_VIEW_DESC* InDesc) const = 0;

	virtual class D3D12Fence* CreateFence(uint64 InitalValue) const = 0;
	virtual class D3D12CommandAllocator* CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE ListType) const = 0;
	virtual class D3D12CommandList* CreateCommandList(D3D12_COMMAND_LIST_TYPE Type, D3D12CommandAllocator* Allocator, ID3D12PipelineState* InitalPipeline) const = 0;
	virtual class D3D12CommandQueue* CreateCommandQueue() const = 0;
	
	virtual class D3D12ComputePipelineState* CreateComputePipelineState(const struct ComputePipelineStateProperties& Properties) const = 0;
	virtual class D3D12GraphicsPipelineState* CreateGraphicsPipelineState(const struct GraphicsPipelineStateProperties& Properties) const = 0;
	virtual class D3D12RayTracingPipelineState* CreateRayTracingPipelineState(const struct RayTracingPipelineStateProperties& Properties) const = 0;
	virtual D3D12RootSignature* CreateRootSignature(const D3D12_ROOT_SIGNATURE_DESC& Desc) const = 0;
	virtual D3D12RootSignature* CreateRootSignature(struct IDxcBlob* ShaderBlob) const = 0;

	virtual class D3D12CommandQueue* GetQueue() const = 0;
	virtual class D3D12SwapChain* GetSwapChain() const = 0;
	virtual TSharedPtr<D3D12ImmediateCommandList> GetImmediateCommandList() const = 0;

	virtual std::string GetAdapterName() const
	{
		return std::string();
	}

	virtual bool IsRayTracingSupported() const
	{
		return false;
	}

	virtual bool UAVSupportsFormat(DXGI_FORMAT Format) const
	{
		UNREFERENCED_VARIABLE(Format);
		return false;
	}

	static RenderingAPI* Make(ERenderingAPI InRenderAPI);
	static RenderingAPI& Get();
	static void Release();
	
	FORCEINLINE static TSharedPtr<D3D12ImmediateCommandList> StaticGetImmediateCommandList()
	{
		return CurrentRenderAPI->GetImmediateCommandList();
	}

protected:
	RenderingAPI() = default;

private:
	static RenderingAPI* CurrentRenderAPI;
};