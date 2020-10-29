#pragma once
#include "Application/Generic/GenericWindow.h"

#include "Engine/EngineGlobals.h"

#include "Containers/TUniquePtr.h"
#include "Containers/TSharedPtr.h"

#include "RenderingCore.h"
#include "Buffer.h"
#include "CommandList.h"

class Texture1D;
class Texture1DArray;
class Texture2D;
class Texture2DArray;
class TextureCube;
class TextureCubeArray;
class Texture3D;
class ShaderResourceView;
class UnorderedAccessView;
class RenderTargetView;
class DepthStencilView;
struct ResourceData;
struct ClearValue;
class RayTracingGeometry;
class RayTracingScene;

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

	virtual bool Init(TSharedRef<GenericWindow> RenderWindow, bool EnableDebug) = 0;

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
	virtual RayTracingScene*	CreateRayTracingScene()		const = 0;
	virtual RayTracingGeometry* CreateRayTracingGeometry()	const = 0;

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
	virtual class ICommandContext* GetCommandContext() const = 0;

	// Getters
	FORCEINLINE virtual std::string GetAdapterName() const
	{
		return std::string();
	}

	FORCEINLINE virtual bool IsRayTracingSupported() const
	{
		return false;
	}

	FORCEINLINE virtual bool UAVSupportsFormat(EFormat Format) const
	{
		UNREFERENCED_VARIABLE(Format);
		return false;
	}

	FORCEINLINE ERenderingAPI GetAPI() const
	{
		return API;
	}

	// Creation
	static bool Initialize(ERenderingAPI InRenderAPI, TSharedRef<GenericWindow> RenderingWindow);
	static void Release();

protected:
	ERenderingAPI API;
};

/*
* Resource Create functions
*/

// Textures
inline Texture1D* CreateTexture1D(
	const ResourceData* InitalData, 
	EFormat Format, 
	Uint32 Usage, 
	Uint32 Width, 
	Uint32 MipLevels, 
	const ClearValue& OptimizedClearValue)
{
	return EngineGlobals::RenderingAPI->CreateTexture1D(
		InitalData, 
		Format, 
		Usage, 
		Width, 
		MipLevels, 
		OptimizedClearValue);
}

inline Texture1DArray* CreateTexture1DArray(
	const ResourceData* InitalData,
	EFormat Format,
	Uint32 Usage,
	Uint32 Width,
	Uint32 MipLevels,
	Uint32 ArrayCount,
	const ClearValue& OptimizedClearValue)
{
	return EngineGlobals::RenderingAPI->CreateTexture1DArray(
		InitalData,
		Format,
		Usage,
		Width,
		MipLevels,
		ArrayCount,
		OptimizedClearValue);
}

inline Texture2D* CreateTexture2D(
	const ResourceData* InitalData, 
	EFormat Format, 
	Uint32 Usage, 
	Uint32 Width, 
	Uint32 Height, 
	Uint32 MipLevels, 
	Uint32 SampleCount, 
	const ClearValue& OptimizedClearValue)
{
	return EngineGlobals::RenderingAPI->CreateTexture2D(
		InitalData, 
		Format, 
		Usage, 
		Width, 
		Height, 
		MipLevels, 
		SampleCount, 
		OptimizedClearValue);
}

inline Texture2DArray* CreateTexture2DArray(
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
	return EngineGlobals::RenderingAPI->CreateTexture2DArray(
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

inline TextureCube* CreateTextureCube(
	const ResourceData* InitalData,
	EFormat Format,
	Uint32 Usage,
	Uint32 Size,
	Uint32 MipLevels,
	Uint32 SampleCount,
	const ClearValue& OptimizedClearValue)
{
	return EngineGlobals::RenderingAPI->CreateTextureCube(
		InitalData, 
		Format,
		Usage,
		Size,
		MipLevels,
		SampleCount,
		OptimizedClearValue);
}

inline TextureCubeArray* CreateTextureCubeArray(
	const ResourceData* InitalData,
	EFormat Format,
	Uint32 Usage,
	Uint32 Size,
	Uint32 MipLevels,
	Uint32 ArrayCount,
	Uint32 SampleCount,
	const ClearValue& OptimizedClearValue)
{
	return EngineGlobals::RenderingAPI->CreateTextureCubeArray(
		InitalData,
		Format,
		Usage,
		Size,
		MipLevels,
		ArrayCount,
		SampleCount,
		OptimizedClearValue);
}

inline Texture3D* CreateTexture3D(
	const ResourceData* InitalData,
	EFormat Format,
	Uint32 Usage,
	Uint32 Width,
	Uint32 Height,
	Uint32 Depth,
	Uint32 MipLevels,
	const ClearValue& OptimizedClearValue)
{
	return EngineGlobals::RenderingAPI->CreateTexture3D(
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
inline VertexBuffer* CreateVertexBuffer(
	const ResourceData* InitalData,
	Uint32 SizeInBytes,
	Uint32 VertexStride,
	Uint32 Usage)
{
	return EngineGlobals::RenderingAPI->CreateVertexBuffer(
		InitalData,
		SizeInBytes,
		VertexStride,
		Usage);
}

template<typename T>
inline VertexBuffer* CreateVertexBuffer(const ResourceData* InitalData, Uint32 VertexCount, Uint32 Usage)
{
	constexpr Uint32 STRIDE = sizeof(T);
	const Uint32 SizeInByte = STRIDE * VertexCount;
	return CreateVertexBuffer(InitalData, SizeInByte, STRIDE, Usage);
}

inline IndexBuffer* CreateIndexBuffer(
	const ResourceData* InitalData,
	Uint32 SizeInBytes,
	EIndexFormat IndexFormat,
	Uint32 Usage)
{
	return EngineGlobals::RenderingAPI->CreateIndexBuffer(
		InitalData,
		SizeInBytes,
		IndexFormat,
		Usage);
}

inline ConstantBuffer* CreateConstantBuffer(const ResourceData* InitalData, Uint32 SizeInBytes, Uint32 Usage)
{
	return EngineGlobals::RenderingAPI->CreateConstantBuffer(InitalData, SizeInBytes, Usage);
}

template<typename T>
inline ConstantBuffer* CreateConstantBuffer(const ResourceData* InitalData, Uint32 Usage)
{
	return CreateConstantBuffer(InitalData, sizeof(T), Usage);
}

template<typename T>
inline ConstantBuffer* CreateConstantBuffer(const ResourceData* InitalData, Uint32 ElementCount, Uint32 Usage)
{
	return CreateConstantBuffer(InitalData, sizeof(T) * ElementCount, Usage);
}

inline StructuredBuffer* CreateStructuredBuffer(
	const ResourceData* InitalData,
	Uint32 SizeInBytes,
	Uint32 Stride,
	Uint32 Usage)
{
	return EngineGlobals::RenderingAPI->CreateStructuredBuffer(
		InitalData,
		SizeInBytes,
		Stride,
		Usage);
}

/*
* RayTracing Create functions
*/

inline RayTracingScene* CreateRayTracingScene()
{
	return EngineGlobals::RenderingAPI->CreateRayTracingScene();
}

inline RayTracingGeometry* CreateRayTracingGeometry()
{
	return EngineGlobals::RenderingAPI->CreateRayTracingGeometry();
}

/*
* Resource View Create Functions
*/

// ShaderResourceView
inline ShaderResourceView* CreateShaderResourceView(
	const Buffer* Buffer,
	Uint32 FirstElement,
	Uint32 ElementCount,
	EFormat Format)
{
	return EngineGlobals::RenderingAPI->CreateShaderResourceView(
		Buffer,
		FirstElement,
		ElementCount,
		Format);
}

inline ShaderResourceView* CreateShaderResourceView(
	const Buffer* Buffer,
	Uint32 FirstElement,
	Uint32 ElementCount,
	Uint32 Stride)
{
	return EngineGlobals::RenderingAPI->CreateShaderResourceView(
		Buffer,
		FirstElement,
		ElementCount,
		Stride);
}

template<typename T>
inline ShaderResourceView* CreateShaderResourceView(const Buffer* Buffer, Uint32 FirstElement, Uint32 ElementCount)
{
	return CreateShaderResourceView(Buffer, FirstElement, ElementCount, sizeof(T));
}

inline ShaderResourceView* CreateShaderResourceView(
	const Texture1D* Texture,
	EFormat Format,
	Uint32 MostDetailedMip,
	Uint32 MipLevels) 
{ 
	return EngineGlobals::RenderingAPI->CreateShaderResourceView(
		Texture,
		Format,
		MostDetailedMip,
		MipLevels);
}

inline ShaderResourceView* CreateShaderResourceView(
	const Texture1DArray* Texture,
	EFormat Format,
	Uint32 MostDetailedMip,
	Uint32 MipLevels,
	Uint32 FirstArraySlice,
	Uint32 ArraySize)
{
	return EngineGlobals::RenderingAPI->CreateShaderResourceView(
		Texture,
		Format,
		MostDetailedMip,
		MipLevels,
		FirstArraySlice,
		ArraySize);
}

inline ShaderResourceView* CreateShaderResourceView(
	const Texture2D* Texture,
	EFormat Format,
	Uint32 MostDetailedMip,
	Uint32 MipLevels)
{
	return EngineGlobals::RenderingAPI->CreateShaderResourceView(
		Texture,
		Format,
		MostDetailedMip,
		MipLevels);
}

inline ShaderResourceView* CreateShaderResourceView(
	const Texture2DArray* Texture,
	EFormat Format,
	Uint32 MostDetailedMip,
	Uint32 MipLevels,
	Uint32 FirstArraySlice,
	Uint32 ArraySize)
{
	return EngineGlobals::RenderingAPI->CreateShaderResourceView(
		Texture,
		Format,
		MostDetailedMip,
		MipLevels,
		FirstArraySlice,
		ArraySize);
}

inline ShaderResourceView* CreateShaderResourceView(
	const TextureCube* Texture,
	EFormat Format,
	Uint32 MostDetailedMip,
	Uint32 MipLevels)
{
	return EngineGlobals::RenderingAPI->CreateShaderResourceView(
		Texture,
		Format,
		MostDetailedMip,
		MipLevels);
}

inline ShaderResourceView* CreateShaderResourceView(
	const TextureCubeArray* Texture,
	EFormat Format,
	Uint32 MostDetailedMip,
	Uint32 MipLevels,
	Uint32 FirstArraySlice,
	Uint32 ArraySize)
{
	return EngineGlobals::RenderingAPI->CreateShaderResourceView(
		Texture,
		Format,
		MostDetailedMip,
		MipLevels,
		FirstArraySlice,
		ArraySize);
}

inline ShaderResourceView* CreateShaderResourceView(
	const Texture3D* Texture,
	EFormat Format,
	Uint32 MostDetailedMip,
	Uint32 MipLevels)
{
	return EngineGlobals::RenderingAPI->CreateShaderResourceView(
		Texture,
		Format,
		MostDetailedMip,
		MipLevels);
}

// UnorderedAccessView
inline UnorderedAccessView* CreateUnorderedAccessView(
	const Buffer* Buffer,
	Uint64 FirstElement,
	Uint32 NumElements,
	EFormat Format,
	Uint64 CounterOffsetInBytes)
{
	return EngineGlobals::RenderingAPI->CreateUnorderedAccessView(
		Buffer,
		FirstElement,
		NumElements,
		Format,
		CounterOffsetInBytes);
}

inline UnorderedAccessView* CreateUnorderedAccessView(
	const Buffer* Buffer,
	Uint64 FirstElement,
	Uint32 NumElements,
	Uint32 StructureByteStride,
	Uint64 CounterOffsetInBytes)
{
	return EngineGlobals::RenderingAPI->CreateUnorderedAccessView(
		Buffer,
		FirstElement,
		NumElements,
		StructureByteStride,
		CounterOffsetInBytes);
}

inline UnorderedAccessView* CreateUnorderedAccessView(const Texture1D* Texture, EFormat Format, Uint32 MipSlice)
{
	return EngineGlobals::RenderingAPI->CreateUnorderedAccessView(Texture, Format, MipSlice);
}

inline UnorderedAccessView* CreateUnorderedAccessView(
	const Texture1DArray* Texture,
	EFormat Format,
	Uint32 MipSlice,
	Uint32 FirstArraySlice,
	Uint32 ArraySize)
{
	return EngineGlobals::RenderingAPI->CreateUnorderedAccessView(
		Texture,
		Format,
		MipSlice,
		FirstArraySlice,
		ArraySize);
}

inline UnorderedAccessView* CreateUnorderedAccessView(const Texture2D* Texture, EFormat Format, Uint32 MipSlice)
{
	return EngineGlobals::RenderingAPI->CreateUnorderedAccessView(Texture, Format, MipSlice);
}

inline UnorderedAccessView* CreateUnorderedAccessView(
	const Texture2DArray* Texture,
	EFormat Format,
	Uint32 MipSlice,
	Uint32 FirstArraySlice,
	Uint32 ArraySize)
{
	return EngineGlobals::RenderingAPI->CreateUnorderedAccessView(
		Texture,
		Format,
		MipSlice,
		FirstArraySlice,
		ArraySize);
}

inline UnorderedAccessView* CreateUnorderedAccessView(const TextureCube* Texture, EFormat Format, Uint32 MipSlice)
{
	return EngineGlobals::RenderingAPI->CreateUnorderedAccessView(
		Texture,
		Format,
		MipSlice);
}

inline UnorderedAccessView* CreateUnorderedAccessView(
	const TextureCubeArray* Texture,
	EFormat Format,
	Uint32 MipSlice,
	Uint32 ArraySlice)
{
	return EngineGlobals::RenderingAPI->CreateUnorderedAccessView(
		Texture,
		Format,
		MipSlice,
		ArraySlice);
}

inline UnorderedAccessView* CreateUnorderedAccessView(
	const Texture3D* Texture,
	EFormat Format,
	Uint32 MipSlice,
	Uint32 FirstDepthSlice,
	Uint32 DepthSlices) 
{
	return EngineGlobals::RenderingAPI->CreateUnorderedAccessView(
		Texture,
		Format,
		MipSlice,
		FirstDepthSlice,
		DepthSlices);
}

// RenderTargetView
inline RenderTargetView* CreateRenderTargetView(const Texture1D* Texture, EFormat Format, Uint32 MipSlice)
{
	return EngineGlobals::RenderingAPI->CreateRenderTargetView(Texture, Format, MipSlice);
}

inline RenderTargetView* CreateRenderTargetView(
	const Texture1DArray* Texture,
	EFormat Format,
	Uint32 MipSlice,
	Uint32 FirstArraySlice,
	Uint32 ArraySize)
{
	return EngineGlobals::RenderingAPI->CreateRenderTargetView(
		Texture,
		Format,
		MipSlice,
		FirstArraySlice,
		ArraySize);
}

inline RenderTargetView* CreateRenderTargetView(const Texture2D* Texture, EFormat Format, Uint32 MipSlice)
{
	return EngineGlobals::RenderingAPI->CreateRenderTargetView(
		Texture,
		Format,
		MipSlice);
}

inline RenderTargetView* CreateRenderTargetView(
	const Texture2DArray* Texture,
	EFormat Format,
	Uint32 MipSlice,
	Uint32 FirstArraySlice,
	Uint32 ArraySize)
{
	return EngineGlobals::RenderingAPI->CreateRenderTargetView(
		Texture,
		Format,
		MipSlice,
		FirstArraySlice,
		ArraySize);
}

inline RenderTargetView* CreateRenderTargetView(
	const TextureCube* Texture,
	EFormat Format,
	Uint32 MipSlice,
	Uint32 FirstFace,
	Uint32 FaceCount)
{
	return EngineGlobals::RenderingAPI->CreateRenderTargetView(
		Texture,
		Format,
		MipSlice,
		FirstFace,
		FaceCount);
}

inline RenderTargetView* CreateRenderTargetView(
	const TextureCubeArray* Texture,
	EFormat Format,
	Uint32 MipSlice,
	Uint32 ArraySlice,
	Uint32 FirstFace,
	Uint32 FaceCount)
{
	return EngineGlobals::RenderingAPI->CreateRenderTargetView(
		Texture,
		Format,
		MipSlice,
		ArraySlice,
		FirstFace,
		FaceCount);
}

inline RenderTargetView* CreateRenderTargetView(
	const Texture3D* Texture,
	EFormat Format,
	Uint32 MipSlice,
	Uint32 FirstDepthSlice,
	Uint32 DepthSlices) 
{
	return EngineGlobals::RenderingAPI->CreateRenderTargetView(
		Texture,
		Format,
		MipSlice,
		FirstDepthSlice,
		DepthSlices);
}

// DepthStencilView
inline DepthStencilView* CreateDepthStencilView(const Texture1D* Texture, EFormat Format, Uint32 MipSlice)
{
	return EngineGlobals::RenderingAPI->CreateDepthStencilView(Texture, Format, MipSlice);
}

inline DepthStencilView* CreateDepthStencilView(
	const Texture1DArray* Texture,
	EFormat Format,
	Uint32 MipSlice,
	Uint32 FirstArraySlice,
	Uint32 ArraySize)
{
	return EngineGlobals::RenderingAPI->CreateDepthStencilView(
		Texture,
		Format,
		MipSlice,
		FirstArraySlice,
		ArraySize);
}

inline DepthStencilView* CreateDepthStencilView(const Texture2D* Texture, EFormat Format, Uint32 MipSlice)
{
	return EngineGlobals::RenderingAPI->CreateDepthStencilView(Texture, Format, MipSlice);
}

inline DepthStencilView* CreateDepthStencilView(
	const Texture2DArray* Texture,
	EFormat Format,
	Uint32 MipSlice,
	Uint32 FirstArraySlice,
	Uint32 ArraySize)
{
	return EngineGlobals::RenderingAPI->CreateDepthStencilView(
		Texture,
		Format,
		MipSlice,
		FirstArraySlice,
		ArraySize);
}

inline DepthStencilView* CreateDepthStencilView(
	const TextureCube* Texture,
	EFormat Format,
	Uint32 MipSlice,
	Uint32 FirstFace,
	Uint32 FaceCount)
{
	return EngineGlobals::RenderingAPI->CreateDepthStencilView(
		Texture,
		Format,
		MipSlice,
		FirstFace,
		FaceCount);
}

inline DepthStencilView* CreateDepthStencilView(
	const TextureCubeArray* Texture,
	EFormat Format,
	Uint32 MipSlice,
	Uint32 ArraySlice,
	Uint32 FirstFace,
	Uint32 FaceCount)
{
	return EngineGlobals::RenderingAPI->CreateDepthStencilView(
		Texture,
		Format,
		MipSlice,
		ArraySlice,
		FirstFace,
		FaceCount);
}

/*
* Getters
*/

inline CommandListExecutor& GetCommandListExecutor()
{
	VALIDATE(EngineGlobals::CmdListExecutor != nullptr);
	return *EngineGlobals::CmdListExecutor;
}