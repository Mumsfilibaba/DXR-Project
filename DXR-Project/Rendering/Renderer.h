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

#include "Application/Clock.h"
#include "Application/InputCodes.h"

#include "Scene/Actor.h"
#include "Scene/Scene.h"
#include "Scene/Camera.h"

#include "Mesh.h"
#include "Material.h"
#include "MeshFactory.h"

class D3D12Texture;
class D3D12GraphicsPipelineState;
class D3D12RayTracingPipelineState;

#define ENABLE_D3D12_DEBUG	1
#define ENABLE_VSM			1

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

	FORCEINLINE bool IsPrePassEnabled() const
	{
		return PrePassEnabled;
	}

	FORCEINLINE bool IsVerticalSyncEnabled() const
	{
		return VSyncEnabled;
	}

	FORCEINLINE TSharedPtr<D3D12Device> GetDevice() const
	{
		return Device;
	}

	FORCEINLINE TSharedPtr<D3D12ImmediateCommandList> GetImmediateCommandList() const
	{
		return ImmediateCommandList;
	}

	static void SetGlobalLightSettings(const LightSettings& InGlobalLightSettings);

	static FORCEINLINE const LightSettings& GetGlobalLightSettings()
	{
		return GlobalLightSettings;
	}

	static Renderer* Make(TSharedPtr<WindowsWindow> RendererWindow);
	static Renderer* Get();
	
private:
	bool Initialize(TSharedPtr<WindowsWindow> RendererWindow);

	bool InitRayTracing();
	bool InitLightBuffers();
	bool InitPrePass();
	bool InitShadowMapPass();
	bool InitDeferred();
	bool InitGBuffer();
	bool InitIntegrationLUT();
	bool InitRayTracingTexture();

	bool CreateShadowMaps();
	void WriteShadowMapDescriptors();

	void GenerateIrradianceMap(D3D12Texture* Source, D3D12Texture* Dest, D3D12CommandList* CommandList);
	void GenerateSpecularIrradianceMap(D3D12Texture* Source, D3D12Texture* Dest, D3D12CommandList* CommandList);

	void WaitForPendingFrames();

	void TraceRays(D3D12Texture* BackBuffer, D3D12CommandList* CommandList);

private:
	TSharedPtr<D3D12Device>					Device;
	TSharedPtr<D3D12ImmediateCommandList>	ImmediateCommandList;
	
	TSharedPtr<D3D12CommandQueue>	Queue;
	TSharedPtr<D3D12CommandQueue>	ComputeQueue;
	TSharedPtr<D3D12CommandList>	CommandList;
	TSharedPtr<D3D12Fence>			Fence;
	TSharedPtr<D3D12SwapChain>		SwapChain;

	TArray<TSharedPtr<D3D12CommandAllocator>> CommandAllocators;
	TArray<TSharedPtr<D3D12Resource>> DeferredResources;

	MeshData Sphere;
	MeshData SkyboxMesh;
	MeshData Cube;

	TSharedPtr<D3D12Buffer> CameraBuffer;
	TSharedPtr<D3D12Buffer> MeshVertexBuffer;
	TSharedPtr<D3D12Buffer> MeshIndexBuffer;
	TSharedPtr<D3D12Buffer> SkyboxVertexBuffer;
	TSharedPtr<D3D12Buffer> SkyboxIndexBuffer;
	TSharedPtr<D3D12Buffer> CubeVertexBuffer;
	TSharedPtr<D3D12Buffer> CubeIndexBuffer;
	TSharedPtr<D3D12Buffer> PointLightBuffer;
	TSharedPtr<D3D12Buffer> DirectionalLightBuffer;

	TSharedPtr<D3D12Texture> Skybox;
	TSharedPtr<D3D12Texture> IrradianceMap;
	TSharedPtr<D3D12Texture> SpecularIrradianceMap;
	TSharedPtr<D3D12Texture> Albedo;
	TSharedPtr<D3D12Texture> Normal;
	TSharedPtr<D3D12Texture> ReflectionTexture;
	TSharedPtr<D3D12Texture> IntegrationLUT;
	TSharedPtr<D3D12Texture> DirLightShadowMaps;
	TSharedPtr<D3D12Texture> VSMDirLightShadowMaps;
	TSharedPtr<D3D12Texture> PointLightShadowMaps;
	TSharedPtr<D3D12Texture> GBuffer[4];
	
	TSharedPtr<D3D12RootSignature>		PrePassRootSignature;
	TSharedPtr<D3D12RootSignature>		ShadowMapRootSignature;
	TSharedPtr<D3D12RootSignature>		GeometryRootSignature;
	TSharedPtr<D3D12RootSignature>		LightRootSignature;
	TSharedPtr<D3D12RootSignature>		SkyboxRootSignature;
	TSharedPtr<D3D12RootSignature>		GlobalRootSignature;
	TSharedPtr<D3D12RootSignature>		IrradianceGenRootSignature;
	TSharedPtr<D3D12RootSignature>		SpecIrradianceGenRootSignature;
	TSharedPtr<D3D12DescriptorTable>	RayGenDescriptorTable;
	TSharedPtr<D3D12DescriptorTable>	GlobalDescriptorTable;
	TSharedPtr<D3D12DescriptorTable>	GeometryDescriptorTable;
	TSharedPtr<D3D12DescriptorTable>	PrePassDescriptorTable;
	TSharedPtr<D3D12DescriptorTable>	LightDescriptorTable;
	TSharedPtr<D3D12DescriptorTable>	SkyboxDescriptorTable;
	TSharedPtr<D3D12RayTracingScene>	RayTracingScene;

	TSharedPtr<D3D12GraphicsPipelineState>		PrePassPSO;
	TSharedPtr<D3D12GraphicsPipelineState>		ShadowMapPSO;
	TSharedPtr<D3D12GraphicsPipelineState>		VSMShadowMapPSO;
	TSharedPtr<D3D12GraphicsPipelineState>		LinearShadowMapPSO;
	TSharedPtr<D3D12GraphicsPipelineState>		GeometryPSO;
	TSharedPtr<D3D12GraphicsPipelineState>		LightPassPSO;
	TSharedPtr<D3D12GraphicsPipelineState>		SkyboxPSO;
	TSharedPtr<D3D12ComputePipelineState>		IrradicanceGenPSO;
	TSharedPtr<D3D12ComputePipelineState>		SpecIrradicanceGenPSO;
	TSharedPtr<D3D12RayTracingPipelineState>	RaytracingPSO;

	TArray<Uint64> FenceValues;
	Uint32 CurrentBackBufferIndex = 0;

	bool PrePassEnabled = true;
	bool VSyncEnabled	= false;

	static LightSettings		GlobalLightSettings;
	static TUniquePtr<Renderer> RendererInstance;
};