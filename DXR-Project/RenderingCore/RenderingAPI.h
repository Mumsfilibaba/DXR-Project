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
		return CurrentRenderingAPI->CreateTexture1D(
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
		return CurrentRenderingAPI->CreateTexture1DArray(
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
		return CurrentRenderingAPI->CreateTexture2D(
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
		return CurrentRenderingAPI->CreateTexture2DArray(
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
		return CurrentRenderingAPI->CreateTextureCube(
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
		return CurrentRenderingAPI->CreateTextureCubeArray(
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
		return CurrentRenderingAPI->CreateTexture3D(
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
		return CurrentRenderingAPI->CreateVertexBuffer(
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
		return CurrentRenderingAPI->CreateIndexBuffer(
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
		return CurrentRenderingAPI->CreateConstantBuffer(InitalData, SizeInBytes, Usage);
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
		return CurrentRenderingAPI->CreateStructuredBuffer(
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
		return CurrentRenderingAPI->CreateRayTracingScene();
	}

	/*
	* RayTracingGeometry
	*/

	FORCEINLINE static RayTracingGeometry* CreateRayTracingGeometry()
	{
		return CurrentRenderingAPI->CreateRayTracingGeometry();
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
		return CurrentRenderingAPI->CreateShaderResourceView(
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
		return CurrentRenderingAPI->CreateShaderResourceView(
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
		return CurrentRenderingAPI->CreateShaderResourceView(
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
		return CurrentRenderingAPI->CreateShaderResourceView(
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
		return CurrentRenderingAPI->CreateShaderResourceView(
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
		return CurrentRenderingAPI->CreateShaderResourceView(
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
		return CurrentRenderingAPI->CreateShaderResourceView(
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
		return CurrentRenderingAPI->CreateShaderResourceView(
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
		return CurrentRenderingAPI->CreateShaderResourceView(
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
		return CurrentRenderingAPI->CreateUnorderedAccessView(
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
		return CurrentRenderingAPI->CreateUnorderedAccessView(
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
		return CurrentRenderingAPI->CreateUnorderedAccessView(Texture, Format, MipSlice);
	}

	FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(
		const Texture1DArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstArraySlice,
		Uint32 ArraySize)
	{
		return CurrentRenderingAPI->CreateUnorderedAccessView(
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
		return CurrentRenderingAPI->CreateUnorderedAccessView(Texture, Format, MipSlice);
	}

	FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(
		const Texture2DArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstArraySlice,
		Uint32 ArraySize)
	{
		return CurrentRenderingAPI->CreateUnorderedAccessView(
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
		return CurrentRenderingAPI->CreateUnorderedAccessView(
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
		return CurrentRenderingAPI->CreateUnorderedAccessView(
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
		return CurrentRenderingAPI->CreateUnorderedAccessView(
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
		return CurrentRenderingAPI->CreateRenderTargetView(Texture, Format, MipSlice);
	}

	FORCEINLINE static RenderTargetView* CreateRenderTargetView(
		const Texture1DArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstArraySlice,
		Uint32 ArraySize)
	{
		return CurrentRenderingAPI->CreateRenderTargetView(
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
		return CurrentRenderingAPI->CreateRenderTargetView(
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
		return CurrentRenderingAPI->CreateRenderTargetView(
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
		Uint32 FaceIndex)
	{
		return CurrentRenderingAPI->CreateRenderTargetView(
			Texture,
			Format,
			MipSlice,
			FaceIndex);
	}

	FORCEINLINE static RenderTargetView* CreateRenderTargetView(
		const TextureCubeArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 ArraySlice,
		Uint32 FaceIndex)
	{
		return CurrentRenderingAPI->CreateRenderTargetView(
			Texture,
			Format,
			MipSlice,
			ArraySlice,
			FaceIndex);
	}

	FORCEINLINE static RenderTargetView* CreateRenderTargetView(
		const Texture3D* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstDepthSlice,
		Uint32 DepthSlices) 
	{
		return CurrentRenderingAPI->CreateRenderTargetView(
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
		return CurrentRenderingAPI->CreateDepthStencilView(Texture, Format, MipSlice);
	}

	FORCEINLINE static DepthStencilView* CreateDepthStencilView(
		const Texture1DArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstArraySlice,
		Uint32 ArraySize)
	{
		return CurrentRenderingAPI->CreateDepthStencilView(
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
		return CurrentRenderingAPI->CreateDepthStencilView(Texture, Format, MipSlice);
	}

	FORCEINLINE static DepthStencilView* CreateDepthStencilView(
		const Texture2DArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 FirstArraySlice,
		Uint32 ArraySize)
	{
		return CurrentRenderingAPI->CreateDepthStencilView(
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
		Uint32 FaceIndex)
	{
		return CurrentRenderingAPI->CreateDepthStencilView(
			Texture,
			Format,
			MipSlice,
			FaceIndex);
	}

	FORCEINLINE static DepthStencilView* CreateDepthStencilView(
		const TextureCubeArray* Texture,
		EFormat Format,
		Uint32 MipSlice,
		Uint32 ArraySlice,
		Uint32 FaceIndex)
	{
		return CurrentRenderingAPI->CreateDepthStencilView(
			Texture,
			Format,
			MipSlice,
			ArraySlice,
			FaceIndex);
	}

	/*
	* Pipeline
	*/

	FORCEINLINE static ComputeShader* CreateComputeShader(
		const TArray<Uint8>& ShaderCode)
	{
		return CurrentRenderingAPI->CreateComputeShader(ShaderCode);
	}

	FORCEINLINE static VertexShader* CreateVertexShader(
		const TArray<Uint8>& ShaderCode)
	{
		return CurrentRenderingAPI->CreateVertexShader(ShaderCode);
	}
	
	FORCEINLINE static HullShader* CreateHullShader(
		const TArray<Uint8>& ShaderCode)
	{
		return CurrentRenderingAPI->CreateHullShader(ShaderCode);
	}
	
	FORCEINLINE static DomainShader* CreateDomainShader(
		const TArray<Uint8>& ShaderCode)
	{
		return CurrentRenderingAPI->CreateDomainShader(ShaderCode);
	}
	
	FORCEINLINE static GeometryShader* CreateGeometryShader(
		const TArray<Uint8>& ShaderCode)
	{
		return CurrentRenderingAPI->CreateGeometryShader(ShaderCode);
	}

	FORCEINLINE static MeshShader* CreateMeshShader(
		const TArray<Uint8>& ShaderCode)
	{
		return CurrentRenderingAPI->CreateMeshShader(ShaderCode);
	}
	
	FORCEINLINE static AmplificationShader* CreateAmplificationShader(
		const TArray<Uint8>& ShaderCode)
	{
		return CurrentRenderingAPI->CreateAmplificationShader(ShaderCode);
	}

	FORCEINLINE static PixelShader* CreatePixelShader(
		const TArray<Uint8>& ShaderCode)
	{
		return CurrentRenderingAPI->CreatePixelShader(ShaderCode);
	}

	FORCEINLINE static RayGenShader* CreateRayGenShader(
		const TArray<Uint8>& ShaderCode)
	{
		return CurrentRenderingAPI->CreateRayGenShader(ShaderCode);
	}
	
	FORCEINLINE static RayHitShader* CreateRayHitShader(
		const TArray<Uint8>& ShaderCode)
	{
		return CurrentRenderingAPI->CreateRayHitShader(ShaderCode);
	}

	FORCEINLINE static RayMissShader* CreateRayMissShader(
		const TArray<Uint8>& ShaderCode)
	{
		return CurrentRenderingAPI->CreateRayMissShader(ShaderCode);
	}

	FORCEINLINE static InputLayoutState* CreateInputLayout(
		const InputLayoutStateCreateInfo& CreateInfo)
	{
		return CurrentRenderingAPI->CreateInputLayout(CreateInfo);
	}

	FORCEINLINE static DepthStencilState* CreateDepthStencilState(
		const DepthStencilStateCreateInfo& CreateInfo)
	{
		return CurrentRenderingAPI->CreateDepthStencilState(CreateInfo);
	}

	FORCEINLINE static RasterizerState* CreateRasterizerState(
		const RasterizerStateCreateInfo& CreateInfo)
	{
		return CurrentRenderingAPI->CreateRasterizerState(CreateInfo);
	}

	FORCEINLINE static BlendState* CreateBlendState(
		const BlendStateCreateInfo& CreateInfo)
	{
		return CurrentRenderingAPI->CreateBlendState(CreateInfo);
	}


	FORCEINLINE static ComputePipelineState* CreateComputePipelineState(
		const ComputePipelineStateCreateInfo& CreateInfo)
	{
		return CurrentRenderingAPI->CreateComputePipelineState(CreateInfo);
	}

	FORCEINLINE static GraphicsPipelineState* CreateGraphicsPipelineState(
		const GraphicsPipelineStateCreateInfo& CreateInfo)
	{
		return CurrentRenderingAPI->CreateGraphicsPipelineState(CreateInfo);
	}

	/*
	* Support Features
	*/

	FORCEINLINE static bool IsRayTracingSupported()
	{
		return CurrentRenderingAPI->IsRayTracingSupported();
	}

	FORCEINLINE static bool UAVSupportsFormat(EFormat Format)
	{
		return CurrentRenderingAPI->UAVSupportsFormat(Format);
	}

	/*
	* Getters
	*/

	FORCEINLINE static class ICommandContext* GetDefaultCommandContext()
	{
		return CurrentRenderingAPI->GetDefaultCommandContext();
	}

	FORCEINLINE static TSharedPtr<GenericRenderingAPI> GetActiveRenderingAPI()
	{
		return CurrentRenderingAPI;
	}

	FORCEINLINE static ERenderingAPI GetAPI()
	{
		return CurrentRenderingAPI->GetAPI();
	}

	FORCEINLINE static std::string GetAdapterName()
	{
		return CurrentRenderingAPI->GetAdapterName();
	}

private:
	static TSharedPtr<GenericRenderingAPI> CurrentRenderingAPI;
};