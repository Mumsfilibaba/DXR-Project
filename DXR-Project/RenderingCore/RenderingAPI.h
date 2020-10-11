#pragma once
#include "Windows/WindowsWindow.h"

#include "Containers/TUniquePtr.h"
#include "Containers/TSharedPtr.h"

#include "D3D12/D3D12RayTracingScene.h"

/*
* ERenderingAPI
*/

enum class ERenderingAPI : Uint32
{
	RENDERING_API_UNKNOWN	= 0,
	RENDERING_API_D3D12		= 1,
};

/*
* RenderingAPI
*/

class RenderingAPI
{
public:
	virtual ~RenderingAPI() = default;

	virtual bool Initialize(TSharedRef<GenericWindow> RenderWindow, bool EnableDebug) = 0;

	// Resources
	virtual class Texture1D*		CreateTexture1D()		const = 0;
	virtual class Texture2D*		CreateTexture2D()		const = 0;
	virtual class Texture2DArray*	CreateTexture2DArray()	const = 0;
	virtual class Texture3D*		CreateTexture3D()		const = 0;
	virtual class TextureCube*		CreateTextureCube()		const = 0;

	virtual class VertexBuffer*		 CreateVertexBuffer(Uint32 SizeInBytes, Uint32 VertexStride) const = 0;
	virtual class IndexBuffer*		 CreateIndexBuffer(Uint32 SizeInBytes, EFormat IndexFormat) const = 0;
	virtual class ConstantBuffer*	 CreateConstantBuffer(Uint32 SizeInBytes) const = 0;
	virtual class StructuredBuffer*	 CreateStructuredBuffer(Uint32 ElementCount, Uint32 StructuredByteStride) const = 0;
	virtual class ByteAddressBuffer* CreateByteAddressBuffer(Uint32 SizeInBytes) const = 0;

	virtual class RayTracingGeometry*	CreateRayTracingGeometry()	const = 0;
	virtual class RayTracingScene*		CreateRayTracingScene()		const = 0;

	// PipelineState
	virtual class Shader* CreateShader() const = 0;

	virtual class DepthStencilState*	CreateDepthStencilState()	const = 0;
	virtual class RasterizerState*		CreateRasterizerState()		const = 0;
	virtual class BlendState*			CreateBlendState()	const = 0;
	virtual class InputLayout*			CreateInputLayout() const = 0;

	virtual class GraphicsPipelineState*	CreateGraphicsPipelineState()	const = 0;
	virtual class ComputePipelineState*		CreateComputePipelineState()	const = 0;
	virtual class RayTracingPipelineState*	CreateRayTracingPipelineState() const = 0;

	// Commands
	virtual class ICommandContext*	CreateCommandContext()		const = 0;
	virtual class CommandList&		GetDefaultCommandList()		const = 0;
	virtual class CommandExecutor&	GetDefaultCommandExecutor() const = 0;

	FORCEINLINE virtual std::string GetAdapterName() const
	{
		return std::string();
	}

	FORCEINLINE virtual bool IsRayTracingSupported() const
	{
		return false;
	}

	FORCEINLINE virtual bool UAVSupportsFormat(DXGI_FORMAT Format) const
	{
		UNREFERENCED_VARIABLE(Format);
		return false;
	}

	static RenderingAPI* Make(ERenderingAPI InRenderAPI);
	static RenderingAPI& Get();
	static void Release();

protected:
	RenderingAPI() = default;

private:
	static RenderingAPI* CurrentRenderAPI;
};