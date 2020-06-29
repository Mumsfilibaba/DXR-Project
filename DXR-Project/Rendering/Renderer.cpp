#include "Renderer.h"
#include "MeshFactory.h"
#include "TextureFactory.h"
#include "GuiContext.h"

#include "D3D12/D3D12Texture.h"
#include "D3D12/D3D12Views.h"

#include "Application/InputManager.h"

std::unique_ptr<Renderer> Renderer::RendererInstance = nullptr;

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
	SAFEDELETE(ResultTexture);
}

void Renderer::Tick()
{
	if (InputManager::Get().IsKeyDown(EKey::KEY_RIGHT))
	{
		SceneCamera.Rotate(0.0f, -0.01f, 0.0f);
	}
	else if (InputManager::Get().IsKeyDown(EKey::KEY_LEFT))
	{
		SceneCamera.Rotate(0.0f, 0.01f, 0.0f);
	}

	if (InputManager::Get().IsKeyDown(EKey::KEY_UP))
	{
		SceneCamera.Rotate(-0.01f, 0.0f, 0.0f);
	}
	else if (InputManager::Get().IsKeyDown(EKey::KEY_DOWN))
	{
		SceneCamera.Rotate(0.01f, 0.0f, 0.0f);
	}

	if (InputManager::Get().IsKeyDown(EKey::KEY_W))
	{
		SceneCamera.Move(0.0f, 0.0f, 0.01f);
	}
	else if (InputManager::Get().IsKeyDown(EKey::KEY_S))
	{
		SceneCamera.Move(0.0f, 0.0f, -0.01f);
	}
	
	if (InputManager::Get().IsKeyDown(EKey::KEY_A))
	{
		SceneCamera.Move(0.01f, 0.0f, 0.0f);
	}
	else if (InputManager::Get().IsKeyDown(EKey::KEY_D))
	{
		SceneCamera.Move(-0.01f, 0.0f, 0.0f);
	}
	
	if (InputManager::Get().IsKeyDown(EKey::KEY_Q))
	{
		SceneCamera.Move(0.0f, 0.01f, 0.0f);
	}
	else if (InputManager::Get().IsKeyDown(EKey::KEY_E))
	{
		SceneCamera.Move(0.0f, -0.01f, 0.0f);
	}

	SceneCamera.UpdateMatrices();

	D3D12Texture* BackBuffer = SwapChain->GetSurfaceResource(CurrentBackBufferIndex);

	CommandAllocators[CurrentBackBufferIndex]->Reset();
	CommandList->Reset(CommandAllocators[CurrentBackBufferIndex].get());
	UploadBuffers[CurrentBackBufferIndex]->Reset();

	//Set constant buffer descriptor heap
	//ID3D12DescriptorHeap* DescriptorHeaps[] = { Device->GetGlobalResourceDescriptorHeap()->GetHeap() };
	//CommandList->SetDescriptorHeaps(DescriptorHeaps, ARRAYSIZE(DescriptorHeaps));

	Uint64 Offset = UploadBuffers[CurrentBackBufferIndex]->GetOffset();
	void* CameraMemory = UploadBuffers[CurrentBackBufferIndex]->Allocate(sizeof(Camera));
	memcpy(CameraMemory, &SceneCamera, sizeof(Camera));
	
	CommandList->TransitionBarrier(CameraBuffer.get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
	CommandList->CopyBuffer(CameraBuffer.get(), 0, UploadBuffers[CurrentBackBufferIndex]->GetBuffer(), Offset, sizeof(Camera));
	CommandList->TransitionBarrier(CameraBuffer.get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	if (Device->IsRayTracingSupported())
	{
		TraceRays(BackBuffer, CommandList.get());
	}
	else
	{
		CommandList->TransitionBarrier(BackBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);

		const Float32 ClearColor[4] = { 0.3921f, 0.5843f, 0.9394f, 1.0f };
		CommandList->ClearRenderTargetView(BackBuffer->GetRenderTargetView().get(), ClearColor);
	}

	D3D12RenderTargetView* RenderTarget[] = { BackBuffer->GetRenderTargetView().get() };
	CommandList->OMSetRenderTargets(RenderTarget, 1, nullptr);

	GuiContext::Get()->Render(CommandList.get());

	CommandList->TransitionBarrier(BackBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

	UploadBuffers[CurrentBackBufferIndex]->Close();
	CommandList->Close();

	// Execute
	Queue->ExecuteCommandList(CommandList.get());

	// Present
	SwapChain->Present(0);

	// Wait for next frame
	const Uint64 CurrentFenceValue = FenceValues[CurrentBackBufferIndex];
	Queue->SignalFence(Fence.get(), CurrentFenceValue);

	CurrentBackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
	if (Fence->WaitForValue(CurrentFenceValue))
	{
		FenceValues[CurrentBackBufferIndex] = CurrentFenceValue + 1;
	}
}

void Renderer::TraceRays(D3D12Texture* BackBuffer, D3D12CommandList* CommandList)
{
	CommandList->TransitionBarrier(ResultTexture, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	D3D12_DISPATCH_RAYS_DESC raytraceDesc = {};
	raytraceDesc.Width	= SwapChain->GetWidth();
	raytraceDesc.Height = SwapChain->GetHeight();
	raytraceDesc.Depth	= 1;

	// Set shader tables
	raytraceDesc.RayGenerationShaderRecord	= PipelineState->GetRayGenerationShaderRecord();
	raytraceDesc.MissShaderTable			= PipelineState->GetMissShaderTable();
	raytraceDesc.HitGroupTable				= PipelineState->GetHitGroupTable();

	// Bind the empty root signature
	CommandList->SetComputeRootSignature(PipelineState->GetGlobalRootSignature());
	//CommandList->SetComputeRootDescriptorTable(CameraBuffer->GetConstantBufferView()->GetOfflineHandle(), 0);

	// Dispatch
	CommandList->SetStateObject(PipelineState->GetStateObject());
	CommandList->DispatchRays(&raytraceDesc);

	// Copy the results to the back-buffer
	CommandList->TransitionBarrier(ResultTexture, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	CommandList->TransitionBarrier(BackBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);

	CommandList->CopyResource(BackBuffer, ResultTexture);

	// Indicate that the back buffer will now be used to present.
	CommandList->TransitionBarrier(BackBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void Renderer::OnResize(Int32 Width, Int32 Height)
{
	WaitForPendingFrames();

	SwapChain->Resize(Width, Height);

	SAFEDELETE(ResultTexture);

	CreateResultTexture();

	CurrentBackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
}

void Renderer::OnMouseMove(Int32 X, Int32 Y)
{
	if (IsCameraAcive)
	{
		static Int32 OldX = X;
		static Int32 OldY = Y;

		const Int32 DeltaX = OldX - X;
		const Int32 DeltaY = Y - OldY;

		SceneCamera.Rotate(XMConvertToRadians(static_cast<Float32>(DeltaY)), XMConvertToRadians(static_cast<Float32>(DeltaX)), 0.0f);
		SceneCamera.UpdateMatrices();

		OldX = X;
		OldY = Y;
	}
}

void Renderer::OnKeyPressed(EKey KeyCode)
{
	if (KeyCode == EKey::KEY_ESCAPE)
	{
		IsCameraAcive = !IsCameraAcive;
	}
}

Renderer* Renderer::Make(std::shared_ptr<WindowsWindow>& RendererWindow)
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

bool Renderer::Initialize(std::shared_ptr<WindowsWindow>& RendererWindow)
{
	Device = std::shared_ptr<D3D12Device>(D3D12Device::Make(true));
	if (!Device)
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
	UploadBuffers.resize(BackBufferCount);

	for (Uint32 i = 0; i < BackBufferCount; i++)
	{
		CommandAllocators[i] = std::make_shared<D3D12CommandAllocator>(Device.get());
		if (!CommandAllocators[i]->Initialize(D3D12_COMMAND_LIST_TYPE_DIRECT))
		{
			return false;
		}

		UploadBuffers[i] = std::make_shared<D3D12UploadStack>();
		if (!UploadBuffers[i]->Initialize(Device.get()))
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

	// Create image
	if (!CreateResultTexture())
	{
		return false;
	}

	// Create mesh
	MeshData Mesh = MeshFactory::CreateSphere(3);
	MeshData Cube = MeshFactory::CreateCube();

	// Create CameraBuffer
	SceneCamera = Camera();

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

		CameraBuffer->SetConstantBufferView(std::make_shared<D3D12ConstantBufferView>(Device.get(), CameraBuffer, &CameraViewDesc));
	}
	else
	{
		return false;
	}

	// Create vertexbuffer
	BufferProps.InitalState		= D3D12_RESOURCE_STATE_GENERIC_READ;
	BufferProps.SizeInBytes		= sizeof(Vertex) * static_cast<Uint64>(Mesh.Vertices.size());
	BufferProps.MemoryType		= EMemoryType::MEMORY_TYPE_UPLOAD;

	std::shared_ptr<D3D12Buffer> MeshVertexBuffer = std::make_shared<D3D12Buffer>(Device.get());
	if (MeshVertexBuffer->Initialize(BufferProps))
	{
		void* BufferMemory = MeshVertexBuffer->Map();
		memcpy(BufferMemory, Mesh.Vertices.data(), BufferProps.SizeInBytes);
		MeshVertexBuffer->Unmap();
	}
	else
	{
		return false;
	}

	BufferProps.SizeInBytes = sizeof(Vertex) * static_cast<Uint64>(Cube.Vertices.size());
	std::shared_ptr<D3D12Buffer> CubeVertexBuffer = std::make_shared<D3D12Buffer>(Device.get());
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

	// Create indexbuffer
	BufferProps.SizeInBytes = sizeof(Uint32) * static_cast<Uint64>(Mesh.Indices.size());
	std::shared_ptr<D3D12Buffer> MeshIndexBuffer = std::make_shared<D3D12Buffer>(Device.get());
	if (MeshIndexBuffer->Initialize(BufferProps))
	{
		void* BufferMemory = MeshIndexBuffer->Map();
		memcpy(BufferMemory, Mesh.Indices.data(), BufferProps.SizeInBytes);
		MeshIndexBuffer->Unmap();
	}
	else
	{
		return false;
	}

	BufferProps.SizeInBytes = sizeof(Uint32) * static_cast<Uint64>(Cube.Indices.size());
	std::shared_ptr<D3D12Buffer> CubeIndexBuffer = std::make_shared<D3D12Buffer>(Device.get());
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

	// Create Texture Cube
	Panorama = std::shared_ptr<D3D12Texture>(TextureFactory::LoadFromFile(Device.get(), "../Assets/Textures/arches.hdr"));
	if (!Panorama)
	{
		return false;	
	}

	// Return before createing accelerationstructure
	if (!Device->IsRayTracingSupported())
	{
		return true;
	}

	// Build Acceleration Structures
	CommandAllocators[0]->Reset();
	CommandList->Reset(CommandAllocators[0].get());

	CommandList->TransitionBarrier(CameraBuffer.get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	// Create BLAS
	std::shared_ptr<D3D12RayTracingGeometry> MeshGeometry = std::make_shared<D3D12RayTracingGeometry>(Device.get());
	MeshGeometry->Initialize(CommandList.get(), MeshVertexBuffer, static_cast<Uint32>(Mesh.Vertices.size()), MeshIndexBuffer, static_cast<Uint64>(Mesh.Indices.size()));

	std::shared_ptr<D3D12RayTracingGeometry> CubeGeometry = std::make_shared<D3D12RayTracingGeometry>(Device.get());
	CubeGeometry->Initialize(CommandList.get(), CubeVertexBuffer, static_cast<Uint32>(Cube.Vertices.size()), CubeIndexBuffer, static_cast<Uint64>(Cube.Indices.size()));

	XMFLOAT3X4 Matrix;
	std::vector<D3D12RayTracingGeometryInstance> Instances;

	constexpr Float32	Offset			= 1.25f;
	constexpr Uint32	SphereCountX	= 8;
	constexpr Float32	StartPositionX	= (-static_cast<Float32>(SphereCountX) * Offset) / 2.0f;
	constexpr Uint32	SphereCountY	= 8;
	constexpr Float32	StartPositionY	= (-static_cast<Float32>(SphereCountY) * Offset) / 2.0f;
	for (Uint32 y = 0; y < SphereCountY; y++)
	{
		for (Uint32 x = 0; x < SphereCountX; x++)
		{
			XMStoreFloat3x4(&Matrix, XMMatrixTranslation(StartPositionX + (x * Offset), StartPositionY + (y * Offset), 0));
			Instances.emplace_back(MeshGeometry, Matrix);
		}
	}

	// Create TLAS
	Scene = std::make_shared<D3D12RayTracingScene>(Device.get());
	Scene->Initialize(CommandList.get(), Instances);

	CommandList->Close();
	Queue->ExecuteCommandList(CommandList.get());

	const Uint64 FenceValue = 1;
	Queue->SignalFence(Fence.get(), FenceValue);
	FenceValues[0] = FenceValue + 1;

	// VertexBuffer
	D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = { };
	SrvDesc.Format						= DXGI_FORMAT_UNKNOWN;
	SrvDesc.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER;
	SrvDesc.Shader4ComponentMapping		= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SrvDesc.Buffer.FirstElement			= 0;
	SrvDesc.Buffer.Flags				= D3D12_BUFFER_SRV_FLAG_NONE;
	SrvDesc.Buffer.NumElements			= static_cast<Uint32>(Mesh.Vertices.size());
	SrvDesc.Buffer.StructureByteStride	= sizeof(Vertex);

	MeshVertexBuffer->SetShaderResourceView(std::make_shared<D3D12ShaderResourceView>(Device.get(), MeshVertexBuffer->GetResource(), &SrvDesc));

	// IndexBuffer
	SrvDesc.Format						= DXGI_FORMAT_R32_TYPELESS;
	SrvDesc.Buffer.Flags				= D3D12_BUFFER_SRV_FLAG_RAW;
	SrvDesc.Buffer.NumElements			= static_cast<Uint32>(Mesh.Indices.size());
	SrvDesc.Buffer.StructureByteStride	= 0;

	MeshIndexBuffer->SetShaderResourceView(std::make_shared<D3D12ShaderResourceView>(Device.get(), MeshIndexBuffer->GetResource(), &SrvDesc));

	// Create PipelineState
	PipelineState = std::make_shared<D3D12RayTracingPipelineState>(Device.get());
	if (!PipelineState->Initialize())
	{
		return false;
	}

	return true;
}

bool Renderer::CreateResultTexture()
{
	TextureProperties OutputProperties = { };
	OutputProperties.Flags			= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	OutputProperties.Width			= SwapChain->GetWidth();
	OutputProperties.Height			= SwapChain->GetHeight();
	OutputProperties.Format			= DXGI_FORMAT_R8G8B8A8_UNORM;
	OutputProperties.MemoryType		= EMemoryType::MEMORY_TYPE_DEFAULT;

	ResultTexture = new D3D12Texture(Device.get());
	if (!ResultTexture->Initialize(OutputProperties))
	{
		return false;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVView = { };
	UAVView.Format					= OutputProperties.Format;
	UAVView.ViewDimension			= D3D12_UAV_DIMENSION_TEXTURE2D;
	UAVView.Texture2D.MipSlice		= 0;
	UAVView.Texture2D.PlaneSlice	= 0;

	ResultTexture->SetUnorderedAccessView(std::make_shared<D3D12UnorderedAccessView>(Device.get(), nullptr, ResultTexture->GetResource(), &UAVView));
	return true;
}

void Renderer::WaitForPendingFrames()
{
	const Uint64 CurrentFenceValue = FenceValues[CurrentBackBufferIndex];

	Queue->SignalFence(Fence.get(), CurrentFenceValue);
	if (Fence->WaitForValue(CurrentFenceValue))
	{
		FenceValues[CurrentBackBufferIndex]++;
	}
}
