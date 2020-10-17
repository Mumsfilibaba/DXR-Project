#pragma once
#include "D3D12/D3D12Device.h"
#include "D3D12/D3D12CommandQueue.h"
#include "D3D12/D3D12CommandAllocator.h"
#include "D3D12/D3D12CommandList.h"
#include "D3D12/D3D12DescriptorHeap.h"
#include "D3D12/D3D12Fence.h"
#include "D3D12/D3D12SwapChain.h"
#include "D3D12/D3D12Buffer.h"
#include "D3D12/D3D12RayTracingScene.h"
#include "D3D12/D3D12ImmediateCommandList.h"

#include "Windows/WindowsWindow.h"

#include "Time/Clock.h"

#include "Application/InputCodes.h"

#include "Scene/Actor.h"
#include "Scene/Scene.h"
#include "Scene/Camera.h"

#include "Mesh.h"
#include "Material.h"
#include "MeshFactory.h"

#include "RenderingCore/RenderingAPI.h"

class D3D12GraphicsPipelineState;
class D3D12RayTracingPipelineState;

#define ENABLE_D3D12_DEBUG	0
#define ENABLE_VSM			0

/*
* LightSettings
*/

struct LightSettings
{
	Uint16 ShadowMapWidth		= 4096;
	Uint16 ShadowMapHeight		= 4096;
	Uint16 PointLightShadowSize	= 1024;
};

/*
* Renderer
*/

class Renderer
{
public:
	Renderer();
	~Renderer();
	
	void Tick(const Scene& CurrentScene);
	
	void OnResize(Int32 Width, Int32 Height);

	void SetPrePassEnable(bool Enabled);
	void SetVerticalSyncEnable(bool Enabled);
	void SetDrawAABBsEnable(bool Enabled);
	void SetFrustumCullEnable(bool Enabled);
	void SetFXAAEnable(bool Enabled);

	FORCEINLINE bool IsDrawAABBsEnabled() const
	{
		return DrawAABBs;
	}

	FORCEINLINE bool IsFXAAEnabled() const
	{
		return FXAAEnabled;
	}

	FORCEINLINE bool IsPrePassEnabled() const
	{
		return PrePassEnabled;
	}

	FORCEINLINE bool IsVerticalSyncEnabled() const
	{
		return VSyncEnabled;
	}

	FORCEINLINE bool IsFrustumCullEnabled() const
	{
		return FrustumCullEnabled;
	}

	static void SetGlobalLightSettings(const LightSettings& InGlobalLightSettings);

	static FORCEINLINE const LightSettings& GetGlobalLightSettings()
	{
		return GlobalLightSettings;
	}

	static Renderer* Make();
	static Renderer* Get();

	static void Release();
	
private:
	bool Initialize();

	bool InitRayTracing();
	bool InitLightBuffers();
	bool InitPrePass();
	bool InitShadowMapPass();
	bool InitDeferred();
	bool InitGBuffer();
	bool InitIntegrationLUT();
	bool InitRayTracingTexture();
	bool InitDebugStates();
	bool InitAA();

	bool CreateShadowMaps();
	void WriteShadowMapDescriptors();

	void GenerateIrradianceMap(TextureCube* Source, TextureCube* Dest, D3D12CommandList* CommandList);
	void GenerateSpecularIrradianceMap(TextureCube* Source, TextureCube* Dest, D3D12CommandList* CommandList);

	void WaitForPendingFrames();

	void TraceRays(Texture2D* BackBuffer, D3D12CommandList* CommandList);

private:
	TSharedPtr<D3D12CommandList>	CommandList;
	TSharedPtr<D3D12Fence>			Fence;

	TArray<TSharedPtr<D3D12CommandAllocator>>	CommandAllocators;
	TArray<TSharedPtr<D3D12Resource>>			DeferredResources;

	MeshData Sphere;
	MeshData SkyboxMesh;
	MeshData Cube;

	TSharedRef<ConstantBuffer> CameraBuffer;
	TSharedRef<ConstantBuffer> PointLightBuffer;
	TSharedRef<ConstantBuffer> DirectionalLightBuffer;

	TSharedRef<VertexBuffer> MeshVertexBuffer;
	TSharedRef<ShaderResourceView> MeshVertexBufferSRV;
	TSharedRef<VertexBuffer> CubeVertexBuffer;
	TSharedRef<ShaderResourceView> CubeVertexBufferSRV;

	TSharedRef<IndexBuffer> MeshIndexBuffer;
	TSharedRef<ShaderResourceView> MeshIndexBufferSRV;
	TSharedRef<IndexBuffer> CubeIndexBuffer;
	TSharedRef<ShaderResourceView> CubeIndexBufferSRV;

	TSharedRef<VertexBuffer> SkyboxVertexBuffer;
	TSharedRef<VertexBuffer> AABBVertexBuffer;
	TSharedRef<IndexBuffer> SkyboxIndexBuffer;
	TSharedRef<IndexBuffer> AABBIndexBuffer;

	TSharedRef<TextureCube>			IrradianceMap;
	TSharedRef<UnorderedAccessView>	IrradianceMapUAV;
	TSharedRef<ShaderResourceView>	IrradianceMapSRV;

	TSharedRef<TextureCube>			SpecularIrradianceMap;
	TSharedRef<ShaderResourceView>	SpecularIrradianceMapSRV;
	TArray<TSharedRef<UnorderedAccessView>>	SpecularIrradianceMapUAVs;

	TSharedRef<TextureCube> Skybox;
	TSharedRef<TextureCube> PointLightShadowMaps;
	
	TSharedRef<Texture2D> Albedo;
	TSharedRef<Texture2D> Normal;
	TSharedRef<Texture2D> DirLightShadowMaps;
	TSharedRef<Texture2D> VSMDirLightShadowMaps;
	
	TSharedRef<Texture2D> ReflectionTexture;
	TSharedRef<ShaderResourceView>	ReflectionTextureSRV;
	TSharedRef<ShaderResourceView>	ReflectionTextureUAV;

	TSharedRef<Texture2D> IntegrationLUT;
	TSharedRef<ShaderResourceView>	IntegrationLUTSRV;

	TSharedRef<Texture2D>			FinalTarget;
	TSharedRef<ShaderResourceView>	FinalTargetSRV;
	TSharedRef<RenderTargetView>	FinalTargetRTV;

	TSharedRef<Texture2D>			GBuffer[4];
	TSharedRef<ShaderResourceView>	GBufferSRVs[4];
	TSharedRef<RenderTargetView>	GBufferRTVs[3];
	TSharedRef<DepthStencilView>	GBufferDSV;
	
	TSharedPtr<D3D12RootSignature>		PrePassRootSignature;
	TSharedPtr<D3D12RootSignature>		ShadowMapRootSignature;
	TSharedPtr<D3D12RootSignature>		GeometryRootSignature;
	TSharedPtr<D3D12RootSignature>		LightRootSignature;
	TSharedPtr<D3D12RootSignature>		SkyboxRootSignature;
	TSharedPtr<D3D12RootSignature>		GlobalRootSignature;
	TSharedPtr<D3D12RootSignature>		IrradianceGenRootSignature;
	TSharedPtr<D3D12RootSignature>		SpecIrradianceGenRootSignature;
	TSharedPtr<D3D12RootSignature>		DebugRootSignature;
	TSharedPtr<D3D12RootSignature>		PostRootSignature;
	TSharedPtr<D3D12DescriptorTable>	RayGenDescriptorTable;
	TSharedPtr<D3D12DescriptorTable>	GlobalDescriptorTable;
	TSharedPtr<D3D12DescriptorTable>	GeometryDescriptorTable;
	TSharedPtr<D3D12DescriptorTable>	PrePassDescriptorTable;
	TSharedPtr<D3D12DescriptorTable>	LightDescriptorTable;
	TSharedPtr<D3D12DescriptorTable>	SkyboxDescriptorTable;
	TSharedPtr<D3D12DescriptorTable>	PostDescriptorTable;
	TSharedPtr<D3D12RayTracingScene>	RayTracingScene;

	TSharedPtr<D3D12GraphicsPipelineState>		PrePassPSO;
	TSharedPtr<D3D12GraphicsPipelineState>		ShadowMapPSO;
	TSharedPtr<D3D12GraphicsPipelineState>		VSMShadowMapPSO;
	TSharedPtr<D3D12GraphicsPipelineState>		LinearShadowMapPSO;
	TSharedPtr<D3D12GraphicsPipelineState>		GeometryPSO;
	TSharedPtr<D3D12GraphicsPipelineState>		LightPassPSO;
	TSharedPtr<D3D12GraphicsPipelineState>		SkyboxPSO;
	TSharedPtr<D3D12GraphicsPipelineState>		DebugBoxPSO;
	TSharedPtr<D3D12GraphicsPipelineState>		PostPSO;
	TSharedPtr<D3D12GraphicsPipelineState>		FXAAPSO;
	TSharedPtr<D3D12ComputePipelineState>		IrradicanceGenPSO;
	TSharedPtr<D3D12ComputePipelineState>		SpecIrradicanceGenPSO;
	TSharedPtr<D3D12RayTracingPipelineState>	RaytracingPSO;

	TArray<MeshDrawCommand> VisibleCommands;

	TArray<Uint64> FenceValues;
	Uint32 CurrentBackBufferIndex = 0;

	bool PrePassEnabled		= true;
	bool DrawAABBs			= false;
	bool VSyncEnabled		= false;
	bool FrustumCullEnabled	= true;
	bool FXAAEnabled		= true;

	static LightSettings		GlobalLightSettings;
	static TUniquePtr<Renderer> RendererInstance;
};