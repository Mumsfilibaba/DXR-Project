#pragma once
#include "Windows/WindowsWindow.h"

#include "Containers/TUniquePtr.h"
#include "Containers/TSharedPtr.h"

#include "D3D12/D3D12RayTracingScene.h"

class Texture1D;
class Texture1DArray;
class Texture2D;
class Texture2DArray;
class TextureCube;
class TextureCubeArray;
class Texture3D;
class VertexBuffer;
class IndexBuffer;
class ConstantBuffer;
class StructuredBuffer;
class ShaderResourceView;
class UnorderedAccessView;
class RenderTargetView;
class DepthStencilView;

/*
* ERenderingAPI
*/

enum class ERenderingAPI : Uint32
{
	RenderingAPI_Unknown	= 0,
	RenderingAPI_D3D12		= 1,
};

/*
* RenderingAPI
*/

class RenderingAPI
{
public:
	inline RenderingAPI(ERenderingAPI InAPI)
		: API(InAPI)
	{
	}

	virtual ~RenderingAPI() = default;

	virtual bool Initialize(TSharedRef<GenericWindow> RenderWindow, bool EnableDebug) = 0;

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
		const ClearValue& OptimizedClearValue) const = 0;

	virtual Texture1DArray* CreateTexture1DArray(
		const ResourceData* InitalData,
		EFormat Format,
		Uint32 Usage,
		Uint32 Width,
		Uint32 MipLevels,
		Uint32 ArrayCount,
		const ClearValue& OptimizedClearValue) const = 0;

	virtual Texture2D* CreateTexture2D(
		const ResourceData* InitalData, 
		EFormat Format, 
		Uint32 Usage, 
		Uint32 Width, 
		Uint32 Height, 
		Uint32 MipLevels, 
		Uint32 SampleCount, 
		const ClearValue& OptimizedClearValue) const = 0;

	virtual Texture2DArray* CreateTexture2DArray(
		const ResourceData* InitalData, 
		EFormat Format, 
		Uint32 Usage, 
		Uint32 Width, 
		Uint32 Height, 
		Uint32 MipLevels, 
		Uint32 ArrayCount, 
		Uint32 SampleCount, 
		const ClearValue& OptimizedClearValue) const = 0;

	virtual TextureCube* CreateTextureCube(
		const ResourceData* InitalData, 
		EFormat Format, 
		Uint32 Usage, 
		Uint32 Size, 
		Uint32 MipLevels, 
		Uint32 SampleCount, 
		const ClearValue& OptimizedClearValue) const = 0;

	virtual TextureCubeArray* CreateTextureCubeArray(
		const ResourceData* InitalData,
		EFormat Format,
		Uint32 Usage,
		Uint32 Size,
		Uint32 MipLevels,
		Uint32 ArrayCount,
		Uint32 SampleCount,
		const ClearValue& OptimizedClearValue) const = 0;

	virtual Texture3D* CreateTexture3D(
		const ResourceData* InitalData, 
		EFormat Format, 
		Uint32 Usage, 
		Uint32 Width, 
		Uint32 Height, 
		Uint32 Depth, 
		Uint32 MipLevels, 
		const ClearValue& OptimizedClearValue) const = 0;

	// Buffers
	virtual VertexBuffer* CreateVertexBuffer(
		const ResourceData* InitalData, 
		Uint32 SizeInBytes, 
		Uint32 VertexStride, 
		Uint32 Usage) const = 0;

	virtual IndexBuffer* CreateIndexBuffer(
		const ResourceData* InitalData, 
		Uint32 SizeInBytes, 
		EIndexFormat IndexFormat, 
		Uint32 Usage) const = 0;

	virtual ConstantBuffer* CreateConstantBuffer(const ResourceData* InitalData, Uint32 SizeInBytes, Uint32 Usage) const = 0;

	virtual StructuredBuffer* CreateStructuredBuffer(
		const ResourceData* InitalData, 
		Uint32 SizeInBytes, 
		Uint32 Stride, 
		Uint32 Usage) const = 0;

	// Ray Tracing
	virtual class RayTracingGeometry* CreateRayTracingGeometry() const = 0;
	virtual class RayTracingScene* CreateRayTracingScene() const = 0;

	/*
	* Resource Views
	*/

	// ShaderResourceView
	virtual ShaderResourceView* CreateShaderResourceView(
		const Buffer* Buffer, 
		Uint32 FirstElement, 
		Uint32 ElementCount,
		EFormat Format) const = 0;

	virtual ShaderResourceView* CreateShaderResourceView(
		const Buffer* Buffer,
		Uint32 FirstElement,
		Uint32 ElementCount,
		Uint32 Stride) const = 0;

	virtual ShaderResourceView* CreateShaderResourceView(
		const Texture1D* Texture,
		EFormat Format,
		Uint32 MostDetailedMip,
		Uint32 MipLevels) const = 0;

	virtual ShaderResourceView* CreateShaderResourceView(
		const Texture1DArray* Texture,
		EFormat Format,
		Uint32 MostDetailedMip, 
		Uint32 MipLevels,
		Uint32 FirstArraySlice,
		Uint32 ArraySize) const = 0;

	virtual ShaderResourceView* CreateShaderResourceView(
		const Texture2D* Texture,
		EFormat Format,
		Uint32 MostDetailedMip, 
		Uint32 MipLevels) const = 0;

	virtual ShaderResourceView* CreateShaderResourceView(
		const Texture2DArray* Texture, 
		EFormat Format,
		Uint32 MostDetailedMip,
		Uint32 MipLevels,
		Uint32 FirstArraySlice,
		Uint32 ArraySize) const = 0;

	virtual ShaderResourceView* CreateShaderResourceView(
		const TextureCube* Texture,
		EFormat Format,
		Uint32 MostDetailedMip,
		Uint32 MipLevels) const = 0;

	virtual ShaderResourceView* CreateShaderResourceView(
		const TextureCubeArray* Texture,
		EFormat Format,
		Uint32 MostDetailedMip,
		Uint32 MipLevels,
		Uint32 FirstArraySlice,
		Uint32 ArraySize) const = 0;

	virtual ShaderResourceView* CreateShaderResourceView(
		const Texture3D* Texture,
		EFormat Format,
		Uint32 MostDetailedMip,
		Uint32 MipLevels) const = 0;

	// UnorderedAccessView
	virtual UnorderedAccessView* CreateUnorderedAccessView(
		const Buffer* Buffer,
		Uint64 FirstElement,
		Uint32 NumElements,
		EFormat Format,
		Uint64 CounterOffsetInBytes) const = 0;

	virtual UnorderedAccessView* CreateUnorderedAccessView(
		const Buffer* Buffer,
		Uint64 FirstElement,
		Uint32 NumElements,
		Uint32 StructureByteStride,
		Uint64 CounterOffsetInBytes) const = 0;

	virtual UnorderedAccessView* CreateUnorderedAccessView(const Texture1D* Texture, EFormat Format, Uint32 MipSlice) const = 0;

	virtual UnorderedAccessView* CreateUnorderedAccessView(
		const Texture1DArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstArraySlice,
		Uint32 ArraySize) const = 0;

	virtual UnorderedAccessView* CreateUnorderedAccessView(const Texture2D* Texture, EFormat Format, Uint32 MipSlice) const = 0;

	virtual UnorderedAccessView* CreateUnorderedAccessView(
		const Texture2DArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstArraySlice,
		Uint32 ArraySize) const = 0;

	virtual UnorderedAccessView* CreateUnorderedAccessView(const TextureCube* Texture, EFormat Format, Uint32 MipSlice) const = 0;

	virtual UnorderedAccessView* CreateUnorderedAccessView(
		const TextureCubeArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 ArraySlice) const = 0;

	virtual UnorderedAccessView* CreateUnorderedAccessView(
		const Texture3D* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstDepthSlice,
		Uint32 DepthSlices) const = 0;

	// RenderTargetView
	virtual RenderTargetView* CreateRenderTargetView(const Texture1D* Texture, EFormat Format, Uint32 MipSlice) const = 0;

	virtual RenderTargetView* CreateRenderTargetView(
		const Texture1DArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstArraySlice,
		Uint32 ArraySize) const = 0;

	virtual RenderTargetView* CreateRenderTargetView(const Texture2D* Texture, EFormat Format, Uint32 MipSlice) const = 0;

	virtual RenderTargetView* CreateRenderTargetView(
		const Texture2DArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstArraySlice,
		Uint32 ArraySize) const = 0;

	virtual RenderTargetView* CreateRenderTargetView(
		const TextureCube* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstFace,
		Uint32 FaceCount) const = 0;

	virtual RenderTargetView* CreateRenderTargetView(
		const TextureCubeArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 ArraySlice,
		Uint32 FirstFace,
		Uint32 FaceCount) const = 0;

	virtual RenderTargetView* CreateRenderTargetView(
		const Texture3D* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstDepthSlice,
		Uint32 DepthSlices) const = 0;
	
	// DepthStencilView
	virtual DepthStencilView* CreateDepthStencilView(const Texture1D* Texture, EFormat Format, Uint32 MipSlice) const = 0;

	virtual DepthStencilView* CreateDepthStencilView(
		const Texture1DArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstArraySlice,
		Uint32 ArraySize) const = 0;

	virtual DepthStencilView* CreateDepthStencilView(const Texture2D* Texture, EFormat Format, Uint32 MipSlice) const = 0;

	virtual DepthStencilView* CreateDepthStencilView(
		const Texture2DArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstArraySlice,
		Uint32 ArraySize) const = 0;

	virtual DepthStencilView* CreateDepthStencilView(
		const TextureCube* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstFace,
		Uint32 FaceCount) const = 0;

	virtual DepthStencilView* CreateDepthStencilView(
		const TextureCubeArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 ArraySlice,
		Uint32 FaceIndex,
		Uint32 FaceCount) const = 0;

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

	FORCEINLINE ERenderingAPI GetAPI() const
	{
		return API;
	}

	static RenderingAPI* Make(ERenderingAPI InRenderAPI);
	static RenderingAPI& Get();
	static void Release();

protected:
	RenderingAPI() = default;

	ERenderingAPI API;
private:
	static RenderingAPI* CurrentRenderAPI;
};

/*
* Resource Create functions
*/

// Textures
inline TSharedRef<Texture1D> CreateTexture1D(
	const ResourceData* InitalData, 
	EFormat Format, 
	Uint32 Usage, 
	Uint32 Width, 
	Uint32 MipLevels, 
	const ClearValue& OptimizedClearValue)
{
	return RenderingAPI::Get().CreateTexture1D(
		InitalData, 
		Format, 
		Usage, 
		Width, 
		MipLevels, 
		OptimizedClearValue);
}

inline TSharedRef<Texture1DArray> CreateTexture1DArray(
	const ResourceData* InitalData,
	EFormat Format,
	Uint32 Usage,
	Uint32 Width,
	Uint32 MipLevels,
	Uint32 ArrayCount,
	const ClearValue& OptimizedClearValue)
{
	return RenderingAPI::Get().CreateTexture1DArray(
		InitalData,
		Format,
		Usage,
		Width,
		MipLevels,
		ArrayCount,
		OptimizedClearValue);
}

inline TSharedRef<Texture2D> CreateTexture2D(
	const ResourceData* InitalData, 
	EFormat Format, 
	Uint32 Usage, 
	Uint32 Width, 
	Uint32 Height, 
	Uint32 MipLevels, 
	Uint32 SampleCount, 
	const ClearValue& OptimizedClearValue)
{
	return RenderingAPI::Get().CreateTexture2D(
		InitalData, 
		Format, 
		Usage, 
		Width, 
		Height, 
		MipLevels, 
		SampleCount, 
		OptimizedClearValue);
}

inline TSharedRef<Texture2DArray> CreateTexture2DArray(
	const ResourceData* InitalData, 
	EFormat Format, 
	Uint32 Usage, 
	Uint32 Width, 
	Uint32 Height, 
	Uint32 MipLevels, 
	Uint32 ArrayCount, 
	Uint32 SampleCount, 
	const ClearValue& OptimizedClearValue) 
{
	return RenderingAPI::Get().CreateTexture2DArray(
		InitalData, 
		Format, 
		Usage, 
		Width, 
		Height, 
		MipLevels, 
		ArrayCount,
		SampleCount, 
		OptimizedClearValue);
}

inline TSharedRef<TextureCube> CreateTextureCube(
	const ResourceData* InitalData,
	EFormat Format,
	Uint32 Usage,
	Uint32 Size,
	Uint32 MipLevels,
	Uint32 SampleCount,
	const ClearValue& OptimizedClearValue)
{
	return RenderingAPI::Get().CreateTextureCube(
		InitalData, 
		Format,
		Usage,
		Size,
		MipLevels,
		SampleCount,
		OptimizedClearValue);
}

inline TSharedRef<TextureCubeArray> CreateTextureCubeArray(
	const ResourceData* InitalData,
	EFormat Format,
	Uint32 Usage,
	Uint32 Size,
	Uint32 MipLevels,
	Uint32 ArrayCount,
	Uint32 SampleCount,
	const ClearValue& OptimizedClearValue)
{
	return RenderingAPI::Get().CreateTextureCubeArray(
		InitalData,
		Format,
		Usage,
		Size,
		MipLevels,
		ArrayCount,
		SampleCount,
		OptimizedClearValue);
}

inline TSharedRef<Texture3D> CreateTexture3D(
	const ResourceData* InitalData,
	EFormat Format,
	Uint32 Usage,
	Uint32 Width,
	Uint32 Height,
	Uint32 Depth,
	Uint32 MipLevels,
	const ClearValue& OptimizedClearValue)
{
	return RenderingAPI::Get().CreateTexture3D(
		InitalData,
		Format,
		Usage,
		Width,
		Height,
		Depth,
		MipLevels,
		OptimizedClearValue);
}

// Buffers
inline TSharedRef<VertexBuffer> CreateVertexBuffer(
	const ResourceData* InitalData,
	Uint32 SizeInBytes,
	Uint32 VertexStride,
	Uint32 Usage)
{
	return RenderingAPI::Get().CreateVertexBuffer(
		InitalData,
		SizeInBytes,
		VertexStride,
		Usage);
}

inline TSharedRef<IndexBuffer> CreateIndexBuffer(
	const ResourceData* InitalData,
	Uint32 SizeInBytes,
	EIndexFormat IndexFormat,
	Uint32 Usage)
{
	return RenderingAPI::Get().CreateIndexBuffer(
		InitalData,
		SizeInBytes,
		IndexFormat,
		Usage);
}

inline TSharedRef<ConstantBuffer> CreateConstantBuffer(const ResourceData* InitalData, Uint32 SizeInBytes, Uint32 Usage)
{
	return RenderingAPI::Get().CreateConstantBuffer(InitalData, SizeInBytes, Usage);
}

inline TSharedRef<StructuredBuffer> CreateStructuredBuffer(
	const ResourceData* InitalData,
	Uint32 SizeInBytes,
	Uint32 Stride,
	Uint32 Usage)
{
	return RenderingAPI::Get().CreateStructuredBuffer(
		InitalData,
		SizeInBytes,
		Stride,
		Usage);
}

/*
* Resource View Create Functions
*/

// ShaderResourceView
inline TSharedRef<ShaderResourceView> CreateShaderResourceView(
	const Buffer* Buffer,
	Uint32 FirstElement,
	Uint32 ElementCount,
	EFormat Format)
{
	return RenderingAPI::Get().CreateShaderResourceView(
		Buffer,
		FirstElement,
		ElementCount,
		Format);
}

inline TSharedRef<ShaderResourceView> CreateShaderResourceView(
	const Buffer* Buffer,
	Uint32 FirstElement,
	Uint32 ElementCount,
	Uint32 Stride)
{
	return RenderingAPI::Get().CreateShaderResourceView(
		Buffer,
		FirstElement,
		ElementCount,
		Stride);
}

inline TSharedRef<ShaderResourceView> CreateShaderResourceView(
	const Texture1D* Texture,
	EFormat Format,
	Uint32 MostDetailedMip,
	Uint32 MipLevels) 
{ 
	return RenderingAPI::Get().CreateShaderResourceView(
		Texture,
		Format,
		MostDetailedMip,
		MipLevels);
}

inline TSharedRef<ShaderResourceView> CreateShaderResourceView(
	const Texture1DArray* Texture,
	EFormat Format,
	Uint32 MostDetailedMip,
	Uint32 MipLevels,
	Uint32 FirstArraySlice,
	Uint32 ArraySize)
{
	return RenderingAPI::Get().CreateShaderResourceView(
		Texture,
		Format,
		MostDetailedMip,
		MipLevels,
		FirstArraySlice,
		ArraySize);
}

inline TSharedRef<ShaderResourceView> CreateShaderResourceView(
	const Texture2D* Texture,
	EFormat Format,
	Uint32 MostDetailedMip,
	Uint32 MipLevels)
{
	return RenderingAPI::Get().CreateShaderResourceView(
		Texture,
		Format,
		MostDetailedMip,
		MipLevels);
}

inline TSharedRef<ShaderResourceView> CreateShaderResourceView(
	const Texture2DArray* Texture,
	EFormat Format,
	Uint32 MostDetailedMip,
	Uint32 MipLevels,
	Uint32 FirstArraySlice,
	Uint32 ArraySize)
{
	return RenderingAPI::Get().CreateShaderResourceView(
		Texture,
		Format,
		MostDetailedMip,
		MipLevels,
		FirstArraySlice,
		ArraySize);
}

inline TSharedRef<ShaderResourceView> CreateShaderResourceView(
	const TextureCube* Texture,
	EFormat Format,
	Uint32 MostDetailedMip,
	Uint32 MipLevels)
{
	return RenderingAPI::Get().CreateShaderResourceView(
		Texture,
		Format,
		MostDetailedMip,
		MipLevels);
}

inline TSharedRef<ShaderResourceView> CreateShaderResourceView(
	const TextureCubeArray* Texture,
	EFormat Format,
	Uint32 MostDetailedMip,
	Uint32 MipLevels,
	Uint32 FirstArraySlice,
	Uint32 ArraySize)
{
	return RenderingAPI::Get().CreateShaderResourceView(
		Texture,
		Format,
		MostDetailedMip,
		MipLevels,
		FirstArraySlice,
		ArraySize);
}

inline TSharedRef<ShaderResourceView> CreateShaderResourceView(
	const Texture3D* Texture,
	EFormat Format,
	Uint32 MostDetailedMip,
	Uint32 MipLevels)
{
	return RenderingAPI::Get().CreateShaderResourceView(
		Texture,
		Format,
		MostDetailedMip,
		MipLevels);
}

// UnorderedAccessView
inline TSharedRef<UnorderedAccessView> CreateUnorderedAccessView(
	const Buffer* Buffer,
	Uint64 FirstElement,
	Uint32 NumElements,
	EFormat Format,
	Uint64 CounterOffsetInBytes)
{
	return RenderingAPI::Get().CreateUnorderedAccessView(
		Buffer,
		FirstElement,
		NumElements,
		Format,
		CounterOffsetInBytes);
}

inline TSharedRef<UnorderedAccessView> CreateUnorderedAccessView(
	const Buffer* Buffer,
	Uint64 FirstElement,
	Uint32 NumElements,
	Uint32 StructureByteStride,
	Uint64 CounterOffsetInBytes)
{
	return RenderingAPI::Get().CreateUnorderedAccessView(
		Buffer,
		FirstElement,
		NumElements,
		StructureByteStride,
		CounterOffsetInBytes);
}

inline TSharedRef<UnorderedAccessView> CreateUnorderedAccessView(const Texture1D* Texture, EFormat Format, Uint32 MipSlice)
{
	return RenderingAPI::Get().CreateUnorderedAccessView(Texture, Format, MipSlice);
}

inline TSharedRef<UnorderedAccessView> CreateUnorderedAccessView(
	const Texture1DArray* Texture,
	EFormat Format,
	Uint32 MipSlice,
	Uint32 FirstArraySlice,
	Uint32 ArraySize)
{
	return RenderingAPI::Get().CreateUnorderedAccessView(
		Texture,
		Format,
		MipSlice,
		FirstArraySlice,
		ArraySize);
}

inline TSharedRef<UnorderedAccessView> CreateUnorderedAccessView(const Texture2D* Texture, EFormat Format, Uint32 MipSlice)
{
	return RenderingAPI::Get().CreateUnorderedAccessView(Texture, Format, MipSlice);
}

inline TSharedRef<UnorderedAccessView> CreateUnorderedAccessView(
	const Texture2DArray* Texture,
	EFormat Format,
	Uint32 MipSlice,
	Uint32 FirstArraySlice,
	Uint32 ArraySize)
{
	return RenderingAPI::Get().CreateUnorderedAccessView(
		Texture,
		Format,
		MipSlice,
		FirstArraySlice,
		ArraySize);
}

inline TSharedRef<UnorderedAccessView> CreateUnorderedAccessView(const TextureCube* Texture, EFormat Format, Uint32 MipSlice)
{
	return RenderingAPI::Get().CreateUnorderedAccessView(
		Texture,
		Format,
		MipSlice);
}

inline TSharedRef<UnorderedAccessView> CreateUnorderedAccessView(
	const TextureCubeArray* Texture,
	EFormat Format,
	Uint32 MipSlice,
	Uint32 ArraySlice)
{
	return RenderingAPI::Get().CreateUnorderedAccessView(
		Texture,
		Format,
		MipSlice,
		ArraySlice);
}

inline TSharedRef<UnorderedAccessView> CreateUnorderedAccessView(
	const Texture3D* Texture,
	EFormat Format,
	Uint32 MipSlice,
	Uint32 FirstDepthSlice,
	Uint32 DepthSlices) 
{
	return RenderingAPI::Get().CreateUnorderedAccessView(
		Texture,
		Format,
		MipSlice,
		FirstDepthSlice,
		DepthSlices);
}

// RenderTargetView
inline TSharedRef<RenderTargetView> CreateRenderTargetView(const Texture1D* Texture, EFormat Format, Uint32 MipSlice)
{
	return RenderingAPI::Get().CreateRenderTargetView(Texture, Format, MipSlice);
}

inline TSharedRef<RenderTargetView> CreateRenderTargetView(
	const Texture1DArray* Texture,
	EFormat Format,
	Uint32 MipSlice,
	Uint32 FirstArraySlice,
	Uint32 ArraySize)
{
	return RenderingAPI::Get().CreateRenderTargetView(
		Texture,
		Format,
		MipSlice,
		FirstArraySlice,
		ArraySize);
}

inline TSharedRef<RenderTargetView> CreateRenderTargetView(const Texture2D* Texture, EFormat Format, Uint32 MipSlice)
{
	return RenderingAPI::Get().CreateRenderTargetView(
		Texture,
		Format,
		MipSlice);
}

inline TSharedRef<RenderTargetView> CreateRenderTargetView(
	const Texture2DArray* Texture,
	EFormat Format,
	Uint32 MipSlice,
	Uint32 FirstArraySlice,
	Uint32 ArraySize)
{
	return RenderingAPI::Get().CreateRenderTargetView(
		Texture,
		Format,
		MipSlice,
		FirstArraySlice,
		ArraySize);
}

inline TSharedRef<RenderTargetView> CreateRenderTargetView(
	const TextureCube* Texture,
	EFormat Format,
	Uint32 MipSlice,
	Uint32 FirstFace,
	Uint32 FaceCount)
{
	return RenderingAPI::Get().CreateRenderTargetView(
		Texture,
		Format,
		MipSlice,
		FirstFace,
		FaceCount);
}

inline TSharedRef<RenderTargetView> CreateRenderTargetView(
	const TextureCubeArray* Texture,
	EFormat Format,
	Uint32 MipSlice,
	Uint32 ArraySlice,
	Uint32 FirstFace,
	Uint32 FaceCount)
{
	return RenderingAPI::Get().CreateRenderTargetView(
		Texture,
		Format,
		MipSlice,
		ArraySlice,
		FirstFace,
		FaceCount);
}

inline TSharedRef<RenderTargetView> CreateRenderTargetView(
	const Texture3D* Texture,
	EFormat Format,
	Uint32 MipSlice,
	Uint32 FirstDepthSlice,
	Uint32 DepthSlices) 
{
	return RenderingAPI::Get().CreateRenderTargetView(
		Texture,
		Format,
		MipSlice,
		FirstDepthSlice,
		DepthSlices);
}

// DepthStencilView
inline TSharedRef<DepthStencilView> CreateDepthStencilView(const Texture1D* Texture, EFormat Format, Uint32 MipSlice)
{
	return RenderingAPI::Get().CreateDepthStencilView(Texture, Format, MipSlice);
}

inline TSharedRef<DepthStencilView> CreateDepthStencilView(
	const Texture1DArray* Texture,
	EFormat Format,
	Uint32 MipSlice,
	Uint32 FirstArraySlice,
	Uint32 ArraySize)
{
	return RenderingAPI::Get().CreateDepthStencilView(
		Texture,
		Format,
		MipSlice,
		FirstArraySlice,
		ArraySize);
}

inline TSharedRef<DepthStencilView> CreateDepthStencilView(const Texture2D* Texture, EFormat Format, Uint32 MipSlice)
{
	return RenderingAPI::Get().CreateDepthStencilView(Texture, Format, MipSlice);
}

inline TSharedRef<DepthStencilView> CreateDepthStencilView(
	const Texture2DArray* Texture,
	EFormat Format,
	Uint32 MipSlice,
	Uint32 FirstArraySlice,
	Uint32 ArraySize)
{
	return RenderingAPI::Get().CreateDepthStencilView(
		Texture,
		Format,
		MipSlice,
		FirstArraySlice,
		ArraySize);
}

inline TSharedRef<DepthStencilView> CreateDepthStencilView(
	const TextureCube* Texture,
	EFormat Format,
	Uint32 MipSlice,
	Uint32 FirstFace,
	Uint32 FaceCount)
{
	return RenderingAPI::Get().CreateDepthStencilView(
		Texture,
		Format,
		MipSlice,
		FirstFace,
		FaceCount);
}

inline TSharedRef<DepthStencilView> CreateDepthStencilView(
	const TextureCubeArray* Texture,
	EFormat Format,
	Uint32 MipSlice,
	Uint32 ArraySlice,
	Uint32 FirstFace,
	Uint32 FaceCount)
{
	return RenderingAPI::Get().CreateDepthStencilView(
		Texture,
		Format,
		MipSlice,
		ArraySlice,
		FirstFace,
		FaceCount);
}
