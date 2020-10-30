#pragma once
#include "GenericRenderingAPI.h"

#include "Engine/EngineGlobals.h"

/*
* RenderingAPI
*/

class RenderingAPI
{
public:
	/*
	* Creation
	*/

	static bool Initialize(ERenderingAPI InRenderAPI, TSharedRef<GenericWindow> RenderingWindow);

	/*
	* Textures
	*/

	inline static Texture1D* CreateTexture1D(
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

	inline static Texture1DArray* CreateTexture1DArray(
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

	inline static Texture2D* CreateTexture2D(
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

	inline static Texture2DArray* CreateTexture2DArray(
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

	inline static TextureCube* CreateTextureCube(
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

	inline static TextureCubeArray* CreateTextureCubeArray(
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

	inline static Texture3D* CreateTexture3D(
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

	/*
	* Buffers
	*/

	inline static VertexBuffer* CreateVertexBuffer(
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
	inline static VertexBuffer* CreateVertexBuffer(
		const ResourceData* InitalData, 
		Uint32 VertexCount, 
		Uint32 Usage)
	{
		constexpr Uint32 STRIDE = sizeof(T);
		const Uint32 SizeInByte = STRIDE * VertexCount;
		return CreateVertexBuffer(InitalData, SizeInByte, STRIDE, Usage);
	}

	inline static IndexBuffer* CreateIndexBuffer(
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

	inline static ConstantBuffer* CreateConstantBuffer(
		const ResourceData* InitalData, 
		Uint32 SizeInBytes, 
		Uint32 Usage)
	{
		return EngineGlobals::RenderingAPI->CreateConstantBuffer(InitalData, SizeInBytes, Usage);
	}

	template<typename T>
	inline static ConstantBuffer* CreateConstantBuffer(
		const ResourceData* InitalData, 
		Uint32 Usage)
	{
		return CreateConstantBuffer(InitalData, sizeof(T), Usage);
	}

	template<typename T>
	inline static ConstantBuffer* CreateConstantBuffer(
		const ResourceData* InitalData, 
		Uint32 ElementCount, 
		Uint32 Usage)
	{
		return CreateConstantBuffer(InitalData, sizeof(T) * ElementCount, Usage);
	}

	inline static StructuredBuffer* CreateStructuredBuffer(
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
	* RayTracingScene
	*/

	inline static RayTracingScene* CreateRayTracingScene()
	{
		return EngineGlobals::RenderingAPI->CreateRayTracingScene();
	}

	/*
	* RayTracingGeometry
	*/

	inline static RayTracingGeometry* CreateRayTracingGeometry()
	{
		return EngineGlobals::RenderingAPI->CreateRayTracingGeometry();
	}

	/*
	* ShaderResourceView
	*/

	inline static ShaderResourceView* CreateShaderResourceView(
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

	inline static ShaderResourceView* CreateShaderResourceView(
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
	inline static ShaderResourceView* CreateShaderResourceView(
		const Buffer* Buffer, 
		Uint32 FirstElement, 
		Uint32 ElementCount)
	{
		return CreateShaderResourceView(Buffer, FirstElement, ElementCount, sizeof(T));
	}

	inline static ShaderResourceView* CreateShaderResourceView(
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

	inline static ShaderResourceView* CreateShaderResourceView(
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

	inline static ShaderResourceView* CreateShaderResourceView(
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

	inline static ShaderResourceView* CreateShaderResourceView(
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

	inline static ShaderResourceView* CreateShaderResourceView(
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

	inline static ShaderResourceView* CreateShaderResourceView(
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

	inline static ShaderResourceView* CreateShaderResourceView(
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

	/*
	* UnorderedAccessView
	*/

	inline static UnorderedAccessView* CreateUnorderedAccessView(
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

	inline static UnorderedAccessView* CreateUnorderedAccessView(
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

	inline static UnorderedAccessView* CreateUnorderedAccessView(
		const Texture1D* Texture, 
		EFormat Format, 
		Uint32 MipSlice)
	{
		return EngineGlobals::RenderingAPI->CreateUnorderedAccessView(Texture, Format, MipSlice);
	}

	inline static UnorderedAccessView* CreateUnorderedAccessView(
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

	inline static UnorderedAccessView* CreateUnorderedAccessView(
		const Texture2D* Texture, 
		EFormat Format, 
		Uint32 MipSlice)
	{
		return EngineGlobals::RenderingAPI->CreateUnorderedAccessView(Texture, Format, MipSlice);
	}

	inline static UnorderedAccessView* CreateUnorderedAccessView(
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

	inline static UnorderedAccessView* CreateUnorderedAccessView(
		const TextureCube* Texture, 
		EFormat Format, 
		Uint32 MipSlice)
	{
		return EngineGlobals::RenderingAPI->CreateUnorderedAccessView(
			Texture,
			Format,
			MipSlice);
	}

	inline static UnorderedAccessView* CreateUnorderedAccessView(
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

	inline static UnorderedAccessView* CreateUnorderedAccessView(
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

	/*
	* RenderTargetView
	*/

	inline static RenderTargetView* CreateRenderTargetView(
		const Texture1D* Texture, 
		EFormat Format, 
		Uint32 MipSlice)
	{
		return EngineGlobals::RenderingAPI->CreateRenderTargetView(Texture, Format, MipSlice);
	}

	inline static RenderTargetView* CreateRenderTargetView(
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

	inline static RenderTargetView* CreateRenderTargetView(
		const Texture2D* Texture, 
		EFormat Format, 
		Uint32 MipSlice)
	{
		return EngineGlobals::RenderingAPI->CreateRenderTargetView(
			Texture,
			Format,
			MipSlice);
	}

	inline static RenderTargetView* CreateRenderTargetView(
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

	inline static RenderTargetView* CreateRenderTargetView(
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

	inline static RenderTargetView* CreateRenderTargetView(
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

	inline static RenderTargetView* CreateRenderTargetView(
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

	/*
	* DepthStencilView
	*/

	inline static DepthStencilView* CreateDepthStencilView(
		const Texture1D* Texture, 
		EFormat Format, 
		Uint32 MipSlice)
	{
		return EngineGlobals::RenderingAPI->CreateDepthStencilView(Texture, Format, MipSlice);
	}

	inline static DepthStencilView* CreateDepthStencilView(
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

	inline static DepthStencilView* CreateDepthStencilView(
		const Texture2D* Texture, 
		EFormat Format, 
		Uint32 MipSlice)
	{
		return EngineGlobals::RenderingAPI->CreateDepthStencilView(Texture, Format, MipSlice);
	}

	inline static DepthStencilView* CreateDepthStencilView(
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

	inline static DepthStencilView* CreateDepthStencilView(
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

	inline static DepthStencilView* CreateDepthStencilView(
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

	inline static CommandListExecutor& GetCommandListExecutor()
	{
		VALIDATE(EngineGlobals::CmdListExecutor != nullptr);
		return *EngineGlobals::CmdListExecutor;
	}
};