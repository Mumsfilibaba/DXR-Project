#pragma once
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
#include "RenderingCore/CommandList.h"

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
	CommandList CmdList;

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
	
	TSharedRef<RayTracingScene>	RayTracingScene;

	TSharedRef<GraphicsPipelineState>	PrePassPSO;
	TSharedRef<GraphicsPipelineState>	ShadowMapPSO;
	TSharedRef<GraphicsPipelineState>	VSMShadowMapPSO;
	TSharedRef<GraphicsPipelineState>	LinearShadowMapPSO;
	TSharedRef<GraphicsPipelineState>	GeometryPSO;
	TSharedRef<GraphicsPipelineState>	LightPassPSO;
	TSharedRef<GraphicsPipelineState>	SkyboxPSO;
	TSharedRef<GraphicsPipelineState>	DebugBoxPSO;
	TSharedRef<GraphicsPipelineState>	PostPSO;
	TSharedRef<GraphicsPipelineState>	FXAAPSO;
	TSharedRef<ComputePipelineState>	IrradicanceGenPSO;
	TSharedRef<ComputePipelineState>	SpecIrradicanceGenPSO;
	TSharedRef<RayTracingPipelineState>	RaytracingPSO;

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