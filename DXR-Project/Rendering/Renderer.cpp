#include "Renderer.h"
#include "TextureFactory.h"
#include "DebugUI.h"
#include "Mesh.h"

#include "Scene/PointLight.h"
#include "Scene/DirectionalLight.h"
#include "Scene/Frustum.h"

#include "D3D12/D3D12Texture.h"
#include "D3D12/D3D12Views.h"
#include "D3D12/D3D12RootSignature.h"
#include "D3D12/D3D12GraphicsPipelineState.h"
#include "D3D12/D3D12RayTracingPipelineState.h"
#include "D3D12/D3D12ComputePipelineState.h"
#include "D3D12/D3D12ShaderCompiler.h"

#include <algorithm>

/*
* Static Settings
*/
TUniquePtr<Renderer>	Renderer::RendererInstance = nullptr;
LightSettings			Renderer::GlobalLightSettings;

static const EFormat	RenderTargetFormat		= EFormat::Format_R8G8B8A8_Unorm;
static const EFormat	NormalFormat			= EFormat::Format_R10G10B10A2_Unorm;
static const EFormat	DepthBufferFormat		= EFormat::Format_D32_Float;
static const EFormat	ShadowMapFormat			= EFormat::Format_D32_Float;
static const Uint32		ShadowMapSampleCount	= 2;

/*
* Renderer
*/
Renderer::Renderer()
{
}

Renderer::~Renderer()
{
}

void Renderer::Tick(const Scene& CurrentScene)
{
	if (FrustumCullEnabled)
	{
		VisibleCommands.Clear();
		
		Camera* Camera = CurrentScene.GetCamera();
		Frustum CameraFrustum = Frustum(Camera->GetFarPlane(), Camera->GetViewMatrix(), Camera->GetProjectionMatrix());
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
				VisibleCommands.EmplaceBack(Command);
			}
		}
	}
	else
	{
		VisibleCommands = CurrentScene.GetMeshDrawCommands();
	}

	// Start frame
	D3D12Texture* BackBuffer = RenderingAPI::Get().GetSwapChain()->GetSurfaceResource(CurrentBackBufferIndex);

	CmdList.Begin();

	// UpdateLightBuffers
	CommandList->TransitionBarrier(PointLightBuffer.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
	CommandList->TransitionBarrier(DirectionalLightBuffer.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);

	Uint32 NumPointLights	= 0;
	Uint32 NumDirLights		= 0;
	for (Light* Light : CurrentScene.GetLights())
	{
		XMFLOAT3	Color		= Light->GetColor();
		Float32		Intensity	= Light->GetIntensity();
		if (IsSubClassOf<PointLight>(Light))
		{
			PointLight* PoiLight = Cast<PointLight>(Light);
			VALIDATE(PoiLight != nullptr);

			PointLightProperties Properties;
			Properties.Color			= XMFLOAT3(Color.x * Intensity, Color.y * Intensity, Color.z * Intensity);
			Properties.Position			= PoiLight->GetPosition();
			Properties.ShadowBias		= PoiLight->GetShadowBias();
			Properties.MaxShadowBias	= PoiLight->GetMaxShadowBias();
			Properties.FarPlane			= PoiLight->GetShadowFarPlane();

			constexpr Uint32 SizeInBytes = sizeof(PointLightProperties);
			CmdList.UpdateBuffer(PointLightBuffer.Get(), 0, NumPointLights * SizeInBytes, &Properties);

			NumPointLights++;
		}
		else if (IsSubClassOf<DirectionalLight>(Light))
		{
			DirectionalLight* DirLight = Cast<DirectionalLight>(Light);
			VALIDATE(DirLight != nullptr);

			DirectionalLightProperties Properties;
			Properties.Color			= XMFLOAT3(Color.x * Intensity, Color.y * Intensity, Color.z * Intensity);
			Properties.ShadowBias		= DirLight->GetShadowBias();
			Properties.Direction		= DirLight->GetDirection();
			Properties.LightMatrix		= DirLight->GetMatrix();
			Properties.MaxShadowBias	= DirLight->GetMaxShadowBias();

			constexpr Uint32 SizeInBytes = sizeof(DirectionalLightProperties);
			CmdList.UpdateBuffer(DirectionalLightBuffer.Get(), 0, NumDirLights * SizeInBytes, &Properties);

			NumDirLights++;
		}
	}

	CommandList->TransitionBarrier(PointLightBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	CommandList->TransitionBarrier(DirectionalLightBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	// Set constant buffer descriptor heap
	CommandList->BindGlobalOnlineDescriptorHeaps();

	// Transition GBuffer
	CommandList->TransitionBarrier(GBuffer[0].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	CommandList->TransitionBarrier(GBuffer[1].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	CommandList->TransitionBarrier(GBuffer[2].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	CommandList->TransitionBarrier(GBuffer[3].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	// Transition ShadowMaps
	CommandList->TransitionBarrier(PointLightShadowMaps.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	CommandList->TransitionBarrier(DirLightShadowMaps.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	
	// Render DirectionalLight ShadowMaps
	CmdList.ClearDepthStencilView(DirLightShadowMaps->GetDepthStencilView(0).Get(), DepthStencilClearValue(1.0f, 0));

#if ENABLE_VSM
	CommandList->TransitionBarrier(VSMDirLightShadowMaps.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);

	Float32 DepthClearColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	CommandList->ClearRenderTargetView(VSMDirLightShadowMaps->GetRenderTargetView(0).Get(), DepthClearColor);
	
	D3D12RenderTargetView* DirLightRTVS[] =
	{
		VSMDirLightShadowMaps->GetRenderTargetView(0).Get(),
	};
	CommandList->OMSetRenderTargets(DirLightRTVS, 1, DirLightShadowMaps->GetDepthStencilView(0).Get());
	CommandList->SetPipelineState(VSMShadowMapPSO->GetPipelineState());
#else
	CmdList.BindRenderTargets(nullptr, 0, DirLightShadowMaps->GetDepthStencilView(0).Get());
	CmdList.BindGraphicsPipelineState(ShadowMapPSO.Get());
#endif

	// Setup view
	Viewport ViewPort = { };
	ViewPort.Width		= static_cast<Float32>(Renderer::GetGlobalLightSettings().ShadowMapWidth);
	ViewPort.Height		= static_cast<Float32>(Renderer::GetGlobalLightSettings().ShadowMapHeight);
	ViewPort.MinDepth	= 0.0f;
	ViewPort.MaxDepth	= 1.0f;
	ViewPort.x			= 0.0f;
	ViewPort.y			= 0.0f;
	CmdList.BindViewport(ViewPort, 1);

	ScissorRect ScissorRect =
	{
		0,
		0,
		static_cast<LONG>(Renderer::GetGlobalLightSettings().ShadowMapWidth),
		static_cast<LONG>(Renderer::GetGlobalLightSettings().ShadowMapHeight)
	};
	CmdList.BindScissorRect(ScissorRect, 1);
	CmdList.BindPrimitiveTopology(EPrimitveTopologyType::PrimitveTopologyType_Triangle);

	// PerObject Structs
	struct PerObject
	{
		XMFLOAT4X4 Matrix;
	} PerObjectBuffer;

	struct PerLight
	{
		XMFLOAT4X4	Matrix;
		XMFLOAT3	Position;
		Float32		FarPlane;
	} PerLightBuffer;

	D3D12_VERTEX_BUFFER_VIEW VBO = { };
	D3D12_INDEX_BUFFER_VIEW IBV = { };
	for (Light* Light : CurrentScene.GetLights())
	{
		if (IsSubClassOf<DirectionalLight>(Light))
		{
			DirectionalLight* DirLight = Cast<DirectionalLight>(Light);
			PerLightBuffer.Matrix	= DirLight->GetMatrix();
			PerLightBuffer.Position	= DirLight->GetShadowMapPosition();
			PerLightBuffer.FarPlane	= DirLight->GetShadowFarPlane();
			CommandList->SetGraphicsRoot32BitConstants(&PerLightBuffer, 20, 0, 1);

			// Draw all objects to depthbuffer
			for (const MeshDrawCommand& Command : CurrentScene.GetMeshDrawCommands())
			{
				CmdList.BindVertexBuffers(&Command.VertexBuffer, 1, 0);
				CmdList.BindIndexBuffer(Command.IndexBuffer);

				PerObjectBuffer.Matrix = Command.CurrentActor->GetTransform().GetMatrix();
				CommandList->SetGraphicsRoot32BitConstants(&PerObjectBuffer, 16, 0, 0);

				CmdList.DrawIndexedInstanced(Command.IndexCount, 1, 0, 0, 0);
			}

			break;
		}
	}

	// Render PointLight ShadowMaps
	const Uint32 PointLightShadowSize = Renderer::GetGlobalLightSettings().PointLightShadowSize;
	ViewPort.Width		= static_cast<Float32>(PointLightShadowSize);
	ViewPort.Height		= static_cast<Float32>(PointLightShadowSize);
	ViewPort.MinDepth	= 0.0f;
	ViewPort.MaxDepth	= 1.0f;
	ViewPort.x			= 0.0f;
	ViewPort.y			= 0.0f;
	CmdList.BindViewport(ViewPort, 1);

	ScissorRect =
	{
		0,
		0,
		static_cast<LONG>(PointLightShadowSize),
		static_cast<LONG>(PointLightShadowSize)
	};
	CmdList.BindScissorRect(ScissorRect, 1);

	CmdList.BindGraphicsPipelineState(LinearShadowMapPSO.Get());
	for (Light* Light : CurrentScene.GetLights())
	{
		if (IsSubClassOf<PointLight>(Light))
		{
			PointLight* PoiLight = Cast<PointLight>(Light);
			for (Uint32 I = 0; I < 6; I++)
			{
				CommandList->ClearDepthStencilView(PointLightShadowMaps->GetDepthStencilView(I).Get(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0);
				CommandList->OMSetRenderTargets(nullptr, 0, PointLightShadowMaps->GetDepthStencilView(I).Get());

				PerLightBuffer.Matrix	= PoiLight->GetMatrix(I);
				PerLightBuffer.Position	= PoiLight->GetPosition();
				PerLightBuffer.FarPlane	= PoiLight->GetShadowFarPlane();
				CommandList->SetGraphicsRoot32BitConstants(&PerLightBuffer, 20, 0, 1);

				// Draw all objects to depthbuffer
				if (FrustumCullEnabled)
				{
					Frustum CameraFrustum = Frustum(PoiLight->GetShadowFarPlane(), PoiLight->GetViewMatrix(I), PoiLight->GetProjectionMatrix(I));
					for (const MeshDrawCommand& Command : CurrentScene.GetMeshDrawCommands())
					{
						const XMFLOAT4X4& Transform = Command.CurrentActor->GetTransform().GetMatrix();
						XMMATRIX XmTransform = XMMatrixTranspose(XMLoadFloat4x4(&Transform));
						XMVECTOR XmTop = XMVectorSetW(XMLoadFloat3(&Command.Mesh->BoundingBox.Top), 1.0f);
						XMVECTOR XmBottom = XMVectorSetW(XMLoadFloat3(&Command.Mesh->BoundingBox.Bottom), 1.0f);
						XmTop = XMVector4Transform(XmTop, XmTransform);
						XmBottom = XMVector4Transform(XmBottom, XmTransform);

						AABB Box;
						XMStoreFloat3(&Box.Top, XmTop);
						XMStoreFloat3(&Box.Bottom, XmBottom);
						if (CameraFrustum.CheckAABB(Box))
						{
							VBO.BufferLocation	= Command.VertexBuffer->GetGPUVirtualAddress();
							VBO.SizeInBytes		= Command.VertexBuffer->GetSizeInBytes();
							VBO.StrideInBytes	= sizeof(Vertex);
							CommandList->IASetVertexBuffers(0, &VBO, 1);

							IBV.BufferLocation	= Command.IndexBuffer->GetGPUVirtualAddress();
							IBV.SizeInBytes		= Command.IndexBuffer->GetSizeInBytes();
							IBV.Format			= DXGI_FORMAT_R32_UINT;
							CommandList->IASetIndexBuffer(&IBV);

							PerObjectBuffer.Matrix = Command.CurrentActor->GetTransform().GetMatrix();
							CommandList->SetGraphicsRoot32BitConstants(&PerObjectBuffer, 16, 0, 0);

							CommandList->DrawIndexedInstanced(Command.IndexCount, 1, 0, 0, 0);
						}
					}
				}
				else
				{
					for (const MeshDrawCommand& Command : CurrentScene.GetMeshDrawCommands())
					{
						VBO.BufferLocation	= Command.VertexBuffer->GetGPUVirtualAddress();
						VBO.SizeInBytes		= Command.VertexBuffer->GetSizeInBytes();
						VBO.StrideInBytes	= sizeof(Vertex);
						CommandList->IASetVertexBuffers(0, &VBO, 1);

						IBV.BufferLocation	= Command.IndexBuffer->GetGPUVirtualAddress();
						IBV.SizeInBytes		= Command.IndexBuffer->GetSizeInBytes();
						IBV.Format			= DXGI_FORMAT_R32_UINT;
						CommandList->IASetIndexBuffer(&IBV);

						PerObjectBuffer.Matrix = Command.CurrentActor->GetTransform().GetMatrix();
						CommandList->SetGraphicsRoot32BitConstants(&PerObjectBuffer, 16, 0, 0);

						CommandList->DrawIndexedInstanced(Command.IndexCount, 1, 0, 0, 0);
					}
				}
			}

			break;
		}
	}

	// Transition ShadowMaps
#if ENABLE_VSM
	CommandList->TransitionBarrier(VSMDirLightShadowMaps.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
#endif
	CommandList->TransitionBarrier(DirLightShadowMaps.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	CommandList->TransitionBarrier(PointLightShadowMaps.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// Update camerabuffer
	struct CameraBufferDesc
	{
		XMFLOAT4X4	ViewProjection;
		XMFLOAT3	Position;
		float		Padding;
		XMFLOAT4X4	ViewProjectionInv;
	} CamBuff;

	CamBuff.ViewProjection		= CurrentScene.GetCamera()->GetViewProjectionMatrix();
	CamBuff.ViewProjectionInv	= CurrentScene.GetCamera()->GetViewProjectionInverseMatrix();
	CamBuff.Position			= CurrentScene.GetCamera()->GetPosition();

	CommandList->TransitionBarrier(CameraBuffer.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
	CommandList->UploadBufferData(CameraBuffer.Get(), 0, &CamBuff, sizeof(CameraBufferDesc));
	CommandList->TransitionBarrier(CameraBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	// Clear GBuffer
	const Float32 BlackClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	CommandList->ClearRenderTargetView(GBuffer[0]->GetRenderTargetView(0).Get(), BlackClearColor);
	CommandList->ClearRenderTargetView(GBuffer[1]->GetRenderTargetView(0).Get(), BlackClearColor);
	CommandList->ClearRenderTargetView(GBuffer[2]->GetRenderTargetView(0).Get(), BlackClearColor);
	CommandList->ClearDepthStencilView(GBuffer[3]->GetDepthStencilView(0).Get(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0);

	// Setup view
	ViewPort.Width		= static_cast<Float32>(RenderingAPI::Get().GetSwapChain()->GetWidth());
	ViewPort.Height		= static_cast<Float32>(RenderingAPI::Get().GetSwapChain()->GetHeight());
	ViewPort.MinDepth	= 0.0f;
	ViewPort.MaxDepth	= 1.0f;
	ViewPort.TopLeftX	= 0.0f;
	ViewPort.TopLeftY	= 0.0f;
	CommandList->RSSetViewports(&ViewPort, 1);

	ScissorRect =
	{
		0,
		0,
		static_cast<LONG>(RenderingAPI::Get().GetSwapChain()->GetWidth()),
		static_cast<LONG>(RenderingAPI::Get().GetSwapChain()->GetHeight())
	};
	CommandList->RSSetScissorRects(&ScissorRect, 1);

	// Perform PrePass
	if (PrePassEnabled)
	{
		// Setup Pipeline
		CommandList->OMSetRenderTargets(nullptr, 0, GBuffer[3]->GetDepthStencilView(0).Get());

		CommandList->SetPipelineState(PrePassPSO->GetPipelineState());
		CommandList->SetGraphicsRootSignature(PrePassRootSignature->GetRootSignature());
		CommandList->SetGraphicsRootDescriptorTable(PrePassDescriptorTable->GetGPUTableStartHandle(), 1);

		// Draw all objects to depthbuffer
		//for (const MeshDrawCommand& Command : CurrentScene.GetMeshDrawCommands())
		for (const MeshDrawCommand& Command : VisibleCommands)
		{
			VBO.BufferLocation	= Command.VertexBuffer->GetGPUVirtualAddress();
			VBO.SizeInBytes		= Command.VertexBuffer->GetSizeInBytes();
			VBO.StrideInBytes	= sizeof(Vertex);
			CommandList->IASetVertexBuffers(0, &VBO, 1);

			IBV.BufferLocation	= Command.IndexBuffer->GetGPUVirtualAddress();
			IBV.SizeInBytes		= Command.IndexBuffer->GetSizeInBytes();
			IBV.Format			= DXGI_FORMAT_R32_UINT;
			CommandList->IASetIndexBuffer(&IBV);

			PerObjectBuffer.Matrix = Command.CurrentActor->GetTransform().GetMatrix();
			CommandList->SetGraphicsRoot32BitConstants(&PerObjectBuffer, 16, 0, 0);

			CommandList->DrawIndexedInstanced(Command.IndexCount, 1, 0, 0, 0);
		}
	}

	// Render all objects to the GBuffer
	D3D12RenderTargetView* GBufferRTVS[] =
	{
		GBuffer[0]->GetRenderTargetView(0).Get(),
		GBuffer[1]->GetRenderTargetView(0).Get(),
		GBuffer[2]->GetRenderTargetView(0).Get()
	};
	CommandList->OMSetRenderTargets(GBufferRTVS, 3, GBuffer[3]->GetDepthStencilView(0).Get());

	// Setup Pipeline
	CommandList->SetPipelineState(GeometryPSO->GetPipelineState());
	CommandList->SetGraphicsRootSignature(GeometryRootSignature->GetRootSignature());
	CommandList->SetGraphicsRootDescriptorTable(GeometryDescriptorTable->GetGPUTableStartHandle(), 1);

	//for (const MeshDrawCommand& Command : CurrentScene.GetMeshDrawCommands())
	for (const MeshDrawCommand& Command : VisibleCommands)
	{
		if (RenderingAPI::Get().IsRayTracingSupported())
		{
			// Command.Geometry->BuildAccelerationStructure(CommandList.Get(), );
		}

		VBO.BufferLocation	= Command.VertexBuffer->GetGPUVirtualAddress();
		VBO.SizeInBytes		= Command.VertexBuffer->GetSizeInBytes();
		VBO.StrideInBytes	= sizeof(Vertex);
		CommandList->IASetVertexBuffers(0, &VBO, 1);

		IBV.BufferLocation	= Command.IndexBuffer->GetGPUVirtualAddress();
		IBV.SizeInBytes		= Command.IndexBuffer->GetSizeInBytes();
		IBV.Format			= DXGI_FORMAT_R32_UINT;
		CommandList->IASetIndexBuffer(&IBV);

		if (Command.Material->IsBufferDirty())
		{
			Command.Material->BuildBuffer(CommandList.Get());
		}
		CommandList->SetGraphicsRootDescriptorTable(Command.Material->GetDescriptorTable()->GetGPUTableStartHandle(), 2);

		PerObjectBuffer.Matrix = Command.CurrentActor->GetTransform().GetMatrix();
		CommandList->SetGraphicsRoot32BitConstants(&PerObjectBuffer, 16, 0, 0);

		CommandList->DrawIndexedInstanced(Command.IndexCount, 1, 0, 0, 0);
	}

	// Setup GBuffer for Read
	CommandList->TransitionBarrier(GBuffer[0].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	CommandList->TransitionBarrier(GBuffer[1].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	CommandList->TransitionBarrier(GBuffer[2].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	CommandList->TransitionBarrier(GBuffer[3].Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// RayTracing
	if (RenderingAPI::Get().IsRayTracingSupported())
	{
		TraceRays(BackBuffer, CommandList.Get());
	}

	// Render to final output
	CommandList->TransitionBarrier(FinalTarget.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	CommandList->TransitionBarrier(BackBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	D3D12RenderTargetView* RenderTarget[] = { FinalTarget->GetRenderTargetView(0).Get() };
	CommandList->OMSetRenderTargets(RenderTarget, 1, nullptr);

	CommandList->RSSetViewports(&ViewPort, 1);
	CommandList->RSSetScissorRects(&ScissorRect, 1);

	// Setup LightPass
	CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	CommandList->SetPipelineState(LightPassPSO->GetPipelineState());
	CommandList->SetGraphicsRootSignature(LightRootSignature->GetRootSignature());
	CommandList->SetGraphicsRootDescriptorTable(LightDescriptorTable->GetGPUTableStartHandle(), 0);

	// Perform LightPass
	CommandList->DrawInstanced(3, 1, 0, 0);

	// Draw skybox
	CommandList->TransitionBarrier(GBuffer[3].Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	CommandList->OMSetRenderTargets(RenderTarget, 1, GBuffer[3]->GetDepthStencilView(0).Get());
	
	CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3D12_VERTEX_BUFFER_VIEW SkyboxVBO = { };
	SkyboxVBO.BufferLocation	= SkyboxVertexBuffer->GetGPUVirtualAddress();
	SkyboxVBO.SizeInBytes		= SkyboxVertexBuffer->GetSizeInBytes();
	SkyboxVBO.StrideInBytes		= sizeof(Vertex);
	CommandList->IASetVertexBuffers(0, &SkyboxVBO, 1);

	D3D12_INDEX_BUFFER_VIEW SkyboxIBV = { };
	SkyboxIBV.BufferLocation	= SkyboxIndexBuffer->GetGPUVirtualAddress();
	SkyboxIBV.SizeInBytes		= SkyboxIndexBuffer->GetSizeInBytes();
	SkyboxIBV.Format			= DXGI_FORMAT_R32_UINT;
	CommandList->IASetIndexBuffer(&SkyboxIBV);

	CommandList->SetPipelineState(SkyboxPSO->GetPipelineState());
	CommandList->SetGraphicsRootSignature(SkyboxRootSignature->GetRootSignature());
	
	struct SimpleCameraBuffer
	{
		XMFLOAT4X4 Matrix;
	} SimpleCamera;
	SimpleCamera.Matrix = CurrentScene.GetCamera()->GetViewProjectionWitoutTranslateMatrix();
	CommandList->SetGraphicsRoot32BitConstants(&SimpleCamera, 16, 0, 0);
	CommandList->SetGraphicsRootDescriptorTable(SkyboxDescriptorTable->GetGPUTableStartHandle(), 1);

	CommandList->DrawIndexedInstanced(static_cast<Uint32>(SkyboxMesh.Indices.Size()), 1, 0, 0, 0);

	// Render to BackBuffer
	CommandList->TransitionBarrier(FinalTarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	
	RenderTarget[0] = BackBuffer->GetRenderTargetView(0).Get();
	CommandList->OMSetRenderTargets(RenderTarget, 1, nullptr);

	CommandList->IASetVertexBuffers(0, nullptr, 0);
	CommandList->IASetIndexBuffer(nullptr);

	CommandList->SetGraphicsRootSignature(PostRootSignature->GetRootSignature());
	CommandList->SetGraphicsRootDescriptorTable(PostDescriptorTable->GetGPUTableStartHandle(), 0);
	
	if (FXAAEnabled)
	{
		struct FXAASettings
		{
			Float32 Width;
			Float32 Height;
		} Settings;

		Settings.Width	= RenderingAPI::Get().GetSwapChain()->GetWidth();
		Settings.Height	= RenderingAPI::Get().GetSwapChain()->GetHeight();

		CommandList->SetGraphicsRoot32BitConstants(&Settings, 2, 0, 1);
		CommandList->SetPipelineState(FXAAPSO->GetPipelineState());
	}
	else
	{
		CommandList->SetPipelineState(PostPSO->GetPipelineState());
	}

	CommandList->DrawInstanced(3, 1, 0, 0);

	// Draw DebugBoxes
	if (DrawAABBs)
	{
		CommandList->SetPipelineState(DebugBoxPSO->GetPipelineState());
		CommandList->SetGraphicsRootSignature(DebugRootSignature->GetRootSignature());
		CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

		SimpleCamera.Matrix = CurrentScene.GetCamera()->GetViewProjectionMatrix();
		CommandList->SetGraphicsRoot32BitConstants(&SimpleCamera, 16, 0, 1);

		D3D12_VERTEX_BUFFER_VIEW DebugVBO = { };
		DebugVBO.BufferLocation = AABBVertexBuffer->GetGPUVirtualAddress();
		DebugVBO.SizeInBytes	= AABBVertexBuffer->GetSizeInBytes();
		DebugVBO.StrideInBytes	= sizeof(XMFLOAT3);
		CommandList->IASetVertexBuffers(0, &DebugVBO, 1);

		D3D12_INDEX_BUFFER_VIEW DebugIBV = { };
		DebugIBV.BufferLocation = AABBIndexBuffer->GetGPUVirtualAddress();
		DebugIBV.SizeInBytes	= AABBIndexBuffer->GetSizeInBytes();
		DebugIBV.Format			= DXGI_FORMAT_R16_UINT;
		CommandList->IASetIndexBuffer(&DebugIBV);

		//for (const MeshDrawCommand& Command : CurrentScene.GetMeshDrawCommands())
		for (const MeshDrawCommand& Command : VisibleCommands)
		{
			AABB& Box = Command.Mesh->BoundingBox;
			XMFLOAT3 Scale = XMFLOAT3(Box.GetWidth(), Box.GetHeight(), Box.GetDepth());
			XMFLOAT3 Position = Box.GetCenter();

			XMMATRIX XmTranslation = XMMatrixTranslation(Position.x, Position.y, Position.z);
			XMMATRIX XmScale = XMMatrixScaling(Scale.x, Scale.y, Scale.z);

			XMFLOAT4X4 Transform = Command.CurrentActor->GetTransform().GetMatrix();
			XMMATRIX XmTransform = XMMatrixTranspose(XMLoadFloat4x4(&Transform));
			XMStoreFloat4x4(&Transform, XMMatrixMultiplyTranspose(XMMatrixMultiply(XmScale, XmTranslation), XmTransform));

			CommandList->SetGraphicsRoot32BitConstants(&Transform, 16, 0, 0);
			CommandList->DrawIndexedInstanced(24, 1, 0, 0, 0);
		}
	}

	// Render UI
	DebugUI::DrawDebugString("DrawCall Count: " + std::to_string(CommandList->GetNumDrawCalls()));
	DebugUI::Render(CommandList.Get());

	// Finalize Commandlist
	CommandList->TransitionBarrier(GBuffer[3].Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	CommandList->TransitionBarrier(BackBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	CommandList->Close();

	// Execute
	RenderingAPI::Get().GetQueue()->ExecuteCommandList(CommandList.Get());

	// Present
	RenderingAPI::Get().GetSwapChain()->Present(VSyncEnabled ? 1 : 0);

	// Wait for next frame
	const Uint64 CurrentFenceValue = FenceValues[CurrentBackBufferIndex];
	RenderingAPI::Get().GetQueue()->SignalFence(Fence.Get(), CurrentFenceValue);

	CurrentBackBufferIndex = RenderingAPI::Get().GetSwapChain()->GetCurrentBackBufferIndex();
	if (Fence->WaitForValue(CurrentFenceValue))
	{
		FenceValues[CurrentBackBufferIndex] = CurrentFenceValue + 1;
	}
}

void Renderer::TraceRays(D3D12Texture* BackBuffer, D3D12CommandList* InCommandList)
{
	InCommandList->TransitionBarrier(ReflectionTexture.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	D3D12_DISPATCH_RAYS_DESC raytraceDesc = {};
	raytraceDesc.Width	= static_cast<Uint32>(ReflectionTexture->GetDesc().Width);	//SwapChain->GetWidth();
	raytraceDesc.Height = static_cast<Uint32>(ReflectionTexture->GetDesc().Height);	//SwapChain->GetHeight();
	raytraceDesc.Depth	= 1;

	// Set shader tables
	raytraceDesc.RayGenerationShaderRecord	= RayTracingScene->GetRayGenerationShaderRecord();
	raytraceDesc.MissShaderTable			= RayTracingScene->GetMissShaderTable();
	raytraceDesc.HitGroupTable				= RayTracingScene->GetHitGroupTable();

	// Bind the empty root signature
	InCommandList->SetComputeRootSignature(GlobalRootSignature->GetRootSignature());
	InCommandList->SetComputeRootDescriptorTable(GlobalDescriptorTable->GetGPUTableStartHandle(), 0);

	// Dispatch
	InCommandList->SetStateObject(RaytracingPSO->GetStateObject());
	InCommandList->DispatchRays(&raytraceDesc);

	// Copy the results to the back-buffer
	InCommandList->TransitionBarrier(ReflectionTexture.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
}

void Renderer::OnResize(Int32 Width, Int32 Height)
{
	WaitForPendingFrames();

	RenderingAPI::Get().GetSwapChain()->Resize(Width, Height);

	// Update deferred
	InitGBuffer();

	if (RenderingAPI::Get().IsRayTracingSupported())
	{
		RayGenDescriptorTable->SetUnorderedAccessView(ReflectionTexture->GetUnorderedAccessView(0).Get(), 0);
		RayGenDescriptorTable->CopyDescriptors();

		GlobalDescriptorTable->SetShaderResourceView(GBuffer[1]->GetShaderResourceView(0).Get(), 5);
		GlobalDescriptorTable->SetShaderResourceView(GBuffer[3]->GetShaderResourceView(0).Get(), 6);
		GlobalDescriptorTable->CopyDescriptors();
	}

	LightDescriptorTable->SetShaderResourceView(GBuffer[0]->GetShaderResourceView(0).Get(), 0);
	LightDescriptorTable->SetShaderResourceView(GBuffer[1]->GetShaderResourceView(0).Get(), 1);
	LightDescriptorTable->SetShaderResourceView(GBuffer[2]->GetShaderResourceView(0).Get(), 2);
	LightDescriptorTable->SetShaderResourceView(GBuffer[3]->GetShaderResourceView(0).Get(), 3);
	LightDescriptorTable->CopyDescriptors();

	CurrentBackBufferIndex = RenderingAPI::Get().GetSwapChain()->GetCurrentBackBufferIndex();
}

void Renderer::SetPrePassEnable(bool Enabled)
{
	PrePassEnabled = Enabled;
}

void Renderer::SetVerticalSyncEnable(bool Enabled)
{
	VSyncEnabled = Enabled;
}

void Renderer::SetDrawAABBsEnable(bool Enabled)
{
	DrawAABBs = Enabled;
}

void Renderer::SetFrustumCullEnable(bool Enabled)
{
	FrustumCullEnabled = Enabled;
}

void Renderer::SetFXAAEnable(bool Enabled)
{
	FXAAEnabled = Enabled;
}

void Renderer::SetGlobalLightSettings(const LightSettings& InGlobalLightSettings)
{
	// Set Settings
	GlobalLightSettings = InGlobalLightSettings;

	// Recreate ShadowMaps
	Renderer* CurrentRenderer = Renderer::Get();
	if (CurrentRenderer)
	{
		CurrentRenderer->WaitForPendingFrames();

		CurrentRenderer->CreateShadowMaps();
		CurrentRenderer->WriteShadowMapDescriptors();

#if ENABLE_VSM
		RenderingAPI::StaticGetImmediateCommandList()->TransitionBarrier(CurrentRenderer->VSMDirLightShadowMaps.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
#endif
		RenderingAPI::StaticGetImmediateCommandList()->TransitionBarrier(CurrentRenderer->DirLightShadowMaps.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		RenderingAPI::StaticGetImmediateCommandList()->TransitionBarrier(CurrentRenderer->PointLightShadowMaps.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		RenderingAPI::StaticGetImmediateCommandList()->Flush();
	}
}

Renderer* Renderer::Make()
{
	RendererInstance = MakeUnique<Renderer>();
	if (RendererInstance->Initialize())
	{
		return RendererInstance.Get();
	}
	else
	{
		return nullptr;
	}
}

Renderer* Renderer::Get()
{
	return RendererInstance.Get();
}

void Renderer::Release()
{
	RendererInstance.Reset();
}

bool Renderer::Initialize()
{
	const Uint32 BackBufferCount = RenderingAPI::Get().GetSwapChain()->GetSurfaceCount();
	CommandAllocators.Resize(BackBufferCount);
	for (Uint32 i = 0; i < BackBufferCount; i++)
	{
		CommandAllocators[i] = RenderingAPI::Get().CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT);
		if (!CommandAllocators[i])
		{
			return false;
		}
	}

	CommandList = RenderingAPI::Get().CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, CommandAllocators[0].Get(), nullptr);
	if (!CommandList)
	{
		return false;
	}

	Fence = RenderingAPI::Get().CreateFence(0);
	if (!Fence)
	{
		return false;
	}

	FenceValues.Resize(RenderingAPI::Get().GetSwapChain()->GetSurfaceCount());

	// Create mesh
	Sphere		= MeshFactory::CreateSphere(3);
	SkyboxMesh	= MeshFactory::CreateSphere(1);
	Cube		= MeshFactory::CreateCube();

	// Create camera
	CameraBuffer = CreateConstantBuffer(nullptr, 256, BufferUsage_Default);
	if (!CameraBuffer)
	{
		LOG_ERROR("[Renderer]: Failed to create camerabuffer");
		return false;
	}

	// Create vertexbuffers
	MeshVertexBuffer = CreateVertexBuffer<Vertex>(nullptr, Sphere.Vertices.Size(), BufferUsage_Dynamic);
	if (MeshVertexBuffer)
	{
		const Uint32 SizeInBytes = Sphere.Vertices.Size() * sizeof(Vertex);

		Range MappedRange = Range(0, SizeInBytes);
		VoidPtr BufferMemory = MeshVertexBuffer->Map(MappedRange);
		memcpy(BufferMemory, Sphere.Vertices.Data(), SizeInBytes);
		MeshVertexBuffer->Unmap(MappedRange);
	}
	else
	{
		return false;
	}

	CubeVertexBuffer = CreateVertexBuffer<Vertex>(nullptr, Cube.Vertices.Size(), BufferUsage_Dynamic);
	if (CubeVertexBuffer)
	{
		const Uint32 SizeInBytes = Cube.Vertices.Size() * sizeof(Vertex);

		Range MappedRange = Range(0, SizeInBytes);
		VoidPtr BufferMemory = CubeVertexBuffer->Map(MappedRange);
		memcpy(BufferMemory, Cube.Vertices.Data(), SizeInBytes);
		CubeVertexBuffer->Unmap(MappedRange);
	}
	else
	{
		return false;
	}

	SkyboxVertexBuffer = CreateVertexBuffer<Vertex>(nullptr, Cube.Vertices.Size(), BufferUsage_Dynamic);
	if (SkyboxVertexBuffer)
	{
		const Uint32 SizeInBytes = SkyboxMesh.Vertices.Size() * sizeof(Vertex);
		
		Range MappedRange = Range(0, SizeInBytes);
		VoidPtr BufferMemory = SkyboxVertexBuffer->Map(MappedRange);
		memcpy(BufferMemory, SkyboxMesh.Vertices.Data(), SizeInBytes);
		SkyboxVertexBuffer->Unmap(MappedRange);
	}
	else
	{
		return false;
	}

	// Create indexbuffers
	{
		const Uint32 SizeInBytes = sizeof(Uint32) * static_cast<Uint64>(Sphere.Indices.Size());
		MeshIndexBuffer = CreateIndexBuffer(nullptr, SizeInBytes, EIndexFormat::IndexFormat_Uint32, BufferUsage_Dynamic);
		if (MeshIndexBuffer)
		{
			Range MappedRange = Range(0, SizeInBytes);
			VoidPtr BufferMemory = MeshIndexBuffer->Map(MappedRange);
			memcpy(BufferMemory, Sphere.Indices.Data(), SizeInBytes);
			MeshIndexBuffer->Unmap(MappedRange);
		}
		else
		{
			return false;
		}
	}

	{
		const Uint32 SizeInBytes = sizeof(Uint32) * static_cast<Uint64>(Cube.Indices.Size());
		CubeIndexBuffer = CreateIndexBuffer(nullptr, SizeInBytes, EIndexFormat::IndexFormat_Uint32, BufferUsage_Dynamic);
		if (CubeIndexBuffer)
		{
			Range MappedRange = Range(0, SizeInBytes);
			VoidPtr BufferMemory = CubeIndexBuffer->Map(MappedRange);
			memcpy(BufferMemory, Cube.Indices.Data(), SizeInBytes);
			CubeIndexBuffer->Unmap(MappedRange);
		}
		else
		{
			return false;
		}
	}

	{
		const Uint32 SizeInBytes = sizeof(Uint32) * static_cast<Uint64>(SkyboxMesh.Indices.Size());
		SkyboxIndexBuffer = CreateIndexBuffer(nullptr, SizeInBytes, EIndexFormat::IndexFormat_Uint32, BufferUsage_Dynamic);
		if (SkyboxIndexBuffer)
		{
			Range MappedRange = Range(0, SizeInBytes);
			VoidPtr BufferMemory = SkyboxIndexBuffer->Map(MappedRange);
			memcpy(BufferMemory, SkyboxMesh.Indices.Data(), SizeInBytes);
			SkyboxIndexBuffer->Unmap(MappedRange);
		}
		else
		{
			return false;
		}
	}

	// Create Texture Cube
	TUniquePtr<D3D12Texture> Panorama = TUniquePtr<D3D12Texture>(TextureFactory::LoadFromFile("../Assets/Textures/arches.hdr", 0, DXGI_FORMAT_R32G32B32A32_FLOAT));
	if (!Panorama)
	{
		return false;	
	}

	Skybox = TSharedPtr<D3D12Texture>(TextureFactory::CreateTextureCubeFromPanorma(Panorama.Get(), 1024, TEXTURE_FACTORY_FLAGS_GENERATE_MIPS, DXGI_FORMAT_R16G16B16A16_FLOAT));
	if (!Skybox)
	{
		return false;
	}
	else
	{
		Skybox->SetName("Skybox");
	}

	// Generate global irradiance (From Skybox)
	const Uint16 IrradianceSize = 32;
	IrradianceMap = CreateTextureCube(nullptr, EFormat::Format_R16G16B16A16_Float, TextureUsage_Default | TextureUsage_UAV, IrradianceSize, 1, 1, ClearValue());
	if (IrradianceMap)
	{
		IrradianceMap->SetName("Irradiance Map");
	}
	else
	{
		return false;
	}

	IrradianceMapUAV = CreateUnorderedAccessView(IrradianceMap.Get(), EFormat::Format_R16G16B16A16_Float, 0);
	if (!IrradianceMapUAV)
	{
		return false;
	}

	IrradianceMapSRV = CreateShaderResourceView(IrradianceMap.Get(), EFormat::Format_R16G16B16A16_Float, 0, 1);
	if (!IrradianceMapSRV)
	{
		return false;
	}

	// Generate global specular irradiance (From Skybox)
	const Uint16 SpecularIrradianceSize			= 128;
	const Uint32 SpecularIrradianceMiplevels	= std::max<Uint32>(std::log2<Uint32>(SpecularIrradianceSize), 1U);
	SpecularIrradianceMap = CreateTextureCube(
		nullptr, 
		EFormat::Format_R16G16B16A16_Float, 
		TextureUsage_Default | TextureUsage_UAV, 
		SpecularIrradianceSize, 
		SpecularIrradianceMiplevels, 
		1, 
		ClearValue());
	if (SpecularIrradianceMap)
	{
		SpecularIrradianceMap->SetName("Specular Irradiance Map");
	}
	else
	{
		return false;
	}

	for (Uint32 MipLevel = 0; MipLevel < SpecularIrradianceMiplevels; MipLevel++)
	{
		TSharedRef<UnorderedAccessView> Uav = CreateUnorderedAccessView(SpecularIrradianceMap.Get(), EFormat::Format_R16G16B16A16_Float, MipLevel);
		if (Uav)
		{
			SpecularIrradianceMapUAVs.EmplaceBack(Uav);
		}
	}

	SpecularIrradianceMapSRV = CreateShaderResourceView(SpecularIrradianceMap.Get(), EFormat::Format_R16G16B16A16_Float, 0, SpecularIrradianceMiplevels);
	if (SpecularIrradianceMapSRV)
	{
		return false;
	}

	GenerateIrradianceMap(Skybox.Get(), IrradianceMap.Get(), RenderingAPI::Get().GetImmediateCommandList().Get());
	RenderingAPI::Get().GetImmediateCommandList()->Flush();

	GenerateSpecularIrradianceMap(Skybox.Get(), SpecularIrradianceMap.Get(), RenderingAPI::Get().GetImmediateCommandList().Get());
	RenderingAPI::Get().GetImmediateCommandList()->Flush();

	// Create albedo for raytracing
	Albedo = TSharedPtr<D3D12Texture>(TextureFactory::LoadFromFile("../Assets/Textures/RockySoil_Albedo.png", TEXTURE_FACTORY_FLAGS_GENERATE_MIPS, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!Albedo)
	{
		return false;
	}
	else
	{
		Albedo->SetDebugName("AlbedoMap");
	}
	
	Normal = TSharedPtr<D3D12Texture>(TextureFactory::LoadFromFile("../Assets/Textures/RockySoil_Normal.png", TEXTURE_FACTORY_FLAGS_GENERATE_MIPS, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!Normal)
	{
		return false;
	}
	else
	{
		Normal->SetDebugName("NormalMap");
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

	RenderingAPI::Get().GetImmediateCommandList()->TransitionBarrier(PointLightBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	RenderingAPI::Get().GetImmediateCommandList()->TransitionBarrier(DirectionalLightBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	RenderingAPI::Get().GetImmediateCommandList()->TransitionBarrier(PointLightShadowMaps.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	RenderingAPI::Get().GetImmediateCommandList()->TransitionBarrier(DirLightShadowMaps.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	if (VSMDirLightShadowMaps)
	{
		RenderingAPI::Get().GetImmediateCommandList()->TransitionBarrier(VSMDirLightShadowMaps.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
	
	RenderingAPI::Get().GetImmediateCommandList()->Flush();

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

	// Init RayTracing if supported
	if (RenderingAPI::Get().IsRayTracingSupported())
	{
		if (!InitRayTracing())
		{
			return false;
		}
	}

	LightDescriptorTable->SetShaderResourceView(ReflectionTexture->GetShaderResourceView(0).Get(), 4);
	LightDescriptorTable->SetShaderResourceView(IrradianceMap->GetShaderResourceView(0).Get(), 5);
	LightDescriptorTable->SetShaderResourceView(SpecularIrradianceMap->GetShaderResourceView(0).Get(), 6);
	LightDescriptorTable->SetShaderResourceView(IntegrationLUT->GetShaderResourceView(0).Get(), 7);
	LightDescriptorTable->CopyDescriptors();

	WriteShadowMapDescriptors();

	return true;
}

bool Renderer::InitRayTracing()
{
	// Create RootSignatures
	TUniquePtr<D3D12RootSignature> RayGenLocalRoot;
	{
		D3D12_DESCRIPTOR_RANGE Ranges[1] = {};
		Ranges[0].BaseShaderRegister				= 0;
		Ranges[0].NumDescriptors					= 1;
		Ranges[0].RegisterSpace						= 0;
		Ranges[0].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		Ranges[0].OffsetInDescriptorsFromTableStart	= 0;

		D3D12_ROOT_PARAMETER RootParams = { };
		RootParams.ParameterType						= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		RootParams.DescriptorTable.NumDescriptorRanges	= 1;
		RootParams.DescriptorTable.pDescriptorRanges	= Ranges;

		D3D12_ROOT_SIGNATURE_DESC RayGenLocalRootDesc = {};
		RayGenLocalRootDesc.NumParameters	= 1;
		RayGenLocalRootDesc.pParameters		= &RootParams;
		RayGenLocalRootDesc.Flags			= D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

		RayGenLocalRoot = RenderingAPI::Get().CreateRootSignature(RayGenLocalRootDesc);
		if (RayGenLocalRoot)
		{
			RayGenLocalRoot->SetDebugName("RayGen Local RootSignature");
		}
		else
		{
			return false;
		}
	}

	TUniquePtr<D3D12RootSignature> HitLocalRoot;
	{
		D3D12_DESCRIPTOR_RANGE Ranges[2] = {};
		// VertexBuffer
		Ranges[0].BaseShaderRegister				= 2;
		Ranges[0].NumDescriptors					= 1;
		Ranges[0].RegisterSpace						= 0;
		Ranges[0].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		Ranges[0].OffsetInDescriptorsFromTableStart = 0;

		// IndexBuffer
		Ranges[1].BaseShaderRegister				= 3;
		Ranges[1].NumDescriptors					= 1;
		Ranges[1].RegisterSpace						= 0;
		Ranges[1].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		Ranges[1].OffsetInDescriptorsFromTableStart	= 1;

		D3D12_ROOT_PARAMETER RootParams = { };
		RootParams.ParameterType						= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		RootParams.DescriptorTable.NumDescriptorRanges	= 2;
		RootParams.DescriptorTable.pDescriptorRanges	= Ranges;

		D3D12_ROOT_SIGNATURE_DESC HitLocalRootDesc = {};
		HitLocalRootDesc.NumParameters	= 1;
		HitLocalRootDesc.pParameters	= &RootParams;
		HitLocalRootDesc.Flags			= D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

		HitLocalRoot = RenderingAPI::Get().CreateRootSignature(HitLocalRootDesc);
		if (HitLocalRoot)
		{
			HitLocalRoot->SetDebugName("Closest Hit Local RootSignature");
		}
		else
		{
			return false;
		}
	}

	TUniquePtr<D3D12RootSignature> MissLocalRoot;
	{
		D3D12_ROOT_SIGNATURE_DESC MissLocalRootDesc = {};
		MissLocalRootDesc.NumParameters	= 0;
		MissLocalRootDesc.pParameters	= nullptr;
		MissLocalRootDesc.Flags			= D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

		MissLocalRoot = RenderingAPI::Get().CreateRootSignature(MissLocalRootDesc);
		if (MissLocalRoot)
		{
			MissLocalRoot->SetDebugName("Miss Local RootSignature");
		}
		else
		{
			return false;
		}
	}

	// Global RootSignature
	{
		D3D12_DESCRIPTOR_RANGE Ranges[7] = {};
		// AccelerationStructure
		Ranges[0].BaseShaderRegister				= 0;
		Ranges[0].NumDescriptors					= 1;
		Ranges[0].RegisterSpace						= 0;
		Ranges[0].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		Ranges[0].OffsetInDescriptorsFromTableStart	= 0;

		// Camera Buffer
		Ranges[1].BaseShaderRegister				= 0;
		Ranges[1].NumDescriptors					= 1;
		Ranges[1].RegisterSpace						= 0;
		Ranges[1].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		Ranges[1].OffsetInDescriptorsFromTableStart	= 1;

		// Skybox
		Ranges[2].BaseShaderRegister				= 1;
		Ranges[2].NumDescriptors					= 1;
		Ranges[2].RegisterSpace						= 0;
		Ranges[2].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		Ranges[2].OffsetInDescriptorsFromTableStart	= 2;

		// Albedo
		Ranges[3].BaseShaderRegister				= 4;
		Ranges[3].NumDescriptors					= 1;
		Ranges[3].RegisterSpace						= 0;
		Ranges[3].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		Ranges[3].OffsetInDescriptorsFromTableStart	= 3;

		// Normal
		Ranges[4].BaseShaderRegister				= 5;
		Ranges[4].NumDescriptors					= 1;
		Ranges[4].RegisterSpace						= 0;
		Ranges[4].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		Ranges[4].OffsetInDescriptorsFromTableStart	= 4;

		// GBuffer NormalMap
		Ranges[5].BaseShaderRegister				= 6;
		Ranges[5].NumDescriptors					= 1;
		Ranges[5].RegisterSpace						= 0;
		Ranges[5].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		Ranges[5].OffsetInDescriptorsFromTableStart = 5;

		// GBuffer Depth
		Ranges[6].BaseShaderRegister				= 7;
		Ranges[6].NumDescriptors					= 1;
		Ranges[6].RegisterSpace						= 0;
		Ranges[6].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		Ranges[6].OffsetInDescriptorsFromTableStart = 6;

		D3D12_ROOT_PARAMETER RootParams = { };
		RootParams.ParameterType						= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		RootParams.DescriptorTable.NumDescriptorRanges	= 7;
		RootParams.DescriptorTable.pDescriptorRanges	= Ranges;

		D3D12_STATIC_SAMPLER_DESC Samplers[2] = { };
		// Generic Sampler
		Samplers[0].ShaderVisibility	= D3D12_SHADER_VISIBILITY_ALL;
		Samplers[0].AddressU			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		Samplers[0].AddressV			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		Samplers[0].AddressW			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		Samplers[0].Filter				= D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		Samplers[0].ShaderRegister		= 0;
		Samplers[0].RegisterSpace		= 0;
		Samplers[0].MinLOD				= 0.0f;
		Samplers[0].MaxLOD				= FLT_MAX;
 
		// GBuffer Sampler
		Samplers[1].ShaderVisibility	= D3D12_SHADER_VISIBILITY_ALL;
		Samplers[1].AddressU			= D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		Samplers[1].AddressV			= D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		Samplers[1].AddressW			= D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		Samplers[1].Filter				= D3D12_FILTER_MIN_MAG_MIP_POINT;
		Samplers[1].ShaderRegister		= 1;
		Samplers[1].RegisterSpace		= 0;
		Samplers[1].MinLOD				= 0.0f;
		Samplers[1].MaxLOD				= FLT_MAX;

		D3D12_ROOT_SIGNATURE_DESC GlobalRootDesc = {};
		GlobalRootDesc.NumStaticSamplers	= 2;
		GlobalRootDesc.pStaticSamplers		= Samplers;
		GlobalRootDesc.NumParameters		= 1;
		GlobalRootDesc.pParameters			= &RootParams;
		GlobalRootDesc.Flags				= D3D12_ROOT_SIGNATURE_FLAG_NONE;

		GlobalRootSignature = RenderingAPI::Get().CreateRootSignature(GlobalRootDesc);
		if (!GlobalRootSignature)
		{
			return false;
		}
	}

	// Create Pipeline
	RayTracingPipelineStateProperties PipelineProperties;
	PipelineProperties.DebugName				= "RayTracing PipelineState";
	PipelineProperties.RayGenRootSignature		= RayGenLocalRoot.Get();
	PipelineProperties.HitGroupRootSignature	= HitLocalRoot.Get();
	PipelineProperties.MissRootSignature		= MissLocalRoot.Get();
	PipelineProperties.GlobalRootSignature		= GlobalRootSignature.Get();
	PipelineProperties.MaxRecursions			= 4;

	RaytracingPSO = RenderingAPI::Get().CreateRayTracingPipelineState(PipelineProperties);
	if (!RaytracingPSO)
	{
		return false;
	}

	// Build Acceleration Structures
	RenderingAPI::Get().GetImmediateCommandList()->TransitionBarrier(CameraBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	// Create BLAS
	TSharedPtr<D3D12RayTracingGeometry> MeshGeometry = TSharedPtr(RenderingAPI::Get().CreateRayTracingGeometry());
	MeshGeometry->BuildAccelerationStructure(RenderingAPI::Get().GetImmediateCommandList().Get(), MeshVertexBuffer, static_cast<Uint32>(Sphere.Vertices.Size()), MeshIndexBuffer, static_cast<Uint32>(Sphere.Indices.Size()));

	TSharedPtr<D3D12RayTracingGeometry> CubeGeometry = TSharedPtr(RenderingAPI::Get().CreateRayTracingGeometry());
	CubeGeometry->BuildAccelerationStructure(RenderingAPI::Get().GetImmediateCommandList().Get(), CubeVertexBuffer, static_cast<Uint32>(Cube.Vertices.Size()), CubeIndexBuffer, static_cast<Uint32>(Cube.Indices.Size()));

	XMFLOAT3X4 Matrix;
	TArray<D3D12RayTracingGeometryInstance> Instances;

	constexpr Float32	Offset = 1.25f;
	constexpr Uint32	SphereCountX = 8;
	constexpr Float32	StartPositionX = (-static_cast<Float32>(SphereCountX) * Offset) / 2.0f;
	constexpr Uint32	SphereCountY = 8;
	constexpr Float32	StartPositionY = (-static_cast<Float32>(SphereCountY) * Offset) / 2.0f;
	for (Uint32 y = 0; y < SphereCountY; y++)
	{
		for (Uint32 x = 0; x < SphereCountX; x++)
		{
			XMStoreFloat3x4(&Matrix, XMMatrixTranslation(StartPositionX + (x * Offset), StartPositionY + (y * Offset), 0));
			Instances.EmplaceBack(MeshGeometry, Matrix, 0, 0);
		}
	}

	XMStoreFloat3x4(&Matrix, XMMatrixTranslation(0.0f, 0.0f, -3.0f));
	Instances.EmplaceBack(CubeGeometry, Matrix, 1, 1);

	// Create DescriptorTables
	TSharedPtr<D3D12DescriptorTable> SphereDescriptorTable	= TSharedPtr(RenderingAPI::Get().CreateDescriptorTable(2)); 
	TSharedPtr<D3D12DescriptorTable> CubeDescriptorTable	= TSharedPtr(RenderingAPI::Get().CreateDescriptorTable(2));
	RayGenDescriptorTable = TSharedPtr(RenderingAPI::Get().CreateDescriptorTable(1));
	GlobalDescriptorTable = TSharedPtr(RenderingAPI::Get().CreateDescriptorTable(7));

	// Create TLAS
	TArray<BindingTableEntry> BindingTableEntries;
	BindingTableEntries.EmplaceBack("RayGen", RayGenDescriptorTable);
	BindingTableEntries.EmplaceBack("HitGroup", SphereDescriptorTable);
	BindingTableEntries.EmplaceBack("HitGroup", CubeDescriptorTable);
	BindingTableEntries.EmplaceBack("Miss", nullptr);

	RayTracingScene = RenderingAPI::Get().CreateRayTracingScene(RaytracingPSO.Get(), BindingTableEntries, 2);
	if (!RayTracingScene)
	{
		return false;
	}

	RayTracingScene->BuildAccelerationStructure(RenderingAPI::Get().GetImmediateCommandList().Get(), Instances);
	RenderingAPI::Get().GetImmediateCommandList()->Flush();
	RenderingAPI::Get().GetImmediateCommandList()->WaitForCompletion();

	// Create shader resource views
	MeshVertexBufferSRV = CreateShaderResourceView<Vertex>(MeshVertexBuffer.Get(), 0, static_cast<Uint32>(Sphere.Vertices.Size()));
	if (!MeshVertexBufferSRV)
	{
		return false;
	}

	CubeVertexBufferSRV = CreateShaderResourceView<Vertex>(CubeVertexBuffer.Get(), 0, static_cast<Uint32>(Cube.Vertices.Size()));
	if (!CubeVertexBufferSRV)
	{
		return false;
	}

	MeshIndexBufferSRV = CreateShaderResourceView(MeshIndexBuffer.Get(), 0, static_cast<Uint32>(Sphere.Indices.Size()), EFormat::Format_R16_Typeless);
	if (!MeshIndexBufferSRV)
	{
		return false;
	}

	CubeIndexBufferSRV = CreateShaderResourceView(CubeIndexBuffer.Get(), 0, static_cast<Uint32>(Cube.Indices.Size()), EFormat::Format_R16_Typeless);
	if (!CubeIndexBufferSRV)
	{
		return false;
	}

	// Populate descriptors
	RayGenDescriptorTable->SetUnorderedAccessView(ReflectionTexture->GetUnorderedAccessView(0).Get(), 0);
	RayGenDescriptorTable->CopyDescriptors();

	SphereDescriptorTable->SetShaderResourceView(MeshVertexBuffer->GetShaderResourceView(0).Get(), 0);
	SphereDescriptorTable->SetShaderResourceView(MeshIndexBuffer->GetShaderResourceView(0).Get(), 1);
	SphereDescriptorTable->CopyDescriptors();

	CubeDescriptorTable->SetShaderResourceView(CubeVertexBuffer->GetShaderResourceView(0).Get(), 0);
	CubeDescriptorTable->SetShaderResourceView(CubeIndexBuffer->GetShaderResourceView(0).Get(), 1);
	CubeDescriptorTable->CopyDescriptors();

	GlobalDescriptorTable->SetShaderResourceView(RayTracingScene->GetShaderResourceView(), 0);
	GlobalDescriptorTable->SetConstantBufferView(CameraBuffer->GetConstantBufferView().Get(), 1);
	GlobalDescriptorTable->SetShaderResourceView(Skybox->GetShaderResourceView(0).Get(), 2);
	GlobalDescriptorTable->SetShaderResourceView(Albedo->GetShaderResourceView(0).Get(), 3);
	GlobalDescriptorTable->SetShaderResourceView(Normal->GetShaderResourceView(0).Get(), 4);
	GlobalDescriptorTable->SetShaderResourceView(GBuffer[1]->GetShaderResourceView(0).Get(), 5);
	GlobalDescriptorTable->SetShaderResourceView(GBuffer[3]->GetShaderResourceView(0).Get(), 6);
	GlobalDescriptorTable->CopyDescriptors();

	return true;
}

bool Renderer::InitLightBuffers()
{
	const Uint32 NumPointLights = 1;
	const Uint32 NumDirLights	= 1;

	PointLightBuffer = CreateConstantBuffer<PointLightProperties>(nullptr, NumPointLights, BufferUsage_Default);
	if (PointLightBuffer)
	{
		PointLightBuffer->SetName("PointLight Buffer");
	}
	else
	{
		return false;
	}

	PointLightBuffer = CreateConstantBuffer<DirectionalLightProperties>(nullptr, NumDirLights, BufferUsage_Default);
	if (PointLightBuffer)
	{
		PointLightBuffer->SetName("DirectionalLight Buffer");
	}
	else
	{
		return false;
	}

	return CreateShadowMaps();
}

bool Renderer::InitPrePass()
{
	using namespace Microsoft::WRL;

	ComPtr<IDxcBlob> VSBlob = D3D12ShaderCompiler::CompileFromFile("Shaders/PrePass.hlsl", "Main", "vs_6_0");
	if (!VSBlob)
	{
		return false;
	}

	// Init RootSignature
	D3D12_DESCRIPTOR_RANGE PerFrameRanges[1] = {};
	// Camera Buffer
	PerFrameRanges[0].BaseShaderRegister				= 1;
	PerFrameRanges[0].NumDescriptors					= 1;
	PerFrameRanges[0].RegisterSpace						= 0;
	PerFrameRanges[0].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
	PerFrameRanges[0].OffsetInDescriptorsFromTableStart	= 0;

	// Transform Constants
	D3D12_ROOT_PARAMETER Parameters[2];
	Parameters[0].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	Parameters[0].Constants.ShaderRegister	= 0;
	Parameters[0].Constants.RegisterSpace	= 0;
	Parameters[0].Constants.Num32BitValues	= 16;
	Parameters[0].ShaderVisibility			= D3D12_SHADER_VISIBILITY_VERTEX;

	// PerFrame DescriptorTable
	Parameters[1].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	Parameters[1].DescriptorTable.NumDescriptorRanges	= 1;
	Parameters[1].DescriptorTable.pDescriptorRanges		= PerFrameRanges;
	Parameters[1].ShaderVisibility						= D3D12_SHADER_VISIBILITY_VERTEX;

	D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = { };
	RootSignatureDesc.NumParameters		= 2;
	RootSignatureDesc.pParameters		= Parameters;
	RootSignatureDesc.NumStaticSamplers = 0;
	RootSignatureDesc.pStaticSamplers	= nullptr;
	RootSignatureDesc.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT	|
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS			|
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS		|
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS		|
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	PrePassRootSignature = RenderingAPI::Get().CreateRootSignature(RootSignatureDesc);
	if (!PrePassRootSignature)
	{
		return false;
	}

	// Init PipelineState
	D3D12_INPUT_ELEMENT_DESC InputElementDesc[] =
	{
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 0,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 12,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT",	0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 24,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT,	0, 36,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	GraphicsPipelineStateProperties PSOProperties = { };
	PSOProperties.DebugName			= "PrePass PipelineState";
	PSOProperties.VSBlob			= VSBlob.Get();
	PSOProperties.PSBlob			= nullptr;
	PSOProperties.RootSignature		= PrePassRootSignature.Get();
	PSOProperties.InputElements		= InputElementDesc;
	PSOProperties.NumInputElements	= 4;
	PSOProperties.EnableDepth		= true;
	PSOProperties.EnableBlending	= false;
	PSOProperties.DepthBufferFormat = DepthBufferFormat;
	PSOProperties.DepthFunc			= D3D12_COMPARISON_FUNC_LESS;
	PSOProperties.CullMode			= D3D12_CULL_MODE_BACK;
	PSOProperties.RTFormats			= nullptr;
	PSOProperties.NumRenderTargets	= 0;

	PrePassPSO = RenderingAPI::Get().CreateGraphicsPipelineState(PSOProperties);
	if (!PrePassPSO)
	{
		return false;
	}

	// Init descriptortable
	PrePassDescriptorTable = RenderingAPI::Get().CreateDescriptorTable(1);
	PrePassDescriptorTable->SetConstantBufferView(CameraBuffer->GetConstantBufferView().Get(), 0);
	PrePassDescriptorTable->CopyDescriptors();

	return true;
}

bool Renderer::InitShadowMapPass()
{
	using namespace Microsoft::WRL;

	// Directional Shadows
#if ENABLE_VSM
	ComPtr<IDxcBlob> VSBlob = D3D12ShaderCompiler::CompileFromFile("Shaders/ShadowMap.hlsl", "VSM_VSMain", "vs_6_0");
	if (!VSBlob)
	{
		return false;
	}

	ComPtr<IDxcBlob> PSBlob = D3D12ShaderCompiler::CompileFromFile("Shaders/ShadowMap.hlsl", "VSM_PSMain", "ps_6_0");
	if (!PSBlob)
	{
		return false;
	}
#else
	ComPtr<IDxcBlob> VSBlob = D3D12ShaderCompiler::CompileFromFile("Shaders/ShadowMap.hlsl", "Main", "vs_6_0");
	if (!VSBlob)
	{
		return false;
	}
#endif

	D3D12_ROOT_PARAMETER Parameters[2];
	// Transform Constants
	Parameters[0].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	Parameters[0].Constants.ShaderRegister	= 0;
	Parameters[0].Constants.RegisterSpace	= 0;
	Parameters[0].Constants.Num32BitValues	= 16;
	Parameters[0].ShaderVisibility			= D3D12_SHADER_VISIBILITY_VERTEX;

	// Camera
	Parameters[1].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	Parameters[1].Constants.ShaderRegister	= 1;
	Parameters[1].Constants.RegisterSpace	= 0;
	Parameters[1].Constants.Num32BitValues	= 20;
	Parameters[1].ShaderVisibility			= D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = { };
	RootSignatureDesc.NumParameters		= 2;
	RootSignatureDesc.pParameters		= Parameters;
	RootSignatureDesc.NumStaticSamplers	= 0;
	RootSignatureDesc.pStaticSamplers	= nullptr;
	RootSignatureDesc.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT	|
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS			|
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS		|
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	ShadowMapRootSignature = RenderingAPI::Get().CreateRootSignature(RootSignatureDesc);
	if (!ShadowMapRootSignature->Initialize(RootSignatureDesc))
	{
		return false;
	}

	// Init PipelineState
	D3D12_INPUT_ELEMENT_DESC InputElementDesc[] =
	{
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 0,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 12,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT",	0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 24,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT,	0, 36,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	GraphicsPipelineStateProperties PSOProperties = { };
	PSOProperties.DebugName				= "ShadowMap PipelineState";
	PSOProperties.VSBlob				= VSBlob.Get();
#if ENABLE_VSM
	PSOProperties.PSBlob				= PSBlob.Get();

	DXGI_FORMAT Format = DXGI_FORMAT_R32G32_FLOAT;
	PSOProperties.RTFormats			= &Format;
	PSOProperties.NumRenderTargets	= 1;
	PSOProperties.MultiSampleEnable	= false;
	PSOProperties.SampleCount		= 1;
	PSOProperties.SampleQuality		= 0;
#else
	PSOProperties.PSBlob			= nullptr;
	PSOProperties.RTFormats			= nullptr;
	PSOProperties.NumRenderTargets	= 0;
#endif
	PSOProperties.DepthBufferFormat		= ShadowMapFormat;
	PSOProperties.RootSignature			= ShadowMapRootSignature.Get();
	PSOProperties.InputElements			= InputElementDesc;
	PSOProperties.NumInputElements		= 4;
	PSOProperties.EnableDepth			= true;
	//PSOProperties.DepthBias			= 3;
	//PSOProperties.SlopeScaleDepthBias	= 0.01f;
	//PSOProperties.DepthBiasClamp		= 0.1f;
	PSOProperties.EnableBlending		= false;
	PSOProperties.DepthFunc				= D3D12_COMPARISON_FUNC_LESS_EQUAL;
	PSOProperties.CullMode				= D3D12_CULL_MODE_BACK;

#if ENABLE_VSM
	VSMShadowMapPSO = MakeShared<D3D12GraphicsPipelineState>(Device.Get());
	if (!VSMShadowMapPSO->Initialize(PSOProperties))
	{
		return false;
	}
#else
	ShadowMapPSO = RenderingAPI::Get().CreateGraphicsPipelineState(PSOProperties);
	if (!ShadowMapPSO->Initialize(PSOProperties))
	{
		return false;
	}
#endif

	// Linear Shadow Maps
	VSBlob = D3D12ShaderCompiler::CompileFromFile("Shaders/ShadowMap.hlsl", "VSMain", "vs_6_0");
	if (!VSBlob)
	{
		return false;
	}

#if !ENABLE_VSM
	ComPtr<IDxcBlob> PSBlob;
#endif
	PSBlob = D3D12ShaderCompiler::CompileFromFile("Shaders/ShadowMap.hlsl", "PSMain", "ps_6_0");
	if (!PSBlob)
	{
		return false;
	}

	PSOProperties.DebugName			= "Linear ShadowMap PipelineState";
	PSOProperties.VSBlob			= VSBlob.Get();
	PSOProperties.PSBlob			= PSBlob.Get();
	PSOProperties.DepthBufferFormat = ShadowMapFormat;
	PSOProperties.RTFormats			= nullptr;
	PSOProperties.NumRenderTargets	= 0;
	PSOProperties.MultiSampleEnable = false;
	PSOProperties.SampleCount		= 1;
	PSOProperties.SampleQuality		= 0;

	LinearShadowMapPSO = RenderingAPI::Get().CreateGraphicsPipelineState(PSOProperties);
	if (!LinearShadowMapPSO)
	{
		return false;
	}

	return true;
}

bool Renderer::InitDeferred()
{
	using namespace Microsoft::WRL;

	// Init PipelineState
	DxcDefine Defines[] =
	{
		{ L"ENABLE_PARALLAX_MAPPING",	L"1" },
		{ L"ENABLE_NORMAL_MAPPING",		L"1" },
	};

	ComPtr<IDxcBlob> VSBlob = D3D12ShaderCompiler::CompileFromFile("Shaders/GeometryPass.hlsl", "VSMain", "vs_6_0", Defines, 2);
	if (!VSBlob)
	{
		return false;
	}

	ComPtr<IDxcBlob> PSBlob = D3D12ShaderCompiler::CompileFromFile("Shaders/GeometryPass.hlsl", "PSMain", "ps_6_0", Defines, 2);
	if (!PSBlob)
	{
		return false;
	}

	// Init RootSignatures
	{
		D3D12_DESCRIPTOR_RANGE PerFrameRanges[1] = {};
		// Camera Buffer
		PerFrameRanges[0].BaseShaderRegister				= 1;
		PerFrameRanges[0].NumDescriptors					= 1;
		PerFrameRanges[0].RegisterSpace						= 0;
		PerFrameRanges[0].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		PerFrameRanges[0].OffsetInDescriptorsFromTableStart = 0;

		D3D12_DESCRIPTOR_RANGE PerObjectRanges[7] = {};
		// Albedo Map
		PerObjectRanges[0].BaseShaderRegister					= 0;
		PerObjectRanges[0].NumDescriptors						= 1;
		PerObjectRanges[0].RegisterSpace						= 0;
		PerObjectRanges[0].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		PerObjectRanges[0].OffsetInDescriptorsFromTableStart	= 0;

		// Normal Map
		PerObjectRanges[1].BaseShaderRegister					= 1;
		PerObjectRanges[1].NumDescriptors						= 1;
		PerObjectRanges[1].RegisterSpace						= 0;
		PerObjectRanges[1].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		PerObjectRanges[1].OffsetInDescriptorsFromTableStart	= 1;

		// Roughness Map
		PerObjectRanges[2].BaseShaderRegister					= 2;
		PerObjectRanges[2].NumDescriptors						= 1;
		PerObjectRanges[2].RegisterSpace						= 0;
		PerObjectRanges[2].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		PerObjectRanges[2].OffsetInDescriptorsFromTableStart	= 2;

		// Height Map
		PerObjectRanges[3].BaseShaderRegister					= 3;
		PerObjectRanges[3].NumDescriptors						= 1;
		PerObjectRanges[3].RegisterSpace						= 0;
		PerObjectRanges[3].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		PerObjectRanges[3].OffsetInDescriptorsFromTableStart	= 3;

		// Metallic Map
		PerObjectRanges[4].BaseShaderRegister					= 4;
		PerObjectRanges[4].NumDescriptors						= 1;
		PerObjectRanges[4].RegisterSpace						= 0;
		PerObjectRanges[4].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		PerObjectRanges[4].OffsetInDescriptorsFromTableStart	= 4;

		// AO Map
		PerObjectRanges[5].BaseShaderRegister					= 5;
		PerObjectRanges[5].NumDescriptors						= 1;
		PerObjectRanges[5].RegisterSpace						= 0;
		PerObjectRanges[5].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		PerObjectRanges[5].OffsetInDescriptorsFromTableStart	= 5;

		// Material Buffer
		PerObjectRanges[6].BaseShaderRegister					= 2;
		PerObjectRanges[6].NumDescriptors						= 1;
		PerObjectRanges[6].RegisterSpace						= 0;
		PerObjectRanges[6].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		PerObjectRanges[6].OffsetInDescriptorsFromTableStart	= 6;

		D3D12_ROOT_PARAMETER Parameters[3];
		// Transform Constants
		Parameters[0].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		Parameters[0].Constants.ShaderRegister	= 0;
		Parameters[0].Constants.RegisterSpace	= 0;
		Parameters[0].Constants.Num32BitValues	= 16;
		Parameters[0].ShaderVisibility			= D3D12_SHADER_VISIBILITY_ALL;

		// PerFrame DescriptorTable
		Parameters[1].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		Parameters[1].DescriptorTable.NumDescriptorRanges	= 1;
		Parameters[1].DescriptorTable.pDescriptorRanges		= PerFrameRanges;
		Parameters[1].ShaderVisibility						= D3D12_SHADER_VISIBILITY_ALL;

		// PerObject DescriptorTable
		Parameters[2].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		Parameters[2].DescriptorTable.NumDescriptorRanges	= 7;
		Parameters[2].DescriptorTable.pDescriptorRanges		= PerObjectRanges;
		Parameters[2].ShaderVisibility						= D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_STATIC_SAMPLER_DESC MaterialSampler = { };
		MaterialSampler.Filter				= D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		MaterialSampler.AddressU			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		MaterialSampler.AddressV			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		MaterialSampler.AddressW			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		MaterialSampler.MipLODBias			= 0.0f;
		MaterialSampler.MaxAnisotropy		= 0;
		MaterialSampler.ComparisonFunc		= D3D12_COMPARISON_FUNC_NEVER;
		MaterialSampler.BorderColor			= D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		MaterialSampler.MinLOD				= 0.0f;
		MaterialSampler.MaxLOD				= FLT_MAX;
		MaterialSampler.ShaderRegister		= 0;
		MaterialSampler.RegisterSpace		= 0;
		MaterialSampler.ShaderVisibility	= D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = { };
		RootSignatureDesc.NumParameters		= 3;
		RootSignatureDesc.pParameters		= Parameters;
		RootSignatureDesc.NumStaticSamplers	= 1;
		RootSignatureDesc.pStaticSamplers	= &MaterialSampler;
		RootSignatureDesc.Flags				= 
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		GeometryRootSignature = RenderingAPI::Get().CreateRootSignature(RootSignatureDesc);
		if (!GeometryRootSignature->Initialize(RootSignatureDesc))
		{
			return false;
		}
	}

	{
		D3D12_DESCRIPTOR_RANGE Ranges[1] = {};
		// Skybox Buffer
		Ranges[0].BaseShaderRegister				= 0;
		Ranges[0].NumDescriptors					= 1;
		Ranges[0].RegisterSpace						= 0;
		Ranges[0].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		Ranges[0].OffsetInDescriptorsFromTableStart	= 0;

		D3D12_ROOT_PARAMETER Parameters[2];
		Parameters[0].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		Parameters[0].Constants.ShaderRegister	= 0;
		Parameters[0].Constants.RegisterSpace	= 0;
		Parameters[0].Constants.Num32BitValues	= 16;
		Parameters[0].ShaderVisibility			= D3D12_SHADER_VISIBILITY_VERTEX;

		Parameters[1].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		Parameters[1].DescriptorTable.NumDescriptorRanges	= 1;
		Parameters[1].DescriptorTable.pDescriptorRanges		= Ranges;
		Parameters[1].ShaderVisibility						= D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_STATIC_SAMPLER_DESC SkyboxSampler = { };
		SkyboxSampler.Filter			= D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		SkyboxSampler.AddressU			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		SkyboxSampler.AddressV			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		SkyboxSampler.AddressW			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		SkyboxSampler.MipLODBias		= 0.0f;
		SkyboxSampler.MaxAnisotropy		= 0;
		SkyboxSampler.ComparisonFunc	= D3D12_COMPARISON_FUNC_NEVER;
		SkyboxSampler.BorderColor		= D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		SkyboxSampler.MinLOD			= 0.0f;
		SkyboxSampler.MaxLOD			= 0.0f;
		SkyboxSampler.ShaderRegister	= 0;
		SkyboxSampler.RegisterSpace		= 0;
		SkyboxSampler.ShaderVisibility	= D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = { };
		RootSignatureDesc.NumParameters		= 2;
		RootSignatureDesc.pParameters		= Parameters;
		RootSignatureDesc.NumStaticSamplers	= 1;
		RootSignatureDesc.pStaticSamplers	= &SkyboxSampler;
		RootSignatureDesc.Flags				=
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		SkyboxRootSignature = RenderingAPI::Get().CreateRootSignature(RootSignatureDesc);
		if (!SkyboxRootSignature)
		{
			return false;
		}
	}

	{
		D3D12_DESCRIPTOR_RANGE Ranges[13] = { };
		// Albedo
		Ranges[0].BaseShaderRegister				= 0;
		Ranges[0].NumDescriptors					= 1;
		Ranges[0].RegisterSpace						= 0;
		Ranges[0].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		Ranges[0].OffsetInDescriptorsFromTableStart = 0;

		// Normal
		Ranges[1].BaseShaderRegister				= 1;
		Ranges[1].NumDescriptors					= 1;
		Ranges[1].RegisterSpace						= 0;
		Ranges[1].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		Ranges[1].OffsetInDescriptorsFromTableStart	= 1;

		// Material
		Ranges[2].BaseShaderRegister				= 2;
		Ranges[2].NumDescriptors					= 1;
		Ranges[2].RegisterSpace						= 0;
		Ranges[2].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		Ranges[2].OffsetInDescriptorsFromTableStart	= 2;

		// Depth
		Ranges[3].BaseShaderRegister				= 3;
		Ranges[3].NumDescriptors					= 1;
		Ranges[3].RegisterSpace						= 0;
		Ranges[3].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		Ranges[3].OffsetInDescriptorsFromTableStart	= 3;

		// DXR-Reflections
		Ranges[4].BaseShaderRegister				= 4;
		Ranges[4].NumDescriptors					= 1;
		Ranges[4].RegisterSpace						= 0;
		Ranges[4].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		Ranges[4].OffsetInDescriptorsFromTableStart = 4;

		// IrradianceMap
		Ranges[5].BaseShaderRegister				= 5;
		Ranges[5].NumDescriptors					= 1;
		Ranges[5].RegisterSpace						= 0;
		Ranges[5].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		Ranges[5].OffsetInDescriptorsFromTableStart = 5;

		// Specular IrradianceMap
		Ranges[6].BaseShaderRegister				= 6;
		Ranges[6].NumDescriptors					= 1;
		Ranges[6].RegisterSpace						= 0;
		Ranges[6].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		Ranges[6].OffsetInDescriptorsFromTableStart = 6;

		// Integration LUT
		Ranges[7].BaseShaderRegister				= 7;
		Ranges[7].NumDescriptors					= 1;
		Ranges[7].RegisterSpace						= 0;
		Ranges[7].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		Ranges[7].OffsetInDescriptorsFromTableStart = 7;

		// Directional ShadowMaps
		Ranges[8].BaseShaderRegister				= 8;
		Ranges[8].NumDescriptors					= 1;
		Ranges[8].RegisterSpace						= 0;
		Ranges[8].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		Ranges[8].OffsetInDescriptorsFromTableStart = 8;

		// PointLight ShadowMaps
		Ranges[9].BaseShaderRegister				= 9;
		Ranges[9].NumDescriptors					= 1;
		Ranges[9].RegisterSpace						= 0;
		Ranges[9].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		Ranges[9].OffsetInDescriptorsFromTableStart = 9;

		// Camera
		Ranges[10].BaseShaderRegister					= 0;
		Ranges[10].NumDescriptors						= 1;
		Ranges[10].RegisterSpace						= 0;
		Ranges[10].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		Ranges[10].OffsetInDescriptorsFromTableStart	= 10;

		// PointLights
		Ranges[11].BaseShaderRegister					= 1;
		Ranges[11].NumDescriptors						= 1;
		Ranges[11].RegisterSpace						= 0;
		Ranges[11].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		Ranges[11].OffsetInDescriptorsFromTableStart	= 11;

		// DirectionalLights
		Ranges[12].BaseShaderRegister					= 2;
		Ranges[12].NumDescriptors						= 1;
		Ranges[12].RegisterSpace						= 0;
		Ranges[12].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		Ranges[12].OffsetInDescriptorsFromTableStart	= 12;

		D3D12_ROOT_PARAMETER Parameters[1];
		Parameters[0].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		Parameters[0].DescriptorTable.NumDescriptorRanges	= 13;
		Parameters[0].DescriptorTable.pDescriptorRanges		= Ranges;
		Parameters[0].ShaderVisibility						= D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_STATIC_SAMPLER_DESC Samplers[5] = { };
		Samplers[0].Filter				= D3D12_FILTER_MIN_MAG_MIP_POINT;
		Samplers[0].AddressU			= D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		Samplers[0].AddressV			= D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		Samplers[0].AddressW			= D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		Samplers[0].MipLODBias			= 0.0f;
		Samplers[0].MaxAnisotropy		= 0;
		Samplers[0].ComparisonFunc		= D3D12_COMPARISON_FUNC_NEVER;
		Samplers[0].BorderColor			= D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		Samplers[0].MinLOD				= 0.0f;
		Samplers[0].MaxLOD				= FLT_MAX;
		Samplers[0].ShaderRegister		= 0;
		Samplers[0].RegisterSpace		= 0;
		Samplers[0].ShaderVisibility	= D3D12_SHADER_VISIBILITY_PIXEL;

		Samplers[1].Filter				= D3D12_FILTER_MIN_MAG_MIP_POINT;
		Samplers[1].AddressU			= D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		Samplers[1].AddressV			= D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		Samplers[1].AddressW			= D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		Samplers[1].MipLODBias			= 0.0f;
		Samplers[1].MaxAnisotropy		= 0;
		Samplers[1].ComparisonFunc		= D3D12_COMPARISON_FUNC_NEVER;
		Samplers[1].BorderColor			= D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		Samplers[1].MinLOD				= 0.0f;
		Samplers[1].MaxLOD				= FLT_MAX;
		Samplers[1].ShaderRegister		= 1;
		Samplers[1].RegisterSpace		= 0;
		Samplers[1].ShaderVisibility	= D3D12_SHADER_VISIBILITY_PIXEL;

		Samplers[2].Filter				= D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		Samplers[2].AddressU			= D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		Samplers[2].AddressV			= D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		Samplers[2].AddressW			= D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		Samplers[2].MipLODBias			= 0.0f;
		Samplers[2].MaxAnisotropy		= 0;
		Samplers[2].ComparisonFunc		= D3D12_COMPARISON_FUNC_NEVER;
		Samplers[2].BorderColor			= D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		Samplers[2].MinLOD				= 0.0f;
		Samplers[2].MaxLOD				= FLT_MAX;
		Samplers[2].ShaderRegister		= 2;
		Samplers[2].RegisterSpace		= 0;
		Samplers[2].ShaderVisibility	= D3D12_SHADER_VISIBILITY_PIXEL;

		Samplers[3].Filter				= D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
		Samplers[3].AddressU			= D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		Samplers[3].AddressV			= D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		Samplers[3].AddressW			= D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		Samplers[3].MipLODBias			= 0.0f;
		Samplers[3].MaxAnisotropy		= 0;
		Samplers[3].ComparisonFunc		= D3D12_COMPARISON_FUNC_LESS;
		Samplers[3].BorderColor			= D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
		Samplers[3].MinLOD				= 0.0f;
		Samplers[3].MaxLOD				= FLT_MAX;
		Samplers[3].ShaderRegister		= 3;
		Samplers[3].RegisterSpace		= 0;
		Samplers[3].ShaderVisibility	= D3D12_SHADER_VISIBILITY_PIXEL;

		Samplers[4].Filter				= D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		Samplers[4].AddressU			= D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		Samplers[4].AddressV			= D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		Samplers[4].AddressW			= D3D12_TEXTURE_ADDRESS_MODE_BORDER;
		Samplers[4].MipLODBias			= 0.0f;
		Samplers[4].MaxAnisotropy		= 0;
		Samplers[4].ComparisonFunc		= D3D12_COMPARISON_FUNC_NEVER;
		Samplers[4].BorderColor			= D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
		Samplers[4].MinLOD				= 0.0f;
		Samplers[4].MaxLOD				= FLT_MAX;
		Samplers[4].ShaderRegister		= 4;
		Samplers[4].RegisterSpace		= 0;
		Samplers[4].ShaderVisibility	= D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = { };
		RootSignatureDesc.NumParameters			= 1;
		RootSignatureDesc.pParameters			= Parameters;
		RootSignatureDesc.NumStaticSamplers		= 5;
		RootSignatureDesc.pStaticSamplers		= Samplers;
		RootSignatureDesc.Flags					= 
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		LightRootSignature = RenderingAPI::Get().CreateRootSignature(RootSignatureDesc);
		if (!LightRootSignature)
		{
			return false;
		}
	}

	// Init PipelineState
	D3D12_INPUT_ELEMENT_DESC InputElementDesc[] =
	{
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 0,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 12,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT",	0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 24,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0, DXGI_FORMAT_R32G32_FLOAT,	0, 36,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	GraphicsPipelineStateProperties PSOProperties = { };
	PSOProperties.DebugName			= "GeometryPass PipelineState";
	PSOProperties.VSBlob			= VSBlob.Get();
	PSOProperties.PSBlob			= PSBlob.Get();
	PSOProperties.RootSignature		= GeometryRootSignature.Get();
	PSOProperties.InputElements		= InputElementDesc;
	PSOProperties.NumInputElements	= 4;
	PSOProperties.EnableDepth		= true;
	PSOProperties.EnableBlending	= false;
	PSOProperties.DepthFunc			= D3D12_COMPARISON_FUNC_LESS_EQUAL;
	PSOProperties.DepthBufferFormat = DepthBufferFormat;
	PSOProperties.CullMode			= D3D12_CULL_MODE_BACK;

	DXGI_FORMAT Formats[] =
	{
		DXGI_FORMAT_R8G8B8A8_UNORM,
		NormalFormat,
		DXGI_FORMAT_R8G8B8A8_UNORM,
	};

	PSOProperties.RTFormats			= Formats;
	PSOProperties.NumRenderTargets	= 3;

	GeometryPSO = RenderingAPI::Get().CreateGraphicsPipelineState(PSOProperties);
	if (!GeometryPSO)
	{
		return false;
	}

	VSBlob = D3D12ShaderCompiler::CompileFromFile("Shaders/FullscreenVS.hlsl", "Main", "vs_6_0");
	if (!VSBlob)
	{
		return false;
	}

	PSBlob = D3D12ShaderCompiler::CompileFromFile("Shaders/LightPassPS.hlsl", "Main", "ps_6_0");
	if (!PSBlob)
	{
		return false;
	}

	PSOProperties.DebugName			= "LightPass PipelineState";
	PSOProperties.VSBlob			= VSBlob.Get();
	PSOProperties.PSBlob			= PSBlob.Get();
	PSOProperties.RootSignature		= LightRootSignature.Get();
	PSOProperties.InputElements		= nullptr;
	PSOProperties.NumInputElements	= 0;
	PSOProperties.EnableDepth		= false;
	PSOProperties.DepthBufferFormat = DXGI_FORMAT_UNKNOWN;

	DXGI_FORMAT FinalFormat[] =
	{
		RenderTargetFormat
	};

	PSOProperties.RTFormats			= FinalFormat;
	PSOProperties.NumRenderTargets	= 1;
	PSOProperties.CullMode			= D3D12_CULL_MODE_NONE;

	LightPassPSO = RenderingAPI::Get().CreateGraphicsPipelineState(PSOProperties);
	if (!LightPassPSO)
	{
		return false;
	}

	VSBlob = D3D12ShaderCompiler::CompileFromFile("Shaders/Skybox.hlsl", "VSMain", "vs_6_0");
	if (!VSBlob)
	{
		return false;
	}

	PSBlob = D3D12ShaderCompiler::CompileFromFile("Shaders/Skybox.hlsl", "PSMain", "ps_6_0");
	if (!PSBlob)
	{
		return false;
	}

	PSOProperties.DebugName			= "Skybox PipelineState";
	PSOProperties.VSBlob			= VSBlob.Get();
	PSOProperties.PSBlob			= PSBlob.Get();
	PSOProperties.RootSignature		= SkyboxRootSignature.Get();
	PSOProperties.InputElements		= InputElementDesc;
	PSOProperties.NumInputElements	= 4;
	PSOProperties.EnableDepth		= true;
	PSOProperties.DepthFunc			= D3D12_COMPARISON_FUNC_LESS_EQUAL;
	PSOProperties.DepthWriteMask	= D3D12_DEPTH_WRITE_MASK_ZERO;
	PSOProperties.DepthBufferFormat = DepthBufferFormat;
	PSOProperties.RTFormats			= FinalFormat;
	PSOProperties.NumRenderTargets	= 1;

	SkyboxPSO = RenderingAPI::Get().CreateGraphicsPipelineState(PSOProperties);
	if (!SkyboxPSO)
	{
		return false;
	}

	// Init descriptortable
	GeometryDescriptorTable = RenderingAPI::Get().CreateDescriptorTable(1);
	GeometryDescriptorTable->SetConstantBufferView(CameraBuffer->GetConstantBufferView().Get(), 0);
	GeometryDescriptorTable->CopyDescriptors();
	
	LightDescriptorTable = RenderingAPI::Get().CreateDescriptorTable(13);
	LightDescriptorTable->SetShaderResourceView(GBuffer[0]->GetShaderResourceView(0).Get(), 0);
	LightDescriptorTable->SetShaderResourceView(GBuffer[1]->GetShaderResourceView(0).Get(), 1);
	LightDescriptorTable->SetShaderResourceView(GBuffer[2]->GetShaderResourceView(0).Get(), 2);
	LightDescriptorTable->SetShaderResourceView(GBuffer[3]->GetShaderResourceView(0).Get(), 3);
	// #4 is set after deferred and raytracing
	// #5 is set after deferred and raytracing
	// #6 is set after deferred and raytracing
	// #7 is set after deferred and raytracing
	// #8 is set after deferred and raytracing
	LightDescriptorTable->SetConstantBufferView(CameraBuffer->GetConstantBufferView().Get(), 10);
	LightDescriptorTable->SetConstantBufferView(PointLightBuffer->GetConstantBufferView().Get(), 11);
	LightDescriptorTable->SetConstantBufferView(DirectionalLightBuffer->GetConstantBufferView().Get(), 12);

	SkyboxDescriptorTable = RenderingAPI::Get().CreateDescriptorTable(2);
	SkyboxDescriptorTable->SetShaderResourceView(Skybox->GetShaderResourceView(0).Get(), 0);
	SkyboxDescriptorTable->SetConstantBufferView(CameraBuffer->GetConstantBufferView().Get(), 1);
	SkyboxDescriptorTable->CopyDescriptors();

	return true;
}

bool Renderer::InitGBuffer()
{
	// Create image
	if (!InitRayTracingTexture())
	{
		return false;
	}

	const Uint32 Width	= RenderingAPI::Get().GetSwapChain()->GetWidth();
	const Uint32 Height	= RenderingAPI::Get().GetSwapChain()->GetWidth();
	const Uint32 Usage	= TextureUsage_Default | TextureUsage_RTV | TextureUsage_SRV;
	const EFormat AlbedoFormat		= EFormat::Format_R8G8B8A8_Unorm;
	const EFormat MaterialFormat	= EFormat::Format_R8G8B8A8_Unorm;

	GBuffer[0] = CreateTexture2D(nullptr, AlbedoFormat, Usage, Width, Height, 1, 1, ClearValue(ColorClearValue(0.0f, 0.0f, 0.0f, 1.0f)));
	if (GBuffer[0])
	{
		GBuffer[0]->SetName("GBuffer Albedo");

		GBufferSRVs[0] = CreateShaderResourceView(GBuffer[0].Get(), AlbedoFormat, 0, 1);
		if (!GBufferSRVs[0])
		{
			return false;
		}

		GBufferRTVs[0] = CreateRenderTargetView(GBuffer[0].Get(), AlbedoFormat, 0);
		if (!GBufferSRVs[0])
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	// Normal
	GBuffer[1] = CreateTexture2D(nullptr, NormalFormat, Usage, Width, Height, 1, 1, ClearValue(ColorClearValue(0.0f, 0.0f, 0.0f, 1.0f)));
	if (GBuffer[1])
	{
		GBuffer[1]->SetName("GBuffer Normal");

		GBufferSRVs[1] = CreateShaderResourceView(GBuffer[1].Get(), NormalFormat, 0, 1);
		if (!GBufferSRVs[1])
		{
			return false;
		}

		GBufferRTVs[1] = CreateRenderTargetView(GBuffer[1].Get(), NormalFormat, 0);
		if (!GBufferSRVs[1])
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	// Material Properties
	GBuffer[2] = CreateTexture2D(nullptr, MaterialFormat, Usage, Width, Height, 1, 1, ClearValue(ColorClearValue(0.0f, 0.0f, 0.0f, 1.0f)));
	if (GBuffer[2])
	{
		GBuffer[2]->SetName("GBuffer Material");

		GBufferSRVs[2] = CreateShaderResourceView(GBuffer[2].Get(), MaterialFormat, 0, 1);
		if (!GBufferSRVs[2])
		{
			return false;
		}

		GBufferRTVs[2] = CreateRenderTargetView(GBuffer[2].Get(), MaterialFormat, 0);
		if (!GBufferSRVs[2])
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	// DepthStencil
	const Uint32 UsageDS = TextureUsage_Default | TextureUsage_DSV | TextureUsage_SRV;
	GBuffer[3] = CreateTexture2D(nullptr, EFormat::Format_R32_Typeless, UsageDS, Width, Height, 1, 1, ClearValue(DepthStencilClearValue(1.0f, 0)));
	if (GBuffer[3])
	{
		GBuffer[3]->SetName("GBuffer DepthStencil");

		GBufferSRVs[3] = CreateShaderResourceView(GBuffer[3].Get(), EFormat::Format_R32_Float, 0, 1);
		if (!GBufferSRVs[3])
		{
			return false;
		}

		GBufferDSV = CreateDepthStencilView(GBuffer[3].Get(), DepthBufferFormat, 0);
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
	FinalTarget = CreateTexture2D(nullptr, RenderTargetFormat, Usage, Width, Height, 1, 1, ClearValue(ColorClearValue(0.0f, 0.0f, 0.0f, 1.0f)));
	if (FinalTarget)
	{
		FinalTarget->SetName("Final Target");

		FinalTargetSRV = CreateShaderResourceView(FinalTarget.Get(), RenderTargetFormat, 0, 1);
		if (!FinalTargetSRV)
		{
			return false;
		}

		FinalTargetRTV = CreateRenderTargetView(FinalTarget.Get(), RenderTargetFormat, 0);
		if (!FinalTargetRTV)
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	// Final image descriptorset
	PostDescriptorTable = RenderingAPI::Get().CreateDescriptorTable(1);
	PostDescriptorTable->SetShaderResourceView(FinalTarget->GetShaderResourceView(0).Get(), 0);
	PostDescriptorTable->CopyDescriptors();

	return true;
}

bool Renderer::InitIntegrationLUT()
{
	constexpr EFormat LUTFormat = EFormat::Format_R16G16_Float;
	constexpr Uint32 LUTSize = 512;
	if (!RenderingAPI::Get().UAVSupportsFormat(EFormat::Format_R16G16_Float))
	{
		LOG_ERROR("[Renderer]: Format_R16G16_Float is not supported for UAVs");
		return false;
	}

	TSharedRef<Texture2D> StagingTexture = CreateTexture2D(nullptr, LUTFormat, TextureUsage_Default | TextureUsage_UAV, LUTSize, LUTSize, 1, 1, ClearValue());
	if (StagingTexture)
	{
		StagingTexture->SetName("IntegrationLUT");
	}
	else
	{
		return false;
	}

	TSharedRef<UnorderedAccessView> Uav = CreateUnorderedAccessView(StagingTexture.Get(), LUTFormat, 0);
	if (!Uav)
	{
		return false;
	}

	IntegrationLUT = CreateTexture2D(nullptr, LUTFormat, TextureUsage_Default | TextureUsage_SRV, LUTSize, LUTSize, 1, 1, ClearValue());
	if (!IntegrationLUT)
	{
		return false;
	}

	IntegrationLUTSRV = CreateShaderResourceView(IntegrationLUT.Get(), LUTFormat, 0, 1);
	if (!IntegrationLUTSRV)
	{
		return false;
	}

	Microsoft::WRL::ComPtr<IDxcBlob> Shader = D3D12ShaderCompiler::CompileFromFile("Shaders/BRDFIntegationGen.hlsl", "Main", "cs_6_0");
	if (!Shader)
	{
		return false;
	}

	TUniquePtr<D3D12RootSignature> RootSignature = TUniquePtr(RenderingAPI::Get().CreateRootSignature(Shader.Get()));
	if (!RootSignature)
	{
		return false;
	}

	ComputePipelineStateProperties PSOProperties;
	PSOProperties.DebugName		= "IntegrationGen PSO";
	PSOProperties.CSBlob		= Shader.Get();
	PSOProperties.RootSignature	= RootSignature.Get();

	TUniquePtr<D3D12ComputePipelineState> PSO = TUniquePtr(RenderingAPI::Get().CreateComputePipelineState(PSOProperties));
	if (!PSO)
	{
		return false;
	}

	TUniquePtr<D3D12DescriptorTable> DescriptorTable = TUniquePtr(RenderingAPI::Get().CreateDescriptorTable(1));
	DescriptorTable->SetUnorderedAccessView(StagingTexture->GetUnorderedAccessView(0).Get(), 0);
	DescriptorTable->CopyDescriptors();

	RenderingAPI::Get().GetImmediateCommandList()->SetComputeRootSignature(RootSignature->GetRootSignature());
	RenderingAPI::Get().GetImmediateCommandList()->SetPipelineState(PSO->GetPipeline());
				
	RenderingAPI::Get().GetImmediateCommandList()->BindGlobalOnlineDescriptorHeaps();
	RenderingAPI::Get().GetImmediateCommandList()->SetComputeRootDescriptorTable(DescriptorTable->GetGPUTableStartHandle(), 0);
				
	RenderingAPI::Get().GetImmediateCommandList()->Dispatch(LUTProperties.Width, LUTProperties.Height, 1);
	RenderingAPI::Get().GetImmediateCommandList()->UnorderedAccessBarrier(StagingTexture.Get());
				
	RenderingAPI::Get().GetImmediateCommandList()->TransitionBarrier(IntegrationLUT.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	RenderingAPI::Get().GetImmediateCommandList()->TransitionBarrier(StagingTexture.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
				
	RenderingAPI::Get().GetImmediateCommandList()->CopyResource(IntegrationLUT.Get(), StagingTexture.Get());
				
	RenderingAPI::Get().GetImmediateCommandList()->TransitionBarrier(IntegrationLUT.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				
	RenderingAPI::Get().GetImmediateCommandList()->Flush();
	RenderingAPI::Get().GetImmediateCommandList()->WaitForCompletion();

	return true;
}

bool Renderer::InitRayTracingTexture()
{
	const Uint32 Width	= RenderingAPI::Get().GetSwapChain()->GetWidth();
	const Uint32 Height	= RenderingAPI::Get().GetSwapChain()->GetHeight();
	const Uint32 Usage	= TextureUsage_Default | TextureUsage_UAV | TextureUsage_SRV;

	ReflectionTexture = CreateTexture2D(nullptr, EFormat::Format_R8G8B8A8_Unorm, Usage, Width, Height, 1, 1, ClearValue());
	if (!ReflectionTexture)
	{
		return false;
	}

	ReflectionTextureUAV = CreateUnorderedAccessView(ReflectionTexture.Get(), EFormat::Format_R8G8B8A8_Unorm, 0);
	if (!ReflectionTextureUAV)
	{
		return false;
	}

	ReflectionTextureSRV = CreateShaderResourceView(ReflectionTexture.Get(), EFormat::Format_R8G8B8A8_Unorm, 0, 1);
	if (!ReflectionTextureSRV)
	{
		return false;
	}
	return true;
}

bool Renderer::InitDebugStates()
{
	using namespace Microsoft::WRL;

	DXGI_FORMAT Formats[] =
	{
		DXGI_FORMAT_R8G8B8A8_UNORM,
	};

	ComPtr<IDxcBlob> VSBlob = D3D12ShaderCompiler::CompileFromFile("Shaders/Debug.hlsl", "VSMain", "vs_6_0");
	if (!VSBlob)
	{
		return false;
	}

	ComPtr<IDxcBlob> PSBlob = D3D12ShaderCompiler::CompileFromFile("Shaders/Debug.hlsl", "PSMain", "ps_6_0");
	if (!PSBlob)
	{
		return false;
	}

	D3D12_ROOT_PARAMETER Parameters[2];
	// Transform Constants
	Parameters[0].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	Parameters[0].Constants.ShaderRegister	= 0;
	Parameters[0].Constants.RegisterSpace	= 0;
	Parameters[0].Constants.Num32BitValues	= 16;
	Parameters[0].ShaderVisibility			= D3D12_SHADER_VISIBILITY_VERTEX;

	// Camera
	Parameters[1].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	Parameters[1].Constants.ShaderRegister	= 1;
	Parameters[1].Constants.RegisterSpace	= 0;
	Parameters[1].Constants.Num32BitValues	= 16;
	Parameters[1].ShaderVisibility			= D3D12_SHADER_VISIBILITY_ALL;

	D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = { };
	RootSignatureDesc.NumParameters		= 2;
	RootSignatureDesc.pParameters		= Parameters;
	RootSignatureDesc.NumStaticSamplers	= 0;
	RootSignatureDesc.pStaticSamplers	= nullptr;
	RootSignatureDesc.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT	|
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS			|
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS		|
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	DebugRootSignature = RenderingAPI::Get().CreateRootSignature(RootSignatureDesc);
	if (!DebugRootSignature)
	{
		return false;
	}

	D3D12_INPUT_ELEMENT_DESC InputElementDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	GraphicsPipelineStateProperties PSOProperties = { };
	PSOProperties.DebugName			= "Debug PSO";
	PSOProperties.VSBlob			= VSBlob.Get();
	PSOProperties.PSBlob			= PSBlob.Get();
	PSOProperties.RootSignature		= DebugRootSignature.Get();
	PSOProperties.InputElements		= InputElementDesc;
	PSOProperties.NumInputElements	= 1;
	PSOProperties.EnableDepth		= false;
	PSOProperties.DepthWriteMask	= D3D12_DEPTH_WRITE_MASK_ZERO;
	PSOProperties.EnableBlending	= false;
	PSOProperties.DepthFunc			= D3D12_COMPARISON_FUNC_LESS_EQUAL;
	PSOProperties.DepthBufferFormat	= DepthBufferFormat;
	PSOProperties.CullMode			= D3D12_CULL_MODE_NONE;
	PSOProperties.RTFormats			= Formats;
	PSOProperties.NumRenderTargets	= 1;
	PSOProperties.PrimitiveType		= D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

	DebugBoxPSO = RenderingAPI::Get().CreateGraphicsPipelineState(PSOProperties);
	if (!DebugBoxPSO)
	{
		return false;
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

	AABBVertexBuffer = CreateVertexBuffer<XMFLOAT3>(nullptr, 8, BufferUsage_Default);
	if (!AABBVertexBuffer)
	{
		return false;
	}

	// Create IndexBuffer
	Uint16 Indices[24] =
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

	AABBIndexBuffer = CreateIndexBuffer(nullptr, sizeof(Uint16) * 24, EIndexFormat::IndexFormat_Uint16, BufferUsage_Default);
	if (!AABBIndexBuffer)
	{
		return false;
	}

	// Upload Data
	RenderingAPI::Get().GetImmediateCommandList()->TransitionBarrier(AABBVertexBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	RenderingAPI::Get().GetImmediateCommandList()->TransitionBarrier(AABBIndexBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

	RenderingAPI::Get().GetImmediateCommandList()->UploadBufferData(AABBVertexBuffer.Get(), 0, Vertices, sizeof(Vertices));
	RenderingAPI::Get().GetImmediateCommandList()->UploadBufferData(AABBIndexBuffer.Get(), 0, Indices, sizeof(Indices));

	RenderingAPI::Get().GetImmediateCommandList()->TransitionBarrier(AABBVertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	RenderingAPI::Get().GetImmediateCommandList()->TransitionBarrier(AABBIndexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);

	RenderingAPI::Get().GetImmediateCommandList()->Flush();
	RenderingAPI::Get().GetImmediateCommandList()->WaitForCompletion();

	return true;
}

bool Renderer::InitAA()
{
	using namespace Microsoft::WRL;

	// No AA
	ComPtr<IDxcBlob> VSBlob = D3D12ShaderCompiler::CompileFromFile("Shaders/FullscreenVS.hlsl", "Main", "vs_6_0");
	if (!VSBlob)
	{
		return false;
	}

	ComPtr<IDxcBlob> PSBlob = D3D12ShaderCompiler::CompileFromFile("Shaders/PostProcessPS.hlsl", "Main", "ps_6_0");
	if (!PSBlob)
	{
		return false;
	}

	D3D12_DESCRIPTOR_RANGE Ranges[1] = {};
	Ranges[0].BaseShaderRegister				= 0;
	Ranges[0].NumDescriptors					= 1;
	Ranges[0].RegisterSpace						= 0;
	Ranges[0].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	Ranges[0].OffsetInDescriptorsFromTableStart	= 0;

	D3D12_ROOT_PARAMETER Parameters[2];
	Parameters[0].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	Parameters[0].ShaderVisibility						= D3D12_SHADER_VISIBILITY_PIXEL;
	Parameters[0].DescriptorTable.NumDescriptorRanges	= 1;
	Parameters[0].DescriptorTable.pDescriptorRanges		= Ranges;

	Parameters[1].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	Parameters[1].ShaderVisibility			= D3D12_SHADER_VISIBILITY_PIXEL;
	Parameters[1].Constants.Num32BitValues	= 2;
	Parameters[1].Constants.RegisterSpace	= 0;
	Parameters[1].Constants.ShaderRegister	= 0;

	D3D12_STATIC_SAMPLER_DESC Samplers[2] = { };
	Samplers[0].Filter				= D3D12_FILTER_MIN_MAG_MIP_POINT;
	Samplers[0].AddressU			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	Samplers[0].AddressV			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	Samplers[0].AddressW			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	Samplers[0].MipLODBias			= 0.0f;
	Samplers[0].MaxAnisotropy		= 0;
	Samplers[0].ComparisonFunc		= D3D12_COMPARISON_FUNC_NEVER;
	Samplers[0].BorderColor			= D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	Samplers[0].MinLOD				= 0.0f;
	Samplers[0].MaxLOD				= 1.0f;
	Samplers[0].ShaderRegister		= 0;
	Samplers[0].RegisterSpace		= 0;
	Samplers[0].ShaderVisibility	= D3D12_SHADER_VISIBILITY_PIXEL;

	Samplers[1].Filter				= D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	Samplers[1].AddressU			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	Samplers[1].AddressV			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	Samplers[1].AddressW			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	Samplers[1].MipLODBias			= 0.0f;
	Samplers[1].MaxAnisotropy		= 0;
	Samplers[1].ComparisonFunc		= D3D12_COMPARISON_FUNC_NEVER;
	Samplers[1].BorderColor			= D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	Samplers[1].MinLOD				= 0.0f;
	Samplers[1].MaxLOD				= 1.0f;
	Samplers[1].ShaderRegister		= 1;
	Samplers[1].RegisterSpace		= 0;
	Samplers[1].ShaderVisibility	= D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = { };
	RootSignatureDesc.NumParameters		= 2;
	RootSignatureDesc.pParameters		= Parameters;
	RootSignatureDesc.NumStaticSamplers	= 2;
	RootSignatureDesc.pStaticSamplers	= Samplers;
	RootSignatureDesc.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS		|
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS	|
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	PostRootSignature = RenderingAPI::Get().CreateRootSignature(RootSignatureDesc);
	if (!PostRootSignature)
	{
		return false;
	}

	DXGI_FORMAT Formats[] =
	{
		DXGI_FORMAT_R8G8B8A8_UNORM,
	};

	GraphicsPipelineStateProperties PSOProperties = { };
	PSOProperties.DebugName			= "Post PSO";
	PSOProperties.VSBlob			= VSBlob.Get();
	PSOProperties.PSBlob			= PSBlob.Get();
	PSOProperties.RootSignature		= PostRootSignature.Get();
	PSOProperties.InputElements		= nullptr;
	PSOProperties.NumInputElements	= 0;
	PSOProperties.EnableDepth		= false;
	PSOProperties.DepthWriteMask	= D3D12_DEPTH_WRITE_MASK_ZERO;
	PSOProperties.EnableBlending	= false;
	PSOProperties.DepthFunc			= D3D12_COMPARISON_FUNC_ALWAYS;
	PSOProperties.DepthBufferFormat	= DXGI_FORMAT_UNKNOWN;
	PSOProperties.CullMode			= D3D12_CULL_MODE_NONE;
	PSOProperties.RTFormats			= Formats;
	PSOProperties.NumRenderTargets	= 1;
	PSOProperties.PrimitiveType		= D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	PostPSO = RenderingAPI::Get().CreateGraphicsPipelineState(PSOProperties);
	if (!PostPSO)
	{
		return false;
	}

	// FXAA
	PSBlob = D3D12ShaderCompiler::CompileFromFile("Shaders/FXAA_PS.hlsl", "Main", "ps_6_0");
	if (!PSBlob)
	{
		return false;
	}

	PSOProperties.DebugName = "FXAA PSO";
	PSOProperties.VSBlob	= VSBlob.Get();
	PSOProperties.PSBlob	= PSBlob.Get();

	FXAAPSO = RenderingAPI::Get().CreateGraphicsPipelineState(PSOProperties);
	if (!FXAAPSO)
	{
		return false;
	}

	return true;
}

bool Renderer::CreateShadowMaps()
{
	// DirLights
	if (DirLightShadowMaps)
	{
		DeferredResources.EmplaceBack(DirLightShadowMaps);
	}

	if (VSMDirLightShadowMaps)
	{
		DeferredResources.EmplaceBack(VSMDirLightShadowMaps);
	}

	TextureProperties ShadowMapProps = { };
	ShadowMapProps.DebugName	= "Directional Light ShadowMaps";
	ShadowMapProps.ArrayCount	= 1;
	ShadowMapProps.Format		= ShadowMapFormat;
	ShadowMapProps.Flags		= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	ShadowMapProps.Height		= Renderer::GetGlobalLightSettings().ShadowMapHeight;
	ShadowMapProps.Width		= Renderer::GetGlobalLightSettings().ShadowMapWidth;
	ShadowMapProps.InitalState	= D3D12_RESOURCE_STATE_COMMON;
	ShadowMapProps.MemoryType	= EMemoryType::MEMORY_TYPE_DEFAULT;
	ShadowMapProps.MipLevels	= 1;
	ShadowMapProps.SampleCount	= 1;
	
	D3D12_CLEAR_VALUE Value0;
	Value0.Format				= ShadowMapFormat;
	Value0.DepthStencil.Depth	= 1.0f;
	Value0.DepthStencil.Stencil	= 0;
	ShadowMapProps.OptimizedClearValue = &Value0;

	DirLightShadowMaps = RenderingAPI::Get().CreateTexture(ShadowMapProps);
	if (DirLightShadowMaps)
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc = { };
		DSVDesc.Format				= ShadowMapFormat;
		DSVDesc.ViewDimension		= D3D12_DSV_DIMENSION_TEXTURE2D;
		DSVDesc.Texture2D.MipSlice	= 0;
		DirLightShadowMaps->SetDepthStencilView(TSharedPtr(RenderingAPI::Get().CreateDepthStencilView(DirLightShadowMaps->GetResource(), &DSVDesc)), 0);

#if !ENABLE_VSM
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = { };
		SRVDesc.Format = DXGI_FORMAT_R32_FLOAT;
		SRVDesc.ViewDimension					= D3D12_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels				= 1;
		SRVDesc.Texture2D.MostDetailedMip		= 0;
		SRVDesc.Texture2D.PlaneSlice			= 0;
		SRVDesc.Texture2D.ResourceMinLODClamp	= 0.0f;
		SRVDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		DirLightShadowMaps->SetShaderResourceView(TSharedPtr(RenderingAPI::Get().CreateShaderResourceView(DirLightShadowMaps->GetResource(), &SRVDesc)), 0);
#endif
	}
	else
	{
		return false;
	}

#if ENABLE_VSM
	ShadowMapProps.DebugName	= "Directional Light ShadowMaps";
	ShadowMapProps.ArrayCount	= 1;
	ShadowMapProps.Format		= DXGI_FORMAT_R32G32_FLOAT;
	ShadowMapProps.Flags		= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	ShadowMapProps.Height		= Renderer::GetGlobalLightSettings().ShadowMapHeight;
	ShadowMapProps.Width		= Renderer::GetGlobalLightSettings().ShadowMapWidth;
	ShadowMapProps.InitalState	= D3D12_RESOURCE_STATE_COMMON;
	ShadowMapProps.MemoryType	= EMemoryType::MEMORY_TYPE_DEFAULT;
	ShadowMapProps.MipLevels	= 1;
	ShadowMapProps.SampleCount	= 1;

	D3D12_CLEAR_VALUE Value1;
	Value1.Format	= ShadowMapProps.Format;
	Value1.Color[0]	= 1.0f;
	Value1.Color[1]	= 1.0f;
	Value1.Color[2]	= 1.0f;
	Value1.Color[3]	= 1.0f;
	ShadowMapProps.OptimizedClearValue = &Value1;

	VSMDirLightShadowMaps = RenderingAPI::Get().CreateTexture(ShadowMapProps);
	if (VSMDirLightShadowMaps)
	{
		D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = { };
		RTVDesc.Format					= ShadowMapProps.Format;
		RTVDesc.ViewDimension			= D3D12_RTV_DIMENSION_TEXTURE2D;
		RTVDesc.Texture2D.MipSlice		= 0;
		RTVDesc.Texture2D.PlaneSlice	= 0;
		VSMDirLightShadowMaps->SetRenderTargetView(TSharedPtr(RenderingAPI::Get().CreateRenderTargetView(VSMDirLightShadowMaps->GetResource(), &RTVDesc)), 0);
	
		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = { };
		SRVDesc.Format							= ShadowMapProps.Format;
		SRVDesc.ViewDimension					= D3D12_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels				= 1;
		SRVDesc.Texture2D.MostDetailedMip		= 0;
		SRVDesc.Texture2D.PlaneSlice			= 0;
		SRVDesc.Texture2D.ResourceMinLODClamp	= 0.0f;
		SRVDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		VSMDirLightShadowMaps->SetShaderResourceView(TSharedPtr(RenderingAPI::Get().CreateShaderResourceView(VSMDirLightShadowMaps->GetResource(), &SRVDesc)), 0);
	}
	else
	{
		return false;
	}
#endif

	// PointLights
	if (PointLightShadowMaps)
	{
		DeferredResources.EmplaceBack(PointLightShadowMaps);
	}

	const Uint16 Size = Renderer::GetGlobalLightSettings().PointLightShadowSize;
	ShadowMapProps.DebugName			= "PointLight ShadowMaps";
	ShadowMapProps.ArrayCount			= 6;
	ShadowMapProps.Format				= ShadowMapFormat;
	ShadowMapProps.Flags				= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	ShadowMapProps.Height				= Size;
	ShadowMapProps.Width				= Size;
	ShadowMapProps.InitalState			= D3D12_RESOURCE_STATE_COMMON;
	ShadowMapProps.MemoryType			= EMemoryType::MEMORY_TYPE_DEFAULT;
	ShadowMapProps.MipLevels			= 1;
	ShadowMapProps.OptimizedClearValue	= &Value0;
	PointLightShadowMaps = RenderingAPI::Get().CreateTexture(ShadowMapProps);
	if (PointLightShadowMaps)
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc = { };
		DSVDesc.Format						= ShadowMapFormat;
		DSVDesc.ViewDimension				= D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		DSVDesc.Texture2DArray.ArraySize	= 1;
		DSVDesc.Texture2DArray.MipSlice		= 0;
		for (Uint32 I = 0; I < 6; I++)
		{
			DSVDesc.Texture2DArray.FirstArraySlice = I;
			PointLightShadowMaps->SetDepthStencilView(TSharedPtr(RenderingAPI::Get().CreateDepthStencilView(PointLightShadowMaps->GetResource(), &DSVDesc)), I);
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = { };
		SRVDesc.Format							= DXGI_FORMAT_R32_FLOAT;
		SRVDesc.ViewDimension					= D3D12_SRV_DIMENSION_TEXTURECUBE;
		SRVDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SRVDesc.TextureCube.MipLevels			= 1;
		SRVDesc.TextureCube.MostDetailedMip		= 0;
		SRVDesc.TextureCube.ResourceMinLODClamp	= 0.0f;
		PointLightShadowMaps->SetShaderResourceView(TSharedPtr(RenderingAPI::Get().CreateShaderResourceView(PointLightShadowMaps->GetResource(), &SRVDesc)), 0);
	}
	else
	{
		return false;
	}

	return true;
}

void Renderer::WriteShadowMapDescriptors()
{
#if ENABLE_VSM
	LightDescriptorTable->SetShaderResourceView(VSMDirLightShadowMaps->GetShaderResourceView(0).Get(), 8);
#else
	LightDescriptorTable->SetShaderResourceView(DirLightShadowMaps->GetShaderResourceView(0).Get(), 8);
#endif
	LightDescriptorTable->SetShaderResourceView(PointLightShadowMaps->GetShaderResourceView(0).Get(), 9);
	LightDescriptorTable->CopyDescriptors();
}

void Renderer::GenerateIrradianceMap(TextureCube* Source, TextureCube* Dest, D3D12CommandList* InCommandList)
{
	const Uint32 Size = static_cast<Uint32>(Dest->GetDesc().Width);

	static TUniquePtr<D3D12DescriptorTable> SrvDescriptorTable;
	if (!SrvDescriptorTable)
	{
		SrvDescriptorTable = RenderingAPI::Get().CreateDescriptorTable(1);
		SrvDescriptorTable->SetShaderResourceView(Source->GetShaderResourceView(0).Get(), 0);
		SrvDescriptorTable->CopyDescriptors();
	}

	static TUniquePtr<D3D12DescriptorTable> UavDescriptorTable;
	if (!UavDescriptorTable)
	{
		UavDescriptorTable = RenderingAPI::Get().CreateDescriptorTable(1);
		UavDescriptorTable->SetUnorderedAccessView(Dest->GetUnorderedAccessView(0).Get(), 0);
		UavDescriptorTable->CopyDescriptors();
	}

	static Microsoft::WRL::ComPtr<IDxcBlob> Shader;
	if (!Shader)
	{
		Shader = D3D12ShaderCompiler::CompileFromFile("Shaders/IrradianceGen.hlsl", "Main", "cs_6_0");
	}

	if (!IrradianceGenRootSignature)
	{
		IrradianceGenRootSignature = RenderingAPI::Get().CreateRootSignature(Shader.Get());
		IrradianceGenRootSignature->SetDebugName("Irradiance Gen RootSignature");
	}

	if (!IrradicanceGenPSO)
	{
		ComputePipelineStateProperties Props = { };
		Props.DebugName		= "Irradiance Gen PSO";
		Props.CSBlob		= Shader.Get();
		Props.RootSignature	= IrradianceGenRootSignature.Get();

		IrradicanceGenPSO = RenderingAPI::Get().CreateComputePipelineState(Props);
	}

	InCommandList->TransitionBarrier(Source, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	InCommandList->TransitionBarrier(Dest, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	InCommandList->SetComputeRootSignature(IrradianceGenRootSignature->GetRootSignature());

	InCommandList->BindGlobalOnlineDescriptorHeaps();
	InCommandList->SetComputeRootDescriptorTable(SrvDescriptorTable->GetGPUTableStartHandle(), 0);
	InCommandList->SetComputeRootDescriptorTable(UavDescriptorTable->GetGPUTableStartHandle(), 1);

	InCommandList->SetPipelineState(IrradicanceGenPSO->GetPipeline());

	InCommandList->Dispatch(Size, Size, 6);

	InCommandList->UnorderedAccessBarrier(Dest);

	InCommandList->TransitionBarrier(Source, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	InCommandList->TransitionBarrier(Dest, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void Renderer::GenerateSpecularIrradianceMap(TextureCube* Source, TextureCube* Dest, D3D12CommandList* InCommandList)
{
	const Uint32 Miplevels = Dest->GetDesc().MipLevels;

	static TUniquePtr<D3D12DescriptorTable> SrvDescriptorTable;
	if (!SrvDescriptorTable)
	{
		SrvDescriptorTable = RenderingAPI::Get().CreateDescriptorTable(1);
		SrvDescriptorTable->SetShaderResourceView(Source->GetShaderResourceView(0).Get(), 0);
		SrvDescriptorTable->CopyDescriptors();
	}

	static TUniquePtr<D3D12DescriptorTable> UavDescriptorTable;
	if (!UavDescriptorTable)
	{
		UavDescriptorTable = RenderingAPI::Get().CreateDescriptorTable(Miplevels);
		for (Uint32 Mip = 0; Mip < Miplevels; Mip++)
		{
			UavDescriptorTable->SetUnorderedAccessView(Dest->GetUnorderedAccessView(Mip).Get(), Mip);
		}

		UavDescriptorTable->CopyDescriptors();
	}

	static Microsoft::WRL::ComPtr<IDxcBlob> Shader;
	if (!Shader)
	{
		Shader = D3D12ShaderCompiler::CompileFromFile("Shaders/SpecularIrradianceGen.hlsl", "Main", "cs_6_0");
	}

	if (!SpecIrradianceGenRootSignature)
	{
		SpecIrradianceGenRootSignature = RenderingAPI::Get().CreateRootSignature(Shader.Get());
		SpecIrradianceGenRootSignature->SetDebugName("Specular Irradiance Gen RootSignature");
	}

	if (!SpecIrradicanceGenPSO)
	{
		ComputePipelineStateProperties Props = { };
		Props.DebugName		= "Specular Irradiance Gen PSO";
		Props.CSBlob		= Shader.Get();
		Props.RootSignature = SpecIrradianceGenRootSignature.Get();

		SpecIrradicanceGenPSO = RenderingAPI::Get().CreateComputePipelineState(Props);
	}

	InCommandList->TransitionBarrier(Source, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	InCommandList->TransitionBarrier(Dest, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	InCommandList->SetComputeRootSignature(SpecIrradianceGenRootSignature->GetRootSignature());

	InCommandList->BindGlobalOnlineDescriptorHeaps();
	InCommandList->SetComputeRootDescriptorTable(SrvDescriptorTable->GetGPUTableStartHandle(), 1);

	InCommandList->SetPipelineState(SpecIrradicanceGenPSO->GetPipeline());

	Uint32	Width		= static_cast<Uint32>(Dest->GetDesc().Width);
	Float32 Roughness	= 0.0f;
	const Float32 RoughnessDelta = 1.0f / (Miplevels - 1);
	for (Uint32 Mip = 0; Mip < Miplevels; Mip++)
	{
		InCommandList->SetComputeRoot32BitConstants(&Roughness, 1, 0, 0);
		InCommandList->SetComputeRootDescriptorTable(UavDescriptorTable->GetGPUTableHandle(Mip), 2);
		
		InCommandList->Dispatch(Width, Width, 6);
		InCommandList->UnorderedAccessBarrier(Dest);

		Width = std::max<Uint32>(Width / 2, 1U);
		Roughness += RoughnessDelta;
	}

	InCommandList->TransitionBarrier(Source, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	InCommandList->TransitionBarrier(Dest, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void Renderer::WaitForPendingFrames()
{
	//const Uint64 CurrentFenceValue = FenceValues[CurrentBackBufferIndex];

	//Queue->SignalFence(Fence.Get(), CurrentFenceValue);
	//if (Fence->WaitForValue(CurrentFenceValue))
	//{
	//	FenceValues[CurrentBackBufferIndex]++;
	//}

	RenderingAPI::Get().GetQueue()->WaitForCompletion();
}