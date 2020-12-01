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

class D3D12Texture;
class D3D12GraphicsPipelineState;
class D3D12RayTracingPipelineState;

#define ENABLE_D3D12_DEBUG	0
#define ENABLE_VSM			0

/*
* LightSettings
*/

struct LightSettings
{
	UInt16 ShadowMapWidth		= 4096;
	UInt16 ShadowMapHeight		= 4096;
	UInt16 PointLightShadowSize	= 1024;
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
	void SetSSAOEnable(bool Enabled);

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

	FORCEINLINE bool IsSSAOEnabled() const
	{
		return SSAOEnabled;
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
	bool InitForwardPass();
	bool InitSSAO();

	bool CreateShadowMaps();
	void WriteShadowMapDescriptors();

	void GenerateIrradianceMap(D3D12Texture* Source, D3D12Texture* Dest, D3D12CommandList* CommandList);
	void GenerateSpecularIrradianceMap(D3D12Texture* Source, D3D12Texture* Dest, D3D12CommandList* CommandList);

	void WaitForPendingFrames();

	void TraceRays(D3D12Texture* BackBuffer, D3D12CommandList* CommandList);

private:
	TSharedPtr<D3D12CommandList>	CommandList;
	TSharedPtr<D3D12Fence>			Fence;

	TArray<TSharedPtr<D3D12CommandAllocator>>	CommandAllocators;
	TArray<TSharedPtr<D3D12Resource>>			DeferredResources;

	MeshData SkyboxMesh;

	TSharedPtr<D3D12Buffer> CameraBuffer;
	TSharedPtr<D3D12Buffer> SkyboxVertexBuffer;
	TSharedPtr<D3D12Buffer> SkyboxIndexBuffer;
	TSharedPtr<D3D12Buffer> AABBVertexBuffer;
	TSharedPtr<D3D12Buffer> AABBIndexBuffer;
	TSharedPtr<D3D12Buffer> PointLightBuffer;
	TSharedPtr<D3D12Buffer> DirectionalLightBuffer;
	TSharedPtr<D3D12Buffer> SSAOSamples;

	TSharedPtr<D3D12Texture> Skybox;
	TSharedPtr<D3D12Texture> IrradianceMap;
	TSharedPtr<D3D12Texture> SpecularIrradianceMap;
	TSharedPtr<D3D12Texture> ReflectionTexture;
	TSharedPtr<D3D12Texture> IntegrationLUT;
	TSharedPtr<D3D12Texture> DirLightShadowMaps;
	TSharedPtr<D3D12Texture> VSMDirLightShadowMaps;
	TSharedPtr<D3D12Texture> PointLightShadowMaps;
	TSharedPtr<D3D12Texture> GBuffer[4];
	TSharedPtr<D3D12Texture> SSAOBuffer;
	TSharedPtr<D3D12Texture> SSAONoiseTex;
	TSharedPtr<D3D12Texture> FinalTarget;
	
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
	TSharedPtr<D3D12RootSignature>		ForwardRootSignature;
	TSharedPtr<D3D12RootSignature>		SSAORootSignature;

	TSharedPtr<D3D12DescriptorTable>	RayGenDescriptorTable;
	TSharedPtr<D3D12DescriptorTable>	GlobalDescriptorTable;
	TSharedPtr<D3D12DescriptorTable>	SSAODescriptorTable;
	TSharedPtr<D3D12DescriptorTable>	GeometryDescriptorTable;
	TSharedPtr<D3D12DescriptorTable>	ForwardDescriptorTable;
	TSharedPtr<D3D12DescriptorTable>	PrePassDescriptorTable;
	TSharedPtr<D3D12DescriptorTable>	LightDescriptorTable;
	TSharedPtr<D3D12DescriptorTable>	SkyboxDescriptorTable;
	TSharedPtr<D3D12DescriptorTable>	PostDescriptorTable;

	TSharedPtr<D3D12RayTracingScene>	RayTracingScene;
	TArray<D3D12RayTracingGeometryInstance> RayTracingGeometryInstances;

	TSharedPtr<D3D12GraphicsPipelineState>	PrePassPSO;
	TSharedPtr<D3D12GraphicsPipelineState>	ShadowMapPSO;
	TSharedPtr<D3D12GraphicsPipelineState>	VSMShadowMapPSO;
	TSharedPtr<D3D12GraphicsPipelineState>	LinearShadowMapPSO;
	TSharedPtr<D3D12GraphicsPipelineState>	GeometryPSO;
	TSharedPtr<D3D12GraphicsPipelineState>	ForwardPSO;
	TSharedPtr<D3D12GraphicsPipelineState>	LightPassPSO;
	TSharedPtr<D3D12GraphicsPipelineState>	SkyboxPSO;
	TSharedPtr<D3D12GraphicsPipelineState>	DebugBoxPSO;
	TSharedPtr<D3D12GraphicsPipelineState>	PostPSO;
	TSharedPtr<D3D12GraphicsPipelineState>	FXAAPSO;
	TSharedPtr<D3D12ComputePipelineState>	SSAOPSO;
	TSharedPtr<D3D12ComputePipelineState>	IrradicanceGenPSO;
	TSharedPtr<D3D12ComputePipelineState>	SpecIrradicanceGenPSO;
	TSharedPtr<D3D12RayTracingPipelineState> RaytracingPSO;

	TArray<MeshDrawCommand> DeferredVisibleCommands;
	TArray<MeshDrawCommand> ForwardVisibleCommands;

	TArray<UInt64> FenceValues;
	UInt32 CurrentBackBufferIndex = 0;

	bool PrePassEnabled		= true;
	bool DrawAABBs			= false;
	bool VSyncEnabled		= false;
	bool FrustumCullEnabled	= true;
	bool FXAAEnabled		= true;
	bool SSAOEnabled		= true;
	bool RayTracingEnabled	= false;

	static LightSettings		GlobalLightSettings;
	static TUniquePtr<Renderer> RendererInstance;
};