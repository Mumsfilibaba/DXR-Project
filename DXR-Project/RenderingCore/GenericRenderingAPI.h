#pragma once
#include "Application/Generic/GenericWindow.h"

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
* GenericRenderingAPI
*/

class GenericRenderingAPI
{
public:
	inline GenericRenderingAPI(ERenderingAPI InAPI)
		: API(InAPI)
	{
	}

	virtual ~GenericRenderingAPI() = default;

	virtual bool Init(TSharedRef<GenericWindow> RenderWindow, bool EnableDebug) = 0;

	/*
	* Textures
	*/

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

	/*
	* Buffers
	*/

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

	virtual ConstantBuffer* CreateConstantBuffer(
		const ResourceData* InitalData, 
		Uint32 SizeInBytes, 
		Uint32 Usage) const = 0;

	virtual StructuredBuffer* CreateStructuredBuffer(
		const ResourceData* InitalData,
		Uint32 SizeInBytes,
		Uint32 Stride,
		Uint32 Usage) const = 0;

	/*
	* RayTracing
	*/

	virtual RayTracingScene*	CreateRayTracingScene()		const = 0;
	virtual RayTracingGeometry* CreateRayTracingGeometry()	const = 0;

	/*
	* ShaderResourceView
	*/

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

	/*
	* UnorderedAccessView
	*/

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

	virtual UnorderedAccessView* CreateUnorderedAccessView(
		const Texture1D* Texture, 
		EFormat Format, 
		Uint32 MipSlice) const = 0;

	virtual UnorderedAccessView* CreateUnorderedAccessView(
		const Texture1DArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstArraySlice,
		Uint32 ArraySize) const = 0;

	virtual UnorderedAccessView* CreateUnorderedAccessView(
		const Texture2D* Texture, 
		EFormat Format, 
		Uint32 MipSlice) const = 0;

	virtual UnorderedAccessView* CreateUnorderedAccessView(
		const Texture2DArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstArraySlice,
		Uint32 ArraySize) const = 0;

	virtual UnorderedAccessView* CreateUnorderedAccessView(
		const TextureCube* Texture, 
		EFormat Format, 
		Uint32 MipSlice) const = 0;

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

	/*
	* RenderTargetView
	*/

	virtual RenderTargetView* CreateRenderTargetView(
		const Texture1D* Texture, 
		EFormat Format, 
		Uint32 MipSlice) const = 0;

	virtual RenderTargetView* CreateRenderTargetView(
		const Texture1DArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstArraySlice,
		Uint32 ArraySize) const = 0;

	virtual RenderTargetView* CreateRenderTargetView(
		const Texture2D* Texture, 
		EFormat Format, 
		Uint32 MipSlice) const = 0;

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

	/*
	* DepthStencilView
	*/

	virtual DepthStencilView* CreateDepthStencilView(
		const Texture1D* Texture, 
		EFormat Format, 
		Uint32 MipSlice) const = 0;

	virtual DepthStencilView* CreateDepthStencilView(
		const Texture1DArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstArraySlice,
		Uint32 ArraySize) const = 0;

	virtual DepthStencilView* CreateDepthStencilView(
		const Texture2D* Texture, 
		EFormat Format, 
		Uint32 MipSlice) const = 0;

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

	/*
	* Pipeline
	*/

	virtual class ComputeShader* CreateComputeShader(
		const TArray<Uint8>& ShaderCode) const = 0;
	
	virtual class VertexShader* CreateVertexShader(
		const TArray<Uint8>& ShaderCode) const = 0;
	
	virtual class HullShader* CreateHullShader(
		const TArray<Uint8>& ShaderCode) const = 0;
	
	virtual class DomainShader* CreateDomainShader(
		const TArray<Uint8>& ShaderCode) const = 0;
	
	virtual class GeometryShader* CreateGeometryShader(
		const TArray<Uint8>& ShaderCode) const = 0;

	virtual class MeshShader* CreateMeshShader(
		const TArray<Uint8>& ShaderCode) const = 0;

	virtual class AmplificationShader* CreateAmplificationShader(
		const TArray<Uint8>& ShaderCode) const = 0;

	virtual class PixelShader* CreatePixelShader(
		const TArray<Uint8>& ShaderCode) const = 0;

	virtual class RayGenShader* CreateRayGenShader(
		const TArray<Uint8>& ShaderCode) const = 0;
	
	virtual class RayHitShader* CreateRayHitShader(
		const TArray<Uint8>& ShaderCode) const = 0;
	
	virtual class RayMissShader* CreateRayMissShader(
		const TArray<Uint8>& ShaderCode) const = 0;

	virtual class DepthStencilState*	CreateDepthStencilState()	const = 0;
	virtual class RasterizerState*		CreateRasterizerState()		const = 0;
	virtual class BlendState*	CreateBlendState()	const = 0;
	virtual class InputLayout*	CreateInputLayout()	const = 0;

	virtual class GraphicsPipelineState*	CreateGraphicsPipelineState()	const = 0;
	virtual class ComputePipelineState*		CreateComputePipelineState(const ComputePipelineStateCreateInfo& Info) const = 0;
	virtual class RayTracingPipelineState*	CreateRayTracingPipelineState() const = 0;

	/*
	* Context
	*/

	virtual class ICommandContext* GetDefaultCommandContext() const = 0;

	/*
	* Getters
	*/

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

protected:
	ERenderingAPI API;
};