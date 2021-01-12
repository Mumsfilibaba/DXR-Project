#pragma once
#include "GenericRenderingAPI.h"

/*
* RenderingAPI
*/

class RenderingAPI
{
public:

	/*
	* Creation
	*/

	static Bool Init(ERenderingAPI InRenderAPI);
	static void Release();

	/*
	* Textures
	*/

	FORCEINLINE static Texture1D* CreateTexture1D(
		const ResourceData* InitalData, 
		EFormat Format, 
		UInt32 Usage, 
		UInt32 Width, 
		UInt32 MipLevels, 
		const ClearValue& OptimizedClearValue = ClearValue())
	{
		return GlobalRenderingAPI->CreateTexture1D(
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
		UInt32 Usage,
		UInt32 Width,
		UInt32 MipLevels,
		UInt32 ArrayCount,
		const ClearValue& OptimizedClearValue = ClearValue())
	{
		return GlobalRenderingAPI->CreateTexture1DArray(
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
		UInt32 Usage, 
		UInt32 Width, 
		UInt32 Height, 
		UInt32 MipLevels, 
		UInt32 SampleCount, 
		const ClearValue& OptimizedClearValue = ClearValue())
	{
		return GlobalRenderingAPI->CreateTexture2D(
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
		UInt32 Usage, 
		UInt32 Width, 
		UInt32 Height, 
		UInt32 MipLevels, 
		UInt32 ArrayCount, 
		UInt32 SampleCount, 
		const ClearValue& OptimizedClearValue = ClearValue())
	{
		return GlobalRenderingAPI->CreateTexture2DArray(
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
		UInt32 Usage,
		UInt32 Size,
		UInt32 MipLevels,
		UInt32 SampleCount,
		const ClearValue& OptimizedClearValue = ClearValue())
	{
		return GlobalRenderingAPI->CreateTextureCube(
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
		UInt32 Usage,
		UInt32 Size,
		UInt32 MipLevels,
		UInt32 ArrayCount,
		UInt32 SampleCount,
		const ClearValue& OptimizedClearValue = ClearValue())
	{
		return GlobalRenderingAPI->CreateTextureCubeArray(
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
		UInt32 Usage,
		UInt32 Width,
		UInt32 Height,
		UInt32 Depth,
		UInt32 MipLevels,
		const ClearValue& OptimizedClearValue = ClearValue())
	{
		return GlobalRenderingAPI->CreateTexture3D(
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
	* Samplers
	*/

	FORCEINLINE static class SamplerState* CreateSamplerState(const struct SamplerStateCreateInfo& CreateInfo)
	{
		return GlobalRenderingAPI->CreateSamplerState(CreateInfo);
	}

	/*
	* Buffers
	*/

	FORCEINLINE static VertexBuffer* CreateVertexBuffer(
		const ResourceData* InitalData,
		UInt32 SizeInBytes,
		UInt32 VertexStride,
		UInt32 Usage)
	{
		return GlobalRenderingAPI->CreateVertexBuffer(
			InitalData,
			SizeInBytes,
			VertexStride,
			Usage);
	}

	template<typename T>
	FORCEINLINE static VertexBuffer* CreateVertexBuffer(
		const ResourceData* InitalData, 
		UInt32 VertexCount, 
		UInt32 Usage)
	{
		constexpr UInt32 STRIDE = sizeof(T);
		const UInt32 SizeInByte = STRIDE * VertexCount;
		return CreateVertexBuffer(InitalData, SizeInByte, STRIDE, Usage);
	}

	FORCEINLINE static IndexBuffer* CreateIndexBuffer(
		const ResourceData* InitalData,
		UInt32 SizeInBytes,
		EIndexFormat IndexFormat,
		UInt32 Usage)
	{
		return GlobalRenderingAPI->CreateIndexBuffer(
			InitalData,
			SizeInBytes,
			IndexFormat,
			Usage);
	}

	FORCEINLINE static ConstantBuffer* CreateConstantBuffer(
		const ResourceData* InitalData, 
		UInt32 SizeInBytes, 
		UInt32 Usage)
	{
		return GlobalRenderingAPI->CreateConstantBuffer(InitalData, SizeInBytes, Usage);
	}

	template<typename T>
	FORCEINLINE static ConstantBuffer* CreateConstantBuffer(
		const ResourceData* InitalData, 
		UInt32 Usage)
	{
		return CreateConstantBuffer(InitalData, sizeof(T), Usage);
	}

	template<typename T>
	FORCEINLINE static ConstantBuffer* CreateConstantBuffer(
		const ResourceData* InitalData, 
		UInt32 ElementCount, 
		UInt32 Usage)
	{
		return CreateConstantBuffer(InitalData, sizeof(T) * ElementCount, Usage);
	}

	FORCEINLINE static StructuredBuffer* CreateStructuredBuffer(
		const ResourceData* InitalData,
		UInt32 SizeInBytes,
		UInt32 Stride,
		UInt32 Usage)
	{
		return GlobalRenderingAPI->CreateStructuredBuffer(
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
		return GlobalRenderingAPI->CreateRayTracingScene();
	}

	/*
	* RayTracingGeometry
	*/

	FORCEINLINE static RayTracingGeometry* CreateRayTracingGeometry()
	{
		return GlobalRenderingAPI->CreateRayTracingGeometry();
	}

	/*
	* ShaderResourceView
	*/

	FORCEINLINE static ShaderResourceView* CreateShaderResourceView(
		const Buffer* Buffer,
		UInt32 FirstElement,
		UInt32 ElementCount)
	{
		return GlobalRenderingAPI->CreateShaderResourceView(
			Buffer,
			FirstElement,
			ElementCount);
	}

	FORCEINLINE static ShaderResourceView* CreateShaderResourceView(
		const Buffer* Buffer,
		UInt32 FirstElement,
		UInt32 ElementCount,
		UInt32 Stride)
	{
		return GlobalRenderingAPI->CreateShaderResourceView(
			Buffer,
			FirstElement,
			ElementCount,
			Stride);
	}

	template<typename T>
	FORCEINLINE static ShaderResourceView* CreateShaderResourceView(
		const Buffer* Buffer, 
		UInt32 FirstElement, 
		UInt32 ElementCount)
	{
		return CreateShaderResourceView(Buffer, FirstElement, ElementCount, sizeof(T));
	}

	FORCEINLINE static ShaderResourceView* CreateShaderResourceView(
		const Texture1D* Texture,
		EFormat Format,
		UInt32 MostDetailedMip,
		UInt32 MipLevels) 
	{ 
		return GlobalRenderingAPI->CreateShaderResourceView(
			Texture,
			Format,
			MostDetailedMip,
			MipLevels);
	}

	FORCEINLINE static ShaderResourceView* CreateShaderResourceView(
		const Texture1DArray* Texture,
		EFormat Format,
		UInt32 MostDetailedMip,
		UInt32 MipLevels,
		UInt32 FirstArraySlice,
		UInt32 ArraySize)
	{
		return GlobalRenderingAPI->CreateShaderResourceView(
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
		UInt32 MostDetailedMip,
		UInt32 MipLevels)
	{
		return GlobalRenderingAPI->CreateShaderResourceView(
			Texture,
			Format,
			MostDetailedMip,
			MipLevels);
	}

	FORCEINLINE static ShaderResourceView* CreateShaderResourceView(
		const Texture2DArray* Texture,
		EFormat Format,
		UInt32 MostDetailedMip,
		UInt32 MipLevels,
		UInt32 FirstArraySlice,
		UInt32 ArraySize)
	{
		return GlobalRenderingAPI->CreateShaderResourceView(
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
		UInt32 MostDetailedMip,
		UInt32 MipLevels)
	{
		return GlobalRenderingAPI->CreateShaderResourceView(
			Texture,
			Format,
			MostDetailedMip,
			MipLevels);
	}

	FORCEINLINE static ShaderResourceView* CreateShaderResourceView(
		const TextureCubeArray* Texture,
		EFormat Format,
		UInt32 MostDetailedMip,
		UInt32 MipLevels,
		UInt32 FirstArraySlice,
		UInt32 ArraySize)
	{
		return GlobalRenderingAPI->CreateShaderResourceView(
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
		UInt32 MostDetailedMip,
		UInt32 MipLevels)
	{
		return GlobalRenderingAPI->CreateShaderResourceView(
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
		UInt64 FirstElement,
		UInt32 NumElements,
		EFormat Format,
		UInt64 CounterOffsetInBytes)
	{
		return GlobalRenderingAPI->CreateUnorderedAccessView(
			Buffer,
			FirstElement,
			NumElements,
			Format,
			CounterOffsetInBytes);
	}

	FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(
		const Buffer* Buffer,
		UInt64 FirstElement,
		UInt32 NumElements,
		UInt32 StructureByteStride,
		UInt64 CounterOffsetInBytes)
	{
		return GlobalRenderingAPI->CreateUnorderedAccessView(
			Buffer,
			FirstElement,
			NumElements,
			StructureByteStride,
			CounterOffsetInBytes);
	}

	FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(
		const Texture1D* Texture, 
		EFormat Format, 
		UInt32 MipSlice)
	{
		return GlobalRenderingAPI->CreateUnorderedAccessView(Texture, Format, MipSlice);
	}

	FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(
		const Texture1DArray* Texture,
		EFormat Format,
		UInt32 MipSlice,
		UInt32 FirstArraySlice,
		UInt32 ArraySize)
	{
		return GlobalRenderingAPI->CreateUnorderedAccessView(
			Texture,
			Format,
			MipSlice,
			FirstArraySlice,
			ArraySize);
	}

	FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(
		const Texture2D* Texture, 
		EFormat Format, 
		UInt32 MipSlice)
	{
		return GlobalRenderingAPI->CreateUnorderedAccessView(Texture, Format, MipSlice);
	}

	FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(
		const Texture2DArray* Texture,
		EFormat Format,
		UInt32 MipSlice,
		UInt32 FirstArraySlice,
		UInt32 ArraySize)
	{
		return GlobalRenderingAPI->CreateUnorderedAccessView(
			Texture,
			Format,
			MipSlice,
			FirstArraySlice,
			ArraySize);
	}

	FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(
		const TextureCube* Texture, 
		EFormat Format, 
		UInt32 MipSlice)
	{
		return GlobalRenderingAPI->CreateUnorderedAccessView(
			Texture,
			Format,
			MipSlice);
	}

	FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(
		const TextureCubeArray* Texture,
		EFormat Format,
		UInt32 MipSlice,
		UInt32 ArraySlice)
	{
		return GlobalRenderingAPI->CreateUnorderedAccessView(
			Texture,
			Format,
			MipSlice,
			ArraySlice);
	}

	FORCEINLINE static UnorderedAccessView* CreateUnorderedAccessView(
		const Texture3D* Texture,
		EFormat Format,
		UInt32 MipSlice,
		UInt32 FirstDepthSlice,
		UInt32 DepthSlices) 
	{
		return GlobalRenderingAPI->CreateUnorderedAccessView(
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
		UInt32 MipSlice)
	{
		return GlobalRenderingAPI->CreateRenderTargetView(Texture, Format, MipSlice);
	}

	FORCEINLINE static RenderTargetView* CreateRenderTargetView(
		const Texture1DArray* Texture,
		EFormat Format,
		UInt32 MipSlice,
		UInt32 FirstArraySlice,
		UInt32 ArraySize)
	{
		return GlobalRenderingAPI->CreateRenderTargetView(
			Texture,
			Format,
			MipSlice,
			FirstArraySlice,
			ArraySize);
	}

	FORCEINLINE static RenderTargetView* CreateRenderTargetView(
		const Texture2D* Texture, 
		EFormat Format, 
		UInt32 MipSlice)
	{
		return GlobalRenderingAPI->CreateRenderTargetView(
			Texture,
			Format,
			MipSlice);
	}

	FORCEINLINE static RenderTargetView* CreateRenderTargetView(
		const Texture2DArray* Texture,
		EFormat Format,
		UInt32 MipSlice,
		UInt32 FirstArraySlice,
		UInt32 ArraySize)
	{
		return GlobalRenderingAPI->CreateRenderTargetView(
			Texture,
			Format,
			MipSlice,
			FirstArraySlice,
			ArraySize);
	}

	FORCEINLINE static RenderTargetView* CreateRenderTargetView(
		const TextureCube* Texture,
		EFormat Format,
		UInt32 MipSlice,
		UInt32 FaceIndex)
	{
		return GlobalRenderingAPI->CreateRenderTargetView(
			Texture,
			Format,
			MipSlice,
			FaceIndex);
	}

	FORCEINLINE static RenderTargetView* CreateRenderTargetView(
		const TextureCubeArray* Texture,
		EFormat Format,
		UInt32 MipSlice,
		UInt32 ArraySlice,
		UInt32 FaceIndex)
	{
		return GlobalRenderingAPI->CreateRenderTargetView(
			Texture,
			Format,
			MipSlice,
			ArraySlice,
			FaceIndex);
	}

	FORCEINLINE static RenderTargetView* CreateRenderTargetView(
		const Texture3D* Texture,
		EFormat Format,
		UInt32 MipSlice,
		UInt32 FirstDepthSlice,
		UInt32 DepthSlices) 
	{
		return GlobalRenderingAPI->CreateRenderTargetView(
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
		UInt32 MipSlice)
	{
		return GlobalRenderingAPI->CreateDepthStencilView(Texture, Format, MipSlice);
	}

	FORCEINLINE static DepthStencilView* CreateDepthStencilView(
		const Texture1DArray* Texture,
		EFormat Format,
		UInt32 MipSlice,
		UInt32 FirstArraySlice,
		UInt32 ArraySize)
	{
		return GlobalRenderingAPI->CreateDepthStencilView(
			Texture,
			Format,
			MipSlice,
			FirstArraySlice,
			ArraySize);
	}

	FORCEINLINE static DepthStencilView* CreateDepthStencilView(
		const Texture2D* Texture, 
		EFormat Format, 
		UInt32 MipSlice)
	{
		return GlobalRenderingAPI->CreateDepthStencilView(Texture, Format, MipSlice);
	}

	FORCEINLINE static DepthStencilView* CreateDepthStencilView(
		const Texture2DArray* Texture,
		EFormat Format,
		UInt32 MipSlice,
		UInt32 FirstArraySlice,
		UInt32 ArraySize)
	{
		return GlobalRenderingAPI->CreateDepthStencilView(
			Texture,
			Format,
			MipSlice,
			FirstArraySlice,
			ArraySize);
	}

	FORCEINLINE static DepthStencilView* CreateDepthStencilView(
		const TextureCube* Texture,
		EFormat Format,
		UInt32 MipSlice,
		UInt32 FaceIndex)
	{
		return GlobalRenderingAPI->CreateDepthStencilView(
			Texture,
			Format,
			MipSlice,
			FaceIndex);
	}

	FORCEINLINE static DepthStencilView* CreateDepthStencilView(
		const TextureCubeArray* Texture,
		EFormat Format,
		UInt32 MipSlice,
		UInt32 ArraySlice,
		UInt32 FaceIndex)
	{
		return GlobalRenderingAPI->CreateDepthStencilView(
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
		const TArray<UInt8>& ShaderCode)
	{
		return GlobalRenderingAPI->CreateComputeShader(ShaderCode);
	}

	FORCEINLINE static VertexShader* CreateVertexShader(
		const TArray<UInt8>& ShaderCode)
	{
		return GlobalRenderingAPI->CreateVertexShader(ShaderCode);
	}
	
	FORCEINLINE static HullShader* CreateHullShader(
		const TArray<UInt8>& ShaderCode)
	{
		return GlobalRenderingAPI->CreateHullShader(ShaderCode);
	}
	
	FORCEINLINE static DomainShader* CreateDomainShader(
		const TArray<UInt8>& ShaderCode)
	{
		return GlobalRenderingAPI->CreateDomainShader(ShaderCode);
	}
	
	FORCEINLINE static GeometryShader* CreateGeometryShader(
		const TArray<UInt8>& ShaderCode)
	{
		return GlobalRenderingAPI->CreateGeometryShader(ShaderCode);
	}

	FORCEINLINE static MeshShader* CreateMeshShader(
		const TArray<UInt8>& ShaderCode)
	{
		return GlobalRenderingAPI->CreateMeshShader(ShaderCode);
	}
	
	FORCEINLINE static AmplificationShader* CreateAmplificationShader(
		const TArray<UInt8>& ShaderCode)
	{
		return GlobalRenderingAPI->CreateAmplificationShader(ShaderCode);
	}

	FORCEINLINE static PixelShader* CreatePixelShader(
		const TArray<UInt8>& ShaderCode)
	{
		return GlobalRenderingAPI->CreatePixelShader(ShaderCode);
	}

	FORCEINLINE static RayGenShader* CreateRayGenShader(
		const TArray<UInt8>& ShaderCode)
	{
		return GlobalRenderingAPI->CreateRayGenShader(ShaderCode);
	}
	
	FORCEINLINE static RayHitShader* CreateRayHitShader(
		const TArray<UInt8>& ShaderCode)
	{
		return GlobalRenderingAPI->CreateRayHitShader(ShaderCode);
	}

	FORCEINLINE static RayMissShader* CreateRayMissShader(
		const TArray<UInt8>& ShaderCode)
	{
		return GlobalRenderingAPI->CreateRayMissShader(ShaderCode);
	}

	FORCEINLINE static InputLayoutState* CreateInputLayout(
		const InputLayoutStateCreateInfo& CreateInfo)
	{
		return GlobalRenderingAPI->CreateInputLayout(CreateInfo);
	}

	FORCEINLINE static DepthStencilState* CreateDepthStencilState(
		const DepthStencilStateCreateInfo& CreateInfo)
	{
		return GlobalRenderingAPI->CreateDepthStencilState(CreateInfo);
	}

	FORCEINLINE static RasterizerState* CreateRasterizerState(
		const RasterizerStateCreateInfo& CreateInfo)
	{
		return GlobalRenderingAPI->CreateRasterizerState(CreateInfo);
	}

	FORCEINLINE static BlendState* CreateBlendState(
		const BlendStateCreateInfo& CreateInfo)
	{
		return GlobalRenderingAPI->CreateBlendState(CreateInfo);
	}


	FORCEINLINE static ComputePipelineState* CreateComputePipelineState(
		const ComputePipelineStateCreateInfo& CreateInfo)
	{
		return GlobalRenderingAPI->CreateComputePipelineState(CreateInfo);
	}

	FORCEINLINE static GraphicsPipelineState* CreateGraphicsPipelineState(
		const GraphicsPipelineStateCreateInfo& CreateInfo)
	{
		return GlobalRenderingAPI->CreateGraphicsPipelineState(CreateInfo);
	}

	/*
	* Viewport
	*/

	FORCEINLINE static class Viewport* CreateViewport(
		GenericWindow* Window,
		UInt32 Width,
		UInt32 Height,
		EFormat ColorFormat,
		EFormat DepthFormat)
	{
		return GlobalRenderingAPI->CreateViewport(
			Window, 
			Width,
			Height,
			ColorFormat, 
			DepthFormat);
	}

	/*
	* Support Features
	*/

	FORCEINLINE static Bool IsRayTracingSupported()
	{
		return GlobalRenderingAPI->IsRayTracingSupported();
	}

	FORCEINLINE static Bool UAVSupportsFormat(EFormat Format)
	{
		return GlobalRenderingAPI->UAVSupportsFormat(Format);
	}

	/*
	* Getters
	*/

	FORCEINLINE static class ICommandContext* GetDefaultCommandContext()
	{
		return GlobalRenderingAPI->GetDefaultCommandContext();
	}

	FORCEINLINE static ERenderingAPI GetAPI()
	{
		return GlobalRenderingAPI->GetAPI();
	}

	FORCEINLINE static std::string GetAdapterName()
	{
		return GlobalRenderingAPI->GetAdapterName();
	}
};