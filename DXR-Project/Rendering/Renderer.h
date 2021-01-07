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

#include "Application/Events/EventQueue.h"
#include "Application/Events/WindowEvent.h"

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

class Renderer : public IEventHandler
{
public:
	Renderer();
	~Renderer();
	
	void Tick(const Scene& CurrentScene);
	
	bool OnEvent(const Event& Event);

	void SetPrePassEnable(bool Enabled);
	void SetVerticalSyncEnable(bool Enabled);
	void SetDrawAABBsEnable(bool Enabled);
	void SetFrustumCullEnable(bool Enabled);
	void SetFXAAEnable(bool Enabled);
	void SetSSAOEnable(bool Enabled);
	
	FORCEINLINE void SetSSAORadius(Float InSSAORadius)
	{
		SSAORadius = InSSAORadius;
	}

	FORCEINLINE void SetSSAOKernelSize(Int32 InSSAOKernelSize)
	{
		SSAOKernelSize = InSSAOKernelSize;
	}

	FORCEINLINE void SetSSAOBias(Float InSSAOBias)
	{
		SSAOBias = InSSAOBias;
	}

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

	FORCEINLINE Float GetSSAORadius() const
	{
		return SSAORadius;
	}

	FORCEINLINE Int32 GetSSAOKernelSize() const
	{
		return SSAOKernelSize;
	}

	FORCEINLINE Float GetSSAOBias() const
	{
		return SSAOBias;
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

	void GenerateIrradianceMap(
		ShaderResourceView* SourceSRV,
		TextureCube* Source,
		UnorderedAccessView* DestUAV,
		TextureCube* Dest,
		CommandList& InCmdList);

	void GenerateSpecularIrradianceMap(
		ShaderResourceView* SourceSRV,
		TextureCube* Source,
		UnorderedAccessView* const* DestUAVs,
		UInt32 NumDestUAVs,
		TextureCube* Dest,
		CommandList& InCmdList);

	void WaitForPendingFrames();

	void TraceRays(Texture2D* BackBuffer, CommandList& InCmdList);

private:
	CommandList CmdList;

	MeshData SkyboxMesh;

	TSharedRef<ConstantBuffer> CameraBuffer;
	TSharedRef<ConstantBuffer> PointLightBuffer;
	TSharedRef<ConstantBuffer> DirectionalLightBuffer;
	TSharedRef<ConstantBuffer> PerShadowMapBuffer;

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
	TArray<UnorderedAccessView*> WeakSpecularIrradianceMapUAVs;

	TSharedRef<TextureCube> Skybox;
	TSharedRef<ShaderResourceView> SkyboxSRV;

	TSharedRef<TextureCube> PointLightShadowMaps;
	TArray<TSharedRef<DepthStencilView>> PointLightShadowMapsDSVs;
	TSharedRef<ShaderResourceView> PointLightShadowMapsSRV;
	
	TSharedRef<ShaderResourceView>	DirLightShadowMapSRV;
	TSharedRef<DepthStencilView>	DirLightShadowMapDSV;
	TSharedRef<Texture2D> DirLightShadowMaps;

	TSharedRef<ShaderResourceView>	VSMDirLightShadowMapSRV;
	TSharedRef<RenderTargetView>	VSMDirLightShadowMapRTV;
	TSharedRef<Texture2D> VSMDirLightShadowMaps;
	
	TSharedRef<Texture2D> ReflectionTexture;
	TSharedRef<ShaderResourceView>	ReflectionTextureSRV;
	TSharedRef<UnorderedAccessView>	ReflectionTextureUAV;

	TSharedRef<Texture2D> IntegrationLUT;
	TSharedRef<ShaderResourceView>	IntegrationLUTSRV;

	TSharedRef<Texture2D>			FinalTarget;
	TSharedRef<ShaderResourceView>	FinalTargetSRV;
	TSharedRef<RenderTargetView>	FinalTargetRTV;

	TSharedRef<Texture2D>			GBuffer[4];
	TSharedRef<ShaderResourceView>	GBufferSRVs[4];
	TSharedRef<RenderTargetView>	GBufferRTVs[3];
	TSharedRef<DepthStencilView>	GBufferDSV;
	
	TSharedRef<InputLayoutState> StdInputLayout;

	TSharedRef<GraphicsPipelineState> PrePassPSO;
	TSharedRef<GraphicsPipelineState> ShadowMapPSO;
	TSharedRef<GraphicsPipelineState> VSMShadowMapPSO;
	TSharedRef<GraphicsPipelineState> LinearShadowMapPSO;
	TSharedPtr<GraphicsPipelineState> ForwardPSO;
	TSharedRef<GraphicsPipelineState> GeometryPSO;
	TSharedRef<GraphicsPipelineState> LightPassPSO;
	TSharedRef<GraphicsPipelineState> SkyboxPSO;
	TSharedRef<GraphicsPipelineState> DebugBoxPSO;
	TSharedRef<GraphicsPipelineState> PostPSO;
	TSharedRef<GraphicsPipelineState> FXAAPSO;

	TSharedRef<RayTracingPipelineState>	RaytracingPSO;

	TSharedRef<ComputeShader> IrradianceGenShader;
	TSharedRef<ComputeShader> SpecIrradianceGenShader;
	
	TSharedRef<ComputePipelineState> IrradicanceGenPSO;
	TSharedRef<ComputePipelineState> SpecIrradicanceGenPSO;
	TSharedPtr<ComputePipelineState> SSAOPSO;
	TSharedPtr<ComputePipelineState> SSAOBlur;

	TSharedRef<StructuredBuffer>	SSAOSamples;
	TSharedRef<ShaderResourceView>	SSAOSamplesSRV;
	TSharedRef<Texture2D>			SSAOBuffer;
	TSharedRef<ShaderResourceView>	SSAOBufferSRV;
	TSharedRef<UnorderedAccessView>	SSAOBufferUAV;
	TSharedRef<Texture2D>			SSAONoiseTex;
	TSharedRef<ShaderResourceView>	SSAONoiseSRV;

	TSharedPtr<RayTracingScene> RayTracingScene;
	TArray<RayTracingGeometryInstance> RayTracingGeometryInstances;

	TArray<MeshDrawCommand> DeferredVisibleCommands;
	TArray<MeshDrawCommand> ForwardVisibleCommands;

	Bool PrePassEnabled		= true;
	Bool DrawAABBs			= false;
	Bool VSyncEnabled		= false;
	Bool FrustumCullEnabled	= true;
	Bool FXAAEnabled		= true;
	Bool RayTracingEnabled	= false;

	Bool SSAOEnabled	= true;
	Float SSAORadius	= 0.3f;
	Float SSAOBias		= 0.0f;
	Int32 SSAOKernelSize = 64;

	static LightSettings		GlobalLightSettings;
	static TUniquePtr<Renderer> RendererInstance;
};