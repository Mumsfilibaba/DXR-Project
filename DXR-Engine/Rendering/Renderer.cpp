#include "Renderer.h"
#include "TextureFactory.h"
#include "DebugUI.h"
#include "Mesh.h"

#include "Scene/Frustum.h"
#include "Scene/Lights/PointLight.h"
#include "Scene/Lights/DirectionalLight.h"

#include "Application/Events/EventDispatcher.h"

#include "RenderingCore/ShaderCompiler.h"

#include "Debug/Profiler.h"
#include "Debug/Console.h"

#include <algorithm>
#include <imgui_internal.h>

/*
* Static Settings
*/

static const EFormat SSAOBufferFormat		= EFormat::Format_R32G32B32A32_Float;
static const EFormat FinalTargetFormat		= EFormat::Format_R16G16B16A16_Float;
static const EFormat RenderTargetFormat		= EFormat::Format_R8G8B8A8_Unorm;
static const EFormat AlbedoFormat			= EFormat::Format_R8G8B8A8_Unorm;
static const EFormat MaterialFormat			= EFormat::Format_R8G8B8A8_Unorm;
static const EFormat NormalFormat			= EFormat::Format_R10G10B10A2_Unorm;
static const EFormat DepthBufferFormat		= EFormat::Format_D32_Float;
static const EFormat LightProbeFormat		= EFormat::Format_R16G16B16A16_Float;
static const EFormat ShadowMapFormat		= EFormat::Format_D32_Float;
static const UInt32	 ShadowMapSampleCount	= 2;

#define GBUFFER_ALBEDO_INDEX	0
#define GBUFFER_NORMAL_INDEX	1
#define GBUFFER_MATERIAL_INDEX	2
#define GBUFFER_DEPTH_INDEX		3

DECL_CONSOLE_VARIABLE(DrawTextureDebugger);
DECL_CONSOLE_VARIABLE(DrawRendererInfo);

DECL_CONSOLE_VARIABLE(SSAORadius);
DECL_CONSOLE_VARIABLE(SSAOBias);
DECL_CONSOLE_VARIABLE(SSAOKernelSize);

/*
* CameraBufferDesc
*/

struct CameraBufferDesc
{
	XMFLOAT4X4 ViewProjection;
	XMFLOAT4X4 View;
	XMFLOAT4X4 ViewInv;
	XMFLOAT4X4 Projection;
	XMFLOAT4X4 ProjectionInv;
	XMFLOAT4X4 ViewProjectionInv;
	XMFLOAT3 Position;
	Float NearPlane;
	Float FarPlane;
	Float AspectRatio;
};

/*
* PerShadowMap
*/

struct PerShadowMap
{
	XMFLOAT4X4	Matrix;
	XMFLOAT3	Position;
	Float		FarPlane;
};

/*
* Renderer
*/

void Renderer::Tick(const Scene& CurrentScene)
{
	// Perform frustum culling
	DeferredVisibleCommands.Clear();
	ForwardVisibleCommands.Clear();
	DebugTextures.Clear();

	PointLightFrame++;
	if (PointLightFrame > 6)
	{
		UpdatePointLight	= true;
		PointLightFrame		= 0;
	}

	DirLightFrame++;
	if (DirLightFrame > 6)
	{
		UpdateDirLight	= true;
		DirLightFrame	= 0;
	}

	if (GlobalFrustumCullEnabled)
	{
		TRACE_SCOPE("Frustum Culling");

		Camera* Camera			= CurrentScene.GetCamera();
		Frustum CameraFrustum	= Frustum(Camera->GetFarPlane(), Camera->GetViewMatrix(), Camera->GetProjectionMatrix());
		for (const MeshDrawCommand& Command : CurrentScene.GetMeshDrawCommands())
		{
			const XMFLOAT4X4& Transform = Command.CurrentActor->GetTransform().GetMatrix();
			XMMATRIX XmTransform	= XMMatrixTranspose(XMLoadFloat4x4(&Transform));
			XMVECTOR XmTop			= XMVectorSetW(XMLoadFloat3(&Command.Mesh->BoundingBox.Top), 1.0f);
			XMVECTOR XmBottom		= XMVectorSetW(XMLoadFloat3(&Command.Mesh->BoundingBox.Bottom), 1.0f);
			XmTop		= XMVector4Transform(XmTop, XmTransform);
			XmBottom	= XMVector4Transform(XmBottom, XmTransform);

			AABB Box;
			XMStoreFloat3(&Box.Top, XmTop);
			XMStoreFloat3(&Box.Bottom, XmBottom);
			if (CameraFrustum.CheckAABB(Box))
			{
				if (Command.Material->HasAlphaMask())
				{
					ForwardVisibleCommands.EmplaceBack(Command);
				}
				else
				{
					DeferredVisibleCommands.EmplaceBack(Command);
				}
			}
		}
	}
	else
	{
		for (const MeshDrawCommand& Command : CurrentScene.GetMeshDrawCommands())
		{
			if (Command.Material->HasAlphaMask())
			{
				ForwardVisibleCommands.EmplaceBack(Command);
			}
			else
			{
				DeferredVisibleCommands.EmplaceBack(Command);
			}
		}
	}

	// Start frame
	Texture2D* BackBuffer				= MainWindowViewport->GetBackBuffer();
	RenderTargetView* BackBufferView	= MainWindowViewport->GetRenderTargetView();

	CmdList.Begin();
	INSERT_DEBUG_CMDLIST_MARKER(CmdList, "--BEGIN FRAME--");

	// Build acceleration structures
	//if (RenderLayer::IsRayTracingSupported() && RayTracingEnabled)
	//{
	//	// Build Bottom-Level
	//	UInt32 HitGroupIndex = 0;
	//	for (const MeshDrawCommand& Command : CurrentScene.GetMeshDrawCommands())
	//	{
	//		CmdList.BuildRayTracingGeometry(Command.Geometry);

	//		XMFLOAT4X4 Matrix		= Command.CurrentActor->GetTransform().GetMatrix();
	//		XMFLOAT3X4 SmallMatrix	= XMFLOAT3X4(reinterpret_cast<Float*>(&Matrix));

	//		//RayTracingGeometryInstances.EmplaceBack(
	//		//	Command.Mesh->RayTracingGeometry,
	//		//	Command.Material,
	//		//	SmallMatrix,
	//		//	HitGroupIndex, 0);

	//		HitGroupIndex++;
	//	}

	//	// Build Top-Level
	//	const Bool NeedsBuild = false;// RayTracingScene->NeedsBuild();
	//	if (NeedsBuild)
	//	{
	//		//TArray<BindingTableEntry> BindingTableEntries;
	//		//BindingTableEntries.Reserve(RayTracingGeometryInstances.Size());

	//		//BindingTableEntries.EmplaceBack("RayGen", RayGenDescriptorTable, nullptr);
	//		//for (D3D12RayTracingGeometryInstance& Geometry : RayTracingGeometryInstances)
	//		//{
	//		//	BindingTableEntries.EmplaceBack(
	//		//		"HitGroup", 
	//		//		Geometry.Material->GetDescriptorTable(),
	//		//		Geometry.Geometry->GetDescriptorTable());
	//		//}
	//		//BindingTableEntries.EmplaceBack("Miss", nullptr, nullptr);

	//		//const UInt32 NumHitGroups = BindingTableEntries.Size() - 2;
	//		//RayTracingScene->BuildAccelerationStructure(
	//		//	CommandList.Get(),
	//		//	RayTracingGeometryInstances,
	//		//	BindingTableEntries,
	//		//	NumHitGroups);

	//		//GlobalDescriptorTable->SetShaderResourceView(RayTracingScene->GetShaderResourceView(), 0);
	//		//GlobalDescriptorTable->CopyDescriptors();
	//	}
	//}

	// UpdateLightBuffers
	INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin Update LightBuffers");

	{
		TRACE_SCOPE("Update LightBuffers");

		CmdList.TransitionBuffer(
			PointLightBuffer.Get(), 
			EResourceState::ResourceState_VertexAndConstantBuffer, 
			EResourceState::ResourceState_CopyDest);

		CmdList.TransitionBuffer(
			DirectionalLightBuffer.Get(), 
			EResourceState::ResourceState_VertexAndConstantBuffer, 
			EResourceState::ResourceState_CopyDest);

		UInt32 NumPointLights	= 0;
		UInt32 NumDirLights		= 0;
		for (Light* Light : CurrentScene.GetLights())
		{
			XMFLOAT3	Color		= Light->GetColor();
			Float		Intensity	= Light->GetIntensity();
			if (IsSubClassOf<PointLight>(Light) && UpdatePointLight)
			{
				PointLight* PoiLight = Cast<PointLight>(Light);
				VALIDATE(PoiLight != nullptr);

				PointLightProperties Properties;
				Properties.Color			= XMFLOAT3(Color.x * Intensity, Color.y * Intensity, Color.z * Intensity);
				Properties.Position			= PoiLight->GetPosition();
				Properties.ShadowBias		= PoiLight->GetShadowBias();
				Properties.MaxShadowBias	= PoiLight->GetMaxShadowBias();
				Properties.FarPlane			= PoiLight->GetShadowFarPlane();

				constexpr UInt32 SizeInBytes = sizeof(PointLightProperties);
				CmdList.UpdateBuffer(
					PointLightBuffer.Get(), 
					NumPointLights* SizeInBytes, 
					SizeInBytes, 
					&Properties);

				NumPointLights++;
			}
			else if (IsSubClassOf<DirectionalLight>(Light) && UpdateDirLight)
			{
				DirectionalLight* DirLight = Cast<DirectionalLight>(Light);
				VALIDATE(DirLight != nullptr);

				DirectionalLightProperties Properties;
				Properties.Color			= XMFLOAT3(Color.x * Intensity, Color.y * Intensity, Color.z * Intensity);
				Properties.ShadowBias		= DirLight->GetShadowBias();
				Properties.Direction		= DirLight->GetDirection();
				Properties.LightMatrix		= DirLight->GetMatrix();
				Properties.MaxShadowBias	= DirLight->GetMaxShadowBias();

				constexpr UInt32 SizeInBytes = sizeof(DirectionalLightProperties);
				CmdList.UpdateBuffer(
					DirectionalLightBuffer.Get(),
					NumDirLights * SizeInBytes,
					SizeInBytes,
					&Properties);

				NumDirLights++;
			}
		}

		CmdList.TransitionBuffer(
			PointLightBuffer.Get(), 
			EResourceState::ResourceState_CopyDest, 
			EResourceState::ResourceState_VertexAndConstantBuffer);
	
		CmdList.TransitionBuffer(
			DirectionalLightBuffer.Get(), 
			EResourceState::ResourceState_CopyDest, 
			EResourceState::ResourceState_VertexAndConstantBuffer);
	}

	INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End Update LightBuffers");

	// Transition GBuffer
	CmdList.TransitionTexture(
		GBuffer[GBUFFER_ALBEDO_INDEX].Get(), 
		EResourceState::ResourceState_PixelShaderResource, 
		EResourceState::ResourceState_RenderTarget);

	CmdList.TransitionTexture(
		GBuffer[GBUFFER_NORMAL_INDEX].Get(), 
		EResourceState::ResourceState_PixelShaderResource, 
		EResourceState::ResourceState_RenderTarget);

	CmdList.TransitionTexture(
		GBuffer[GBUFFER_MATERIAL_INDEX].Get(), 
		EResourceState::ResourceState_PixelShaderResource, 
		EResourceState::ResourceState_RenderTarget);

	CmdList.TransitionTexture(
		GBuffer[GBUFFER_DEPTH_INDEX].Get(), 
		EResourceState::ResourceState_PixelShaderResource, 
		EResourceState::ResourceState_DepthWrite);

	// Transition ShadowMaps
	CmdList.TransitionTexture(
		PointLightShadowMaps.Get(), 
		EResourceState::ResourceState_PixelShaderResource, 
		EResourceState::ResourceState_DepthWrite);
	
	CmdList.TransitionTexture(
		DirLightShadowMaps.Get(), 
		EResourceState::ResourceState_PixelShaderResource, 
		EResourceState::ResourceState_DepthWrite);
	
	// Render DirectionalLight ShadowMaps
	INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin Render DirectionalLight ShadowMaps");
	
	if (UpdateDirLight)
	{
		TRACE_SCOPE("Render DirectionalLight ShadowMaps");

		CmdList.ClearDepthStencilView(DirLightShadowMapDSV.Get(), DepthStencilClearValue(1.0f, 0));

#if ENABLE_VSM
		CmdList.TransitionTexture(VSMDirLightShadowMaps.Get(), EResourceState::ResourceState_PixelShaderResource, EResourceState::ResourceState_RenderTarget);

		//Float32 DepthClearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
		//CmdList.ClearRenderTargetView(VSMDirLightShadowMaps->GetRenderTargetView(0).Get(), DepthClearColor);
		//
		//D3D12RenderTargetView* DirLightRTVS[] =
		//{
		//	VSMDirLightShadowMaps->GetRenderTargetView(0).Get(),
		//};
		//CmdList.BindRenderTargets(DirLightRTVS, 1, DirLightShadowMaps->GetDepthStencilView(0).Get());
		//CmdList.BindGraphicsPipelineState(VSMShadowMapPSO.Get());
#else
		CmdList.BindRenderTargets(nullptr, 0, DirLightShadowMapDSV.Get());
		CmdList.BindGraphicsPipelineState(ShadowMapPSO.Get());
#endif

		// Setup view
		CmdList.BindViewport(
			static_cast<Float>(CurrentLightSettings.ShadowMapWidth),
			static_cast<Float>(CurrentLightSettings.ShadowMapHeight),
			0.0f,
			1.0f,
			0.0f,
			0.0f);

		CmdList.BindScissorRect(
			CurrentLightSettings.ShadowMapWidth,
			CurrentLightSettings.ShadowMapHeight,
			 0, 0);

		CmdList.BindPrimitiveTopology(EPrimitiveTopology::PrimitiveTopology_TriangleList);

		// PerObject Structs
		struct ShadowPerObject
		{
			XMFLOAT4X4	Matrix;
			Float		ShadowOffset;
		} ShadowPerObjectBuffer;
	
		PerShadowMap PerShadowMapData;
		for (Light* Light : CurrentScene.GetLights())
		{
			if (IsSubClassOf<DirectionalLight>(Light))
			{
				DirectionalLight* DirLight = Cast<DirectionalLight>(Light);
				PerShadowMapData.Matrix		= DirLight->GetMatrix();
				PerShadowMapData.Position	= DirLight->GetShadowMapPosition();
				PerShadowMapData.FarPlane	= DirLight->GetShadowFarPlane();
			
				CmdList.TransitionBuffer(
					PerShadowMapBuffer.Get(),
					EResourceState::ResourceState_VertexAndConstantBuffer,
					EResourceState::ResourceState_CopyDest);

				CmdList.UpdateBuffer(
					PerShadowMapBuffer.Get(),
					0, sizeof(PerShadowMap),
					&PerShadowMapData);

				CmdList.TransitionBuffer(
					PerShadowMapBuffer.Get(),
					EResourceState::ResourceState_CopyDest,
					EResourceState::ResourceState_VertexAndConstantBuffer);

				CmdList.BindConstantBuffers(
					EShaderStage::ShaderStage_Vertex,
					PerShadowMapBuffer.GetAddressOf(), 
					1, 0);

				// Draw all objects to depthbuffer
				for (const MeshDrawCommand& Command : CurrentScene.GetMeshDrawCommands())
				{
					CmdList.BindVertexBuffers(&Command.VertexBuffer, 1, 0);
					CmdList.BindIndexBuffer(Command.IndexBuffer);

					ShadowPerObjectBuffer.Matrix		= Command.CurrentActor->GetTransform().GetMatrix();
					ShadowPerObjectBuffer.ShadowOffset	= Command.Mesh->ShadowOffset;

					CmdList.Bind32BitShaderConstants(
						EShaderStage::ShaderStage_Vertex,
						&ShadowPerObjectBuffer, 17);

					CmdList.DrawIndexedInstanced(Command.IndexCount, 1, 0, 0, 0);
				}

				break;
			}
		}

		UpdateDirLight = false;
	}
	else
	{
		CmdList.BindPrimitiveTopology(EPrimitiveTopology::PrimitiveTopology_TriangleList);
	}

	INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End Render DirectionalLight ShadowMaps");

	// Render PointLight ShadowMaps
	INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin Render PointLight ShadowMaps");
	
	if (UpdatePointLight)
	{
		TRACE_SCOPE("Render PointLight ShadowMaps");

		const UInt32 PointLightShadowSize = CurrentLightSettings.PointLightShadowSize;
		CmdList.BindViewport(
			static_cast<Float>(PointLightShadowSize),
			static_cast<Float>(PointLightShadowSize),
			0.0f,
			1.0f,
			0.0f,
			0.0f);

		CmdList.BindScissorRect(
			static_cast<Float>(PointLightShadowSize),
			static_cast<Float>(PointLightShadowSize),
			0, 0);

		CmdList.BindGraphicsPipelineState(LinearShadowMapPSO.Get());

		// PerObject Structs
		struct ShadowPerObject
		{
			XMFLOAT4X4	Matrix;
			Float		ShadowOffset;
		} ShadowPerObjectBuffer;

		PerShadowMap PerShadowMapData;
		for (Light* Light : CurrentScene.GetLights())
		{
			if (IsSubClassOf<PointLight>(Light))
			{
				PointLight* PoiLight = Cast<PointLight>(Light);
				for (UInt32 i = 0; i < 6; i++)
				{
					CmdList.ClearDepthStencilView(PointLightShadowMapsDSVs[i].Get(), DepthStencilClearValue(1.0f, 0));
					CmdList.BindRenderTargets(nullptr, 0, PointLightShadowMapsDSVs[i].Get());

					PerShadowMapData.Matrix		= PoiLight->GetMatrix(i);
					PerShadowMapData.Position	= PoiLight->GetPosition();
					PerShadowMapData.FarPlane	= PoiLight->GetShadowFarPlane();

					CmdList.TransitionBuffer(
						PerShadowMapBuffer.Get(),
						EResourceState::ResourceState_VertexAndConstantBuffer,
						EResourceState::ResourceState_CopyDest);

					CmdList.UpdateBuffer(
						PerShadowMapBuffer.Get(),
						0, sizeof(PerShadowMap),
						&PerShadowMapData);

					CmdList.TransitionBuffer(
						PerShadowMapBuffer.Get(),
						EResourceState::ResourceState_CopyDest,
						EResourceState::ResourceState_VertexAndConstantBuffer);

					CmdList.BindConstantBuffers(
						EShaderStage::ShaderStage_Vertex,
						PerShadowMapBuffer.GetAddressOf(),
						1, 0);

					// Draw all objects to depthbuffer
					if (GlobalFrustumCullEnabled)
					{
						Frustum CameraFrustum = Frustum(PoiLight->GetShadowFarPlane(), PoiLight->GetViewMatrix(i), PoiLight->GetProjectionMatrix(i));
						for (const MeshDrawCommand& Command : CurrentScene.GetMeshDrawCommands())
						{
							const XMFLOAT4X4& Transform = Command.CurrentActor->GetTransform().GetMatrix();
							XMMATRIX XmTransform	= XMMatrixTranspose(XMLoadFloat4x4(&Transform));
							XMVECTOR XmTop			= XMVectorSetW(XMLoadFloat3(&Command.Mesh->BoundingBox.Top), 1.0f);
							XMVECTOR XmBottom		= XMVectorSetW(XMLoadFloat3(&Command.Mesh->BoundingBox.Bottom), 1.0f);
							XmTop		= XMVector4Transform(XmTop, XmTransform);
							XmBottom	= XMVector4Transform(XmBottom, XmTransform);

							AABB Box;
							XMStoreFloat3(&Box.Top, XmTop);
							XMStoreFloat3(&Box.Bottom, XmBottom);
							if (CameraFrustum.CheckAABB(Box))
							{
								CmdList.BindVertexBuffers(&Command.VertexBuffer, 1, 0);
								CmdList.BindIndexBuffer(Command.IndexBuffer);

								ShadowPerObjectBuffer.Matrix		= Command.CurrentActor->GetTransform().GetMatrix();
								ShadowPerObjectBuffer.ShadowOffset	= Command.Mesh->ShadowOffset;
							
								CmdList.Bind32BitShaderConstants(
									EShaderStage::ShaderStage_Vertex,
									&ShadowPerObjectBuffer, 17);

								CmdList.DrawIndexedInstanced(Command.IndexCount, 1, 0, 0, 0);
							}
						}
					}
					else
					{
						for (const MeshDrawCommand& Command : CurrentScene.GetMeshDrawCommands())
						{
							CmdList.BindVertexBuffers(&Command.VertexBuffer, 1, 0);
							CmdList.BindIndexBuffer(Command.IndexBuffer);

							ShadowPerObjectBuffer.Matrix		= Command.CurrentActor->GetTransform().GetMatrix();
							ShadowPerObjectBuffer.ShadowOffset	= Command.Mesh->ShadowOffset;
						
							CmdList.Bind32BitShaderConstants(
								EShaderStage::ShaderStage_Vertex,
								&ShadowPerObjectBuffer, 17);

							CmdList.DrawIndexedInstanced(Command.IndexCount, 1, 0, 0, 0);
						}
					}
				}

				break;
			}
		}

		UpdatePointLight = false;
	}

	INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End Render PointLight ShadowMaps");

	// Transition ShadowMaps
#if ENABLE_VSM
	CmdList.TransitionTexture(
		VSMDirLightShadowMaps.Get(), 
		EResourceState::ResourceState_RenderTarget, 
		EResourceState::ResourceState_PixelShaderResource);
#endif
	CmdList.TransitionTexture(
		DirLightShadowMaps.Get(), 
		EResourceState::ResourceState_DepthWrite, 
		EResourceState::ResourceState_PixelShaderResource);
	
	DebugTextures.EmplaceBack(
		DirLightShadowMapSRV,
		DirLightShadowMaps,
		EResourceState::ResourceState_PixelShaderResource,
		EResourceState::ResourceState_PixelShaderResource);

	CmdList.TransitionTexture(
		PointLightShadowMaps.Get(), 
		EResourceState::ResourceState_DepthWrite, 
		EResourceState::ResourceState_PixelShaderResource);

	// Update camerabuffer
	CameraBufferDesc CamBuff;
	CamBuff.ViewProjection		= CurrentScene.GetCamera()->GetViewProjectionMatrix();
	CamBuff.View				= CurrentScene.GetCamera()->GetViewMatrix();
	CamBuff.ViewInv				= CurrentScene.GetCamera()->GetViewInverseMatrix();
	CamBuff.Projection			= CurrentScene.GetCamera()->GetProjectionMatrix();
	CamBuff.ProjectionInv		= CurrentScene.GetCamera()->GetProjectionInverseMatrix();
	CamBuff.ViewProjectionInv	= CurrentScene.GetCamera()->GetViewProjectionInverseMatrix();
	CamBuff.Position			= CurrentScene.GetCamera()->GetPosition();
	CamBuff.NearPlane			= CurrentScene.GetCamera()->GetNearPlane();
	CamBuff.FarPlane			= CurrentScene.GetCamera()->GetFarPlane();
	CamBuff.AspectRatio			= CurrentScene.GetCamera()->GetAspectRatio();

	CmdList.TransitionBuffer(
		CameraBuffer.Get(), 
		EResourceState::ResourceState_VertexAndConstantBuffer, 
		EResourceState::ResourceState_CopyDest);

	CmdList.UpdateBuffer(CameraBuffer.Get(), 0, sizeof(CameraBufferDesc), &CamBuff);
	
	CmdList.TransitionBuffer(
		CameraBuffer.Get(), 
		EResourceState::ResourceState_CopyDest, 
		EResourceState::ResourceState_VertexAndConstantBuffer);

	// Clear GBuffer
	ColorClearValue BlackClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	CmdList.ClearRenderTargetView(GBufferRTVs[GBUFFER_ALBEDO_INDEX].Get(), BlackClearColor);
	CmdList.ClearRenderTargetView(GBufferRTVs[GBUFFER_NORMAL_INDEX].Get(), BlackClearColor);
	CmdList.ClearRenderTargetView(GBufferRTVs[GBUFFER_MATERIAL_INDEX].Get(), BlackClearColor);
	CmdList.ClearDepthStencilView(GBufferDSV.Get(), DepthStencilClearValue(1.0f, 0));

	// Setup view
	const UInt32 RenderWidth	= MainWindowViewport->GetWidth();
	const UInt32 RenderHeight	= MainWindowViewport->GetHeight();

	CmdList.BindViewport(
		static_cast<Float>(RenderWidth),
		static_cast<Float>(RenderHeight),
		0.0f,
		1.0f,
		0.0f,
		0.0f);

	CmdList.BindScissorRect(
		static_cast<Float>(RenderWidth),
		static_cast<Float>(RenderHeight),
		0, 0);

	// Perform PrePass
	if (GlobalPrePassEnabled)
	{
		INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin PrePass");

		TRACE_SCOPE("PrePass");

		struct PerObject
		{
			XMFLOAT4X4 Matrix;
		} PerObjectBuffer;

		// Setup Pipeline
		CmdList.BindRenderTargets(nullptr, 0, GBufferDSV.Get());

		CmdList.BindGraphicsPipelineState(PrePassPSO.Get());
		
		CmdList.BindConstantBuffers(
			EShaderStage::ShaderStage_Vertex,
			CameraBuffer.GetAddressOf(),
			1, 0);

		// Draw all objects to depthbuffer
		for (const MeshDrawCommand& Command : DeferredVisibleCommands)
		{
			if (!Command.Material->HasHeightMap())
			{
				CmdList.BindVertexBuffers(&Command.VertexBuffer, 1, 0);
				CmdList.BindIndexBuffer(Command.IndexBuffer);

				PerObjectBuffer.Matrix = Command.CurrentActor->GetTransform().GetMatrix();
			
				CmdList.Bind32BitShaderConstants(
					EShaderStage::ShaderStage_Vertex,
					&PerObjectBuffer, 16);

				CmdList.DrawIndexedInstanced(Command.IndexCount, 1, 0, 0, 0);
			}
		}

		INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End PrePass");
	}

	// Render all objects to the GBuffer
	INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin GeometryPass");

	{
		TRACE_SCOPE("GeometryPass");
	
		RenderTargetView* RenderTargets[] =
		{
			GBufferRTVs[GBUFFER_ALBEDO_INDEX].Get(),
			GBufferRTVs[GBUFFER_NORMAL_INDEX].Get(),
			GBufferRTVs[GBUFFER_MATERIAL_INDEX].Get()
		};
		CmdList.BindRenderTargets(RenderTargets, 3, GBufferDSV.Get());

		// Setup Pipeline
		CmdList.BindGraphicsPipelineState(GeometryPSO.Get());

		struct TransformBuffer
		{
			XMFLOAT4X4 Transform;
			XMFLOAT4X4 TransformInv;
		} TransformPerObject;

		for (const MeshDrawCommand& Command : DeferredVisibleCommands)
		{
			CmdList.BindVertexBuffers(&Command.VertexBuffer, 1, 0);
			CmdList.BindIndexBuffer(Command.IndexBuffer);

			if (Command.Material->IsBufferDirty())
			{
				Command.Material->BuildBuffer(CmdList);
			}

			CmdList.BindConstantBuffers(
				EShaderStage::ShaderStage_Vertex,
				&CameraBuffer,
				1, 0);

			ConstantBuffer* MaterialBuffer = Command.Material->GetMaterialBuffer();
			CmdList.BindConstantBuffers(
				EShaderStage::ShaderStage_Pixel,
				&MaterialBuffer,
				1, 1);

			TransformPerObject.Transform	= Command.CurrentActor->GetTransform().GetMatrix();
			TransformPerObject.TransformInv	= Command.CurrentActor->GetTransform().GetMatrixInverse();
		
			ShaderResourceView* const* ShaderResourceViews = Command.Material->GetShaderResourceViews();
			CmdList.BindShaderResourceViews(
				EShaderStage::ShaderStage_Pixel,
				ShaderResourceViews, 
				6, 0);

			SamplerState* Sampler = Command.Material->GetMaterialSampler();
			CmdList.BindSamplerStates(
				EShaderStage::ShaderStage_Pixel,
				&Sampler,
				1, 0);

			CmdList.Bind32BitShaderConstants(
				EShaderStage::ShaderStage_Vertex,
				&TransformPerObject, 32);

			CmdList.DrawIndexedInstanced(Command.IndexCount, 1, 0, 0, 0);
		}

		// Setup GBuffer for Read
		CmdList.TransitionTexture(
			GBuffer[GBUFFER_ALBEDO_INDEX].Get(), 
			EResourceState::ResourceState_RenderTarget, 
			EResourceState::ResourceState_PixelShaderResource);
	
		DebugTextures.EmplaceBack(
			GBufferSRVs[GBUFFER_ALBEDO_INDEX],
			GBuffer[GBUFFER_ALBEDO_INDEX],
			EResourceState::ResourceState_PixelShaderResource,
			EResourceState::ResourceState_PixelShaderResource);

		CmdList.TransitionTexture(
			GBuffer[GBUFFER_NORMAL_INDEX].Get(), 
			EResourceState::ResourceState_RenderTarget, 
			EResourceState::ResourceState_PixelShaderResource);

		DebugTextures.EmplaceBack(
			GBufferSRVs[GBUFFER_NORMAL_INDEX],
			GBuffer[GBUFFER_NORMAL_INDEX],
			EResourceState::ResourceState_PixelShaderResource,
			EResourceState::ResourceState_PixelShaderResource);

		CmdList.TransitionTexture(
			GBuffer[GBUFFER_MATERIAL_INDEX].Get(), 
			EResourceState::ResourceState_RenderTarget, 
			EResourceState::ResourceState_PixelShaderResource);

		DebugTextures.EmplaceBack(
			GBufferSRVs[GBUFFER_MATERIAL_INDEX],
			GBuffer[GBUFFER_MATERIAL_INDEX],
			EResourceState::ResourceState_PixelShaderResource,
			EResourceState::ResourceState_PixelShaderResource);

		CmdList.TransitionTexture(
			GBuffer[GBUFFER_DEPTH_INDEX].Get(), 
			EResourceState::ResourceState_DepthWrite, 
			EResourceState::ResourceState_PixelShaderResource);
	}

	INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End GeometryPass");

	// RayTracing
	if (RenderLayer::IsRayTracingSupported() && GlobalRayTracingEnabled)
	{
		INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin RayTracing");

		TraceRays(BackBuffer, CmdList);
		RayTracingGeometryInstances.Clear();

		INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End RayTracing");
	}

	// SSAO
	CmdList.TransitionTexture(
	SSAOBuffer.Get(), 
	EResourceState::ResourceState_PixelShaderResource, 
	EResourceState::ResourceState_UnorderedAccess);

	const Float WhiteColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	CmdList.ClearUnorderedAccessView(SSAOBufferUAV.Get(), WhiteColor);

	if (GlobalSSAOEnabled)
	{
		INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin SSAO");
		
		TRACE_SCOPE("SSAO");

		CmdList.TransitionTexture(
			GBuffer[GBUFFER_NORMAL_INDEX].Get(),
			EResourceState::ResourceState_PixelShaderResource,
			EResourceState::ResourceState_NonPixelShaderResource);

		CmdList.TransitionTexture(
			GBuffer[GBUFFER_DEPTH_INDEX].Get(),
			EResourceState::ResourceState_PixelShaderResource,
			EResourceState::ResourceState_NonPixelShaderResource);

		struct SSAOSettings
		{
			XMFLOAT2 ScreenSize;
			XMFLOAT2 NoiseSize;
			Float Radius;
			Float Bias;
			Int32 KernelSize;
		} SSAOSettings;

		const UInt32 Width	= RenderWidth;
		const UInt32 Height	= RenderHeight;
		SSAOSettings.ScreenSize	= XMFLOAT2(Float(Width), Float(Height));
		SSAOSettings.NoiseSize	= XMFLOAT2(4.0f, 4.0f);
		SSAOSettings.Radius		= SSAORadius->GetFloat();
		SSAOSettings.KernelSize = SSAOKernelSize->GetInt32();
		SSAOSettings.Bias		= SSAOBias->GetFloat();

		ShaderResourceView* ShaderResourceViews[] =
		{
			GBufferSRVs[GBUFFER_NORMAL_INDEX].Get(),
			GBufferSRVs[GBUFFER_DEPTH_INDEX].Get(),
			SSAONoiseSRV.Get(),
			SSAOSamplesSRV.Get()
		};

		CmdList.BindComputePipelineState(SSAOPSO.Get());
		
		CmdList.BindShaderResourceViews(
			EShaderStage::ShaderStage_Compute,
			ShaderResourceViews,
			4, 0);

		SamplerState* SamplerStates[] =
		{
			GBufferSampler.Get()
		};

		CmdList.BindSamplerStates(
			EShaderStage::ShaderStage_Compute,
			SamplerStates,
			1, 0);

		CmdList.BindConstantBuffers(
			EShaderStage::ShaderStage_Compute,
			CameraBuffer.GetAddressOf(),
			1, 0);

		CmdList.BindUnorderedAccessViews(
			EShaderStage::ShaderStage_Compute,
			SSAOBufferUAV.GetAddressOf(),
			1, 0);

		CmdList.Bind32BitShaderConstants(
			EShaderStage::ShaderStage_Compute,
			&SSAOSettings, 7);

		constexpr UInt32 ThreadCount = 32;
		const UInt32 DispatchWidth	= Math::AlignUp<UInt32>(Width, ThreadCount) / ThreadCount;
		const UInt32 DispatchHeight = Math::AlignUp<UInt32>(Height, ThreadCount) / ThreadCount;
		CmdList.Dispatch(DispatchWidth, DispatchHeight, 1);

		CmdList.UnorderedAccessTextureBarrier(SSAOBuffer.Get());

		CmdList.BindComputePipelineState(SSAOBlur.Get());

		CmdList.Bind32BitShaderConstants(
			EShaderStage::ShaderStage_Compute,
			&SSAOSettings.ScreenSize, 2);

		CmdList.Dispatch(DispatchWidth, DispatchHeight, 1);

		CmdList.UnorderedAccessTextureBarrier(SSAOBuffer.Get());

		CmdList.TransitionTexture(
			GBuffer[GBUFFER_NORMAL_INDEX].Get(),
			EResourceState::ResourceState_NonPixelShaderResource,
			EResourceState::ResourceState_PixelShaderResource);

		CmdList.TransitionTexture(
			GBuffer[GBUFFER_DEPTH_INDEX].Get(),
			EResourceState::ResourceState_NonPixelShaderResource,
			EResourceState::ResourceState_PixelShaderResource);

		INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End SSAO");
	}

	CmdList.TransitionTexture(
		SSAOBuffer.Get(),
		EResourceState::ResourceState_UnorderedAccess,
		EResourceState::ResourceState_PixelShaderResource);

	DebugTextures.EmplaceBack(
		SSAOBufferSRV,
		SSAOBuffer,
		EResourceState::ResourceState_PixelShaderResource,
		EResourceState::ResourceState_PixelShaderResource);

	// Render to final output
	CmdList.TransitionTexture(
		FinalTarget.Get(), 
		EResourceState::ResourceState_PixelShaderResource, 
		EResourceState::ResourceState_RenderTarget);
	
	CmdList.TransitionTexture(
		BackBuffer, 
		EResourceState::ResourceState_Present, 
		EResourceState::ResourceState_RenderTarget);

	RenderTargetView* RenderTarget[] = { FinalTargetRTV.Get() };
	CmdList.BindRenderTargets(RenderTarget, 1, nullptr);

	CmdList.BindViewport(
		static_cast<Float>(RenderWidth),
		static_cast<Float>(RenderHeight),
		0.0f,
		1.0f,
		0.0f,
		0.0f);

	CmdList.BindScissorRect(
		static_cast<Float>(RenderWidth),
		static_cast<Float>(RenderHeight),
		0, 0);

	// Setup LightPass
	INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin LightPass");

	{
		TRACE_SCOPE("LightPass");

		CmdList.BindPrimitiveTopology(EPrimitiveTopology::PrimitiveTopology_TriangleList);

		CmdList.BindGraphicsPipelineState(LightPassPSO.Get());
	
		{
			ShaderResourceView* ShaderResourceViews[] =
			{
				GBufferSRVs[GBUFFER_ALBEDO_INDEX].Get(),
				GBufferSRVs[GBUFFER_NORMAL_INDEX].Get(),
				GBufferSRVs[GBUFFER_MATERIAL_INDEX].Get(),
				GBufferSRVs[GBUFFER_DEPTH_INDEX].Get(),
				ReflectionTextureSRV.Get(),
				IrradianceMapSRV.Get(),
				SpecularIrradianceMapSRV.Get(),
				IntegrationLUTSRV.Get(),
				DirLightShadowMapSRV.Get(),
				PointLightShadowMapsSRV.Get(),
				SSAOBufferSRV.Get()
			};
	
			CmdList.BindShaderResourceViews(
				EShaderStage::ShaderStage_Pixel,
				ShaderResourceViews, 
				11, 0);
		}

		{
			ConstantBuffer* ConstantBuffers[] =
			{
				CameraBuffer.Get(),
				PointLightBuffer.Get(),
				DirectionalLightBuffer.Get()
			};

			CmdList.BindConstantBuffers(
				EShaderStage::ShaderStage_Pixel,
				ConstantBuffers, 
				3, 0);
		}

		{
			SamplerState* SamplerStates[] =
			{
				GBufferSampler.Get(),
				IntegrationLUTSampler.Get(),
				IrradianceSampler.Get(),
				ShadowMapCompSampler.Get(),
				ShadowMapSampler.Get()
			};

			CmdList.BindSamplerStates(
				EShaderStage::ShaderStage_Pixel,
				SamplerStates,
				5,
				0);
		}

		// Perform LightPass
		CmdList.DrawInstanced(3, 1, 0, 0);
	}

	INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End LightPass");

	// Draw skybox
	INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin Skybox");

	{
		TRACE_SCOPE("Render Skybox");

		CmdList.TransitionTexture(
			GBuffer[GBUFFER_DEPTH_INDEX].Get(), 
			EResourceState::ResourceState_PixelShaderResource, 
			EResourceState::ResourceState_DepthWrite);
	
		CmdList.BindRenderTargets(RenderTarget, 1, GBufferDSV.Get());
	
		CmdList.BindPrimitiveTopology(EPrimitiveTopology::PrimitiveTopology_TriangleList);
		CmdList.BindVertexBuffers(&SkyboxVertexBuffer, 1, 0);
		CmdList.BindIndexBuffer(SkyboxIndexBuffer.Get());
		CmdList.BindGraphicsPipelineState(SkyboxPSO.Get());
	
		struct SimpleCameraBuffer
		{
			XMFLOAT4X4 Matrix;
		} SimpleCamera;
		SimpleCamera.Matrix = CurrentScene.GetCamera()->GetViewProjectionWitoutTranslateMatrix();

		CmdList.Bind32BitShaderConstants(
			EShaderStage::ShaderStage_Vertex,
			&SimpleCamera, 16);

		CmdList.BindShaderResourceViews(
			EShaderStage::ShaderStage_Pixel,
			&SkyboxSRV,
			1, 0);

		CmdList.BindSamplerStates(
			EShaderStage::ShaderStage_Pixel,
			&SkyboxSampler,
			1, 0);

		CmdList.DrawIndexedInstanced(
			static_cast<UInt32>(SkyboxMesh.Indices.Size()), 
			1, 0, 0, 0);
	}

	INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End Skybox");

	// Render to BackBuffer
	CmdList.TransitionTexture(
		FinalTarget.Get(), 
		EResourceState::ResourceState_RenderTarget, 
		EResourceState::ResourceState_PixelShaderResource);

	DebugTextures.EmplaceBack(
		FinalTargetSRV,
		FinalTarget,
		EResourceState::ResourceState_PixelShaderResource,
		EResourceState::ResourceState_PixelShaderResource);
	
	CmdList.BindRenderTargets(&BackBufferView, 1, nullptr);

	CmdList.BindShaderResourceViews(
		EShaderStage::ShaderStage_Pixel,
		&FinalTargetSRV, 1, 0);

	CmdList.BindSamplerStates(
		EShaderStage::ShaderStage_Pixel,
		&GBufferSampler, 1, 0);

	if (GlobalFXAAEnabled)
	{
		INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin FXAA");
		
		TRACE_SCOPE("FXAA");

		struct FXAASettings
		{
			Float Width;
			Float Height;
		} Settings;

		Settings.Width	= static_cast<Float>(RenderWidth);
		Settings.Height	= static_cast<Float>(RenderHeight);

		CmdList.Bind32BitShaderConstants(
			EShaderStage::ShaderStage_Pixel, 
			&Settings, 2);

		CmdList.BindGraphicsPipelineState(FXAAPSO.Get());

		CmdList.DrawInstanced(3, 1, 0, 0);

		INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End FXAA");
	}
	else
	{
		INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin Draw BackBuffer");

		CmdList.BindGraphicsPipelineState(PostPSO.Get());
		CmdList.DrawInstanced(3, 1, 0, 0);

		INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End Draw BackBuffer");
	}

	// Forward Pass
	INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin ForwardPass");

	{
		TRACE_SCOPE("ForwardPass");

		CmdList.BindViewport(
			static_cast<Float>(RenderWidth),
			static_cast<Float>(RenderHeight),
			0.0f,
			1.0f,
			0.0f,
			0.0f);

		CmdList.BindScissorRect(
			static_cast<Float>(RenderWidth),
			static_cast<Float>(RenderHeight),
			0, 0);

		// Render all transparent objects;
		CmdList.BindRenderTargets(
			&BackBufferView, 1, 
			GBufferDSV.Get());

		// Setup Pipeline
		{
			ConstantBuffer* ConstantBuffers[] =
			{
				CameraBuffer.Get(),
				PointLightBuffer.Get(),
				DirectionalLightBuffer.Get(),
			};

			CmdList.BindConstantBuffers(
				EShaderStage::ShaderStage_Pixel,
				ConstantBuffers,
				3, 0);
		}

		{
			ShaderResourceView* ShaderResourceViews[] =
			{
				IrradianceMapSRV.Get(),
				SpecularIrradianceMapSRV.Get(),
				IntegrationLUTSRV.Get(),
				DirLightShadowMapSRV.Get(),
				PointLightShadowMapsSRV.Get(),
			};

			CmdList.BindShaderResourceViews(
				EShaderStage::ShaderStage_Pixel,
				ShaderResourceViews,
				5, 0);
		}

		{
			SamplerState* SamplerStates[] =
			{
				IntegrationLUTSampler.Get(),
				IrradianceSampler.Get(),
				ShadowMapCompSampler.Get(),
				ShadowMapSampler.Get()
			};

			CmdList.BindSamplerStates(
				EShaderStage::ShaderStage_Pixel,
				SamplerStates,
				4, 1);
		}

		struct TransformBuffer
		{
			XMFLOAT4X4 Transform;
			XMFLOAT4X4 TransformInv;
		} TransformPerObject;

		CmdList.BindGraphicsPipelineState(ForwardPSO.Get());
		for (const MeshDrawCommand& Command : ForwardVisibleCommands)
		{
			CmdList.BindVertexBuffers(&Command.VertexBuffer, 1, 0);
			CmdList.BindIndexBuffer(Command.IndexBuffer);

			if (Command.Material->IsBufferDirty())
			{
				Command.Material->BuildBuffer(CmdList);
			}
		
			ConstantBuffer* ConstantBuffer = Command.Material->GetMaterialBuffer();
			CmdList.BindConstantBuffers(
				EShaderStage::ShaderStage_Pixel,
				&ConstantBuffer,
				1, 3);

			ShaderResourceView* const* ShaderResourceViews = Command.Material->GetShaderResourceViews();
			CmdList.BindShaderResourceViews(
				EShaderStage::ShaderStage_Pixel,
				ShaderResourceViews,
				7, 5);

			SamplerState* SamplerState = Command.Material->GetMaterialSampler();
			CmdList.BindSamplerStates(
				EShaderStage::ShaderStage_Pixel,
				&SamplerState,
				1, 0);

			TransformPerObject.Transform	= Command.CurrentActor->GetTransform().GetMatrix();
			TransformPerObject.TransformInv	= Command.CurrentActor->GetTransform().GetMatrixInverse();

			CmdList.Bind32BitShaderConstants(
				EShaderStage::ShaderStage_Vertex,
				&TransformPerObject, 32);

			CmdList.DrawIndexedInstanced(Command.IndexCount, 1, 0, 0, 0);
		}
	}

	INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End ForwardPass");

	// Draw DebugBoxes
	if (GlobalDrawAABBs)
	{
		INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin DebugPass");

		TRACE_SCOPE("DebugPass");

		CmdList.BindGraphicsPipelineState(DebugBoxPSO.Get());

		CmdList.BindPrimitiveTopology(EPrimitiveTopology::PrimitiveTopology_LineList);

		CmdList.BindConstantBuffers(
			EShaderStage::ShaderStage_Vertex,
			&CameraBuffer,
			1, 0);

		CmdList.BindVertexBuffers(&AABBVertexBuffer, 1, 0);
		CmdList.BindIndexBuffer(AABBIndexBuffer.Get());

		for (const MeshDrawCommand& Command : DeferredVisibleCommands)
		{
			AABB& Box = Command.Mesh->BoundingBox;
			XMFLOAT3 Scale		= XMFLOAT3(Box.GetWidth(), Box.GetHeight(), Box.GetDepth());
			XMFLOAT3 Position	= Box.GetCenter();

			XMMATRIX XmTranslation	= XMMatrixTranslation(Position.x, Position.y, Position.z);
			XMMATRIX XmScale		= XMMatrixScaling(Scale.x, Scale.y, Scale.z);

			XMFLOAT4X4 Transform = Command.CurrentActor->GetTransform().GetMatrix();
			XMMATRIX XmTransform = XMMatrixTranspose(XMLoadFloat4x4(&Transform));
			XMStoreFloat4x4(&Transform, XMMatrixMultiplyTranspose(XMMatrixMultiply(XmScale, XmTranslation), XmTransform));

			CmdList.Bind32BitShaderConstants(
				EShaderStage::ShaderStage_Vertex, 
				&Transform, 16);

			CmdList.DrawIndexedInstanced(24, 1, 0, 0, 0);
		}

		INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End DebugPass");
	}

	// Render UI
	INSERT_DEBUG_CMDLIST_MARKER(CmdList, "Begin UI Render");

	{
		TRACE_SCOPE("Render UI");

		if (DrawTextureDebugger->GetBool())
		{
			DebugTextures.EmplaceBack(
				GBufferSRVs[GBUFFER_DEPTH_INDEX],
				GBuffer[GBUFFER_DEPTH_INDEX],
				EResourceState::ResourceState_DepthWrite,
				EResourceState::ResourceState_PixelShaderResource);

			DebugUI::DrawUI([]()
			{
				constexpr Float InvAspectRatio	= 16.0f / 9.0f;
				constexpr Float AspectRatio		= 9.0f / 16.0f;

				const UInt32 WindowWidth	= GlobalMainWindow->GetWidth();
				const UInt32 WindowHeight	= GlobalMainWindow->GetHeight();
				const Float Width			= Math::Max(WindowWidth * 0.6f, 400.0f);
				const Float Height			= WindowHeight * 0.75f;

				ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.3f, 0.3f, 0.3f, 0.6f));
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.2f, 1.0f));

				ImGui::SetNextWindowPos(
					ImVec2(Float(WindowWidth) * 0.5f, Float(WindowHeight) * 0.175f),
					ImGuiCond_Appearing,
					ImVec2(0.5f, 0.0f));

				ImGui::SetNextWindowSize(
					ImVec2(Width, Height),
					ImGuiCond_Appearing);
				
				const ImGuiWindowFlags Flags =
					ImGuiWindowFlags_NoResize			|
					ImGuiWindowFlags_NoScrollbar		|
					ImGuiWindowFlags_NoCollapse			|
					ImGuiWindowFlags_NoFocusOnAppearing |
					ImGuiWindowFlags_NoSavedSettings;

				Bool TempDrawTextureDebugger = DrawTextureDebugger->GetBool();
				if (ImGui::Begin(
					"FrameBuffer Debugger",
					&TempDrawTextureDebugger,
					Flags))
				{
					ImGui::BeginChild(
						"##ScrollBox",
						ImVec2(Width * 0.985f, Height * 0.125f),
						true,
						ImGuiWindowFlags_HorizontalScrollbar);

					const Int32 Count = GlobalRenderer->DebugTextures.Size();
					static Int32 SelectedImage = -1;
					if (SelectedImage >= Count)
					{
						SelectedImage = -1;
					}

					for (Int32 i = 0; i < Count; i++)
					{
						ImGui::PushID(i);

						constexpr Float MenuImageSize = 96.0f;
						Int32	FramePadding = 2;
						ImVec2	Size = ImVec2(MenuImageSize * InvAspectRatio, MenuImageSize);
						ImVec2	Uv0 = ImVec2(0.0f, 0.0f);
						ImVec2	Uv1 = ImVec2(1.0f, 1.0f);
						ImVec4	BgCol = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
						ImVec4	TintCol = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

						ImGuiImage* CurrImage = &GlobalRenderer->DebugTextures[i];
						if (ImGui::ImageButton(CurrImage, Size, Uv0, Uv1, FramePadding, BgCol, TintCol))
						{
							SelectedImage = i;
						}

						ImGui::PopID();

						if (i != Count - 1)
						{
							ImGui::SameLine();
						}
					}

					ImGui::EndChild();

					const Float ImageWidth = Width * 0.985f;
					const Float ImageHeight = ImageWidth * AspectRatio;
					const Int32 ImageIndex = SelectedImage < 0 ? 0 : SelectedImage;
					ImGuiImage* CurrImage = &GlobalRenderer->DebugTextures[ImageIndex];
					ImGui::Image(CurrImage, ImVec2(ImageWidth, ImageHeight));

					ImGui::PopStyleColor();
					ImGui::PopStyleColor();
				}

				ImGui::End();

				DrawTextureDebugger->SetBool(TempDrawTextureDebugger);
			});
		}
		else
		{
			CmdList.TransitionTexture(
				GBuffer[GBUFFER_DEPTH_INDEX].Get(), 
				EResourceState::ResourceState_DepthWrite, 
				EResourceState::ResourceState_PixelShaderResource);
		}

		if (DrawRendererInfo->GetBool())
		{
			DebugUI::DrawUI([]()
			{
				const UInt32 WindowWidth	= GlobalMainWindow->GetWidth();
				const UInt32 WindowHeight	= GlobalMainWindow->GetHeight();
				const Float Width			= 300.0f;
				const Float Height			= WindowHeight * 0.1f;

				ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.3f, 0.3f, 0.3f, 0.6f));
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.2f, 1.0f));

				ImGui::SetNextWindowPos(
					ImVec2(Float(WindowWidth), 10.0f),
					ImGuiCond_Always,
					ImVec2(1.0f, 0.0f));

				ImGui::SetNextWindowSize(
					ImVec2(Width, Height),
					ImGuiCond_Always);

				ImGui::Begin(
					"Renderer Window",
					nullptr,
					ImGuiWindowFlags_NoMove				|
					ImGuiWindowFlags_NoDecoration		|
					ImGuiWindowFlags_NoFocusOnAppearing |
					ImGuiWindowFlags_NoSavedSettings);

				ImGui::Text("Renderer Status:");
				ImGui::Separator();

				ImGui::Columns(2, nullptr, false);
				ImGui::SetColumnWidth(0, 100.0f);

				const std::string AdapterName = RenderLayer::GetAdapterName();
				ImGui::Text("Adapter: ");
				ImGui::NextColumn();

				ImGui::Text("%s", AdapterName.c_str());
				ImGui::NextColumn();

				ImGui::Text("DrawCalls: ");
				ImGui::NextColumn();

				ImGui::Text("%d", GlobalRenderer->LastFrameNumDrawCalls);
				ImGui::NextColumn();

				ImGui::Text("DispatchCalls: ");
				ImGui::NextColumn();

				ImGui::Text("%d", GlobalRenderer->LastFrameNumDispatchCalls);
				ImGui::NextColumn();

				ImGui::Text("Command Count: ");
				ImGui::NextColumn();

				ImGui::Text("%d", GlobalRenderer->LastFrameNumCommands);

				ImGui::Columns(1);

				ImGui::PopStyleColor();
				ImGui::PopStyleColor();
				ImGui::End();
			});
		}

		DebugUI::Render(CmdList);
	}

	INSERT_DEBUG_CMDLIST_MARKER(CmdList, "End UI Render");

	// Finalize Commandlist
	CmdList.TransitionTexture(
		BackBuffer, 
		EResourceState::ResourceState_RenderTarget, 
		EResourceState::ResourceState_Present);
	
	INSERT_DEBUG_CMDLIST_MARKER(CmdList, "--END FRAME--");

	CmdList.End();

	LastFrameNumDrawCalls		= CmdList.GetNumDrawCalls();
	LastFrameNumDispatchCalls	= CmdList.GetNumDispatchCalls();
	LastFrameNumCommands		= CmdList.GetNumCommands();

	{
		TRACE_SCOPE("ExecuteCommandList");
		GlobalCmdListExecutor.ExecuteCommandList(CmdList);
	}

	{
		TRACE_SCOPE("Present");
		MainWindowViewport->Present(GlobalVSyncEnabled);
	}
}

void Renderer::TraceRays(Texture2D* BackBuffer, CommandList& InCmdList)
{
	TRACE_SCOPE("TraceRays");

	InCmdList.TransitionTexture(
		ReflectionTexture.Get(), 
		EResourceState::ResourceState_CopySource, 
		EResourceState::ResourceState_UnorderedAccess);

	UNREFERENCED_VARIABLE(BackBuffer);

	//D3D12_DISPATCH_RAYS_DESC raytraceDesc = {};
	//raytraceDesc.Width	= static_cast<UInt32>(ReflectionTexture->GetDesc().Width);
	//raytraceDesc.Height = static_cast<UInt32>(ReflectionTexture->GetDesc().Height);
	//raytraceDesc.Depth	= 1;

	// Set shader tables
	InCmdList.BindRayTracingScene(RayTracingScene.Get());

	// Bind the empty root signature
	// InCommandList->SetComputeRootSignature(GlobalRootSignature->GetRootSignature());
	// InCommandList->SetComputeRootDescriptorTable(GlobalDescriptorTable->GetGPUTableStartHandle(), 0);

	// Dispatch
	InCmdList.BindRayTracingPipelineState(RaytracingPSO.Get());

	// TODO: Input width and height
	InCmdList.DispatchRays(0, 0, 1);

	// Copy the results to the back-buffer
	InCmdList.TransitionTexture(
		ReflectionTexture.Get(), 
		EResourceState::ResourceState_UnorderedAccess, 
		EResourceState::ResourceState_CopySource);
}

Bool Renderer::OnEvent(const Event& Event)
{
	if (!IsEventOfType<WindowResizeEvent>(Event))
	{
		return false;
	}

	const WindowResizeEvent& ResizeEvent = CastEvent<WindowResizeEvent>(Event);
	const UInt32 Width	= ResizeEvent.Width;
	const UInt32 Height	= ResizeEvent.Height;

	GlobalCmdListExecutor.WaitForGPU();

	MainWindowViewport->Resize(Width, Height);

	InitGBuffer();
	InitSSAO_RenderTarget();

	return true;
}

void Renderer::SetLightSettings(const LightSettings& InLightSettings)
{
	CurrentLightSettings = InLightSettings;

	GlobalCmdListExecutor.WaitForGPU();

	CreateShadowMaps();

#if ENABLE_VSM
	CmdList.TransitionTexture(
		CurrentRenderer->VSMDirLightShadowMaps.Get(), 
		EResourceState::ResourceState_Common, 
		EResourceState::ResourceState_PixelShaderResource);
#endif
	CmdList.TransitionTexture(
		DirLightShadowMaps.Get(), 
		EResourceState::ResourceState_Common, 
		EResourceState::ResourceState_PixelShaderResource);
		
	CmdList.TransitionTexture(
		PointLightShadowMaps.Get(),
		EResourceState::ResourceState_Common, 
		EResourceState::ResourceState_PixelShaderResource);

	GlobalCmdListExecutor.ExecuteCommandList(CmdList);
}

Bool Renderer::Init()
{
	INIT_CONSOLE_VARIABLE(DrawTextureDebugger, ConsoleVariableType_Bool);
	DrawTextureDebugger->SetBool(false);

	INIT_CONSOLE_VARIABLE(DrawRendererInfo, ConsoleVariableType_Bool);
	DrawRendererInfo->SetBool(false);

	INIT_CONSOLE_VARIABLE(SSAOKernelSize, ConsoleVariableType_Int);
	SSAOKernelSize->SetInt32(48);

	INIT_CONSOLE_VARIABLE(SSAOBias, ConsoleVariableType_Float);
	SSAOBias->SetFloat(0.0001f);

	INIT_CONSOLE_VARIABLE(SSAORadius, ConsoleVariableType_Float);
	SSAORadius->SetFloat(0.3f);

	// Viewport
	MainWindowViewport = RenderLayer::CreateViewport(
		GlobalMainWindow,
		0, 0,
		EFormat::Format_R8G8B8A8_Unorm,
		EFormat::Format_Unknown);
	if (!MainWindowViewport)
	{
		return false;
	}
	else
	{
		MainWindowViewport->SetName("Main Window Viewport");
	}

	// Create mesh
	SkyboxMesh	= MeshFactory::CreateSphere(1);

	// Create camera
	CameraBuffer = RenderLayer::CreateConstantBuffer<CameraBufferDesc>(
		nullptr, 
		BufferUsage_Default,
		EResourceState::ResourceState_Common);
	if (!CameraBuffer)
	{
		LOG_ERROR("[Renderer]: Failed to create camerabuffer");
		return false;
	}
	else
	{
		CameraBuffer->SetName("CameraBuffer");
	}

	// Create VertexBuffers
	{
		ResourceData VertexData = ResourceData(SkyboxMesh.Vertices.Data());
		SkyboxVertexBuffer = RenderLayer::CreateVertexBuffer<Vertex>(
			&VertexData, 
			SkyboxMesh.Vertices.Size(), 
			BufferUsage_Dynamic);
		if (!SkyboxVertexBuffer)
		{
			return false;
		}
		else
		{
			SkyboxVertexBuffer->SetName("SkyboxVertexBuffer");
		}
	}

	// Create indexbuffers
	{
		ResourceData IndexData = ResourceData(SkyboxMesh.Indices.Data());

		const UInt32 SizeInBytes = sizeof(UInt32) * static_cast<UInt64>(SkyboxMesh.Indices.Size());
		SkyboxIndexBuffer = RenderLayer::CreateIndexBuffer(
			&IndexData, 
			SizeInBytes, 
			EIndexFormat::IndexFormat_UInt32, 
			BufferUsage_Dynamic);
		if (!SkyboxIndexBuffer)
		{
			return false;
		}
		else
		{
			SkyboxIndexBuffer->SetName("SkyboxIndexBuffer");
		}
	}

	// Create Texture Cube
	const std::string PanoramaSourceFilename = "../Assets/Textures/arches.hdr";
	SampledTexture2D Panorama = TextureFactory::LoadSampledTextureFromFile(
		PanoramaSourceFilename,
		0, 
		EFormat::Format_R32G32B32A32_Float);
	if (!Panorama)
	{
		return false;	
	}
	else
	{
		Panorama.SetName(PanoramaSourceFilename);
	}

	Skybox = TextureFactory::CreateTextureCubeFromPanorma(
		Panorama,
		1024, 
		TextureFactoryFlag_GenerateMips, 
		EFormat::Format_R16G16B16A16_Float);
	if (!Skybox)
	{
		return false;
	}
	else
	{
		Skybox->SetName("Skybox");
	}

	SkyboxSRV = RenderLayer::CreateShaderResourceView(
		Skybox.Get(),
		EFormat::Format_R16G16B16A16_Float,
		0, 
		Skybox->GetMipLevels());
	if (!SkyboxSRV)
	{
		return false;
	}
	else
	{
		SkyboxSRV->SetName("Skybox SRV");
	}

	{
		SamplerStateCreateInfo CreateInfo;
		CreateInfo.AddressU	= ESamplerMode::SamplerMode_Wrap;
		CreateInfo.AddressV	= ESamplerMode::SamplerMode_Wrap;
		CreateInfo.AddressW	= ESamplerMode::SamplerMode_Wrap;
		CreateInfo.Filter	= ESamplerFilter::SamplerFilter_MinMagMipLinear;
		CreateInfo.MinLOD	= 0.0f;
		CreateInfo.MaxLOD	= 0.0f;

		SkyboxSampler = RenderLayer::CreateSamplerState(CreateInfo);
		if (!SkyboxSampler)
		{
			return false;
		}
	}

	// Generate global irradiance (From Skybox)
	const UInt16 IrradianceSize = 32;
	IrradianceMap = RenderLayer::CreateTextureCube(
		nullptr, 
		EFormat::Format_R16G16B16A16_Float, 
		TextureUsage_Default | TextureUsage_RWTexture,
		IrradianceSize, 
		1, 
		1);
	if (IrradianceMap)
	{
		IrradianceMap->SetName("Irradiance Map");
	}
	else
	{
		return false;
	}

	IrradianceMapUAV = RenderLayer::CreateUnorderedAccessView(
		IrradianceMap.Get(), 
		EFormat::Format_R16G16B16A16_Float, 0);
	if (!IrradianceMapUAV)
	{
		return false;
	}

	IrradianceMapSRV = RenderLayer::CreateShaderResourceView(
		IrradianceMap.Get(), 
		EFormat::Format_R16G16B16A16_Float, 
		0, 1);
	if (!IrradianceMapSRV)
	{
		return false;
	}

	// Generate global specular irradiance (From Skybox)
	const UInt16 SpecularIrradianceSize			= 128;
	const UInt16 SpecularIrradianceMiplevels	= UInt16(std::max(std::log2(SpecularIrradianceSize), 1.0));
	SpecularIrradianceMap = RenderLayer::CreateTextureCube(
		nullptr, 
		EFormat::Format_R16G16B16A16_Float, 
		TextureUsage_Default | TextureUsage_RWTexture, 
		SpecularIrradianceSize, 
		SpecularIrradianceMiplevels, 
		1);
	if (SpecularIrradianceMap)
	{
		SpecularIrradianceMap->SetName("Specular Irradiance Map");
	}
	else
	{
		Debug::DebugBreak();
		return false;
	}

	for (UInt32 MipLevel = 0; MipLevel < SpecularIrradianceMiplevels; MipLevel++)
	{
		TSharedRef<UnorderedAccessView> Uav = RenderLayer::CreateUnorderedAccessView(
			SpecularIrradianceMap.Get(), 
			EFormat::Format_R16G16B16A16_Float, 
			MipLevel);
		if (Uav)
		{
			SpecularIrradianceMapUAVs.EmplaceBack(Uav);
			WeakSpecularIrradianceMapUAVs.EmplaceBack(Uav.Get());
		}
	}

	SpecularIrradianceMapSRV = RenderLayer::CreateShaderResourceView(
		SpecularIrradianceMap.Get(), 
		EFormat::Format_R16G16B16A16_Float, 
		0, 
		SpecularIrradianceMiplevels);
	if (!SpecularIrradianceMapSRV)
	{
		return false;
	}

	{
		SamplerStateCreateInfo CreateInfo;
		CreateInfo.AddressU	= ESamplerMode::SamplerMode_Wrap;
		CreateInfo.AddressV	= ESamplerMode::SamplerMode_Wrap;
		CreateInfo.AddressW	= ESamplerMode::SamplerMode_Wrap;
		CreateInfo.Filter	= ESamplerFilter::SamplerFilter_MinMagMipLinear;

		IrradianceSampler = RenderLayer::CreateSamplerState(CreateInfo);
		if (!IrradianceSampler)
		{
			return false;
		}
	}

	CmdList.Begin();

	GenerateIrradianceMap(
		SkyboxSRV.Get(), 
		Skybox.Get(), 
		IrradianceMapUAV.Get(), 
		IrradianceMap.Get(), 
		CmdList);

	GenerateSpecularIrradianceMap(
		SkyboxSRV.Get(),
		Skybox.Get(), 
		WeakSpecularIrradianceMapUAVs.Data(),
		WeakSpecularIrradianceMapUAVs.Size(),
		SpecularIrradianceMap.Get(), 
		CmdList);

	CmdList.End();
	GlobalCmdListExecutor.ExecuteCommandList(CmdList);

	// Init standard inputlayout
	InputLayoutStateCreateInfo InputLayout =
	{
		{ "POSITION",	0, EFormat::Format_R32G32B32_Float,	0, 0,	EInputClassification::InputClassification_Vertex, 0 },
		{ "NORMAL",		0, EFormat::Format_R32G32B32_Float,	0, 12,	EInputClassification::InputClassification_Vertex, 0 },
		{ "TANGENT",	0, EFormat::Format_R32G32B32_Float,	0, 24,	EInputClassification::InputClassification_Vertex, 0 },
		{ "TEXCOORD",	0, EFormat::Format_R32G32_Float,	0, 36,	EInputClassification::InputClassification_Vertex, 0 },
	};

	StdInputLayout = RenderLayer::CreateInputLayout(InputLayout);
	if (!StdInputLayout)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		StdInputLayout->SetName("Standard InputLayoutState");
	}

	// Init Deferred Rendering
	if (!InitLightBuffers())
	{
		return false;
	}

	if (!InitShadowMapPass())
	{
		return false;
	}

	CmdList.Begin();
	
	CmdList.TransitionBuffer(
		PointLightBuffer.Get(), 
		EResourceState::ResourceState_Common, 
		EResourceState::ResourceState_VertexAndConstantBuffer);
	
	CmdList.TransitionBuffer(
		DirectionalLightBuffer.Get(), 
		EResourceState::ResourceState_Common, 
		EResourceState::ResourceState_VertexAndConstantBuffer);
	
	CmdList.TransitionBuffer(
		PerShadowMapBuffer.Get(),
		EResourceState::ResourceState_Common,
		EResourceState::ResourceState_VertexAndConstantBuffer);

	CmdList.TransitionTexture(
		PointLightShadowMaps.Get(), 
		EResourceState::ResourceState_Common, 
		EResourceState::ResourceState_PixelShaderResource);

	CmdList.TransitionTexture(
		DirLightShadowMaps.Get(), 
		EResourceState::ResourceState_Common, 
		EResourceState::ResourceState_PixelShaderResource);

	if (VSMDirLightShadowMaps)
	{
		CmdList.TransitionTexture(
			VSMDirLightShadowMaps.Get(), 
			EResourceState::ResourceState_Common, 
			EResourceState::ResourceState_PixelShaderResource);
	}
	
	CmdList.End();
	GlobalCmdListExecutor.ExecuteCommandList(CmdList);

	if (!InitGBuffer())
	{
		return false;
	}

	if (!InitPrePass())
	{
		return false;
	}
	
	if (!InitDeferred())
	{
		return false;
	}

	if (!InitIntegrationLUT())
	{
		return false;
	}

	if (!InitDebugStates())
	{
		return false;
	}

	if (!InitAA())
	{
		return false;
	}

	if (!InitForwardPass())
	{
		return false;
	}

	if (!InitSSAO())
	{
		return false;
	}

	// Init RayTracing if supported
	if (RenderLayer::IsRayTracingSupported() && GlobalRayTracingEnabled)
	{
		if (!InitRayTracing())
		{
			return false;
		}
	}

	// Register EventFunc
	GlobalEventDispatcher->RegisterEventHandler(this, EEventCategory::EventCategory_Window);

	return true;
}

Bool Renderer::InitRayTracing()
{
	// Create RootSignatures
	//TUniquePtr<D3D12RootSignature> RayGenLocalRoot;
	//{
	//	D3D12_DESCRIPTOR_RANGE Ranges[1] = {};
	//	Ranges[0].BaseShaderRegister				= 0;
	//	Ranges[0].NumDescriptors					= 1;
	//	Ranges[0].RegisterSpace						= 0;
	//	Ranges[0].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	//	Ranges[0].OffsetInDescriptorsFromTableStart	= 0;

	//	D3D12_ROOT_PARAMETER RootParams = { };
	//	RootParams.ParameterType						= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//	RootParams.ShaderVisibility						= D3D12_SHADER_VISIBILITY_ALL;
	//	RootParams.DescriptorTable.NumDescriptorRanges	= 1;
	//	RootParams.DescriptorTable.pDescriptorRanges	= Ranges;

	//	D3D12_ROOT_SIGNATURE_DESC RayGenLocalRootDesc = {};
	//	RayGenLocalRootDesc.NumParameters	= 1;
	//	RayGenLocalRootDesc.pParameters		= &RootParams;
	//	RayGenLocalRootDesc.Flags			= D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	//	RayGenLocalRoot = RenderLayer::Get().CreateRootSignature(RayGenLocalRootDesc);
	//	if (RayGenLocalRoot)
	//	{
	//		RayGenLocalRoot->SetDebugName("RayGen Local RootSignature");
	//	}
	//	else
	//	{
	//		return false;
	//	}
	//}

	//TUniquePtr<D3D12RootSignature> HitLocalRoot;
	//{
	//	constexpr UInt32 NumRanges0 = 7;
	//	D3D12_DESCRIPTOR_RANGE Ranges0[NumRanges0] = {};
	//	// Albedo
	//	Ranges0[0].BaseShaderRegister					= 0;
	//	Ranges0[0].NumDescriptors						= 1;
	//	Ranges0[0].RegisterSpace						= 1;
	//	Ranges0[0].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	//	Ranges0[0].OffsetInDescriptorsFromTableStart	= 0;

	//	// Normal
	//	Ranges0[1].BaseShaderRegister					= 1;
	//	Ranges0[1].NumDescriptors						= 1;
	//	Ranges0[1].RegisterSpace						= 1;
	//	Ranges0[1].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	//	Ranges0[1].OffsetInDescriptorsFromTableStart	= 1;

	//	// Roughness
	//	Ranges0[2].BaseShaderRegister					= 2;
	//	Ranges0[2].NumDescriptors						= 1;
	//	Ranges0[2].RegisterSpace						= 1;
	//	Ranges0[2].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	//	Ranges0[2].OffsetInDescriptorsFromTableStart	= 2;

	//	// Height
	//	Ranges0[3].BaseShaderRegister					= 3;
	//	Ranges0[3].NumDescriptors						= 1;
	//	Ranges0[3].RegisterSpace						= 1;
	//	Ranges0[3].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	//	Ranges0[3].OffsetInDescriptorsFromTableStart	= 3;

	//	// Metallic
	//	Ranges0[4].BaseShaderRegister					= 4;
	//	Ranges0[4].NumDescriptors						= 1;
	//	Ranges0[4].RegisterSpace						= 1;
	//	Ranges0[4].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	//	Ranges0[4].OffsetInDescriptorsFromTableStart	= 4;

	//	// AO
	//	Ranges0[5].BaseShaderRegister					= 5;
	//	Ranges0[5].NumDescriptors						= 1;
	//	Ranges0[5].RegisterSpace						= 1;
	//	Ranges0[5].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	//	Ranges0[5].OffsetInDescriptorsFromTableStart	= 5;

	//	// MaterialBuffer
	//	Ranges0[6].BaseShaderRegister					= 0;
	//	Ranges0[6].NumDescriptors						= 1;
	//	Ranges0[6].RegisterSpace						= 1;
	//	Ranges0[6].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	//	Ranges0[6].OffsetInDescriptorsFromTableStart	= 6;

	//	constexpr UInt32 NumRanges1 = 2;
	//	D3D12_DESCRIPTOR_RANGE Ranges1[NumRanges1] = {};
	//	// VertexBuffer
	//	Ranges1[0].BaseShaderRegister					= 6;
	//	Ranges1[0].NumDescriptors						= 1;
	//	Ranges1[0].RegisterSpace						= 1;
	//	Ranges1[0].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	//	Ranges1[0].OffsetInDescriptorsFromTableStart	= 0;

	//	// IndexBuffer
	//	Ranges1[1].BaseShaderRegister					= 7;
	//	Ranges1[1].NumDescriptors						= 1;
	//	Ranges1[1].RegisterSpace						= 1;
	//	Ranges1[1].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	//	Ranges1[1].OffsetInDescriptorsFromTableStart	= 1;

	//	const UInt32 NumRootParams = 2;
	//	D3D12_ROOT_PARAMETER RootParams[NumRootParams];
	//	RootParams[0].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//	RootParams[0].DescriptorTable.NumDescriptorRanges	= NumRanges0;
	//	RootParams[0].DescriptorTable.pDescriptorRanges		= Ranges0;
	//	RootParams[0].ShaderVisibility						= D3D12_SHADER_VISIBILITY_ALL;

	//	RootParams[1].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//	RootParams[1].DescriptorTable.NumDescriptorRanges	= NumRanges1;
	//	RootParams[1].DescriptorTable.pDescriptorRanges		= Ranges1;
	//	RootParams[1].ShaderVisibility						= D3D12_SHADER_VISIBILITY_ALL;

	//	D3D12_ROOT_SIGNATURE_DESC HitLocalRootDesc = {};
	//	HitLocalRootDesc.NumParameters	= NumRootParams;
	//	HitLocalRootDesc.pParameters	= RootParams;
	//	HitLocalRootDesc.Flags			= D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	//	HitLocalRoot = RenderLayer::Get().CreateRootSignature(HitLocalRootDesc);
	//	if (HitLocalRoot)
	//	{
	//		HitLocalRoot->SetDebugName("Closest Hit Local RootSignature");
	//	}
	//	else
	//	{
	//		return false;
	//	}
	//}

	//TUniquePtr<D3D12RootSignature> MissLocalRoot;
	//{
	//	D3D12_ROOT_SIGNATURE_DESC MissLocalRootDesc = {};
	//	MissLocalRootDesc.NumParameters	= 0;
	//	MissLocalRootDesc.pParameters	= nullptr;
	//	MissLocalRootDesc.Flags			= D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	//TUniquePtr<D3D12RootSignature> HitLocalRoot;
	//{
	//	D3D12_DESCRIPTOR_RANGE Ranges[2] = {};
	//	// VertexBuffer
	//	Ranges[0].BaseShaderRegister				= 2;
	//	Ranges[0].NumDescriptors					= 1;
	//	Ranges[0].RegisterSpace						= 0;
	//	Ranges[0].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	//	Ranges[0].OffsetInDescriptorsFromTableStart = 0;

	//	// IndexBuffer
	//	Ranges[1].BaseShaderRegister				= 3;
	//	Ranges[1].NumDescriptors					= 1;
	//	Ranges[1].RegisterSpace						= 0;
	//	Ranges[1].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	//	Ranges[1].OffsetInDescriptorsFromTableStart	= 1;

	//	D3D12_ROOT_PARAMETER RootParams = { };
	//	RootParams.ParameterType						= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//	RootParams.DescriptorTable.NumDescriptorRanges	= 2;
	//	RootParams.DescriptorTable.pDescriptorRanges	= Ranges;

	//	D3D12_ROOT_SIGNATURE_DESC HitLocalRootDesc = {};
	//	HitLocalRootDesc.NumParameters	= 1;
	//	HitLocalRootDesc.pParameters	= &RootParams;
	//	HitLocalRootDesc.Flags			= D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	//	HitLocalRoot = RenderLayer::Get().CreateRootSignature(HitLocalRootDesc);
	//	if (HitLocalRoot)
	//	{
	//		HitLocalRoot->SetName("Closest Hit Local RootSignature");
	//	}
	//	else
	//	{
	//		return false;
	//	}
	//}

	// Global RootSignature
	//{
	//	constexpr UInt32 NumRanges = 5;
	//	D3D12_DESCRIPTOR_RANGE Ranges[NumRanges] = {};
	//	// AccelerationStructure
	//	Ranges[0].BaseShaderRegister				= 0;
	//	Ranges[0].NumDescriptors					= 1;
	//	Ranges[0].RegisterSpace						= 0;
	//	Ranges[0].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	//	Ranges[0].OffsetInDescriptorsFromTableStart	= 0;

	//	// Camera Buffer
	//	Ranges[1].BaseShaderRegister				= 0;
	//	Ranges[1].NumDescriptors					= 1;
	//	Ranges[1].RegisterSpace						= 0;
	//	Ranges[1].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	//	Ranges[1].OffsetInDescriptorsFromTableStart	= 1;

	//	// GBuffer NormalMap
	//	Ranges[2].BaseShaderRegister				= 6;
	//	Ranges[2].NumDescriptors					= 1;
	//	Ranges[2].RegisterSpace						= 0;
	//	Ranges[2].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	//	Ranges[2].OffsetInDescriptorsFromTableStart = 2;

	//	// GBuffer Depth
	//	Ranges[3].BaseShaderRegister				= 7;
	//	Ranges[3].NumDescriptors					= 1;
	//	Ranges[3].RegisterSpace						= 0;
	//	Ranges[3].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	//	Ranges[3].OffsetInDescriptorsFromTableStart = 3;

	//	// Skybox
	//	Ranges[4].BaseShaderRegister				= 1;
	//	Ranges[4].NumDescriptors					= 1;
	//	Ranges[4].RegisterSpace						= 0;
	//	Ranges[4].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	//	Ranges[4].OffsetInDescriptorsFromTableStart	= 4;

	//	D3D12_ROOT_PARAMETER RootParams = { };
	//	RootParams.ParameterType						= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//	RootParams.ShaderVisibility						= D3D12_SHADER_VISIBILITY_ALL;
	//	RootParams.DescriptorTable.NumDescriptorRanges	= NumRanges;
	//	RootParams.DescriptorTable.pDescriptorRanges	= Ranges;

	//	D3D12_STATIC_SAMPLER_DESC Samplers[2] = { };
	//	// Generic Sampler
	//	Samplers[0].ShaderVisibility	= D3D12_SHADER_VISIBILITY_ALL;
	//	Samplers[0].AddressU			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//	Samplers[0].AddressV			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//	Samplers[0].AddressW			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//	Samplers[0].Filter				= D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	//	Samplers[0].ShaderRegister		= 0;
	//	Samplers[0].RegisterSpace		= 0;
	//	Samplers[0].MinLOD				= 0.0f;
	//	Samplers[0].MaxLOD				= FLT_MAX;
 //
	//	// GBuffer Sampler
	//	Samplers[1].ShaderVisibility	= D3D12_SHADER_VISIBILITY_ALL;
	//	Samplers[1].AddressU			= D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	//	Samplers[1].AddressV			= D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	//	Samplers[1].AddressW			= D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	//	Samplers[1].Filter				= D3D12_FILTER_MIN_MAG_MIP_POINT;
	//	Samplers[1].ShaderRegister		= 1;
	//	Samplers[1].RegisterSpace		= 0;
	//	Samplers[1].MinLOD				= 0.0f;
	//	Samplers[1].MaxLOD				= FLT_MAX;

	//	D3D12_ROOT_SIGNATURE_DESC GlobalRootDesc = {};
	//	GlobalRootDesc.NumStaticSamplers	= 2;
	//	GlobalRootDesc.pStaticSamplers		= Samplers;
	//	GlobalRootDesc.NumParameters		= 1;
	//	GlobalRootDesc.pParameters			= &RootParams;
	//	GlobalRootDesc.Flags				= D3D12_ROOT_SIGNATURE_FLAG_NONE;

	//	GlobalRootSignature = RenderLayer::Get().CreateRootSignature(GlobalRootDesc);
	//	if (!GlobalRootSignature)
	//	{
	//		return false;
	//	}
	//}

	//// Global RootSignature
	//{
	//	D3D12_DESCRIPTOR_RANGE Ranges[7] = {};
	//	// AccelerationStructure
	//	Ranges[0].BaseShaderRegister				= 0;
	//	Ranges[0].NumDescriptors					= 1;
	//	Ranges[0].RegisterSpace						= 0;
	//	Ranges[0].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	//	Ranges[0].OffsetInDescriptorsFromTableStart	= 0;

	//	// Camera Buffer
	//	Ranges[1].BaseShaderRegister				= 0;
	//	Ranges[1].NumDescriptors					= 1;
	//	Ranges[1].RegisterSpace						= 0;
	//	Ranges[1].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	//	Ranges[1].OffsetInDescriptorsFromTableStart	= 1;

	//	// Skybox
	//	Ranges[2].BaseShaderRegister				= 1;
	//	Ranges[2].NumDescriptors					= 1;
	//	Ranges[2].RegisterSpace						= 0;
	//	Ranges[2].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	//	Ranges[2].OffsetInDescriptorsFromTableStart	= 2;

	//	// Albedo
	//	Ranges[3].BaseShaderRegister				= 4;
	//	Ranges[3].NumDescriptors					= 1;
	//	Ranges[3].RegisterSpace						= 0;
	//	Ranges[3].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	//	Ranges[3].OffsetInDescriptorsFromTableStart	= 3;

	//	// Normal
	//	Ranges[4].BaseShaderRegister				= 5;
	//	Ranges[4].NumDescriptors					= 1;
	//	Ranges[4].RegisterSpace						= 0;
	//	Ranges[4].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	//	Ranges[4].OffsetInDescriptorsFromTableStart	= 4;

	//	// GBuffer NormalMap
	//	Ranges[5].BaseShaderRegister				= 6;
	//	Ranges[5].NumDescriptors					= 1;
	//	Ranges[5].RegisterSpace						= 0;
	//	Ranges[5].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	//	Ranges[5].OffsetInDescriptorsFromTableStart = 5;

	//	// GBuffer Depth
	//	Ranges[6].BaseShaderRegister				= 7;
	//	Ranges[6].NumDescriptors					= 1;
	//	Ranges[6].RegisterSpace						= 0;
	//	Ranges[6].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	//	Ranges[6].OffsetInDescriptorsFromTableStart = 6;

	//	D3D12_ROOT_PARAMETER RootParams = { };
	//	RootParams.ParameterType						= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//	RootParams.DescriptorTable.NumDescriptorRanges	= 7;
	//	RootParams.DescriptorTable.pDescriptorRanges	= Ranges;

	//	D3D12_STATIC_SAMPLER_DESC Samplers[2] = { };
	//	// Generic Sampler
	//	Samplers[0].ShaderVisibility	= D3D12_SHADER_VISIBILITY_ALL;
	//	Samplers[0].AddressU			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//	Samplers[0].AddressV			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//	Samplers[0].AddressW			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//	Samplers[0].Filter				= D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	//	Samplers[0].ShaderRegister		= 0;
	//	Samplers[0].RegisterSpace		= 0;
	//	Samplers[0].MinLOD				= 0.0f;
	//	Samplers[0].MaxLOD				= FLT_MAX;
 //
	//	// GBuffer Sampler
	//	Samplers[1].ShaderVisibility	= D3D12_SHADER_VISIBILITY_ALL;
	//	Samplers[1].AddressU			= D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	//	Samplers[1].AddressV			= D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	//	Samplers[1].AddressW			= D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	//	Samplers[1].Filter				= D3D12_FILTER_MIN_MAG_MIP_POINT;
	//	Samplers[1].ShaderRegister		= 1;
	//	Samplers[1].RegisterSpace		= 0;
	//	Samplers[1].MinLOD				= 0.0f;
	//	Samplers[1].MaxLOD				= FLT_MAX;

	//	D3D12_ROOT_SIGNATURE_DESC GlobalRootDesc = {};
	//	GlobalRootDesc.NumStaticSamplers	= 2;
	//	GlobalRootDesc.pStaticSamplers		= Samplers;
	//	GlobalRootDesc.NumParameters		= 1;
	//	GlobalRootDesc.pParameters			= &RootParams;
	//	GlobalRootDesc.Flags				= D3D12_ROOT_SIGNATURE_FLAG_NONE;

	//	GlobalRootSignature = RenderLayer::Get().CreateRootSignature(GlobalRootDesc);
	//	if (!GlobalRootSignature)
	//	{
	//		return false;
	//	}
	//}

	//// Create Pipeline
	//RayTracingPipelineStateProperties PipelineProperties;
	//PipelineProperties.DebugName				= "RayTracing PipelineState";
	//PipelineProperties.RayGenRootSignature		= RayGenLocalRoot.Get();
	//PipelineProperties.HitGroupRootSignature	= HitLocalRoot.Get();
	//PipelineProperties.MissRootSignature		= MissLocalRoot.Get();
	//PipelineProperties.GlobalRootSignature		= GlobalRootSignature.Get();
	//PipelineProperties.MaxRecursions			= 4;

	//RaytracingPSO = RenderLayer::Get().CreateRayTracingPipelineState(PipelineProperties);
	//if (!RaytracingPSO)
	//{
	//	return false;
	//}

	//// Build Acceleration Structures
	//RenderLayer::Get().GetImmediateCommandList()->TransitionBarrier(CameraBuffer.Get(), EResourceState::ResourceState_Common, EResourceState::ResourceState_VertexAndConstantBuffer);

	// Create DescriptorTables
	//RayGenDescriptorTable = TSharedPtr(RenderLayer::Get().CreateDescriptorTable(1));
	//GlobalDescriptorTable = TSharedPtr(RenderLayer::Get().CreateDescriptorTable(7));

	// Create TLAS
	//RayTracingScene = RenderLayer::CreateRayTracingScene();
	//if (!RayTracingScene)
	//{
	//	return false;
	//}

	// Populate descriptors
	//RayGenDescriptorTable->SetUnorderedAccessView(ReflectionTexture->GetUnorderedAccessView(0).Get(), 0);
	//RayGenDescriptorTable->CopyDescriptors();

	//GlobalDescriptorTable->SetConstantBufferView(CameraBuffer->GetConstantBufferView().Get(), 1);
	//GlobalDescriptorTable->SetShaderResourceView(GBuffer[GBUFFER_NORMAL_INDEX]->GetShaderResourceView(0).Get(), 2);
	//GlobalDescriptorTable->SetShaderResourceView(GBuffer[GBUFFER_DEPTH_INDEX]->GetShaderResourceView(0).Get(), 3);
	//GlobalDescriptorTable->SetShaderResourceView(Skybox->GetShaderResourceView(0).Get(), 4);
	//GlobalDescriptorTable->CopyDescriptors();

	return true;
}

Bool Renderer::InitLightBuffers()
{
	const UInt32 NumPointLights = 1;
	const UInt32 NumDirLights	= 1;

	PointLightBuffer = RenderLayer::CreateConstantBuffer<PointLightProperties>(
		nullptr, 
		NumPointLights, 
		BufferUsage_Default,
		EResourceState::ResourceState_Common);
	if (!PointLightBuffer)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		PointLightBuffer->SetName("PointLight Buffer");
	}

	DirectionalLightBuffer = RenderLayer::CreateConstantBuffer<DirectionalLightProperties>(
		nullptr, 
		NumDirLights, 
		BufferUsage_Default,
		EResourceState::ResourceState_Common);
	if (!DirectionalLightBuffer)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		DirectionalLightBuffer->SetName("DirectionalLight Buffer");
	}

	PerShadowMapBuffer = RenderLayer::CreateConstantBuffer<PerShadowMap>(
		nullptr,
		NumDirLights,
		BufferUsage_Default,
		EResourceState::ResourceState_Common);
	if (!PerShadowMapBuffer)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		PerShadowMapBuffer->SetName("PerShadowMap Buffer");
	}

	{
		SamplerStateCreateInfo CreateInfo;
		CreateInfo.AddressU	= ESamplerMode::SamplerMode_Wrap;
		CreateInfo.AddressV	= ESamplerMode::SamplerMode_Wrap;
		CreateInfo.AddressW	= ESamplerMode::SamplerMode_Wrap;
		CreateInfo.Filter	= ESamplerFilter::SamplerFilter_MinMagMipPoint;

		ShadowMapSampler = RenderLayer::CreateSamplerState(CreateInfo);
		if (!ShadowMapSampler)
		{
			return false;
		}
	}

	{
		SamplerStateCreateInfo CreateInfo;
		CreateInfo.AddressU			= ESamplerMode::SamplerMode_Wrap;
		CreateInfo.AddressV			= ESamplerMode::SamplerMode_Wrap;
		CreateInfo.AddressW			= ESamplerMode::SamplerMode_Wrap;
		CreateInfo.Filter			= ESamplerFilter::SamplerFilter_Comparison_MinMagMipLinear;
		CreateInfo.ComparisonFunc	= EComparisonFunc::ComparisonFunc_LessEqual;

		ShadowMapCompSampler = RenderLayer::CreateSamplerState(CreateInfo);
		if (!ShadowMapCompSampler)
		{
			return false;
		}
	}

	return CreateShadowMaps();
}

Bool Renderer::InitPrePass()
{
	using namespace Microsoft::WRL;

	TArray<UInt8> ShaderCode;
	if (!ShaderCompiler::CompileFromFile(
		"../DXR-Engine/Shaders/PrePass.hlsl",
		"Main",
		nullptr,
		EShaderStage::ShaderStage_Vertex,
		EShaderModel::ShaderModel_6_0,
		ShaderCode))
	{
		Debug::DebugBreak();
		return false;
	}

	TSharedRef<VertexShader> VShader = RenderLayer::CreateVertexShader(ShaderCode);
	if (!VShader)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		VShader->SetName("PrePass VertexShader");
	}

	DepthStencilStateCreateInfo DepthStencilStateInfo;
	DepthStencilStateInfo.DepthFunc			= EComparisonFunc::ComparisonFunc_Less;
	DepthStencilStateInfo.DepthEnable		= true;
	DepthStencilStateInfo.DepthWriteMask	= EDepthWriteMask::DepthWriteMask_All;

	TSharedRef<DepthStencilState> DepthStencilState = RenderLayer::CreateDepthStencilState(DepthStencilStateInfo);
	if (!DepthStencilState)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		DepthStencilState->SetName("Prepass DepthStencilState");
	}

	RasterizerStateCreateInfo RasterizerStateInfo;
	RasterizerStateInfo.CullMode = ECullMode::CullMode_Back;

	TSharedRef<RasterizerState> RasterizerState = RenderLayer::CreateRasterizerState(RasterizerStateInfo);
	if (!RasterizerState)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		RasterizerState->SetName("Prepass RasterizerState");
	}

	BlendStateCreateInfo BlendStateInfo;
	BlendStateInfo.IndependentBlendEnable		= false;
	BlendStateInfo.RenderTarget[0].BlendEnable	= false;

	TSharedRef<BlendState> BlendState = RenderLayer::CreateBlendState(BlendStateInfo);
	if (!BlendState)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		BlendState->SetName("Prepass BlendState");
	}

	GraphicsPipelineStateCreateInfo PSOInfo;
	PSOInfo.InputLayoutState					= StdInputLayout.Get();
	PSOInfo.BlendState							= BlendState.Get();
	PSOInfo.DepthStencilState					= DepthStencilState.Get();
	PSOInfo.RasterizerState						= RasterizerState.Get();
	PSOInfo.ShaderState.VertexShader			= VShader.Get();
	PSOInfo.PipelineFormats.DepthStencilFormat	= DepthBufferFormat;

	PrePassPSO = RenderLayer::CreateGraphicsPipelineState(PSOInfo);
	if (!PrePassPSO)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		PrePassPSO->SetName("PrePass PipelineState");
	}

	return true;
}

Bool Renderer::InitShadowMapPass()
{
	using namespace Microsoft::WRL;

	// Directional Shadows
	TArray<UInt8> ShaderCode;

#if ENABLE_VSM
	if (!ShaderCompiler::CompileFromFile(
		"../DXR-Engine/Shaders/ShadowMap.hlsl",
		"VSM_VSMain",
		nullptr,
		EShaderStage::ShaderStage_Vertex,
		EShaderModel::ShaderModel_6_0,
		ShaderCode))
	{
		Debug::DebugBreak();
		return false;
	}

	TSharedRef<VertexShader> VSShader = RenderLayer::CreateVertexShader(ShaderCode);
	if (!VSShader)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		VSShader->SetName("ShadowMap VertexShader");
	}

	if (!ShaderCompiler::CompileFromFile(
		"../DXR-Engine/Shaders/ShadowMap.hlsl",
		"VSM_PSMain",
		nullptr,
		EShaderStage::ShaderStage_Pixel,
		EShaderModel::ShaderModel_6_0,
		ShaderCode))
	{
		Debug::DebugBreak();
		return false;
	}

	TSharedRef<PixelShader> PSShader = RenderLayer::CreatePixelShader(ShaderCode);
	if (!PSShader)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		PSShader->SetName("ShadowMap PixelShader");
	}
#else
	if (!ShaderCompiler::CompileFromFile(
		"../DXR-Engine/Shaders/ShadowMap.hlsl", 
		"Main",
		nullptr, 
		EShaderStage::ShaderStage_Vertex,
		EShaderModel::ShaderModel_6_0,
		ShaderCode))
	{
		Debug::DebugBreak();
		return false;
	}

	TSharedRef<VertexShader> VShader = RenderLayer::CreateVertexShader(ShaderCode);
	if (!VShader)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		VShader->SetName("ShadowMap VertexShader");
	}
#endif

	// Init PipelineState
	DepthStencilStateCreateInfo DepthStencilStateInfo;
	DepthStencilStateInfo.DepthFunc			= EComparisonFunc::ComparisonFunc_LessEqual;
	DepthStencilStateInfo.DepthEnable		= true;
	DepthStencilStateInfo.DepthWriteMask	= EDepthWriteMask::DepthWriteMask_All;
	
	TSharedRef<DepthStencilState> DepthStencilState = RenderLayer::CreateDepthStencilState(DepthStencilStateInfo);
	if (!DepthStencilState)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		DepthStencilState->SetName("Shadow DepthStencilState");
	}

	RasterizerStateCreateInfo RasterizerStateInfo;
	RasterizerStateInfo.CullMode = ECullMode::CullMode_Back;

	TSharedRef<RasterizerState> RasterizerState = RenderLayer::CreateRasterizerState(RasterizerStateInfo);
	if (!RasterizerState)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		RasterizerState->SetName("Shadow RasterizerState");
	}

	BlendStateCreateInfo BlendStateInfo;
	TSharedRef<BlendState> BlendState = RenderLayer::CreateBlendState(BlendStateInfo);
	if (!BlendState)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		BlendState->SetName("Shadow BlendState");
	}

	GraphicsPipelineStateCreateInfo ShadowMapPSOInfo;
	ShadowMapPSOInfo.BlendState							= BlendState.Get();
	ShadowMapPSOInfo.DepthStencilState					= DepthStencilState.Get();
	ShadowMapPSOInfo.IBStripCutValue					= EIndexBufferStripCutValue::IndexBufferStripCutValue_Disabled;
	ShadowMapPSOInfo.InputLayoutState					= StdInputLayout.Get();
	ShadowMapPSOInfo.PrimitiveTopologyType				= EPrimitiveTopologyType::PrimitiveTopologyType_Triangle;
	ShadowMapPSOInfo.RasterizerState					= RasterizerState.Get();
	ShadowMapPSOInfo.SampleCount						= 1;
	ShadowMapPSOInfo.SampleQuality						= 0;
	ShadowMapPSOInfo.SampleMask							= 0xffffffff;
	ShadowMapPSOInfo.ShaderState.VertexShader			= VShader.Get();
	ShadowMapPSOInfo.ShaderState.PixelShader			= nullptr;
	ShadowMapPSOInfo.PipelineFormats.NumRenderTargets	= 0;
	ShadowMapPSOInfo.PipelineFormats.DepthStencilFormat	= ShadowMapFormat;

#if ENABLE_VSM
	VSMShadowMapPSO = MakeShared<D3D12GraphicsPipelineState>(Device.Get());
	if (!VSMShadowMapPSO->Initialize(PSOProperties))
	{
		return false;
	}
#else
	ShadowMapPSO = RenderLayer::CreateGraphicsPipelineState(ShadowMapPSOInfo);
	if (!ShadowMapPSO)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		ShadowMapPSO->SetName("ShadowMap PipelineState");
	}
#endif

	// Linear Shadow Maps
	if (!ShaderCompiler::CompileFromFile(
		"../DXR-Engine/Shaders/ShadowMap.hlsl",
		"VSMain",
		nullptr,
		EShaderStage::ShaderStage_Vertex,
		EShaderModel::ShaderModel_6_0,
		ShaderCode))
	{
		Debug::DebugBreak();
		return false;
	}

	VShader = RenderLayer::CreateVertexShader(ShaderCode);
	if (!VShader)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		VShader->SetName("Linear ShadowMap VertexShader");
	}

#if !ENABLE_VSM
	TSharedRef<PixelShader> PShader;
#endif
	if (!ShaderCompiler::CompileFromFile(
		"../DXR-Engine/Shaders/ShadowMap.hlsl",
		"PSMain",
		nullptr,
		EShaderStage::ShaderStage_Pixel,
		EShaderModel::ShaderModel_6_0,
		ShaderCode))
	{
		Debug::DebugBreak();
		return false;
	}

	PShader = RenderLayer::CreatePixelShader(ShaderCode);
	if (!PShader)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		PShader->SetName("Linear ShadowMap PixelShader");
	}

	// Linear ShadowMap PipelineState
	ShadowMapPSOInfo.ShaderState.VertexShader	= VShader.Get();
	ShadowMapPSOInfo.ShaderState.PixelShader	= PShader.Get();

	LinearShadowMapPSO = RenderLayer::CreateGraphicsPipelineState(ShadowMapPSOInfo);
	if (!LinearShadowMapPSO)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		LinearShadowMapPSO->SetName("Linear ShadowMap PipelineState");
	}

	return true;
}

Bool Renderer::InitDeferred()
{
	using namespace Microsoft::WRL;

	// GeometryPass
	TArray<ShaderDefine> Defines =
	{
		{ "ENABLE_PARALLAX_MAPPING", "1" },
		{ "ENABLE_NORMAL_MAPPING",	 "1" },
	};

	TArray<UInt8> ShaderCode;
	if (!ShaderCompiler::CompileFromFile(
		"../DXR-Engine/Shaders/GeometryPass.hlsl", 
		"VSMain",
		&Defines,
		EShaderStage::ShaderStage_Vertex,
		EShaderModel::ShaderModel_6_0,
		ShaderCode))
	{
		Debug::DebugBreak();
		return false;
	}

	TSharedRef<VertexShader> VShader = RenderLayer::CreateVertexShader(ShaderCode);
	if (!VShader)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		VShader->SetName("GeometryPass VertexShader");
	}

	if (!ShaderCompiler::CompileFromFile(
		"../DXR-Engine/Shaders/GeometryPass.hlsl",
		"PSMain",
		&Defines,
		EShaderStage::ShaderStage_Pixel,
		EShaderModel::ShaderModel_6_0,
		ShaderCode))
	{
		Debug::DebugBreak();
		return false;
	}

	TSharedRef<PixelShader> PShader = RenderLayer::CreatePixelShader(ShaderCode);
	if (!PShader)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		PShader->SetName("GeometryPass PixelShader");
	}

	DepthStencilStateCreateInfo DepthStencilStateInfo;
	DepthStencilStateInfo.DepthFunc			= EComparisonFunc::ComparisonFunc_LessEqual;
	DepthStencilStateInfo.DepthEnable		= true;
	DepthStencilStateInfo.DepthWriteMask	= EDepthWriteMask::DepthWriteMask_All;

	TSharedRef<DepthStencilState> GeometryDepthStencilState = RenderLayer::CreateDepthStencilState(DepthStencilStateInfo);
	if (!GeometryDepthStencilState)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		GeometryDepthStencilState->SetName("GeometryPass DepthStencilState");
	}

	RasterizerStateCreateInfo RasterizerStateInfo;
	RasterizerStateInfo.CullMode = ECullMode::CullMode_Back;

	TSharedRef<RasterizerState> GeometryRasterizerState = RenderLayer::CreateRasterizerState(RasterizerStateInfo);
	if (!GeometryRasterizerState)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		GeometryRasterizerState->SetName("GeometryPass RasterizerState");
	}

	BlendStateCreateInfo BlendStateInfo;
	BlendStateInfo.IndependentBlendEnable		= false;
	BlendStateInfo.RenderTarget[0].BlendEnable	= false;

	TSharedRef<BlendState> BlendState = RenderLayer::CreateBlendState(BlendStateInfo);
	if (!BlendState)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		BlendState->SetName("GeometryPass BlendState");
	}

	GraphicsPipelineStateCreateInfo PSOProperties;
	PSOProperties.InputLayoutState							= StdInputLayout.Get();
	PSOProperties.BlendState								= BlendState.Get();
	PSOProperties.DepthStencilState							= GeometryDepthStencilState.Get();
	PSOProperties.RasterizerState							= GeometryRasterizerState.Get();
	PSOProperties.ShaderState.VertexShader					= VShader.Get();
	PSOProperties.ShaderState.PixelShader					= PShader.Get();
	PSOProperties.PipelineFormats.RenderTargetFormats[0]	= EFormat::Format_R8G8B8A8_Unorm;
	PSOProperties.PipelineFormats.RenderTargetFormats[1]	= NormalFormat;
	PSOProperties.PipelineFormats.RenderTargetFormats[2]	= EFormat::Format_R8G8B8A8_Unorm;
	PSOProperties.PipelineFormats.NumRenderTargets			= 3;
	PSOProperties.PipelineFormats.DepthStencilFormat		= DepthBufferFormat;

	GeometryPSO = RenderLayer::CreateGraphicsPipelineState(PSOProperties);
	if (!GeometryPSO)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		GeometryPSO->SetName("GeometryPass PipelineState");
	}

	// LightPass
	if (!ShaderCompiler::CompileFromFile(
		"../DXR-Engine/Shaders/FullscreenVS.hlsl",
		"Main",
		nullptr,
		EShaderStage::ShaderStage_Vertex,
		EShaderModel::ShaderModel_6_0,
		ShaderCode))
	{
		Debug::DebugBreak();
		return false;
	}

	VShader = RenderLayer::CreateVertexShader(ShaderCode);
	if (!VShader)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		VShader->SetName("Fullscreen VertexShader");
	}

	const char* Value = (RenderLayer::IsRayTracingSupported() && GlobalRayTracingEnabled) ? "1" : "0";
	Defines =
	{
		{ "ENABLE_RAYTRACING",	Value },
	};

	if (!ShaderCompiler::CompileFromFile(
		"../DXR-Engine/Shaders/LightPassPS.hlsl",
		"Main",
		&Defines,
		EShaderStage::ShaderStage_Pixel,
		EShaderModel::ShaderModel_6_0,
		ShaderCode))
	{
		Debug::DebugBreak();
		return false;
	}

	PShader = RenderLayer::CreatePixelShader(ShaderCode);
	if (!PShader)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		PShader->SetName("../DXR-Engine/LightPass PixelShader");
	}

	DepthStencilStateInfo.DepthFunc			= EComparisonFunc::ComparisonFunc_Never;
	DepthStencilStateInfo.DepthEnable		= false;
	DepthStencilStateInfo.DepthWriteMask	= EDepthWriteMask::DepthWriteMask_Zero;

	TSharedRef<DepthStencilState> LightDepthStencilState = RenderLayer::CreateDepthStencilState(DepthStencilStateInfo);
	if (!LightDepthStencilState)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		LightDepthStencilState->SetName("LightPass DepthStencilState");
	}

	RasterizerStateInfo.CullMode = ECullMode::CullMode_None;

	TSharedRef<RasterizerState> NoCullRasterizerState = RenderLayer::CreateRasterizerState(RasterizerStateInfo);
	if (!NoCullRasterizerState)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		NoCullRasterizerState->SetName("No Cull RasterizerState");
	}

	PSOProperties.InputLayoutState	= nullptr;
	PSOProperties.DepthStencilState = LightDepthStencilState.Get();
	PSOProperties.RasterizerState	= NoCullRasterizerState.Get();
	PSOProperties.ShaderState.VertexShader	= VShader.Get();
	PSOProperties.ShaderState.PixelShader	= PShader.Get();
	PSOProperties.PipelineFormats.RenderTargetFormats[0]	= FinalTargetFormat;
	PSOProperties.PipelineFormats.NumRenderTargets			= 1;
	PSOProperties.PipelineFormats.DepthStencilFormat		= EFormat::Format_Unknown;

	LightPassPSO = RenderLayer::CreateGraphicsPipelineState(PSOProperties);
	if (!LightPassPSO)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		LightPassPSO->SetName("LightPass PipelineState");
	}

	// SkyboxPass
	if (!ShaderCompiler::CompileFromFile(
		"../DXR-Engine/Shaders/Skybox.hlsl",
		"VSMain",
		nullptr,
		EShaderStage::ShaderStage_Vertex,
		EShaderModel::ShaderModel_6_0,
		ShaderCode))
	{
		Debug::DebugBreak();
		return false;
	}

	VShader = RenderLayer::CreateVertexShader(ShaderCode);
	if (!VShader)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		VShader->SetName("Skybox VertexShader");
	}

	if (!ShaderCompiler::CompileFromFile(
		"../DXR-Engine/Shaders/Skybox.hlsl",
		"PSMain",
		nullptr,
		EShaderStage::ShaderStage_Pixel,
		EShaderModel::ShaderModel_6_0,
		ShaderCode))
	{
		Debug::DebugBreak();
		return false;
	}

	PShader = RenderLayer::CreatePixelShader(ShaderCode);
	if (!PShader)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		PShader->SetName("Skybox PixelShader");
	}

	PSOProperties.InputLayoutState	= StdInputLayout.Get();
	PSOProperties.DepthStencilState	= GeometryDepthStencilState.Get();
	PSOProperties.ShaderState.VertexShader	= VShader.Get();
	PSOProperties.ShaderState.PixelShader	= PShader.Get();
	PSOProperties.PipelineFormats.RenderTargetFormats[0]	= FinalTargetFormat;
	PSOProperties.PipelineFormats.NumRenderTargets			= 1;
	PSOProperties.PipelineFormats.DepthStencilFormat		= DepthBufferFormat;

	SkyboxPSO = RenderLayer::CreateGraphicsPipelineState(PSOProperties);
	if (!SkyboxPSO)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		SkyboxPSO->SetName("SkyboxPSO PipelineState");
	}

	return true;
}

Bool Renderer::InitGBuffer()
{
	// Create image
	if (!InitRayTracingTexture())
	{
		return false;
	}

	const UInt32 Width	= MainWindowViewport->GetWidth();
	const UInt32 Height	= MainWindowViewport->GetHeight();
	const UInt32 Usage	= TextureUsage_Default | TextureUsage_RenderTarget;

	GBuffer[GBUFFER_ALBEDO_INDEX] = RenderLayer::CreateTexture2D(
		nullptr, 
		AlbedoFormat, 
		Usage, 
		Width, 
		Height, 
		1, 1, 
		ClearValue(ColorClearValue(0.0f, 0.0f, 0.0f, 1.0f)));
	if (GBuffer[GBUFFER_ALBEDO_INDEX])
	{
		GBuffer[GBUFFER_ALBEDO_INDEX]->SetName("GBuffer Albedo");

		GBufferSRVs[GBUFFER_ALBEDO_INDEX] = RenderLayer::CreateShaderResourceView(GBuffer[GBUFFER_ALBEDO_INDEX].Get(), AlbedoFormat, 0, 1);
		if (!GBufferSRVs[GBUFFER_ALBEDO_INDEX])
		{
			return false;
		}

		GBufferRTVs[GBUFFER_ALBEDO_INDEX] = RenderLayer::CreateRenderTargetView(GBuffer[GBUFFER_ALBEDO_INDEX].Get(), AlbedoFormat, 0);
		if (!GBufferSRVs[GBUFFER_ALBEDO_INDEX])
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	// Normal
	GBuffer[GBUFFER_NORMAL_INDEX] = RenderLayer::CreateTexture2D(
		nullptr, 
		NormalFormat, 
		Usage, 
		Width, 
		Height, 
		1, 1, 
		ClearValue(ColorClearValue(0.0f, 0.0f, 0.0f, 1.0f)));
	if (GBuffer[GBUFFER_NORMAL_INDEX])
	{
		GBuffer[GBUFFER_NORMAL_INDEX]->SetName("GBuffer Normal");

		GBufferSRVs[GBUFFER_NORMAL_INDEX] = RenderLayer::CreateShaderResourceView(GBuffer[GBUFFER_NORMAL_INDEX].Get(), NormalFormat, 0, 1);
		if (!GBufferSRVs[GBUFFER_NORMAL_INDEX])
		{
			return false;
		}

		GBufferRTVs[GBUFFER_NORMAL_INDEX] = RenderLayer::CreateRenderTargetView(GBuffer[GBUFFER_NORMAL_INDEX].Get(), NormalFormat, 0);
		if (!GBufferSRVs[GBUFFER_NORMAL_INDEX])
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	// Material Properties
	GBuffer[GBUFFER_MATERIAL_INDEX] = RenderLayer::CreateTexture2D(
		nullptr, 
		MaterialFormat, 
		Usage, 
		Width, 
		Height, 
		1, 1, 
		ClearValue(ColorClearValue(0.0f, 0.0f, 0.0f, 1.0f)));
	if (GBuffer[GBUFFER_MATERIAL_INDEX])
	{
		GBuffer[GBUFFER_MATERIAL_INDEX]->SetName("GBuffer Material");

		GBufferSRVs[GBUFFER_MATERIAL_INDEX] = RenderLayer::CreateShaderResourceView(GBuffer[GBUFFER_MATERIAL_INDEX].Get(), MaterialFormat, 0, 1);
		if (!GBufferSRVs[GBUFFER_MATERIAL_INDEX])
		{
			return false;
		}

		GBufferRTVs[GBUFFER_MATERIAL_INDEX] = RenderLayer::CreateRenderTargetView(GBuffer[GBUFFER_MATERIAL_INDEX].Get(), MaterialFormat, 0);
		if (!GBufferSRVs[GBUFFER_MATERIAL_INDEX])
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	// DepthStencil
	const UInt32 UsageDS = TextureUsage_Default | TextureUsage_DSV | TextureUsage_SRV;
	GBuffer[GBUFFER_DEPTH_INDEX] = RenderLayer::CreateTexture2D(
		nullptr, 
		EFormat::Format_R32_Typeless, 
		UsageDS, 
		Width, 
		Height, 
		1, 1, 
		ClearValue(DepthStencilClearValue(1.0f, 0)));
	if (GBuffer[GBUFFER_DEPTH_INDEX])
	{
		GBuffer[GBUFFER_DEPTH_INDEX]->SetName("GBuffer DepthStencil");

		GBufferSRVs[GBUFFER_DEPTH_INDEX] = RenderLayer::CreateShaderResourceView(
			GBuffer[GBUFFER_DEPTH_INDEX].Get(), 
			EFormat::Format_R32_Float, 
			0, 
			1);
		if (!GBufferSRVs[GBUFFER_DEPTH_INDEX])
		{
			return false;
		}

		GBufferDSV = RenderLayer::CreateDepthStencilView(
			GBuffer[GBUFFER_DEPTH_INDEX].Get(),
			DepthBufferFormat, 
			0);
		if (!GBufferDSV)
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	// Final Image
	FinalTarget = RenderLayer::CreateTexture2D(
		nullptr, 
		FinalTargetFormat,
		Usage, 
		Width, 
		Height, 
		1, 1, 
		ClearValue(ColorClearValue(0.0f, 0.0f, 0.0f, 1.0f)));
	if (FinalTarget)
	{
		FinalTarget->SetName("Final Target");

		FinalTargetSRV = RenderLayer::CreateShaderResourceView(FinalTarget.Get(), FinalTargetFormat, 0, 1);
		if (!FinalTargetSRV)
		{
			return false;
		}

		FinalTargetRTV = RenderLayer::CreateRenderTargetView(FinalTarget.Get(), FinalTargetFormat, 0);
		if (!FinalTargetRTV)
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	{
		SamplerStateCreateInfo CreateInfo;
		CreateInfo.AddressU	= ESamplerMode::SamplerMode_Clamp;
		CreateInfo.AddressV	= ESamplerMode::SamplerMode_Clamp;
		CreateInfo.AddressW	= ESamplerMode::SamplerMode_Clamp;
		CreateInfo.Filter	= ESamplerFilter::SamplerFilter_MinMagMipPoint;
		
		GBufferSampler = RenderLayer::CreateSamplerState(CreateInfo);
		if (!GBufferSampler)
		{
			return false;
		}
	}

	return true;
}

Bool Renderer::InitIntegrationLUT()
{
	constexpr UInt32 LUTSize	= 512;
	constexpr EFormat LUTFormat	= EFormat::Format_R16G16_Float;
	if (!RenderLayer::UAVSupportsFormat(EFormat::Format_R16G16_Float))
	{
		LOG_ERROR("[Renderer]: Format_R16G16_Float is not supported for UAVs");

		Debug::DebugBreak();
		return false;
	}

	TSharedRef<Texture2D> StagingTexture = RenderLayer::CreateTexture2D(
		nullptr, 
		LUTFormat, 
		TextureUsage_Default | TextureUsage_UAV, 
		LUTSize, 
		LUTSize, 
		1, 1);
	if (!StagingTexture)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		StagingTexture->SetName("Staging IntegrationLUT");
	}

	TSharedRef<UnorderedAccessView> StagingTextureUAV = RenderLayer::CreateUnorderedAccessView(StagingTexture.Get(), LUTFormat, 0);
	if (!StagingTextureUAV)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		StagingTextureUAV->SetName("IntegrationLUT UAV");
	}

	IntegrationLUT = RenderLayer::CreateTexture2D(
		nullptr, 
		LUTFormat, 
		TextureUsage_Default | TextureUsage_SRV,
		LUTSize, 
		LUTSize, 
		1, 1);
	if (!IntegrationLUT)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		IntegrationLUT->SetName("IntegrationLUT");
	}

	IntegrationLUTSRV = RenderLayer::CreateShaderResourceView(IntegrationLUT.Get(), LUTFormat, 0, 1);
	if (!IntegrationLUTSRV)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		IntegrationLUTSRV->SetName("IntegrationLUT SRV");
	}

	{
		SamplerStateCreateInfo CreateInfo;
		CreateInfo.AddressU	= ESamplerMode::SamplerMode_Clamp;
		CreateInfo.AddressV	= ESamplerMode::SamplerMode_Clamp;
		CreateInfo.AddressW	= ESamplerMode::SamplerMode_Clamp;
		CreateInfo.Filter	= ESamplerFilter::SamplerFilter_MinMagMipPoint;

		IntegrationLUTSampler = RenderLayer::CreateSamplerState(CreateInfo);
		if (!IntegrationLUTSampler)
		{
			return false;
		}
	}

	TArray<UInt8> ShaderCode;
	if (!ShaderCompiler::CompileFromFile(
		"../DXR-Engine/Shaders/BRDFIntegationGen.hlsl",
		"Main",
		nullptr,
		EShaderStage::ShaderStage_Compute,
		EShaderModel::ShaderModel_6_0,
		ShaderCode))
	{
		Debug::DebugBreak();
		return false;
	}

	TSharedRef<ComputeShader> CShader = RenderLayer::CreateComputeShader(ShaderCode);
	if (!CShader)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		CShader->SetName("BRDFIntegationGen ComputeShader");
	}

	ComputePipelineStateCreateInfo PSOInfo;
	PSOInfo.Shader = CShader.Get();

	TSharedRef<ComputePipelineState> BRDFPSO = RenderLayer::CreateComputePipelineState(PSOInfo);
	if (!BRDFPSO)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		BRDFPSO->SetName("BRDFIntegationGen PipelineState");
	}

	CmdList.Begin();

	CmdList.TransitionTexture(
		StagingTexture.Get(),
		EResourceState::ResourceState_Common,
		EResourceState::ResourceState_UnorderedAccess);

	CmdList.BindComputePipelineState(BRDFPSO.Get());

	CmdList.BindUnorderedAccessViews(
		EShaderStage::ShaderStage_Compute,
		&StagingTextureUAV, 1, 0);

	const UInt32 DispatchWidth	= Math::DivideByMultiple(LUTSize, 1);
	const UInt32 DispatchHeight	= Math::DivideByMultiple(LUTSize, 1);
	CmdList.Dispatch(DispatchWidth, DispatchHeight, 1);

	CmdList.UnorderedAccessTextureBarrier(StagingTexture.Get());

	CmdList.TransitionTexture(
		StagingTexture.Get(),
		EResourceState::ResourceState_UnorderedAccess,
		EResourceState::ResourceState_CopySource);

	CmdList.TransitionTexture(
		IntegrationLUT.Get(),
		EResourceState::ResourceState_Common,
		EResourceState::ResourceState_CopyDest);

	CmdList.CopyTexture(IntegrationLUT.Get(), StagingTexture.Get());

	CmdList.TransitionTexture(
		IntegrationLUT.Get(),
		EResourceState::ResourceState_CopyDest,
		EResourceState::ResourceState_PixelShaderResource);

	CmdList.DestroyResource(StagingTexture.Get());
	CmdList.DestroyResource(StagingTextureUAV.Get());
	CmdList.DestroyResource(BRDFPSO.Get());

	CmdList.End();
	GlobalCmdListExecutor.ExecuteCommandList(CmdList);

	return true;
}

Bool Renderer::InitRayTracingTexture()
{
	const UInt32 Width	= MainWindowViewport->GetWidth();
	const UInt32 Height	= MainWindowViewport->GetHeight();
	const UInt32 Usage	= TextureUsage_Default | TextureUsage_RWTexture;
	ReflectionTexture = RenderLayer::CreateTexture2D(
		nullptr, 
		EFormat::Format_R8G8B8A8_Unorm, 
		Usage, 
		Width, 
		Height, 
		1, 1);
	if (!ReflectionTexture)
	{
		return false;
	}

	ReflectionTextureUAV = RenderLayer::CreateUnorderedAccessView(ReflectionTexture.Get(), EFormat::Format_R8G8B8A8_Unorm, 0);
	if (!ReflectionTextureUAV)
	{
		return false;
	}

	ReflectionTextureSRV = RenderLayer::CreateShaderResourceView(ReflectionTexture.Get(), EFormat::Format_R8G8B8A8_Unorm, 0, 1);
	if (!ReflectionTextureSRV)
	{
		return false;
	}
	return true;
}

Bool Renderer::InitDebugStates()
{
	using namespace Microsoft::WRL;

	TArray<UInt8> ShaderCode;
	if (!ShaderCompiler::CompileFromFile(
		"../DXR-Engine/Shaders/Debug.hlsl",
		"VSMain",
		nullptr,
		EShaderStage::ShaderStage_Vertex,
		EShaderModel::ShaderModel_6_0,
		ShaderCode))
	{
		Debug::DebugBreak();
		return false;
	}

	TSharedRef<VertexShader> VShader = RenderLayer::CreateVertexShader(ShaderCode);
	if (!VShader)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		VShader->SetName("Debug VertexShader");
	}

	if (!ShaderCompiler::CompileFromFile(
		"../DXR-Engine/Shaders/Debug.hlsl",
		"PSMain",
		nullptr,
		EShaderStage::ShaderStage_Pixel,
		EShaderModel::ShaderModel_6_0,
		ShaderCode))
	{
		Debug::DebugBreak();
		return false;
	}

	TSharedRef<PixelShader> PShader = RenderLayer::CreatePixelShader(ShaderCode);
	if (!PShader)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		PShader->SetName("Debug PixelShader");
	}

	// Debug Inputlayout
	InputLayoutStateCreateInfo InputLayout =
	{
		{ "POSITION", 0, EFormat::Format_R32G32B32_Float, 0, 0, EInputClassification::InputClassification_Vertex, 0 },
	};

	TSharedRef<InputLayoutState> InputLayoutState = RenderLayer::CreateInputLayout(InputLayout);
	if (!InputLayoutState)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		InputLayoutState->SetName("Debug InputLayoutState");
	}

	DepthStencilStateCreateInfo DepthStencilStateInfo;
	DepthStencilStateInfo.DepthFunc			= EComparisonFunc::ComparisonFunc_LessEqual;
	DepthStencilStateInfo.DepthEnable		= false;
	DepthStencilStateInfo.DepthWriteMask	= EDepthWriteMask::DepthWriteMask_Zero;

	TSharedRef<DepthStencilState> DepthStencilState = RenderLayer::CreateDepthStencilState(DepthStencilStateInfo);
	if (!DepthStencilState)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		DepthStencilState->SetName("Debug DepthStencilState");
	}

	RasterizerStateCreateInfo RasterizerStateInfo;
	RasterizerStateInfo.CullMode = ECullMode::CullMode_None;

	TSharedRef<RasterizerState> RasterizerState = RenderLayer::CreateRasterizerState(RasterizerStateInfo);
	if (!RasterizerState)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		RasterizerState->SetName("Debug RasterizerState");
	}

	// Debug blendstate
	BlendStateCreateInfo BlendStateInfo;

	TSharedRef<BlendState> BlendState = RenderLayer::CreateBlendState(BlendStateInfo);
	if (!BlendState)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		BlendState->SetName("Debug BlendState");
	}

	GraphicsPipelineStateCreateInfo PSOProperties;
	PSOProperties.BlendState								= BlendState.Get();
	PSOProperties.DepthStencilState							= DepthStencilState.Get();
	PSOProperties.InputLayoutState							= InputLayoutState.Get();
	PSOProperties.RasterizerState							= RasterizerState.Get();
	PSOProperties.ShaderState.VertexShader					= VShader.Get();
	PSOProperties.ShaderState.PixelShader					= PShader.Get();
	PSOProperties.PrimitiveTopologyType						= EPrimitiveTopologyType::PrimitiveTopologyType_Line;
	PSOProperties.PipelineFormats.RenderTargetFormats[0]	= RenderTargetFormat;
	PSOProperties.PipelineFormats.NumRenderTargets			= 1;
	PSOProperties.PipelineFormats.DepthStencilFormat		= DepthBufferFormat;

	DebugBoxPSO = RenderLayer::CreateGraphicsPipelineState(PSOProperties);
	if (!DebugBoxPSO)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		DebugBoxPSO->SetName("Debug PipelineState");
	}

	// Create VertexBuffer
	XMFLOAT3 Vertices[8] =
	{
		{ -0.5f, -0.5f,  0.5f },
		{  0.5f, -0.5f,  0.5f },
		{ -0.5f,  0.5f,  0.5f },
		{  0.5f,  0.5f,  0.5f },

		{  0.5f, -0.5f, -0.5f },
		{ -0.5f, -0.5f, -0.5f },
		{  0.5f,  0.5f, -0.5f },
		{ -0.5f,  0.5f, -0.5f }
	};

	ResourceData VertexData(Vertices);

	AABBVertexBuffer = RenderLayer::CreateVertexBuffer<XMFLOAT3>(&VertexData, 8, BufferUsage_Default);
	if (!AABBVertexBuffer)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		AABBVertexBuffer->SetName("AABB VertexBuffer");
	}

	// Create IndexBuffer
	UInt16 Indices[24] =
	{
		0, 1,
		1, 3,
		3, 2,
		2, 0,
		1, 4,
		3, 6,
		6, 4,
		4, 5,
		5, 7,
		7, 6,
		0, 5,
		2, 7,
	};

	ResourceData IndexData(Indices);

	AABBIndexBuffer = RenderLayer::CreateIndexBuffer(
		&IndexData, 
		sizeof(UInt16) * 24, 
		EIndexFormat::IndexFormat_UInt16, 
		BufferUsage_Default);
	if (!AABBIndexBuffer)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		AABBIndexBuffer->SetName("AABB IndexBuffer");
	}

	return true;
}

Bool Renderer::InitAA()
{
	using namespace Microsoft::WRL;

	// No AA
	TArray<UInt8> ShaderCode;
	if (!ShaderCompiler::CompileFromFile(
		"../DXR-Engine/Shaders/FullscreenVS.hlsl",
		"Main",
		nullptr,
		EShaderStage::ShaderStage_Vertex,
		EShaderModel::ShaderModel_6_0,
		ShaderCode))
	{
		Debug::DebugBreak();
		return false;
	}

	TSharedRef<VertexShader> VShader = RenderLayer::CreateVertexShader(ShaderCode);
	if (!VShader)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		VShader->SetName("Fullscreen VertexShader");
	}

	if (!ShaderCompiler::CompileFromFile(
		"../DXR-Engine/Shaders/PostProcessPS.hlsl",
		"Main",
		nullptr,
		EShaderStage::ShaderStage_Pixel,
		EShaderModel::ShaderModel_6_0,
		ShaderCode))
	{
		Debug::DebugBreak();
		return false;
	}

	TSharedRef<PixelShader> PShader = RenderLayer::CreatePixelShader(ShaderCode);
	if (!PShader)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		PShader->SetName("PostProcess PixelShader");
	}

	DepthStencilStateCreateInfo DepthStencilStateInfo;
	DepthStencilStateInfo.DepthFunc			= EComparisonFunc::ComparisonFunc_Always;
	DepthStencilStateInfo.DepthEnable		= false;
	DepthStencilStateInfo.DepthWriteMask	= EDepthWriteMask::DepthWriteMask_Zero;

	TSharedRef<DepthStencilState> DepthStencilState = RenderLayer::CreateDepthStencilState(DepthStencilStateInfo);
	if (!DepthStencilState)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		DepthStencilState->SetName("PostProcess DepthStencilState");
	}

	RasterizerStateCreateInfo RasterizerStateInfo;
	RasterizerStateInfo.CullMode = ECullMode::CullMode_None;

	TSharedRef<RasterizerState> RasterizerState = RenderLayer::CreateRasterizerState(RasterizerStateInfo);
	if (!RasterizerState)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		RasterizerState->SetName("PostProcess RasterizerState");
	}

	BlendStateCreateInfo BlendStateInfo;
	BlendStateInfo.IndependentBlendEnable		= false;
	BlendStateInfo.RenderTarget[0].BlendEnable	= false;

	TSharedRef<BlendState> BlendState = RenderLayer::CreateBlendState(BlendStateInfo);
	if (!BlendState)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		BlendState->SetName("PostProcess BlendState");
	}

	GraphicsPipelineStateCreateInfo PSOProperties;
	PSOProperties.InputLayoutState							= nullptr;
	PSOProperties.BlendState								= BlendState.Get();
	PSOProperties.DepthStencilState							= DepthStencilState.Get();
	PSOProperties.RasterizerState							= RasterizerState.Get();
	PSOProperties.ShaderState.VertexShader					= VShader.Get();
	PSOProperties.ShaderState.PixelShader					= PShader.Get();
	PSOProperties.PrimitiveTopologyType						= EPrimitiveTopologyType::PrimitiveTopologyType_Triangle;
	PSOProperties.PipelineFormats.RenderTargetFormats[0]	= EFormat::Format_R8G8B8A8_Unorm;
	PSOProperties.PipelineFormats.NumRenderTargets			= 1;
	PSOProperties.PipelineFormats.DepthStencilFormat		= EFormat::Format_Unknown;

	PostPSO = RenderLayer::CreateGraphicsPipelineState(PSOProperties);
	if (!PostPSO)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		PostPSO->SetName("PostProcess PipelineState");
	}

	// FXAA
	if (!ShaderCompiler::CompileFromFile(
		"../DXR-Engine/Shaders/FXAA_PS.hlsl",
		"Main",
		nullptr,
		EShaderStage::ShaderStage_Pixel,
		EShaderModel::ShaderModel_6_0,
		ShaderCode))
	{
		Debug::DebugBreak();
		return false;
	}

	PShader = RenderLayer::CreatePixelShader(ShaderCode);
	if (!PShader)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		PShader->SetName("FXAA PixelShader");
	}

	PSOProperties.ShaderState.PixelShader = PShader.Get();

	FXAAPSO = RenderLayer::CreateGraphicsPipelineState(PSOProperties);
	if (!FXAAPSO)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		FXAAPSO->SetName("FXAA PipelineState");
	}

	return true;
}

Bool Renderer::InitForwardPass()
{
	using namespace Microsoft::WRL;

	// Init PipelineState
	TArray<ShaderDefine> Defines =
	{
		{ "ENABLE_PARALLAX_MAPPING", "1" },
		{ "ENABLE_NORMAL_MAPPING",	 "1" },
	};

	TArray<UInt8> ShaderCode;
	if (!ShaderCompiler::CompileFromFile(
		"../DXR-Engine/Shaders/ForwardPass.hlsl",
		"VSMain",
		&Defines,
		EShaderStage::ShaderStage_Vertex,
		EShaderModel::ShaderModel_6_0,
		ShaderCode))
	{
		Debug::DebugBreak();
		return false;
	}

	TSharedRef<VertexShader> VShader = RenderLayer::CreateVertexShader(ShaderCode);
	if (!VShader)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		VShader->SetName("ForwardPass VertexShader");
	}

	if (!ShaderCompiler::CompileFromFile(
		"../DXR-Engine/Shaders/ForwardPass.hlsl",
		"PSMain",
		&Defines,
		EShaderStage::ShaderStage_Pixel,
		EShaderModel::ShaderModel_6_0,
		ShaderCode))
	{
		Debug::DebugBreak();
		return false;
	}

	TSharedRef<PixelShader> PShader = RenderLayer::CreatePixelShader(ShaderCode);
	if (!PShader)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		PShader->SetName("ForwardPass PixelShader");
	}

	DepthStencilStateCreateInfo DepthStencilStateInfo;
	DepthStencilStateInfo.DepthFunc			= EComparisonFunc::ComparisonFunc_LessEqual;
	DepthStencilStateInfo.DepthEnable		= true;
	DepthStencilStateInfo.DepthWriteMask	= EDepthWriteMask::DepthWriteMask_All;

	TSharedRef<DepthStencilState> DepthStencilState = RenderLayer::CreateDepthStencilState(DepthStencilStateInfo);
	if (!DepthStencilState)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		DepthStencilState->SetName("ForwardPass DepthStencilState");
	}

	RasterizerStateCreateInfo RasterizerStateInfo;
	RasterizerStateInfo.CullMode = ECullMode::CullMode_None;

	TSharedRef<RasterizerState> RasterizerState = RenderLayer::CreateRasterizerState(RasterizerStateInfo);
	if (!RasterizerState)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		RasterizerState->SetName("ForwardPass RasterizerState");
	}

	BlendStateCreateInfo BlendStateInfo;
	BlendStateInfo.IndependentBlendEnable		= false;
	BlendStateInfo.RenderTarget[0].BlendEnable	= true;

	TSharedRef<BlendState> BlendState = RenderLayer::CreateBlendState(BlendStateInfo);
	if (!BlendState)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		BlendState->SetName("ForwardPass BlendState");
	}

	GraphicsPipelineStateCreateInfo PSOProperties = { };
	PSOProperties.ShaderState.VertexShader					= VShader.Get();
	PSOProperties.ShaderState.PixelShader					= PShader.Get();
	PSOProperties.InputLayoutState							= StdInputLayout.Get();
	PSOProperties.DepthStencilState							= DepthStencilState.Get();
	PSOProperties.BlendState								= BlendState.Get();
	PSOProperties.RasterizerState							= RasterizerState.Get();
	PSOProperties.PipelineFormats.RenderTargetFormats[0]	= MainWindowViewport->GetColorFormat();
	PSOProperties.PipelineFormats.NumRenderTargets			= 1;
	PSOProperties.PipelineFormats.DepthStencilFormat		= DepthBufferFormat;
	PSOProperties.PrimitiveTopologyType						= EPrimitiveTopologyType::PrimitiveTopologyType_Triangle;

	ForwardPSO = RenderLayer::CreateGraphicsPipelineState(PSOProperties);
	if (!ForwardPSO)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		ForwardPSO->SetName("Forward PipelineState");
	}

	return true;
}

Bool Renderer::InitSSAO()
{
	using namespace Microsoft::WRL;

	if (!InitSSAO_RenderTarget())
	{
		return false;
	}

	// Load shader
	TArray<UInt8> ShaderCode;
	if (!ShaderCompiler::CompileFromFile(
		"../DXR-Engine/Shaders/SSAO.hlsl",
		"Main",
		nullptr,
		EShaderStage::ShaderStage_Compute,
		EShaderModel::ShaderModel_6_0,
		ShaderCode))
	{
		Debug::DebugBreak();
		return false;
	}

	TSharedRef<ComputeShader> CShader = RenderLayer::CreateComputeShader(ShaderCode);
	if (!CShader)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		CShader->SetName("SSAO Shader");
	}

	// Init PipelineState
	{
		ComputePipelineStateCreateInfo PSOProperties = { };
		PSOProperties.Shader = CShader.Get();

		SSAOPSO = RenderLayer::CreateComputePipelineState(PSOProperties);
		if (!SSAOPSO)
		{
			Debug::DebugBreak();
			return false;
		}
		else
		{
			SSAOPSO->SetName("SSAO PipelineState");
		}
	}

	// Generate SSAO Kernel
	std::uniform_real_distribution<Float> RandomFloats(0.0f, 1.0f);
	std::default_random_engine Generator;
	TArray<XMFLOAT3> SSAOKernel;
	for (UInt32 i = 0; i < 64; ++i)
	{
		XMVECTOR XmSample = XMVectorSet(
			RandomFloats(Generator) * 2.0f - 1.0f,
			RandomFloats(Generator) * 2.0f - 1.0f,
			RandomFloats(Generator), 
			0.0f);

		Float Scale = RandomFloats(Generator);
		XmSample = XMVector3Normalize(XmSample);
		XmSample = XMVectorScale(XmSample, Scale);

		Scale = Float(i) / 64.0f;
		Scale = Math::Lerp(0.1f, 1.0f, Scale * Scale);
		XmSample = XMVectorScale(XmSample, Scale);

		XMFLOAT3 Sample;
		XMStoreFloat3(&Sample, XmSample);
		SSAOKernel.PushBack(Sample);
	}

	// Generate noise
	TArray<Float16> SSAONoise;
	for (UInt32 i = 0; i < 16; i++)
	{
		const Float x = RandomFloats(Generator) * 2.0f - 1.0f;
		const Float y = RandomFloats(Generator) * 2.0f - 1.0f;
		SSAONoise.EmplaceBack(x);
		SSAONoise.EmplaceBack(y);
		SSAONoise.EmplaceBack(0.0f);
		SSAONoise.EmplaceBack(0.0f);
	}

	// Init texture
	{
		SSAONoiseTex = RenderLayer::CreateTexture2D(
			nullptr,
			EFormat::Format_R16G16B16A16_Float,
			TextureUsage_SRV,
			4,
			4,
			1, 1);
		if (!SSAONoiseTex)
		{
			Debug::DebugBreak();
			return false;
		}
		else
		{
			SSAONoiseTex->SetName("SSAO Noise Texture");
		}

		SSAONoiseSRV = RenderLayer::CreateShaderResourceView(
			SSAONoiseTex.Get(),
			EFormat::Format_R16G16B16A16_Float,
			0, 1);
		if (!SSAONoiseSRV)
		{
			Debug::DebugBreak();
			return false;
		}
		else
		{
			SSAOBufferSRV->SetName("SSAO Noise Texture SRV");
		}
	}

	CmdList.Begin();
	CmdList.TransitionTexture(
		SSAOBuffer.Get(), 
		EResourceState::ResourceState_Common, 
		EResourceState::ResourceState_PixelShaderResource);

	CmdList.TransitionTexture(
		SSAONoiseTex.Get(),
		EResourceState::ResourceState_Common,
		EResourceState::ResourceState_CopyDest);

	CmdList.UpdateTexture2D(
		SSAONoiseTex.Get(),
		4, 4, 0,
		SSAONoise.Data());

	CmdList.TransitionTexture(
		SSAONoiseTex.Get(),
		EResourceState::ResourceState_CopyDest,
		EResourceState::ResourceState_NonPixelShaderResource);

	CmdList.End();
	GlobalCmdListExecutor.ExecuteCommandList(CmdList);

	// Init samples
	{
		const UInt32 Stride			= sizeof(XMFLOAT3);
		const UInt32 SizeInBytes	= Stride * SSAOKernel.Size();
		ResourceData SSAOSampleData(SSAOKernel.Data());
		SSAOSamples = RenderLayer::CreateStructuredBuffer(
			&SSAOSampleData,
			SizeInBytes,
			Stride,
			BufferUsage_SRV | BufferUsage_Default);
		if (!SSAOSamples)
		{
			Debug::DebugBreak();
			return false;
		}
		else
		{
			SSAOSamples->SetName("SSAO Samples");
		}
	}

	SSAOSamplesSRV = RenderLayer::CreateShaderResourceView(
		SSAOSamples.Get(),
		0, SSAOKernel.Size(), 
		sizeof(XMFLOAT3));
	if (!SSAOSamplesSRV)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		SSAOSamplesSRV->SetName("SSAO Samples SRV");
	}

	// Load shader
	if (!ShaderCompiler::CompileFromFile(
		"../DXR-Engine/Shaders/Blur.hlsl",
		"Main",
		nullptr,
		EShaderStage::ShaderStage_Compute,
		EShaderModel::ShaderModel_6_0,
		ShaderCode))
	{
		Debug::DebugBreak();
		return false;
	}

	CShader = RenderLayer::CreateComputeShader(ShaderCode);
	if (!CShader)
	{
		Debug::DebugBreak();
		return false;
	}
	else
	{
		CShader->SetName("SSAO Blur Shader");
	}

	{
		// Init PipelineState
		ComputePipelineStateCreateInfo PSOProperties = { };
		PSOProperties.Shader = CShader.Get();

		SSAOBlur = RenderLayer::CreateComputePipelineState(PSOProperties);
		if (!SSAOBlur)
		{
			Debug::DebugBreak();
			return false;
		}
	}

	return true;
}

Bool Renderer::InitSSAO_RenderTarget()
{
	// Init texture
	{
		const UInt32 Width	= MainWindowViewport->GetWidth();
		const UInt32 Height	= MainWindowViewport->GetHeight();
		const UInt32 Usage	= TextureUsage_Default | TextureUsage_RWTexture;

		SSAOBuffer = RenderLayer::CreateTexture2D(
			nullptr,
			SSAOBufferFormat,
			Usage,
			Width,
			Height,
			1, 1);
		if (!SSAOBuffer)
		{
			Debug::DebugBreak();
			return false;
		}
		else
		{
			SSAOBuffer->SetName("SSAO Buffer");
		}

		SSAOBufferUAV = RenderLayer::CreateUnorderedAccessView(
			SSAOBuffer.Get(),
			SSAOBufferFormat,
			0);
		if (!SSAOBufferUAV)
		{
			Debug::DebugBreak();
			return false;
		}
		else
		{
			SSAOBufferUAV->SetName("SSAO Buffer UAV");
		}

		SSAOBufferSRV = RenderLayer::CreateShaderResourceView(
			SSAOBuffer.Get(),
			SSAOBufferFormat,
			0, 1);
		if (!SSAOBufferSRV)
		{
			Debug::DebugBreak();
			return false;
		}
		else
		{
			SSAOBufferSRV->SetName("SSAO Buffer SRV");
		}
	}

	return true;
}

Bool Renderer::CreateShadowMaps()
{
	DirLightShadowMaps = RenderLayer::CreateTexture2D(
		nullptr,
		ShadowMapFormat,
		TextureUsage_ShadowMap,
		CurrentLightSettings.ShadowMapWidth,
		CurrentLightSettings.ShadowMapHeight,
		1, 1,
		ClearValue(DepthStencilClearValue(1.0f, 0)));
	if (DirLightShadowMaps)
	{
		DirLightShadowMaps->SetName("Directional Light ShadowMaps");

		DirLightShadowMapDSV = RenderLayer::CreateDepthStencilView(
			DirLightShadowMaps.Get(),
			ShadowMapFormat,
			0);
		if (!DirLightShadowMapDSV)
		{
			Debug::DebugBreak();
		}

#if !ENABLE_VSM
		DirLightShadowMapSRV = RenderLayer::CreateShaderResourceView(
			DirLightShadowMaps.Get(),
			EFormat::Format_R32_Float,
			0, 1);
		if (!DirLightShadowMapSRV)
		{
			Debug::DebugBreak();
		}
#endif
	}
	else
	{
		return false;
	}

#if ENABLE_VSM
	VSMDirLightShadowMaps = RenderLayer::CreateTexture2D(
		nullptr,
		EFormat::Format_R32G32_Float,
		TextureUsage_RenderTarget,
		Renderer::GetGlobalLightSettings().ShadowMapWidth,
		Renderer::GetGlobalLightSettings().ShadowMapHeight,
		1, 1,
		ClearValue(ColorClearValue(1.0f, 1.0f, 1.0f, 1.0f)));
	if (VSMDirLightShadowMaps)
	{
		VSMDirLightShadowMaps->SetName("Directional Light VSM");

		VSMDirLightShadowMapRTV = RenderLayer::CreateRenderTargetView(
			VSMDirLightShadowMaps.Get(),
			EFormat::Format_R32G32_Float,
			0);
		if (!VSMDirLightShadowMapRTV)
		{
			Debug::DebugBreak();
		}

		VSMDirLightShadowMapSRV = RenderLayer::CreateShaderResourceView(
			VSMDirLightShadowMaps.Get(),
			EFormat::Format_R32G32_Float,
			0, 1);
		if (!VSMDirLightShadowMapSRV)
		{
			Debug::DebugBreak();
		}
	}
	else
	{
		return false;
	}
#endif

	const UInt16 Size = CurrentLightSettings.PointLightShadowSize;
	PointLightShadowMaps = RenderLayer::CreateTextureCube(
		nullptr,
		ShadowMapFormat,
		TextureUsage_ShadowMap,
		Size,
		1, 1,
		ClearValue(DepthStencilClearValue(1.0f, 0)));
	if (PointLightShadowMaps)
	{
		PointLightShadowMaps->SetName("PointLight ShadowMaps");

		PointLightShadowMapsDSVs.Resize(6);
		for (UInt32 i = 0; i < 6; i++)
		{
			PointLightShadowMapsDSVs[i] = RenderLayer::CreateDepthStencilView(
				PointLightShadowMaps.Get(),
				ShadowMapFormat,
				0, i);
		}

		PointLightShadowMapsSRV = RenderLayer::CreateShaderResourceView(
			PointLightShadowMaps.Get(),
			EFormat::Format_R32_Float,
			0, 1);
	}
	else
	{
		return false;
	}

	return true;
}

void Renderer::GenerateIrradianceMap(
	ShaderResourceView* SourceSRV,
	TextureCube* Source,
	UnorderedAccessView* DestUAV,
	TextureCube* Dest,
	CommandList& InCmdList)
{
	const UInt32 Size = static_cast<UInt32>(Dest->GetWidth());

	// Create irradiancemap if it is not created
	if (!IrradicanceGenPSO)
	{
		TArray<UInt8> Code;
		if (!ShaderCompiler::CompileFromFile(
			"../DXR-Engine/Shaders/IrradianceGen.hlsl",
			"Main",
			nullptr,
			EShaderStage::ShaderStage_Compute,
			EShaderModel::ShaderModel_6_0,
			Code))
		{
			LOG_ERROR("Failed to compile IrradianceGen Shader");
			Debug::DebugBreak();
		}

		IrradianceGenShader = RenderLayer::CreateComputeShader(Code);
		if (!IrradianceGenShader)
		{
			LOG_ERROR("Failed to create IrradianceGen Shader");
			Debug::DebugBreak();
		}

		IrradicanceGenPSO = RenderLayer::CreateComputePipelineState(ComputePipelineStateCreateInfo(IrradianceGenShader.Get()));
		if (!IrradicanceGenPSO)
		{
			LOG_ERROR("Failed to create IrradianceGen PipelineState");
			Debug::DebugBreak();
		}
		else
		{
			IrradicanceGenPSO->SetName("Irradiance Gen PSO");
		}
	}

	InCmdList.TransitionTexture(
		Source, 
		EResourceState::ResourceState_PixelShaderResource, 
		EResourceState::ResourceState_NonPixelShaderResource);
	
	InCmdList.TransitionTexture(
		Dest, 
		EResourceState::ResourceState_Common, 
		EResourceState::ResourceState_UnorderedAccess);

	InCmdList.BindComputePipelineState(IrradicanceGenPSO.Get());

	InCmdList.BindShaderResourceViews(
		EShaderStage::ShaderStage_Compute, 
		&SourceSRV, 1, 0);

	InCmdList.BindUnorderedAccessViews(
		EShaderStage::ShaderStage_Compute,
		&DestUAV, 1, 0);

	InCmdList.Dispatch(Size, Size, 6);

	InCmdList.UnorderedAccessTextureBarrier(Dest);

	InCmdList.TransitionTexture(
		Source, 
		EResourceState::ResourceState_NonPixelShaderResource, 
		EResourceState::ResourceState_PixelShaderResource);
	
	InCmdList.TransitionTexture(
		Dest, 
		EResourceState::ResourceState_UnorderedAccess,
		EResourceState::ResourceState_PixelShaderResource);
}

void Renderer::GenerateSpecularIrradianceMap(
	ShaderResourceView* SourceSRV,
	TextureCube* Source, 
	UnorderedAccessView* const * DestUAVs,
	UInt32 NumDestUAVs,
	TextureCube* Dest, 
	CommandList& InCmdList)
{
	const UInt32 Miplevels = Dest->GetMipLevels();
	VALIDATE(Miplevels == NumDestUAVs);

	if (!SpecIrradicanceGenPSO)
	{
		TArray<UInt8> Code;
		if (!ShaderCompiler::CompileFromFile(
			"../DXR-Engine/Shaders/SpecularIrradianceGen.hlsl",
			"Main",
			nullptr,
			EShaderStage::ShaderStage_Compute,
			EShaderModel::ShaderModel_6_0,
			Code))
		{
			LOG_ERROR("Failed to compile SpecularIrradianceGen Shader");
			Debug::DebugBreak();
		}

		SpecIrradianceGenShader = RenderLayer::CreateComputeShader(Code);
		if (!SpecIrradianceGenShader)
		{
			LOG_ERROR("Failed to create SpecularIrradianceGen Shader");
			Debug::DebugBreak();
		}

		SpecIrradicanceGenPSO = RenderLayer::CreateComputePipelineState(ComputePipelineStateCreateInfo(SpecIrradianceGenShader.Get()));
		if (!SpecIrradicanceGenPSO)
		{
			LOG_ERROR("Failed to create SpecularIrradianceGen PipelineState");
			Debug::DebugBreak();
		}
		else
		{
			SpecIrradicanceGenPSO->SetName("Specular Irradiance Gen PSO");
		}
	}

	InCmdList.TransitionTexture(
		Source, 
		EResourceState::ResourceState_PixelShaderResource, 
		EResourceState::ResourceState_NonPixelShaderResource);
	
	InCmdList.TransitionTexture(
		Dest, 
		EResourceState::ResourceState_Common, 
		EResourceState::ResourceState_UnorderedAccess);

	InCmdList.BindShaderResourceViews(
		EShaderStage::ShaderStage_Compute,
		&SourceSRV, 1, 0);

	InCmdList.BindComputePipelineState(SpecIrradicanceGenPSO.Get());

	UInt32 Width	= static_cast<UInt32>(Dest->GetWidth());
	Float Roughness	= 0.0f;

	const Float RoughnessDelta = 1.0f / (Miplevels - 1);
	for (UInt32 Mip = 0; Mip < Miplevels; Mip++)
	{
		InCmdList.Bind32BitShaderConstants(
			EShaderStage::ShaderStage_Compute,
			&Roughness, 1);

		InCmdList.BindUnorderedAccessViews(
			EShaderStage::ShaderStage_Compute,
			&DestUAVs[Mip], 1, 0);
		
		InCmdList.Dispatch(Width, Width, 6);
		InCmdList.UnorderedAccessTextureBarrier(Dest);

		Width = std::max<UInt32>(Width / 2, 1U);
		Roughness += RoughnessDelta;
	}

	InCmdList.TransitionTexture(
		Source, 
		EResourceState::ResourceState_NonPixelShaderResource, 
		EResourceState::ResourceState_PixelShaderResource);
	
	InCmdList.TransitionTexture(
		Dest, 
		EResourceState::ResourceState_UnorderedAccess,
		EResourceState::ResourceState_PixelShaderResource);
}
