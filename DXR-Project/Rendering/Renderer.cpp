#include "Renderer.h"
#include "TextureFactory.h"
#include "GuiContext.h"
#include "Mesh.h"

#include "D3D12/D3D12Texture.h"
#include "D3D12/D3D12Views.h"
#include "D3D12/D3D12RootSignature.h"
#include "D3D12/D3D12GraphicsPipelineState.h"
#include "D3D12/D3D12RayTracingPipelineState.h"
#include "D3D12/D3D12ComputePipelineState.h"
#include "D3D12/D3D12ShaderCompiler.h"

#include "Application/InputManager.h"

#include <algorithm>

std::unique_ptr<Renderer> Renderer::RendererInstance = nullptr;

static const DXGI_FORMAT	NormalFormat		= DXGI_FORMAT_R10G10B10A2_UNORM;
static const DXGI_FORMAT	DepthBufferFormat	= DXGI_FORMAT_D32_FLOAT;
static const Uint32			PresentInterval		= 0;

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
}

void Renderer::Tick(const Scene& CurrentScene)
{
	D3D12Texture* BackBuffer = SwapChain->GetSurfaceResource(CurrentBackBufferIndex);
	CommandAllocators[CurrentBackBufferIndex]->Reset();
	CommandList->Reset(CommandAllocators[CurrentBackBufferIndex].get());
	if (CurrentBackBufferIndex == 0)
	{
		CommandList->ReleaseDeferredResources();
	}
	
	// Set constant buffer descriptor heap
	ID3D12DescriptorHeap* DescriptorHeaps[] = { Device->GetGlobalOnlineResourceHeap()->GetHeap() };
	CommandList->SetDescriptorHeaps(DescriptorHeaps, ARRAYSIZE(DescriptorHeaps));

	CommandList->TransitionBarrier(GBuffer[0].get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	CommandList->TransitionBarrier(GBuffer[1].get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	CommandList->TransitionBarrier(GBuffer[2].get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
	CommandList->TransitionBarrier(GBuffer[3].get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);

	struct CameraBufferDesc
	{
		XMFLOAT4X4	ViewProjection;
		XMFLOAT3	Position;
		float		Padding;
		XMFLOAT4X4	ViewProjectionInv;
	} CamBuff;

	CamBuff.ViewProjection		= CurrentScene.GetCamera()->GetViewProjection();
	CamBuff.ViewProjectionInv	= CurrentScene.GetCamera()->GetViewProjectionInverse();
	CamBuff.Position			= CurrentScene.GetCamera()->GetPosition();

	CommandList->TransitionBarrier(CameraBuffer.get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
	CommandList->UploadBufferData(CameraBuffer.get(), 0, &CamBuff, sizeof(Camera));
	CommandList->TransitionBarrier(CameraBuffer.get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	const Float32 BlackClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	CommandList->ClearRenderTargetView(GBuffer[0]->GetRenderTargetView().get(), BlackClearColor);
	CommandList->ClearRenderTargetView(GBuffer[1]->GetRenderTargetView().get(), BlackClearColor);
	CommandList->ClearRenderTargetView(GBuffer[2]->GetRenderTargetView().get(), BlackClearColor);
	CommandList->ClearDepthStencilView(GBuffer[3]->GetDepthStencilView().get(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0);

	D3D12_VIEWPORT ViewPort = { };
	ViewPort.Width		= static_cast<Float32>(SwapChain->GetWidth());
	ViewPort.Height		= static_cast<Float32>(SwapChain->GetHeight());
	ViewPort.MinDepth	= 0.0f;
	ViewPort.MaxDepth	= 1.0f;
	ViewPort.TopLeftX	= 0.0f;
	ViewPort.TopLeftY	= 0.0f;
	CommandList->RSSetViewports(&ViewPort, 1);

	CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	CommandList->SetPipelineState(GeometryPSO->GetPipelineState());
	CommandList->SetGraphicsRootSignature(GeometryRootSignature->GetRootSignature());

	const D3D12_RECT ScissorRect =
	{
		0,
		0,
		static_cast<LONG>(SwapChain->GetWidth()),
		static_cast<LONG>(SwapChain->GetHeight())
	};
	CommandList->RSSetScissorRects(&ScissorRect, 1);

	D3D12RenderTargetView* GBufferRTVS[] = 
	{ 
		GBuffer[0]->GetRenderTargetView().get(),
		GBuffer[1]->GetRenderTargetView().get(),
		GBuffer[2]->GetRenderTargetView().get()
	};
	CommandList->OMSetRenderTargets(GBufferRTVS, 3, GBuffer[3]->GetDepthStencilView().get());

	CommandList->SetGraphicsRootDescriptorTable(GeometryDescriptorTable->GetGPUTableStartHandle(), 1);

	// Render all objects to the GBuffer
	struct PerObject
	{
		XMFLOAT4X4	Matrix;
		Float32		Roughness;
		Float32		Metallic;
		Float32		AO;
	} PerObjectBuffer;

	for (Actor* CurrentActor : CurrentScene.GetActors())
	{
		MeshComponent* RComponent = reinterpret_cast<MeshComponent*>(CurrentActor->GetComponent());
		Mesh*		CurrentMesh		= RComponent->Mesh.get();
		Material*	CurrentMaterial = RComponent->Material.get();
		
		if (Device->IsRayTracingSupported())
		{
			CurrentMesh->BuildAccelerationStructure(CommandList.get());
		}

		D3D12_VERTEX_BUFFER_VIEW VBO = { };
		VBO.BufferLocation	= CurrentMesh->VertexBuffer->GetGPUVirtualAddress();
		VBO.SizeInBytes		= CurrentMesh->VertexBuffer->GetSizeInBytes();
		VBO.StrideInBytes	= sizeof(Vertex);
		CommandList->IASetVertexBuffers(0, &VBO, 1);

		D3D12_INDEX_BUFFER_VIEW IBV = { };
		IBV.BufferLocation	= CurrentMesh->IndexBuffer->GetGPUVirtualAddress();
		IBV.SizeInBytes		= CurrentMesh->IndexBuffer->GetSizeInBytes();
		IBV.Format			= DXGI_FORMAT_R32_UINT;
		CommandList->IASetIndexBuffer(&IBV);

		PerObjectBuffer.Matrix		= CurrentActor->GetTransform().GetMatrix();
		PerObjectBuffer.Roughness	= CurrentMaterial->GetMaterialProperties().Roughness;
		PerObjectBuffer.Metallic	= CurrentMaterial->GetMaterialProperties().Metallic;
		PerObjectBuffer.AO			= CurrentMaterial->GetMaterialProperties().AO;
		CommandList->SetGraphicsRoot32BitConstants(&PerObjectBuffer, 19, 0, 0);
		CommandList->SetGraphicsRootDescriptorTable(CurrentMaterial->GetDescriptorTable()->GetGPUTableStartHandle(), 2);

		CommandList->DrawIndexedInstanced(CurrentMesh->IndexCount, 1, 0, 0, 0);
	}

	CommandList->TransitionBarrier(GBuffer[0].get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	CommandList->TransitionBarrier(GBuffer[1].get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	CommandList->TransitionBarrier(GBuffer[2].get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	CommandList->TransitionBarrier(GBuffer[3].get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// RayTracing
	if (Device->IsRayTracingSupported())
	{
		TraceRays(BackBuffer, CommandList.get());
	}

	CommandList->TransitionBarrier(BackBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

	D3D12RenderTargetView* RenderTarget[] = { BackBuffer->GetRenderTargetView().get() };
	CommandList->OMSetRenderTargets(RenderTarget, 1, nullptr);

	CommandList->RSSetViewports(&ViewPort, 1);
	CommandList->RSSetScissorRects(&ScissorRect, 1);

	CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	CommandList->SetPipelineState(LightPassPSO->GetPipelineState());
	CommandList->SetGraphicsRootSignature(LightRootSignature->GetRootSignature());
	CommandList->SetGraphicsRootDescriptorTable(LightDescriptorTable->GetGPUTableStartHandle(), 0);

	CommandList->DrawInstanced(3, 1, 0, 0);

	D3D12_VERTEX_BUFFER_VIEW SkyboxVBO = { };
	SkyboxVBO.BufferLocation	= SkyboxVertexBuffer->GetGPUVirtualAddress();
	SkyboxVBO.SizeInBytes		= SkyboxVertexBuffer->GetSizeInBytes();
	SkyboxVBO.StrideInBytes		= sizeof(Vertex);
	CommandList->IASetVertexBuffers(0, &SkyboxVBO, 1);

	D3D12_INDEX_BUFFER_VIEW SkyboxIBV = { };
	SkyboxIBV.BufferLocation	= SkyboxIndexBuffer->GetGPUVirtualAddress();
	SkyboxIBV.Format			= DXGI_FORMAT_R32_UINT;
	SkyboxIBV.SizeInBytes		= SkyboxIndexBuffer->GetSizeInBytes();
	CommandList->IASetIndexBuffer(&SkyboxIBV);

	CommandList->SetPipelineState(SkyboxPSO->GetPipelineState());
	CommandList->SetGraphicsRootSignature(SkyboxRootSignature->GetRootSignature());

	struct SkyboxCameraBuffer
	{
		XMFLOAT4X4 Matrix;
	} PerSkybox;
	PerSkybox.Matrix = CurrentScene.GetCamera()->GetViewProjectionWitoutTranslate();

	CommandList->SetGraphicsRoot32BitConstants(&PerSkybox, 16, 0, 0);
	CommandList->SetGraphicsRootDescriptorTable(SkyboxDescriptorTable->GetGPUTableStartHandle(), 1);

	CommandList->TransitionBarrier(GBuffer[3].get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_DEPTH_READ);

	CommandList->OMSetRenderTargets(RenderTarget, 1, GBuffer[3]->GetDepthStencilView().get());

	CommandList->DrawIndexedInstanced(static_cast<Uint32>(SkyboxMesh.Indices.size()), 1, 0, 0, 0);

	CommandList->TransitionBarrier(GBuffer[3].get(), D3D12_RESOURCE_STATE_DEPTH_READ, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	// Render UI
	GuiContext::Get()->Render(CommandList.get());

	CommandList->TransitionBarrier(BackBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	CommandList->Close();

	// Execute
	Queue->ExecuteCommandList(CommandList.get());

	// Present
	SwapChain->Present(PresentInterval);

	// Wait for next frame
	const Uint64 CurrentFenceValue = FenceValues[CurrentBackBufferIndex];
	Queue->SignalFence(Fence.get(), CurrentFenceValue);

	CurrentBackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
	if (Fence->WaitForValue(CurrentFenceValue))
	{
		FenceValues[CurrentBackBufferIndex] = CurrentFenceValue + 1;
	}
}

void Renderer::TraceRays(D3D12Texture* BackBuffer, D3D12CommandList* InCommandList)
{
	InCommandList->TransitionBarrier(ReflectionTexture.get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	D3D12_DISPATCH_RAYS_DESC raytraceDesc = {};
	raytraceDesc.Width	= static_cast<Uint32>(ReflectionTexture->GetDesc().Width); //SwapChain->GetWidth();
	raytraceDesc.Height = static_cast<Uint32>(ReflectionTexture->GetDesc().Height); //SwapChain->GetHeight();
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
	InCommandList->TransitionBarrier(ReflectionTexture.get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
}

void Renderer::OnResize(Int32 Width, Int32 Height)
{
	WaitForPendingFrames();

	SwapChain->Resize(Width, Height);

	// Update deferred
	InitGBuffer();

	if (Device->IsRayTracingSupported())
	{
		RayGenDescriptorTable->SetUnorderedAccessView(ReflectionTexture->GetUnorderedAccessView(0).get(), 0);
		RayGenDescriptorTable->CopyDescriptors();

		GlobalDescriptorTable->SetShaderResourceView(GBuffer[1]->GetShaderResourceView(0).get(), 5);
		GlobalDescriptorTable->SetShaderResourceView(GBuffer[3]->GetShaderResourceView(0).get(), 6);
		GlobalDescriptorTable->CopyDescriptors();
	}

	LightDescriptorTable->SetShaderResourceView(GBuffer[0]->GetShaderResourceView(0).get(), 0);
	LightDescriptorTable->SetShaderResourceView(GBuffer[1]->GetShaderResourceView(0).get(), 1);
	LightDescriptorTable->SetShaderResourceView(GBuffer[2]->GetShaderResourceView(0).get(), 2);
	LightDescriptorTable->SetShaderResourceView(GBuffer[3]->GetShaderResourceView(0).get(), 3);
	LightDescriptorTable->CopyDescriptors();

	CurrentBackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
}

Renderer* Renderer::Make(std::shared_ptr<WindowsWindow> RendererWindow)
{
	RendererInstance = std::make_unique<Renderer>();
	if (RendererInstance->Initialize(RendererWindow))
	{
		return RendererInstance.get();
	}
	else
	{
		return nullptr;
	}
}

Renderer* Renderer::Get()
{
	return RendererInstance.get();
}

bool Renderer::Initialize(std::shared_ptr<WindowsWindow> RendererWindow)
{
	Device = std::shared_ptr<D3D12Device>(D3D12Device::Make(false));
	if (!Device)
	{
		return false;
	}

	ImmediateCommandList = std::make_shared<D3D12ImmediateCommandList>(Device.get());
	if (!ImmediateCommandList->Initialize(D3D12_COMMAND_LIST_TYPE_DIRECT))
	{
		return false;
	}

	Queue = std::make_shared<D3D12CommandQueue>(Device.get());
	if (!Queue->Initialize(D3D12_COMMAND_LIST_TYPE_DIRECT))
	{
		return false;
	}

	SwapChain = std::make_shared<D3D12SwapChain>(Device.get());
	if (!SwapChain->Initialize(RendererWindow.get(), Queue.get()))
	{
		return false;
	}
	else
	{
		CurrentBackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
	}

	const Uint32 BackBufferCount = SwapChain->GetSurfaceCount();
	CommandAllocators.resize(BackBufferCount);
	for (Uint32 i = 0; i < BackBufferCount; i++)
	{
		CommandAllocators[i] = std::make_shared<D3D12CommandAllocator>(Device.get());
		if (!CommandAllocators[i]->Initialize(D3D12_COMMAND_LIST_TYPE_DIRECT))
		{
			return false;
		}
	}

	CommandList = std::make_shared<D3D12CommandList>(Device.get());
	if (!CommandList->Initialize(D3D12_COMMAND_LIST_TYPE_DIRECT, CommandAllocators[0].get(), nullptr))
	{
		return false;
	}

	Fence = std::make_shared<D3D12Fence>(Device.get());
	if (!Fence->Initialize(0))
	{
		return false;
	}

	FenceValues.resize(SwapChain->GetSurfaceCount());

	// Create mesh
	Sphere		= MeshFactory::CreateSphere(3);
	SkyboxMesh	= MeshFactory::CreateSphere(1);
	Cube		= MeshFactory::CreateCube();

	// Create CameraBuffer
	BufferProperties BufferProps = { };
	BufferProps.SizeInBytes		= 256; // Must be multiple of 256
	BufferProps.Flags			= D3D12_RESOURCE_FLAG_NONE;
	BufferProps.InitalState		= D3D12_RESOURCE_STATE_COMMON;
	BufferProps.MemoryType		= EMemoryType::MEMORY_TYPE_DEFAULT;

	CameraBuffer = std::make_shared<D3D12Buffer>(Device.get());
	if (CameraBuffer->Initialize(BufferProps))
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC CameraViewDesc = { };
		CameraViewDesc.BufferLocation	= CameraBuffer->GetGPUVirtualAddress();
		CameraViewDesc.SizeInBytes		= static_cast<Uint32>(BufferProps.SizeInBytes);

		CameraBuffer->SetConstantBufferView(std::make_shared<D3D12ConstantBufferView>(Device.get(), CameraBuffer->GetResource(), &CameraViewDesc));
	}
	else
	{
		return false;
	}

	// Create vertexbuffer
	BufferProps.InitalState	= D3D12_RESOURCE_STATE_GENERIC_READ;
	BufferProps.SizeInBytes	= sizeof(Vertex) * static_cast<Uint64>(Sphere.Vertices.size());
	BufferProps.MemoryType	= EMemoryType::MEMORY_TYPE_UPLOAD;

	MeshVertexBuffer = std::make_shared<D3D12Buffer>(Device.get());
	if (MeshVertexBuffer->Initialize(BufferProps))
	{
		void* BufferMemory = MeshVertexBuffer->Map();
		memcpy(BufferMemory, Sphere.Vertices.data(), BufferProps.SizeInBytes);
		MeshVertexBuffer->Unmap();
	}
	else
	{
		return false;
	}

	BufferProps.SizeInBytes = sizeof(Vertex) * static_cast<Uint64>(Cube.Vertices.size());
	CubeVertexBuffer = std::make_shared<D3D12Buffer>(Device.get());
	if (CubeVertexBuffer->Initialize(BufferProps))
	{
		void* BufferMemory = CubeVertexBuffer->Map();
		memcpy(BufferMemory, Cube.Vertices.data(), BufferProps.SizeInBytes);
		CubeVertexBuffer->Unmap();
	}
	else
	{
		return false;
	}

	BufferProps.SizeInBytes = sizeof(Vertex) * static_cast<Uint64>(SkyboxMesh.Vertices.size());
	SkyboxVertexBuffer = std::make_shared<D3D12Buffer>(Device.get());
	if (SkyboxVertexBuffer->Initialize(BufferProps))
	{
		void* BufferMemory = SkyboxVertexBuffer->Map();
		memcpy(BufferMemory, SkyboxMesh.Vertices.data(), BufferProps.SizeInBytes);
		SkyboxVertexBuffer->Unmap();
	}
	else
	{
		return false;
	}

	// Create indexbuffer
	BufferProps.SizeInBytes = sizeof(Uint32) * static_cast<Uint64>(Sphere.Indices.size());
	MeshIndexBuffer = std::make_shared<D3D12Buffer>(Device.get());
	if (MeshIndexBuffer->Initialize(BufferProps))
	{
		void* BufferMemory = MeshIndexBuffer->Map();
		memcpy(BufferMemory, Sphere.Indices.data(), BufferProps.SizeInBytes);
		MeshIndexBuffer->Unmap();
	}
	else
	{
		return false;
	}

	BufferProps.SizeInBytes = sizeof(Uint32) * static_cast<Uint64>(Cube.Indices.size());
	CubeIndexBuffer = std::make_shared<D3D12Buffer>(Device.get());
	if (CubeIndexBuffer->Initialize(BufferProps))
	{
		void* BufferMemory = CubeIndexBuffer->Map();
		memcpy(BufferMemory, Cube.Indices.data(), BufferProps.SizeInBytes);
		CubeIndexBuffer->Unmap();
	}
	else
	{
		return false;
	}

	BufferProps.SizeInBytes = sizeof(Uint32) * static_cast<Uint64>(SkyboxMesh.Indices.size());
	SkyboxIndexBuffer = std::make_shared<D3D12Buffer>(Device.get());
	if (SkyboxIndexBuffer->Initialize(BufferProps))
	{
		void* BufferMemory = SkyboxIndexBuffer->Map();
		memcpy(BufferMemory, SkyboxMesh.Indices.data(), BufferProps.SizeInBytes);
		SkyboxIndexBuffer->Unmap();
	}
	else
	{
		return false;
	}

	// Create Texture Cube
	std::unique_ptr<D3D12Texture> Panorama = std::unique_ptr<D3D12Texture>(TextureFactory::LoadFromFile(Device.get(), "../Assets/Textures/arches.hdr", 0, DXGI_FORMAT_R32G32B32A32_FLOAT));
	if (!Panorama)
	{
		return false;	
	}

	Skybox = std::shared_ptr<D3D12Texture>(TextureFactory::CreateTextureCubeFromPanorma(Device.get(), Panorama.get(), 1024, TEXTURE_FACTORY_FLAGS_GENERATE_MIPS, DXGI_FORMAT_R16G16B16A16_FLOAT));
	if (!Skybox)
	{
		return false;
	}
	else
	{
		Skybox->SetDebugName("Skybox");
	}

	// Generate global irradiance (From Skybox)
	const Uint16 IrradianceSize = 32;
	TextureProperties IrradianceMapProps = { };
	IrradianceMapProps.DebugName	= "Irradiance Map";
	IrradianceMapProps.Flags		= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	IrradianceMapProps.Width		= IrradianceSize;
	IrradianceMapProps.Height		= IrradianceSize;
	IrradianceMapProps.ArrayCount	= 6;
	IrradianceMapProps.MipLevels	= 1;
	IrradianceMapProps.Format		= DXGI_FORMAT_R16G16B16A16_FLOAT;
	IrradianceMapProps.MemoryType	= EMemoryType::MEMORY_TYPE_DEFAULT;
	IrradianceMapProps.InitalState	= D3D12_RESOURCE_STATE_COMMON;

	IrradianceMap = std::make_shared<D3D12Texture>(Device.get());
	if (!IrradianceMap->Initialize(IrradianceMapProps))
	{
		return false;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc = { };
	UavDesc.ViewDimension					= D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
	UavDesc.Texture2DArray.ArraySize		= 6;
	UavDesc.Texture2DArray.FirstArraySlice	= 0;
	UavDesc.Texture2DArray.MipSlice			= 0;
	UavDesc.Texture2DArray.PlaneSlice		= 0;
	IrradianceMap->SetUnorderedAccessView(std::make_shared<D3D12UnorderedAccessView>(Device.get(), nullptr, IrradianceMap->GetResource(), &UavDesc), 0);

	D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = { };
	SrvDesc.Format							= IrradianceMapProps.Format;
	SrvDesc.ViewDimension					= D3D12_SRV_DIMENSION_TEXTURECUBE;
	SrvDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SrvDesc.TextureCube.MipLevels			= 1;
	SrvDesc.TextureCube.MostDetailedMip		= 0;
	SrvDesc.TextureCube.ResourceMinLODClamp	= 0.0f;
	IrradianceMap->SetShaderResourceView(std::make_shared<D3D12ShaderResourceView>(Device.get(), IrradianceMap->GetResource(), &SrvDesc), 0);

	// Generate global specular irradiance (From Skybox)
	const Uint16 SpecularIrradianceSize = 128;
	const Uint32 Miplevels				= std::max<Uint32>(std::log2<Uint32>(SpecularIrradianceSize), 1U);
	TextureProperties SpecualarIrradianceMapProps = { };
	SpecualarIrradianceMapProps.DebugName	= "Specular Irradiance Map";
	SpecualarIrradianceMapProps.Flags		= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	SpecualarIrradianceMapProps.Width		= SpecularIrradianceSize;
	SpecualarIrradianceMapProps.Height		= SpecularIrradianceSize;
	SpecualarIrradianceMapProps.ArrayCount	= 6;
	SpecualarIrradianceMapProps.MipLevels	= static_cast<Uint16>(Miplevels);
	SpecualarIrradianceMapProps.Format		= DXGI_FORMAT_R16G16B16A16_FLOAT;
	SpecualarIrradianceMapProps.MemoryType	= EMemoryType::MEMORY_TYPE_DEFAULT;
	SpecualarIrradianceMapProps.InitalState	= D3D12_RESOURCE_STATE_COMMON;

	SpecularIrradianceMap = std::make_shared<D3D12Texture>(Device.get());
	if (!SpecularIrradianceMap->Initialize(SpecualarIrradianceMapProps))
	{
		return false;
	}

	UavDesc.ViewDimension					= D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
	UavDesc.Texture2DArray.ArraySize		= 6;
	UavDesc.Texture2DArray.FirstArraySlice	= 0;
	UavDesc.Texture2DArray.PlaneSlice		= 0;
	for (Uint32 Miplevel = 0; Miplevel < Miplevels; Miplevel++)
	{
		UavDesc.Texture2DArray.MipSlice = Miplevel;
		SpecularIrradianceMap->SetUnorderedAccessView(std::make_shared<D3D12UnorderedAccessView>(Device.get(), nullptr, SpecularIrradianceMap->GetResource(), &UavDesc), Miplevel);
	}

	SrvDesc.Format							= SpecualarIrradianceMapProps.Format;
	SrvDesc.ViewDimension					= D3D12_SRV_DIMENSION_TEXTURECUBE;
	SrvDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SrvDesc.TextureCube.MipLevels			= SpecualarIrradianceMapProps.MipLevels;
	SrvDesc.TextureCube.MostDetailedMip		= 0;
	SrvDesc.TextureCube.ResourceMinLODClamp	= 0.0f;
	SpecularIrradianceMap->SetShaderResourceView(std::make_shared<D3D12ShaderResourceView>(Device.get(), SpecularIrradianceMap->GetResource(), &SrvDesc), 0);

	GenerateIrradianceMap(Skybox.get(), IrradianceMap.get(), ImmediateCommandList.get());
	ImmediateCommandList->Flush();

	GenerateSpecularIrradianceMap(Skybox.get(), SpecularIrradianceMap.get(), ImmediateCommandList.get());
	ImmediateCommandList->Flush();

	// Create albedo for raytracing
	Albedo = std::shared_ptr<D3D12Texture>(TextureFactory::LoadFromFile(Device.get(), "../Assets/Textures/RockySoil_Albedo.png", TEXTURE_FACTORY_FLAGS_GENERATE_MIPS, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!Albedo)
	{
		return false;
	}
	else
	{
		Albedo->SetDebugName("AlbedoMap");
	}
	
	Normal = std::shared_ptr<D3D12Texture>(TextureFactory::LoadFromFile(Device.get(), "../Assets/Textures/RockySoil_Normal.png", TEXTURE_FACTORY_FLAGS_GENERATE_MIPS, DXGI_FORMAT_R8G8B8A8_UNORM));
	if (!Normal)
	{
		return false;
	}
	else
	{
		Normal->SetDebugName("NormalMap");
	}

	// Init Deferred Rendering
	if (!InitDeferred())
	{
		return false;
	}

	if (!InitIntegrationLUT())
	{
		return false;
	}

	// Init RayTracing if supported
	if (Device->IsRayTracingSupported())
	{
		if (!InitRayTracing())
		{
			return false;
		}
	}

	LightDescriptorTable->SetShaderResourceView(ReflectionTexture->GetShaderResourceView(0).get(), 4);
	LightDescriptorTable->SetShaderResourceView(IrradianceMap->GetShaderResourceView(0).get(), 5);
	LightDescriptorTable->SetShaderResourceView(SpecularIrradianceMap->GetShaderResourceView(0).get(), 6);
	LightDescriptorTable->SetShaderResourceView(IntegrationLUT->GetShaderResourceView(0).get(), 7);
	LightDescriptorTable->CopyDescriptors();

	return true;
}

bool Renderer::InitRayTracing()
{
	// Create RootSignatures
	std::unique_ptr<D3D12RootSignature> RayGenLocalRoot;
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

		RayGenLocalRoot = std::make_unique<D3D12RootSignature>(Device.get());
		if (RayGenLocalRoot->Initialize(RayGenLocalRootDesc))
		{
			RayGenLocalRoot->SetDebugName("RayGen Local RootSignature");
		}
		else
		{
			return false;
		}
	}

	std::unique_ptr<D3D12RootSignature> HitLocalRoot;
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

		HitLocalRoot = std::make_unique<D3D12RootSignature>(Device.get());
		if (HitLocalRoot->Initialize(HitLocalRootDesc))
		{
			HitLocalRoot->SetDebugName("Closest Hit Local RootSignature");
		}
		else
		{
			return false;
		}
	}

	std::unique_ptr<D3D12RootSignature> MissLocalRoot;
	{
		D3D12_ROOT_SIGNATURE_DESC MissLocalRootDesc = {};
		MissLocalRootDesc.NumParameters	= 0;
		MissLocalRootDesc.pParameters	= nullptr;
		MissLocalRootDesc.Flags			= D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

		MissLocalRoot = std::make_unique<D3D12RootSignature>(Device.get());
		if (MissLocalRoot->Initialize(MissLocalRootDesc))
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

		GlobalRootSignature = std::make_shared<D3D12RootSignature>(Device.get());
		if (!GlobalRootSignature->Initialize(GlobalRootDesc))
		{
			return false;
		}
	}

	// Create Pipeline
	RayTracingPipelineStateProperties PipelineProperties;
	PipelineProperties.DebugName				= "RayTracing PipelineState";
	PipelineProperties.RayGenRootSignature		= RayGenLocalRoot.get();
	PipelineProperties.HitGroupRootSignature	= HitLocalRoot.get();
	PipelineProperties.MissRootSignature		= MissLocalRoot.get();
	PipelineProperties.GlobalRootSignature		= GlobalRootSignature.get();
	PipelineProperties.MaxRecursions			= 4;

	RaytracingPSO = std::make_shared<D3D12RayTracingPipelineState>(Device.get());
	if (!RaytracingPSO->Initialize(PipelineProperties))
	{
		return false;
	}

	// Build Acceleration Structures
	ImmediateCommandList->TransitionBarrier(CameraBuffer.get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	// Create BLAS
	std::shared_ptr<D3D12RayTracingGeometry> MeshGeometry = std::make_shared<D3D12RayTracingGeometry>(Device.get());
	MeshGeometry->BuildAccelerationStructure(ImmediateCommandList.get(), MeshVertexBuffer, static_cast<Uint32>(Sphere.Vertices.size()), MeshIndexBuffer, static_cast<Uint32>(Sphere.Indices.size()));

	std::shared_ptr<D3D12RayTracingGeometry> CubeGeometry = std::make_shared<D3D12RayTracingGeometry>(Device.get());
	CubeGeometry->BuildAccelerationStructure(ImmediateCommandList.get(), CubeVertexBuffer, static_cast<Uint32>(Cube.Vertices.size()), CubeIndexBuffer, static_cast<Uint32>(Cube.Indices.size()));

	XMFLOAT3X4 Matrix;
	std::vector<D3D12RayTracingGeometryInstance> Instances;

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
			Instances.emplace_back(MeshGeometry, Matrix, 0, 0);
		}
	}

	XMStoreFloat3x4(&Matrix, XMMatrixTranslation(0.0f, 0.0f, -3.0f));
	Instances.emplace_back(CubeGeometry, Matrix, 1, 1);

	// Create DescriptorTables
	std::shared_ptr<D3D12DescriptorTable> SphereDescriptorTable = std::make_shared<D3D12DescriptorTable>(Device.get(), 2);
	std::shared_ptr<D3D12DescriptorTable> CubeDescriptorTable	= std::make_shared<D3D12DescriptorTable>(Device.get(), 2);
	RayGenDescriptorTable = std::make_shared<D3D12DescriptorTable>(Device.get(), 1);
	GlobalDescriptorTable = std::make_shared<D3D12DescriptorTable>(Device.get(), 7);

	// Create TLAS
	std::vector<BindingTableEntry> BindingTableEntries;
	BindingTableEntries.emplace_back("RayGen", RayGenDescriptorTable);
	BindingTableEntries.emplace_back("HitGroup", SphereDescriptorTable);
	BindingTableEntries.emplace_back("HitGroup", CubeDescriptorTable);
	BindingTableEntries.emplace_back("Miss", nullptr);

	RayTracingScene = std::make_shared<D3D12RayTracingScene>(Device.get());
	if (!RayTracingScene->Initialize(RaytracingPSO.get(), BindingTableEntries, 2))
	{
		return false;
	}

	RayTracingScene->BuildAccelerationStructure(ImmediateCommandList.get(), Instances);

	ImmediateCommandList->Flush();
	ImmediateCommandList->WaitForCompletion();

	// VertexBuffer
	D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = { };
	SrvDesc.Format						= DXGI_FORMAT_UNKNOWN;
	SrvDesc.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER;
	SrvDesc.Shader4ComponentMapping		= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SrvDesc.Buffer.FirstElement			= 0;
	SrvDesc.Buffer.Flags				= D3D12_BUFFER_SRV_FLAG_NONE;
	SrvDesc.Buffer.NumElements			= static_cast<Uint32>(Sphere.Vertices.size());
	SrvDesc.Buffer.StructureByteStride	= sizeof(Vertex);

	MeshVertexBuffer->SetShaderResourceView(std::make_shared<D3D12ShaderResourceView>(Device.get(), MeshVertexBuffer->GetResource(), &SrvDesc), 0);

	SrvDesc.Buffer.NumElements = static_cast<Uint32>(Cube.Vertices.size());
	CubeVertexBuffer->SetShaderResourceView(std::make_shared<D3D12ShaderResourceView>(Device.get(), CubeVertexBuffer->GetResource(), &SrvDesc), 0);

	// IndexBuffer
	SrvDesc.Format						= DXGI_FORMAT_R32_TYPELESS;
	SrvDesc.Buffer.Flags				= D3D12_BUFFER_SRV_FLAG_RAW;
	SrvDesc.Buffer.NumElements			= static_cast<Uint32>(Sphere.Indices.size());
	SrvDesc.Buffer.StructureByteStride	= 0;

	MeshIndexBuffer->SetShaderResourceView(std::make_shared<D3D12ShaderResourceView>(Device.get(), MeshIndexBuffer->GetResource(), &SrvDesc), 0);

	SrvDesc.Buffer.NumElements = static_cast<Uint32>(Cube.Indices.size());
	CubeIndexBuffer->SetShaderResourceView(std::make_shared<D3D12ShaderResourceView>(Device.get(), CubeIndexBuffer->GetResource(), &SrvDesc), 0);

	// Populate descriptors
	RayGenDescriptorTable->SetUnorderedAccessView(ReflectionTexture->GetUnorderedAccessView(0).get(), 0);
	RayGenDescriptorTable->CopyDescriptors();

	SphereDescriptorTable->SetShaderResourceView(MeshVertexBuffer->GetShaderResourceView(0).get(), 0);
	SphereDescriptorTable->SetShaderResourceView(MeshIndexBuffer->GetShaderResourceView(0).get(), 1);
	SphereDescriptorTable->CopyDescriptors();

	CubeDescriptorTable->SetShaderResourceView(CubeVertexBuffer->GetShaderResourceView(0).get(), 0);
	CubeDescriptorTable->SetShaderResourceView(CubeIndexBuffer->GetShaderResourceView(0).get(), 1);
	CubeDescriptorTable->CopyDescriptors();

	GlobalDescriptorTable->SetShaderResourceView(RayTracingScene->GetShaderResourceView(), 0);
	GlobalDescriptorTable->SetConstantBufferView(CameraBuffer->GetConstantBufferView().get(), 1);
	GlobalDescriptorTable->SetShaderResourceView(Skybox->GetShaderResourceView(0).get(), 2);
	GlobalDescriptorTable->SetShaderResourceView(Albedo->GetShaderResourceView(0).get(), 3);
	GlobalDescriptorTable->SetShaderResourceView(Normal->GetShaderResourceView(0).get(), 4);
	GlobalDescriptorTable->SetShaderResourceView(GBuffer[1]->GetShaderResourceView(0).get(), 5);
	GlobalDescriptorTable->SetShaderResourceView(GBuffer[3]->GetShaderResourceView(0).get(), 6);
	GlobalDescriptorTable->CopyDescriptors();

	return true;
}

bool Renderer::InitDeferred()
{
	using namespace Microsoft::WRL;

	// Start with the GBuffer
	if (!InitGBuffer())
	{
		return false;
	}

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

		D3D12_DESCRIPTOR_RANGE PerObjectRanges[6] = {};
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

		D3D12_ROOT_PARAMETER Parameters[3];
		// Transform Constants
		Parameters[0].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		Parameters[0].Constants.ShaderRegister	= 0;
		Parameters[0].Constants.RegisterSpace	= 0;
		Parameters[0].Constants.Num32BitValues	= 19;
		Parameters[0].ShaderVisibility			= D3D12_SHADER_VISIBILITY_ALL;

		// PerFrame DescriptorTable
		Parameters[1].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		Parameters[1].DescriptorTable.NumDescriptorRanges	= 1;
		Parameters[1].DescriptorTable.pDescriptorRanges		= PerFrameRanges;
		Parameters[1].ShaderVisibility						= D3D12_SHADER_VISIBILITY_ALL;

		// PerObject DescriptorTable
		Parameters[2].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		Parameters[2].DescriptorTable.NumDescriptorRanges	= 6;
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

		GeometryRootSignature = std::make_shared<D3D12RootSignature>(Device.get());
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

		SkyboxRootSignature = std::make_shared<D3D12RootSignature>(Device.get());
		if (!SkyboxRootSignature->Initialize(RootSignatureDesc))
		{
			return false;
		}
	}

	{
		D3D12_DESCRIPTOR_RANGE Ranges[9] = {};
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

		// Camera
		Ranges[8].BaseShaderRegister				= 0;
		Ranges[8].NumDescriptors					= 1;
		Ranges[8].RegisterSpace						= 0;
		Ranges[8].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		Ranges[8].OffsetInDescriptorsFromTableStart	= 8;

		D3D12_ROOT_PARAMETER Parameters[1];
		Parameters[0].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		Parameters[0].DescriptorTable.NumDescriptorRanges	= 9;
		Parameters[0].DescriptorTable.pDescriptorRanges		= Ranges;
		Parameters[0].ShaderVisibility						= D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_STATIC_SAMPLER_DESC Samplers[3] = { };
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

		D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = { };
		RootSignatureDesc.NumParameters			= 1;
		RootSignatureDesc.pParameters			= Parameters;
		RootSignatureDesc.NumStaticSamplers		= 3;
		RootSignatureDesc.pStaticSamplers		= Samplers;
		RootSignatureDesc.Flags					= 
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

		LightRootSignature = std::make_shared<D3D12RootSignature>(Device.get());
		if (!LightRootSignature->Initialize(RootSignatureDesc))
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
	PSOProperties.RootSignature		= GeometryRootSignature.get();
	PSOProperties.InputElements		= InputElementDesc;
	PSOProperties.NumInputElements	= 4;
	PSOProperties.EnableDepth		= true;
	PSOProperties.EnableBlending	= false;
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

	GeometryPSO = std::make_shared<D3D12GraphicsPipelineState>(Device.get());
	if (!GeometryPSO->Initialize(PSOProperties))
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
	PSOProperties.RootSignature		= LightRootSignature.get();
	PSOProperties.InputElements		= nullptr;
	PSOProperties.NumInputElements	= 0;
	PSOProperties.EnableDepth		= false;
	PSOProperties.DepthBufferFormat = DXGI_FORMAT_UNKNOWN;
	PSOProperties.RTFormats			= Formats;
	PSOProperties.NumRenderTargets	= 1;
	PSOProperties.CullMode			= D3D12_CULL_MODE_NONE;

	LightPassPSO = std::make_shared<D3D12GraphicsPipelineState>(Device.get());
	if (!LightPassPSO->Initialize(PSOProperties))
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
	PSOProperties.RootSignature		= SkyboxRootSignature.get();
	PSOProperties.InputElements		= InputElementDesc;
	PSOProperties.NumInputElements	= 4;
	PSOProperties.EnableDepth		= true;
	PSOProperties.DepthFunc			= D3D12_COMPARISON_FUNC_LESS_EQUAL;
	PSOProperties.DepthWriteMask	= D3D12_DEPTH_WRITE_MASK_ZERO;
	PSOProperties.DepthBufferFormat = DepthBufferFormat;
	PSOProperties.RTFormats			= Formats;
	PSOProperties.NumRenderTargets	= 1;

	SkyboxPSO = std::make_shared<D3D12GraphicsPipelineState>(Device.get());
	if (!SkyboxPSO->Initialize(PSOProperties))
	{
		return false;
	}

	// Init descriptortable
	GeometryDescriptorTable = std::make_shared<D3D12DescriptorTable>(Device.get(), 1);
	GeometryDescriptorTable->SetConstantBufferView(CameraBuffer->GetConstantBufferView().get(), 0);
	GeometryDescriptorTable->CopyDescriptors();
	
	LightDescriptorTable = std::make_shared<D3D12DescriptorTable>(Device.get(), 9);
	LightDescriptorTable->SetShaderResourceView(GBuffer[0]->GetShaderResourceView(0).get(), 0);
	LightDescriptorTable->SetShaderResourceView(GBuffer[1]->GetShaderResourceView(0).get(), 1);
	LightDescriptorTable->SetShaderResourceView(GBuffer[2]->GetShaderResourceView(0).get(), 2);
	LightDescriptorTable->SetShaderResourceView(GBuffer[3]->GetShaderResourceView(0).get(), 3);
	// #4 is set after deferred and raytracing
	// #5 is set after deferred and raytracing
	// #6 is set after deferred and raytracing
	// #7 is set after deferred and raytracing
	LightDescriptorTable->SetConstantBufferView(CameraBuffer->GetConstantBufferView().get(), 8);

	SkyboxDescriptorTable = std::make_shared<D3D12DescriptorTable>(Device.get(), 2);
	SkyboxDescriptorTable->SetShaderResourceView(Skybox->GetShaderResourceView(0).get(), 0);
	SkyboxDescriptorTable->SetConstantBufferView(CameraBuffer->GetConstantBufferView().get(), 1);
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

	// Albedo
	TextureProperties GBufferProperties = { };
	GBufferProperties.DebugName = "GBuffer Albedo";
	GBufferProperties.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	GBufferProperties.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	GBufferProperties.ArrayCount = 1;
	GBufferProperties.Width = static_cast<Uint16>(SwapChain->GetWidth());
	GBufferProperties.Height = static_cast<Uint16>(SwapChain->GetHeight());
	GBufferProperties.InitalState = D3D12_RESOURCE_STATE_COMMON;
	GBufferProperties.MemoryType = EMemoryType::MEMORY_TYPE_DEFAULT;
	GBufferProperties.MipLevels = 1;

	D3D12_CLEAR_VALUE Value;
	Value.Format	= GBufferProperties.Format;
	Value.Color[0]	= 0.0f;
	Value.Color[1]	= 0.0f;
	Value.Color[2]	= 0.0f;
	Value.Color[3]	= 1.0f;
	GBufferProperties.OptimizedClearValue = &Value;
	
	// ShaderResourceView
	D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = { };
	SrvDesc.Format							= GBufferProperties.Format;
	SrvDesc.ViewDimension					= D3D12_SRV_DIMENSION_TEXTURE2D;
	SrvDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SrvDesc.Texture2D.MipLevels				= 1;
	SrvDesc.Texture2D.MostDetailedMip		= 0;
	SrvDesc.Texture2D.PlaneSlice			= 0;
	SrvDesc.Texture2D.ResourceMinLODClamp	= 0.0f;

	D3D12_RENDER_TARGET_VIEW_DESC RtvDesc = { };
	RtvDesc.Format					= GBufferProperties.Format;
	RtvDesc.ViewDimension			= D3D12_RTV_DIMENSION_TEXTURE2D;
	RtvDesc.Texture2D.MipSlice		= 0;
	RtvDesc.Texture2D.PlaneSlice	= 0;

	GBuffer[0] = std::make_shared<D3D12Texture>(Device.get());
	if (GBuffer[0]->Initialize(GBufferProperties))
	{
		GBuffer[0]->SetShaderResourceView(std::make_shared<D3D12ShaderResourceView>(Device.get(), GBuffer[0]->GetResource(), &SrvDesc), 0);
		GBuffer[0]->SetRenderTargetView(std::make_shared<D3D12RenderTargetView>(Device.get(), GBuffer[0]->GetResource(), &RtvDesc));
	}
	else
	{
		return false;
	}

	// Normal
	GBufferProperties.DebugName = "GBuffer Normal";
	GBufferProperties.Format	= NormalFormat;
	
	Value.Format = GBufferProperties.Format;

	SrvDesc.Format = GBufferProperties.Format;
	RtvDesc.Format = GBufferProperties.Format;

	GBuffer[1] = std::make_shared<D3D12Texture>(Device.get());
	if (GBuffer[1]->Initialize(GBufferProperties))
	{
		GBuffer[1]->SetShaderResourceView(std::make_shared<D3D12ShaderResourceView>(Device.get(), GBuffer[1]->GetResource(), &SrvDesc), 0);
		GBuffer[1]->SetRenderTargetView(std::make_shared<D3D12RenderTargetView>(Device.get(), GBuffer[1]->GetResource(), &RtvDesc));
	}
	else
	{
		return false;
	}

	// Material Properties
	GBufferProperties.DebugName = "GBuffer Material";
	GBufferProperties.Format	= DXGI_FORMAT_R8G8B8A8_UNORM;

	Value.Format = GBufferProperties.Format;

	SrvDesc.Format = GBufferProperties.Format;
	RtvDesc.Format = GBufferProperties.Format;

	GBuffer[2] = std::make_shared<D3D12Texture>(Device.get());
	if (GBuffer[2]->Initialize(GBufferProperties))
	{
		GBuffer[2]->SetShaderResourceView(std::make_shared<D3D12ShaderResourceView>(Device.get(), GBuffer[2]->GetResource(), &SrvDesc), 0);
		GBuffer[2]->SetRenderTargetView(std::make_shared<D3D12RenderTargetView>(Device.get(), GBuffer[2]->GetResource(), &RtvDesc));
	}
	else
	{
		return false;
	}

	// DepthStencil
	GBufferProperties.DebugName	= "GBuffer DepthStencil";
	GBufferProperties.Flags		= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	GBufferProperties.Format	= DXGI_FORMAT_R32_TYPELESS;
	
	D3D12_CLEAR_VALUE ClearValue = { };
	ClearValue.Format				= DepthBufferFormat;
	ClearValue.DepthStencil.Depth	= 1.0f;
	ClearValue.DepthStencil.Stencil	= 0;
	GBufferProperties.OptimizedClearValue = &ClearValue;

	SrvDesc.Format = DXGI_FORMAT_R32_FLOAT;

	D3D12_DEPTH_STENCIL_VIEW_DESC DsvDesc = { };
	DsvDesc.Format				= DepthBufferFormat;
	DsvDesc.ViewDimension		= D3D12_DSV_DIMENSION_TEXTURE2D;
	DsvDesc.Texture2D.MipSlice	= 0;

	GBuffer[3] = std::make_shared<D3D12Texture>(Device.get());
	if (GBuffer[3]->Initialize(GBufferProperties))
	{
		GBuffer[3]->SetShaderResourceView(std::make_shared<D3D12ShaderResourceView>(Device.get(), GBuffer[3]->GetResource(), &SrvDesc), 0);
		GBuffer[3]->SetDepthStencilView(std::make_shared<D3D12DepthStencilView>(Device.get(), GBuffer[3]->GetResource(), &DsvDesc));
	}
	else
	{
		return false;
	}

	return true;
}

bool Renderer::InitIntegrationLUT()
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS FeatureData;
	ZERO_MEMORY(&FeatureData, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));

	HRESULT Result = Device->GetDevice()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &FeatureData, sizeof(D3D12_FEATURE_DATA_D3D12_OPTIONS));
	if (SUCCEEDED(Result))
	{
		if (FeatureData.TypedUAVLoadAdditionalFormats)
		{
			D3D12_FEATURE_DATA_FORMAT_SUPPORT FormatSupport =
			{
				DXGI_FORMAT_R16G16_FLOAT,
				D3D12_FORMAT_SUPPORT1_NONE,
				D3D12_FORMAT_SUPPORT2_NONE
			};

			Result = Device->GetDevice()->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &FormatSupport, sizeof(FormatSupport));
			if (FAILED(Result) || (FormatSupport.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) == 0)
			{
				LOG_ERROR("[Renderer]: DXGI_FORMAT_R16G16_FLOAT is not supported for UAVs");
				return false;
			}
		}
	}

	TextureProperties LUTProperties = { };
	LUTProperties.DebugName		= "IntegrationLUT Staging Texture";
	LUTProperties.Flags			= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	LUTProperties.Width			= 512;
	LUTProperties.Height		= 512;
	LUTProperties.MipLevels		= 1;
	LUTProperties.ArrayCount	= 1;
	LUTProperties.Format		= DXGI_FORMAT_R16G16_FLOAT;
	LUTProperties.MemoryType	= EMemoryType::MEMORY_TYPE_DEFAULT;
	LUTProperties.InitalState	= D3D12_RESOURCE_STATE_COMMON;

	std::unique_ptr<D3D12Texture> StagingTexture = std::make_unique<D3D12Texture>(Device.get());
	if (!StagingTexture->Initialize(LUTProperties))
	{
		return false;
	}
	else
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC UavDesc = { };
		UavDesc.Format					= LUTProperties.Format;
		UavDesc.ViewDimension			= D3D12_UAV_DIMENSION_TEXTURE2D;
		UavDesc.Texture2D.MipSlice		= 0;
		UavDesc.Texture2D.PlaneSlice	= 0;

		StagingTexture->SetUnorderedAccessView(std::make_shared<D3D12UnorderedAccessView>(Device.get(), nullptr, StagingTexture->GetResource(), &UavDesc), 0);
	}

	LUTProperties.DebugName	= "IntegrationLUT";
	LUTProperties.Flags		= D3D12_RESOURCE_FLAG_NONE;

	IntegrationLUT = std::make_shared<D3D12Texture>(Device.get());
	if (!IntegrationLUT->Initialize(LUTProperties))
	{
		return false;
	}
	else
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = { };
		SrvDesc.Format							= LUTProperties.Format;
		SrvDesc.ViewDimension					= D3D12_SRV_DIMENSION_TEXTURE2D;
		SrvDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		SrvDesc.Texture2D.MipLevels				= 1;
		SrvDesc.Texture2D.MostDetailedMip		= 0;
		SrvDesc.Texture2D.PlaneSlice			= 0;
		SrvDesc.Texture2D.ResourceMinLODClamp	= 0;

		IntegrationLUT->SetShaderResourceView(std::make_shared<D3D12ShaderResourceView>(Device.get(), IntegrationLUT->GetResource(), &SrvDesc), 0);
	}

	Microsoft::WRL::ComPtr<IDxcBlob> Shader = D3D12ShaderCompiler::CompileFromFile("Shaders/BRDFIntegationGen.hlsl", "Main", "cs_6_0");
	if (!Shader)
	{
		return false;
	}

	std::unique_ptr<D3D12RootSignature> RootSignature = std::make_unique<D3D12RootSignature>(Device.get());
	if (!RootSignature->Initialize(Shader.Get()))
	{
		return false;
	}

	ComputePipelineStateProperties PSOProperties;
	PSOProperties.DebugName		= "IntegrationGen PSO";
	PSOProperties.CSBlob		= Shader.Get();
	PSOProperties.RootSignature	= RootSignature.get();

	std::unique_ptr<D3D12ComputePipelineState> PSO = std::make_unique<D3D12ComputePipelineState>(Device.get());
	if (!PSO->Initialize(PSOProperties))
	{
		return false;
	}

	std::unique_ptr<D3D12DescriptorTable> DescriptorTable = std::make_unique<D3D12DescriptorTable>(Device.get(), 1);
	DescriptorTable->SetUnorderedAccessView(StagingTexture->GetUnorderedAccessView(0).get(), 0);
	DescriptorTable->CopyDescriptors();

	ImmediateCommandList->SetComputeRootSignature(RootSignature->GetRootSignature());
	ImmediateCommandList->SetPipelineState(PSO->GetPipeline());
	
	ID3D12DescriptorHeap* DescriptorHeaps[] = { Device->GetGlobalOnlineResourceHeap()->GetHeap() };
	ImmediateCommandList->SetDescriptorHeaps(DescriptorHeaps, 1);
	ImmediateCommandList->SetComputeRootDescriptorTable(DescriptorTable->GetGPUTableStartHandle(), 0);

	ImmediateCommandList->Dispatch(LUTProperties.Width, LUTProperties.Height, 1);
	ImmediateCommandList->UnorderedAccessBarrier(StagingTexture.get());
	
	ImmediateCommandList->TransitionBarrier(IntegrationLUT.get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	ImmediateCommandList->TransitionBarrier(StagingTexture.get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);

	ImmediateCommandList->CopyResource(IntegrationLUT.get(), StagingTexture.get());
	
	ImmediateCommandList->TransitionBarrier(IntegrationLUT.get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	ImmediateCommandList->Flush();
	ImmediateCommandList->WaitForCompletion();

	return true;
}

bool Renderer::InitRayTracingTexture()
{
	TextureProperties OutputProperties = { };
	OutputProperties.DebugName	= "RayTracing Output";
	OutputProperties.Flags		= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	OutputProperties.Width		= static_cast<Uint16>(SwapChain->GetWidth());
	OutputProperties.Height		= static_cast<Uint16>(SwapChain->GetHeight());
	OutputProperties.MipLevels	= 1;
	OutputProperties.ArrayCount	= 1;
	OutputProperties.Format		= DXGI_FORMAT_R8G8B8A8_UNORM;
	OutputProperties.MemoryType	= EMemoryType::MEMORY_TYPE_DEFAULT;

	ReflectionTexture = std::shared_ptr<D3D12Texture>(new D3D12Texture(Device.get()));
	if (!ReflectionTexture->Initialize(OutputProperties))
	{
		return false;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVView = { };
	UAVView.Format					= OutputProperties.Format;
	UAVView.ViewDimension			= D3D12_UAV_DIMENSION_TEXTURE2D;
	UAVView.Texture2D.MipSlice		= 0;
	UAVView.Texture2D.PlaneSlice	= 0;

	ReflectionTexture->SetUnorderedAccessView(std::make_shared<D3D12UnorderedAccessView>(Device.get(), nullptr, ReflectionTexture->GetResource(), &UAVView), 0);
	
	D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = { };
	SrvDesc.Format							= OutputProperties.Format;
	SrvDesc.ViewDimension					= D3D12_SRV_DIMENSION_TEXTURE2D;
	SrvDesc.Shader4ComponentMapping			= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SrvDesc.Texture2D.MipLevels				= 1;
	SrvDesc.Texture2D.MostDetailedMip		= 0;
	SrvDesc.Texture2D.PlaneSlice			= 0;
	SrvDesc.Texture2D.ResourceMinLODClamp	= 0.0f;
	
	ReflectionTexture->SetShaderResourceView(std::make_shared<D3D12ShaderResourceView>(Device.get(), ReflectionTexture->GetResource(), &SrvDesc), 0);

	return true;
}

void Renderer::GenerateIrradianceMap(D3D12Texture* Source, D3D12Texture* Dest, D3D12CommandList* InCommandList)
{
	const Uint32 Size = static_cast<Uint32>(Dest->GetDesc().Width);

	static std::unique_ptr<D3D12DescriptorTable> SrvDescriptorTable;
	if (!SrvDescriptorTable)
	{
		SrvDescriptorTable = std::make_unique<D3D12DescriptorTable>(Device.get(), 1);
		SrvDescriptorTable->SetShaderResourceView(Source->GetShaderResourceView(0).get(), 0);
		SrvDescriptorTable->CopyDescriptors();
	}

	static std::unique_ptr<D3D12DescriptorTable> UavDescriptorTable;
	if (!UavDescriptorTable)
	{
		UavDescriptorTable = std::make_unique<D3D12DescriptorTable>(Device.get(), 1);
		UavDescriptorTable->SetUnorderedAccessView(Dest->GetUnorderedAccessView(0).get(), 0);
		UavDescriptorTable->CopyDescriptors();
	}

	static Microsoft::WRL::ComPtr<IDxcBlob> Shader;
	if (!Shader)
	{
		Shader = D3D12ShaderCompiler::CompileFromFile("Shaders/IrradianceGen.hlsl", "Main", "cs_6_0");
	}

	if (!IrradianceGenRootSignature)
	{
		IrradianceGenRootSignature = std::make_shared<D3D12RootSignature>(Device.get());
		IrradianceGenRootSignature->Initialize(Shader.Get());
		IrradianceGenRootSignature->SetDebugName("Irradiance Gen RootSignature");
	}

	if (!IrradicanceGenPSO)
	{
		ComputePipelineStateProperties Props = { };
		Props.DebugName		= "Irradiance Gen PSO";
		Props.CSBlob		= Shader.Get();
		Props.RootSignature	= IrradianceGenRootSignature.get();

		IrradicanceGenPSO = std::make_unique<D3D12ComputePipelineState>(Device.get());
		IrradicanceGenPSO->Initialize(Props);
	}

	InCommandList->TransitionBarrier(Source, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	InCommandList->TransitionBarrier(Dest, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	InCommandList->SetComputeRootSignature(IrradianceGenRootSignature->GetRootSignature());

	ID3D12DescriptorHeap* DescriptorHeaps[] = { Device->GetGlobalOnlineResourceHeap()->GetHeap() };
	InCommandList->SetDescriptorHeaps(DescriptorHeaps, 1);
	InCommandList->SetComputeRootDescriptorTable(SrvDescriptorTable->GetGPUTableStartHandle(), 0);
	InCommandList->SetComputeRootDescriptorTable(UavDescriptorTable->GetGPUTableStartHandle(), 1);

	InCommandList->SetPipelineState(IrradicanceGenPSO->GetPipeline());

	InCommandList->Dispatch(Size, Size, 6);

	InCommandList->UnorderedAccessBarrier(Dest);

	InCommandList->TransitionBarrier(Source, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	InCommandList->TransitionBarrier(Dest, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void Renderer::GenerateSpecularIrradianceMap(D3D12Texture* Source, D3D12Texture* Dest, D3D12CommandList* InCommandList)
{
	const Uint32 Miplevels = Dest->GetDesc().MipLevels;

	static std::unique_ptr<D3D12DescriptorTable> SrvDescriptorTable;
	if (!SrvDescriptorTable)
	{
		SrvDescriptorTable = std::make_unique<D3D12DescriptorTable>(Device.get(), 1);
		SrvDescriptorTable->SetShaderResourceView(Source->GetShaderResourceView(0).get(), 0);
		SrvDescriptorTable->CopyDescriptors();
	}

	static std::unique_ptr<D3D12DescriptorTable> UavDescriptorTable;
	if (!UavDescriptorTable)
	{
		UavDescriptorTable = std::make_unique<D3D12DescriptorTable>(Device.get(), Miplevels);
		for (Uint32 Mip = 0; Mip < Miplevels; Mip++)
		{
			UavDescriptorTable->SetUnorderedAccessView(Dest->GetUnorderedAccessView(Mip).get(), Mip);
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
		SpecIrradianceGenRootSignature = std::make_shared<D3D12RootSignature>(Device.get());
		SpecIrradianceGenRootSignature->Initialize(Shader.Get());
		SpecIrradianceGenRootSignature->SetDebugName("Specular Irradiance Gen RootSignature");
	}

	if (!SpecIrradicanceGenPSO)
	{
		ComputePipelineStateProperties Props = { };
		Props.DebugName		= "Specular Irradiance Gen PSO";
		Props.CSBlob		= Shader.Get();
		Props.RootSignature = SpecIrradianceGenRootSignature.get();

		SpecIrradicanceGenPSO = std::make_unique<D3D12ComputePipelineState>(Device.get());
		SpecIrradicanceGenPSO->Initialize(Props);
	}

	InCommandList->TransitionBarrier(Source, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
	InCommandList->TransitionBarrier(Dest, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);

	InCommandList->SetComputeRootSignature(SpecIrradianceGenRootSignature->GetRootSignature());

	ID3D12DescriptorHeap* DescriptorHeaps[] = { Device->GetGlobalOnlineResourceHeap()->GetHeap() };
	InCommandList->SetDescriptorHeaps(DescriptorHeaps, 1);
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

	//Queue->SignalFence(Fence.get(), CurrentFenceValue);
	//if (Fence->WaitForValue(CurrentFenceValue))
	//{
	//	FenceValues[CurrentBackBufferIndex]++;
	//}

	Queue->WaitForCompletion();
}