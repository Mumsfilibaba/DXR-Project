#pragma once
#include "Windows/WindowsWindow.h"

#include "Time/Clock.h"

#include "Application/InputCodes.h"
#include "Application/Events/EventHandler.h"

#include "Scene/Actor.h"
#include "Scene/Scene.h"
#include "Scene/Camera.h"

#include "Mesh.h"
#include "Material.h"
#include "MeshFactory.h"

#include "RenderingCore/RenderLayer.h"
#include "RenderingCore/CommandList.h"
#include "RenderingCore/Viewport.h"

#include "DebugUI.h"

#define ENABLE_VSM 0

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
	Renderer()	= default;
	~Renderer()	= default;

	Bool Init();

	void Tick(const Scene& CurrentScene);
	
	Bool OnEvent(const Event& Event);

	void SetLightSettings(const LightSettings& InLightSettings);
	
	FORCEINLINE const LightSettings& GetLightSettings()
	{
		return CurrentLightSettings;
	}
	
private:
	Bool InitRayTracing();
	Bool InitLightBuffers();
	Bool InitPrePass();
	Bool InitShadowMapPass();
	Bool InitDeferred();
	Bool InitGBuffer();
	Bool InitIntegrationLUT();
	Bool InitRayTracingTexture();
	Bool InitDebugStates();
	Bool InitAA();
	Bool InitForwardPass();
	Bool InitSSAO();
	Bool InitSSAO_RenderTarget();

	Bool CreateShadowMaps();

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

	void TraceRays(Texture2D* BackBuffer, CommandList& InCmdList);

private:
	CommandList CmdList;

	MeshData SkyboxMesh;

	TSharedRef<ConstantBuffer> CameraBuffer;
	TSharedRef<ConstantBuffer> PointLightBuffer;
	TSharedRef<ConstantBuffer> DirectionalLightBuffer;
	TSharedRef<ConstantBuffer> PerShadowMapBuffer;

	TSharedRef<VertexBuffer>	SkyboxVertexBuffer;
	TSharedRef<VertexBuffer>	AABBVertexBuffer;
	TSharedRef<IndexBuffer>		SkyboxIndexBuffer;
	TSharedRef<IndexBuffer>		AABBIndexBuffer;

	TSharedRef<TextureCube>			IrradianceMap;
	TSharedRef<UnorderedAccessView>	IrradianceMapUAV;
	TSharedRef<ShaderResourceView>	IrradianceMapSRV;
	TSharedRef<SamplerState>		IrradianceSampler;

	TSharedRef<TextureCube>					SpecularIrradianceMap;
	TSharedRef<ShaderResourceView>			SpecularIrradianceMapSRV;
	TArray<TSharedRef<UnorderedAccessView>>	SpecularIrradianceMapUAVs;
	TArray<UnorderedAccessView*>			WeakSpecularIrradianceMapUAVs;

	TSharedRef<TextureCube>			Skybox;
	TSharedRef<ShaderResourceView>	SkyboxSRV;
	TSharedRef<SamplerState>		SkyboxSampler;

	TSharedRef<TextureCube>					PointLightShadowMaps;
	TArray<TSharedRef<DepthStencilView>>	PointLightShadowMapsDSVs;
	TSharedRef<ShaderResourceView>			PointLightShadowMapsSRV;
	TSharedRef<SamplerState>				ShadowMapSampler;
	TSharedRef<SamplerState>				ShadowMapCompSampler;

	TSharedRef<ShaderResourceView>	DirLightShadowMapSRV;
	TSharedRef<DepthStencilView>	DirLightShadowMapDSV;
	TSharedRef<Texture2D>			DirLightShadowMaps;

	TSharedRef<ShaderResourceView>	VSMDirLightShadowMapSRV;
	TSharedRef<RenderTargetView>	VSMDirLightShadowMapRTV;
	TSharedRef<Texture2D>			VSMDirLightShadowMaps;
	
	TSharedRef<Texture2D>			ReflectionTexture;
	TSharedRef<ShaderResourceView>	ReflectionTextureSRV;
	TSharedRef<UnorderedAccessView>	ReflectionTextureUAV;

	TSharedRef<Texture2D>			IntegrationLUT;
	TSharedRef<ShaderResourceView>	IntegrationLUTSRV;
	TSharedRef<SamplerState>		IntegrationLUTSampler;

	TSharedRef<Texture2D>			FinalTarget;
	TSharedRef<ShaderResourceView>	FinalTargetSRV;
	TSharedRef<RenderTargetView>	FinalTargetRTV;
	TSharedRef<UnorderedAccessView>	FinalTargetUAV;

	TSharedRef<Texture2D>			GBuffer[4];
	TSharedRef<ShaderResourceView>	GBufferSRVs[4];
	TSharedRef<RenderTargetView>	GBufferRTVs[3];
	TSharedRef<DepthStencilView>	GBufferDSV;
	TSharedRef<SamplerState>		GBufferSampler;

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
	TSharedPtr<ComputePipelineState> DeferredLightPass;

	TSharedRef<StructuredBuffer>	SSAOSamples;
	TSharedRef<ShaderResourceView>	SSAOSamplesSRV;
	TSharedRef<Texture2D>			SSAOBuffer;
	TSharedRef<ShaderResourceView>	SSAOBufferSRV;
	TSharedRef<UnorderedAccessView>	SSAOBufferUAV;
	TSharedRef<Texture2D>			SSAONoiseTex;
	TSharedRef<ShaderResourceView>	SSAONoiseSRV;

	TSharedPtr<RayTracingScene>			RayTracingScene;
	TArray<RayTracingGeometryInstance>	RayTracingGeometryInstances;

	TArray<MeshDrawCommand> DeferredVisibleCommands;
	TArray<MeshDrawCommand> ForwardVisibleCommands;

	TArray<ImGuiImage> DebugTextures;

	TSharedRef<Viewport> MainWindowViewport;

	Bool UpdatePointLight	= true;
	Bool UpdateDirLight		= true;
	UInt64 PointLightFrame	= 0;
	UInt64 DirLightFrame	= 0;

	UInt32 LastFrameNumDrawCalls		= 0;
	UInt32 LastFrameNumDispatchCalls	= 0;
	UInt32 LastFrameNumCommands			= 0;

	LightSettings CurrentLightSettings;
};