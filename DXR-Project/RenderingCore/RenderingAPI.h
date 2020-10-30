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

	FORCEINLINE static Texture1D* CreateTexture1D(
		const ResourceData* InitalData, 
		EFormat Format, 
		Uint32 Usage, 
		Uint32 Width, 
		Uint32 MipLevels, 
		const ClearValue& OptimizedClearValue = ClearValue())
	{
		return ActiveRenderingAPI->CreateTexture1D(
			InitalData, 
			Format, 
			Usage, 
			Width, 
			MipLevels, 
			OptimizedClearValue);
	}

	FORCEINLINE static Texture1DArray* CreateTexture1DArray(
		const ResourceData* InitalData,
		EFormat Format,
		Uint32 Usage,
		Uint32 Width,
		Uint32 MipLevels,
		Uint32 ArrayCount,
		const ClearValue& OptimizedClearValue = ClearValue())
	{
		return ActiveRenderingAPI->CreateTexture1DArray(
			InitalData,
			Format,
			Usage,
			Width,
			MipLevels,
			ArrayCount,
			OptimizedClearValue);
	}

	FORCEINLINE static Texture2D* CreateTexture2D(
		const ResourceData* InitalData, 
		EFormat Format, 
		Uint32 Usage, 
		Uint32 Width, 
		Uint32 Height, 
		Uint32 MipLevels, 
		Uint32 SampleCount, 
		const ClearValue& OptimizedClearValue = ClearValue())
	{
		return ActiveRenderingAPI->CreateTexture2D(
			InitalData, 
			Format, 
			Usage, 
			Width, 
			Height, 
			MipLevels, 
			SampleCount, 
			OptimizedClearValue);
	}

	FORCEINLINE static Texture2DArray* CreateTexture2DArray(
		const ResourceData* InitalData, 
		EFormat Format, 
		Uint32 Usage, 
		Uint32 Width, 
		Uint32 Height, 
		Uint32 MipLevels, 
		Uint32 ArrayCount, 
		Uint32 SampleCount, 
		const ClearValue& OptimizedClearValue = ClearValue())
	{
		return ActiveRenderingAPI->CreateTexture2DArray(
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

	FORCEINLINE static TextureCube* CreateTextureCube(
		const ResourceData* InitalData,
		EFormat Format,
		Uint32 Usage,
		Uint32 Size,
		Uint32 MipLevels,
		Uint32 SampleCount,
		const ClearValue& OptimizedClearValue = ClearValue())
	{
		return ActiveRenderingAPI->CreateTextureCube(
			InitalData, 
			Format,
			Usage,
			Size,
			MipLevels,
			SampleCount,
			OptimizedClearValue);
	}

	FORCEINLINE static TextureCubeArray* CreateTextureCubeArray(
		const ResourceData* InitalData,
		EFormat Format,
		Uint32 Usage,
		Uint32 Size,
		Uint32 MipLevels,
		Uint32 ArrayCount,
		Uint32 SampleCount,
		const ClearValue& OptimizedClearValue = ClearValue())
	{
		return ActiveRenderingAPI->CreateTextureCubeArray(
			InitalData,
			Format,
			Usage,
			Size,
			MipLevels,
			ArrayCount,
			SampleCount,
			OptimizedClearValue);
	}

	FORCEINLINE static Texture3D* CreateTexture3D(
		const ResourceData* InitalData,
		EFormat Format,
		Uint32 Usage,
		Uint32 Width,
		Uint32 Height,
		Uint32 Depth,
		Uint32 MipLevels,
		const ClearValue& OptimizedClearValue = ClearValue())
	{
		return ActiveRenderingAPI->CreateTexture3D(
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

	FORCEINLINE static VertexBuffer* CreateVertexBuffer(
		const ResourceData* InitalData,
		Uint32 SizeInBytes,
		Uint32 VertexStride,
		Uint32 Usage)
	{
		return ActiveRenderingAPI->CreateVertexBuffer(
			InitalData,
			SizeInBytes,
			VertexStride,
			Usage);
	}

	template<typename T>
	FORCEINLINE static VertexBuffer* CreateVertexBuffer(
		const ResourceData* InitalData, 
		Uint32 VertexCount, 
		Uint32 Usage)
	{
		constexpr Uint32 STRIDE = sizeof(T);
		const Uint32 SizeInByte = STRIDE * VertexCount;
		return CreateVertexBuffer(InitalData, SizeInByte, STRIDE, Usage);
	}

	FORCEINLINE static IndexBuffer* CreateIndexBuffer(
		const ResourceData* InitalData,
		Uint32 SizeInBytes,
		EIndexFormat IndexFormat,
		Uint32 Usage)
	{
		return ActiveRenderingAPI->CreateIndexBuffer(
			InitalData,
			SizeInBytes,
			IndexFormat,
			Usage);
	}

	FORCEINLINE static ConstantBuffer* CreateConstantBuffer(
		const ResourceData* InitalData, 
		Uint32 SizeInBytes, 
		Uint32 Usage)
	{
		return ActiveRenderingAPI->CreateConstantBuffer(InitalData, SizeInBytes, Usage);
	}

	template<typename T>
	FORCEINLINE static ConstantBuffer* CreateConstantBuffer(
		const ResourceData* InitalData, 
		Uint32 Usage)
	{
		return CreateConstantBuffer(InitalData, sizeof(T), Usage);
	}

	template<typename T>
	FORCEINLINE static ConstantBuffer* CreateConstantBuffer(
		const ResourceData* InitalData, 
		Uint32 ElementCount, 
		Uint32 Usage)
	{
		return CreateConstantBuffer(InitalData, sizeof(T) * ElementCount, Usage);
	}

	FORCEINLINE static StructuredBuffer* CreateStructuredBuffer(
		const ResourceData* InitalData,
		Uint32 SizeInBytes,
		Uint32 Stride,
		Uint32 Usage)
	{
		return ActiveRenderingAPI->CreateStructuredBuffer(
			InitalData,
			SizeInBytes,
			Stride,
			Usage);
	}

	/*
	* RayTracingScene
	*/

	FORCEINLINE static RayTracingScene* CreateRayTracingScene()
	{
		return ActiveRenderingAPI->CreateRayTracingScene();
	}

	/*
	* RayTracingGeometry
	*/

	FORCEINLINE static RayTracingGeometry* CreateRayTracingGeometry()
	{
		return ActiveRenderingAPI->CreateRayTracingGeometry();
	}

	/*
	* ShaderResourceView
	*/

	FORCEINLINE static ShaderResourceView* CreateShaderResourceView(
		const Buffer* Buffer,
		Uint32 FirstElement,
		Uint32 ElementCount,
		EFormat Format)
	{
		return ActiveRenderingAPI->CreateShaderResourceView(
			Buffer,
			FirstElement,
			ElementCount,
			Format);
	}

	FORCEINLINE static ShaderResourceView* CreateShaderResourceView(
		const Buffer* Buffer,
		Uint32 FirstElement,
		Uint32 ElementCount,
		Uint32 Stride)
	{
		return ActiveRenderingAPI->CreateShaderResourceView(
			Buffer,
			FirstElement,
			ElementCount,
			Stride);
	}

	template<typename T>
	FORCEINLINE static ShaderResourceView* CreateShaderResourceView(
		const Buffer* Buffer, 
		Uint32 FirstElement, 
		Uint32 ElementCount)
	{
		return CreateShaderResourceView(Buffer, FirstElement, ElementCount, sizeof(T));
	}

	FORCEINLINE static ShaderResourceView* CreateShaderResourceView(
		const Texture1D* Texture,
		EFormat Format,
		Uint32 MostDetailedMip,
		Uint32 MipLevels) 
	{ 
		return ActiveRenderingAPI->CreateShaderResourceView(
			Texture,
			Format,
			MostDetailedMip,
			MipLevels);
	}

	FORCEINLINE static ShaderResourceView* CreateShaderResourceView(
		const Texture1DArray* Texture,
		EFormat Format,
		Uint32 MostDetailedMip,
		Uint32 MipLevels,
		Uint32 FirstArraySlice,
		Uint32 ArraySize)
	{
		return ActiveRenderingAPI->CreateShaderResourceView(
			Texture,
			Format,
			MostDetailedMip,
			MipLevels,
			FirstArraySlice,
			ArraySize);
	}

	FORCEINLINE static ShaderResourceView* CreateShaderResourceView(
		const Texture2D* Texture,
		EFormat Format,
		Uint32 MostDetailedMip,
		Uint32 MipLevels)
	{
		return ActiveRenderingAPI->CreateShaderResourceView(
			Texture,
			Format,
			MostDetailedMip,
			MipLevels);
	}

	FORCEINLINE static ShaderResourceView* CreateShaderResourceView(
		const Texture2DArray* Texture,
		EFormat Format,
		Uint32 MostDetailedMip,
		Uint32 MipLevels,
		Uint32 FirstArraySlice,
		Uint32 ArraySize)
	{
		return ActiveRenderingAPI->CreateShaderResourceView(
			Texture,
			Format,
			MostDetailedMip,
			MipLevels,
			FirstArraySlice,
			ArraySize);
	}

	FORCEINLINE static ShaderResourceView* CreateShaderResourceView(
		const TextureCube* Texture,
		EFormat Format,
		Uint32 MostDetailedMip,
		Uint32 MipLevels)
	{
		return ActiveRenderingAPI->CreateShaderResourceView(
			Texture,
			Format,
			MostDetailedMip,
			MipLevels);
	}

	FORCEINLINE static ShaderResourceView* CreateShaderResourceView(
		const TextureCubeArray* Texture,
		EFormat Format,
		Uint32 MostDetailedMip,
		Uint32 MipLevels,
		Uint32 FirstArraySlice,
		Uint32 ArraySize)
	{
		return ActiveRenderingAPI->CreateShaderResourceView(
			Texture,
			Format,
			MostDetailedMip,
			MipLevels,
			FirstArraySlice,
			ArraySize);
	}

	FORCEINLINE static ShaderResourceView* CreateShaderResourceView(
		const Texture3D* Texture,
		EFormat Format,
		Uint32 MostDetailedMip,
		Uint32 MipLevels)
	{
		return ActiveRenderingAPI->CreateShaderResourceView(
			Texture,
			Format,
			MostDetailedMip,
			MipLevels);
	}

	/*
	* UnorderedAccessView
	*/

	FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(
		const Buffer* Buffer,
		Uint64 FirstElement,
		Uint32 NumElements,
		EFormat Format,
		Uint64 CounterOffsetInBytes)
	{
		return ActiveRenderingAPI->CreateUnorderedAccessView(
			Buffer,
			FirstElement,
			NumElements,
			Format,
			CounterOffsetInBytes);
	}

	FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(
		const Buffer* Buffer,
		Uint64 FirstElement,
		Uint32 NumElements,
		Uint32 StructureByteStride,
		Uint64 CounterOffsetInBytes)
	{
		return ActiveRenderingAPI->CreateUnorderedAccessView(
			Buffer,
			FirstElement,
			NumElements,
			StructureByteStride,
			CounterOffsetInBytes);
	}

	FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(
		const Texture1D* Texture, 
		EFormat Format, 
		Uint32 MipSlice)
	{
		return ActiveRenderingAPI->CreateUnorderedAccessView(Texture, Format, MipSlice);
	}

	FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(
		const Texture1DArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstArraySlice,
		Uint32 ArraySize)
	{
		return ActiveRenderingAPI->CreateUnorderedAccessView(
			Texture,
			Format,
			MipSlice,
			FirstArraySlice,
			ArraySize);
	}

	FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(
		const Texture2D* Texture, 
		EFormat Format, 
		Uint32 MipSlice)
	{
		return ActiveRenderingAPI->CreateUnorderedAccessView(Texture, Format, MipSlice);
	}

	FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(
		const Texture2DArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstArraySlice,
		Uint32 ArraySize)
	{
		return ActiveRenderingAPI->CreateUnorderedAccessView(
			Texture,
			Format,
			MipSlice,
			FirstArraySlice,
			ArraySize);
	}

	FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(
		const TextureCube* Texture, 
		EFormat Format, 
		Uint32 MipSlice)
	{
		return ActiveRenderingAPI->CreateUnorderedAccessView(
			Texture,
			Format,
			MipSlice);
	}

	FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(
		const TextureCubeArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 ArraySlice)
	{
		return ActiveRenderingAPI->CreateUnorderedAccessView(
			Texture,
			Format,
			MipSlice,
			ArraySlice);
	}

	FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(
		const Texture3D* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstDepthSlice,
		Uint32 DepthSlices) 
	{
		return ActiveRenderingAPI->CreateUnorderedAccessView(
			Texture,
			Format,
			MipSlice,
			FirstDepthSlice,
			DepthSlices);
	}

	/*
	* RenderTargetView
	*/

	FORCEINLINE static RenderTargetView* CreateRenderTargetView(
		const Texture1D* Texture, 
		EFormat Format, 
		Uint32 MipSlice)
	{
		return ActiveRenderingAPI->CreateRenderTargetView(Texture, Format, MipSlice);
	}

	FORCEINLINE static RenderTargetView* CreateRenderTargetView(
		const Texture1DArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstArraySlice,
		Uint32 ArraySize)
	{
		return ActiveRenderingAPI->CreateRenderTargetView(
			Texture,
			Format,
			MipSlice,
			FirstArraySlice,
			ArraySize);
	}

	FORCEINLINE static RenderTargetView* CreateRenderTargetView(
		const Texture2D* Texture, 
		EFormat Format, 
		Uint32 MipSlice)
	{
		return ActiveRenderingAPI->CreateRenderTargetView(
			Texture,
			Format,
			MipSlice);
	}

	FORCEINLINE static RenderTargetView* CreateRenderTargetView(
		const Texture2DArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstArraySlice,
		Uint32 ArraySize)
	{
		return ActiveRenderingAPI->CreateRenderTargetView(
			Texture,
			Format,
			MipSlice,
			FirstArraySlice,
			ArraySize);
	}

	FORCEINLINE static RenderTargetView* CreateRenderTargetView(
		const TextureCube* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstFace,
		Uint32 FaceCount)
	{
		return ActiveRenderingAPI->CreateRenderTargetView(
			Texture,
			Format,
			MipSlice,
			FirstFace,
			FaceCount);
	}

	FORCEINLINE static RenderTargetView* CreateRenderTargetView(
		const TextureCubeArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 ArraySlice,
		Uint32 FirstFace,
		Uint32 FaceCount)
	{
		return ActiveRenderingAPI->CreateRenderTargetView(
			Texture,
			Format,
			MipSlice,
			ArraySlice,
			FirstFace,
			FaceCount);
	}

	FORCEINLINE static RenderTargetView* CreateRenderTargetView(
		const Texture3D* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstDepthSlice,
		Uint32 DepthSlices) 
	{
		return ActiveRenderingAPI->CreateRenderTargetView(
			Texture,
			Format,
			MipSlice,
			FirstDepthSlice,
			DepthSlices);
	}

	/*
	* DepthStencilView
	*/

	FORCEINLINE static DepthStencilView* CreateDepthStencilView(
		const Texture1D* Texture, 
		EFormat Format, 
		Uint32 MipSlice)
	{
		return ActiveRenderingAPI->CreateDepthStencilView(Texture, Format, MipSlice);
	}

	FORCEINLINE static DepthStencilView* CreateDepthStencilView(
		const Texture1DArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstArraySlice,
		Uint32 ArraySize)
	{
		return ActiveRenderingAPI->CreateDepthStencilView(
			Texture,
			Format,
			MipSlice,
			FirstArraySlice,
			ArraySize);
	}

	FORCEINLINE static DepthStencilView* CreateDepthStencilView(
		const Texture2D* Texture, 
		EFormat Format, 
		Uint32 MipSlice)
	{
		return ActiveRenderingAPI->CreateDepthStencilView(Texture, Format, MipSlice);
	}

	FORCEINLINE static DepthStencilView* CreateDepthStencilView(
		const Texture2DArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstArraySlice,
		Uint32 ArraySize)
	{
		return ActiveRenderingAPI->CreateDepthStencilView(
			Texture,
			Format,
			MipSlice,
			FirstArraySlice,
			ArraySize);
	}

	FORCEINLINE static DepthStencilView* CreateDepthStencilView(
		const TextureCube* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstFace,
		Uint32 FaceCount)
	{
		return ActiveRenderingAPI->CreateDepthStencilView(
			Texture,
			Format,
			MipSlice,
			FirstFace,
			FaceCount);
	}

	FORCEINLINE static DepthStencilView* CreateDepthStencilView(
		const TextureCubeArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 ArraySlice,
		Uint32 FirstFace,
		Uint32 FaceCount)
	{
		return ActiveRenderingAPI->CreateDepthStencilView(
			Texture,
			Format,
			MipSlice,
			ArraySlice,
			FirstFace,
			FaceCount);
	}

	/*
	* Support Features
	*/

	FORCEINLINE static bool IsRayTracingSupported()
	{
		return ActiveRenderingAPI->IsRayTracingSupported();
	}

	FORCEINLINE static bool UAVSupportsFormat(EFormat Format)
	{
		return ActiveRenderingAPI->UAVSupportsFormat(Format);
	}

	/*
	* Getters
	*/

	FORCEINLINE static CommandListExecutor& GetCommandListExecutor()
	{
		return CmdExecutor;
	}

	FORCEINLINE static class ICommandContext* GetDefaultCommandContext()
	{
		return ActiveRenderingAPI->GetDefaultCommandContext();
	}

	FORCEINLINE static TSharedPtr<GenericRenderingAPI> GetActiveRenderingAPI()
	{
		return ActiveRenderingAPI;
	}

	FORCEINLINE static ERenderingAPI GetAPI()
	{
		return ActiveRenderingAPI->GetAPI();
	}

	FORCEINLINE static std::string GetAdapterName()
	{
		return ActiveRenderingAPI->GetAdapterName();
	}

private:
	static CommandListExecutor CmdExecutor;
	static TSharedPtr<GenericRenderingAPI> ActiveRenderingAPI;
};