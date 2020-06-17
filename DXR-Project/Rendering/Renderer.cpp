#include "Renderer.h"
#include "MeshFactory.h"

#include "D3D12/D3D12Texture.h"
#include "D3D12/HeapProps.h"

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
	if (InputManager::Get().IsKeyDown(EKey::KEY_ESCAPE))
	{
		IsCameraAcive = !IsCameraAcive;
	}
	else if (InputManager::Get().IsKeyDown(EKey::KEY_RIGHT))
	{
		SceneCamera.Rotate(0.0f, -0.01f, 0.0f);
	}
	else if (InputManager::Get().IsKeyDown(EKey::KEY_LEFT))
	{
		SceneCamera.Rotate(0.0f, 0.01f, 0.0f);
	}
	else if (InputManager::Get().IsKeyDown(EKey::KEY_UP))
	{
		SceneCamera.Rotate(-0.01f, 0.0f, 0.0f);
	}
	else if (InputManager::Get().IsKeyDown(EKey::KEY_DOWN))
	{
		SceneCamera.Rotate(0.01f, 0.0f, 0.0f);
	}
	// W
	else if (InputManager::Get().IsKeyDown(EKey::KEY_W))
	{
		SceneCamera.Move(0.0f, 0.0f, 0.01f);
	}
	// S
	else if (InputManager::Get().IsKeyDown(EKey::KEY_S))
	{
		SceneCamera.Move(0.0f, 0.0f, -0.01f);
	}
	// A
	else if (InputManager::Get().IsKeyDown(EKey::KEY_A))
	{
		SceneCamera.Move(0.01f, 0.0f, 0.0f);
	}
	// D
	else if (InputManager::Get().IsKeyDown(EKey::KEY_D))
	{
		SceneCamera.Move(-0.01f, 0.0f, 0.0f);
	}
	// Q
	else if (InputManager::Get().IsKeyDown(EKey::KEY_Q))
	{
		SceneCamera.Move(0.0f, 0.01f, 0.0f);
	}
	// E
	else if (InputManager::Get().IsKeyDown(EKey::KEY_E))
	{
		SceneCamera.Move(0.0f, -0.01f, 0.0f);
	}

	SceneCamera.UpdateMatrices();

	Uint32			BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
	ID3D12Resource*	BackBuffer		= SwapChain->GetSurfaceResource(BackBufferIndex);

	CommandAllocators[BackBufferIndex]->Reset();
	CommandList->Reset(CommandAllocators[BackBufferIndex].get());

	//Set constant buffer descriptor heap
	ID3D12DescriptorHeap* DescriptorHeaps[] = { Device->GetGlobalResourceDescriptorHeap()->GetHeap() };
	CommandList->SetDescriptorHeaps(DescriptorHeaps, ARRAYSIZE(DescriptorHeaps));

	CommandList->TransitionBarrier(ResultTexture->GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	void* BufferMemory = CameraBuffer->Map();
	memcpy(BufferMemory, &SceneCamera, sizeof(Camera));
	CameraBuffer->Unmap();

	D3D12_DISPATCH_RAYS_DESC raytraceDesc = {};
	raytraceDesc.Width	= SwapChain->GetWidth();
	raytraceDesc.Height	= SwapChain->GetHeight();
	raytraceDesc.Depth	= 1;

	// Set shader tables
	raytraceDesc.RayGenerationShaderRecord	= PipelineState->GetRayGenerationShaderRecord();
	raytraceDesc.MissShaderTable			= PipelineState->GetMissShaderTable();
	raytraceDesc.HitGroupTable				= PipelineState->GetHitGroupTable();

	// Bind the empty root signature
	CommandList->SetComputeRootSignature(PipelineState->GetGlobalRootSignature());
	CommandList->SetComputeRootDescriptorTable(CameraBufferGPUHandle, 0);

	// Dispatch
	CommandList->SetStateObject(PipelineState->GetStateObject());
	CommandList->DispatchRays(&raytraceDesc);

	// Copy the results to the back-buffer
	CommandList->TransitionBarrier(ResultTexture->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	CommandList->TransitionBarrier(BackBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);

	CommandList->CopyResource(BackBuffer, ResultTexture->GetResource());

	// Indicate that the back buffer will now be used to present.
	CommandList->TransitionBarrier(BackBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);

	CommandList->Close();

	// Execute
	Queue->ExecuteCommandList(CommandList.get());

	// Present
	SwapChain->Present(1);

	// Wait for next frame
	const Uint64 CurrentFenceValue = FenceValues[BackBufferIndex];
	Queue->SignalFence(Fence.get(), CurrentFenceValue);

	BackBufferIndex = SwapChain->GetCurrentBackBufferIndex();
	if (Fence->WaitForValue(CurrentFenceValue))
	{
		FenceValues[BackBufferIndex] = CurrentFenceValue + 1;
	}
}

void Renderer::OnResize(Int32 NewWidth, Int32 NewHeight)
{
	WaitForPendingFrames();

	SwapChain->Resize(NewWidth, NewHeight);
	CreateRenderTargetViews();

	SAFEDELETE(ResultTexture);

	CreateResultTexture();
}

void Renderer::OnKeyDown(Uint32 KeyCode)
{
	
}

Renderer* Renderer::Create(std::shared_ptr<WindowsWindow> RendererWindow)
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
	Device = std::shared_ptr<D3D12Device>(D3D12Device::Create(true));
	if (!Device)
	{
		return false;
	}

	Queue = std::make_shared<D3D12CommandQueue>(Device.get());
	if (!Queue->Initialize())
	{
		return false;
	}

	RenderTargetHeap = std::make_shared<D3D12DescriptorHeap>(Device.get());
	if (!RenderTargetHeap->Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 3, D3D12_DESCRIPTOR_HEAP_FLAG_NONE))
	{
		return false;
	}

	DepthStencilHeap = std::make_shared<D3D12DescriptorHeap>(Device.get());
	if (!DepthStencilHeap->Initialize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_NONE))
	{
		return false;
	}

	SwapChain = std::make_shared<D3D12SwapChain>(Device.get());
	if (!SwapChain->Initialize(RendererWindow.get(), Queue.get()))
	{
		return false;
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

	// Create RenderTargetViews
	CreateRenderTargetViews();

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
	BufferProps.InitalState		= D3D12_RESOURCE_STATE_GENERIC_READ;
	BufferProps.HeapProperties	= HeapProps::UploadHeap();

	CameraBuffer = std::make_shared<D3D12Buffer>(Device.get());
	if (CameraBuffer->Initialize(BufferProps))
	{
		void* BufferMemory = CameraBuffer->Map();
		memcpy(BufferMemory, &SceneCamera, sizeof(Camera));
		CameraBuffer->Unmap();

		D3D12_CONSTANT_BUFFER_VIEW_DESC CameraViewDesc = { };
		CameraViewDesc.BufferLocation	= CameraBuffer->GetVirtualAddress();
		CameraViewDesc.SizeInBytes		= static_cast<Uint32>(BufferProps.SizeInBytes);

		CameraBufferGPUHandle = Device->GetGlobalResourceDescriptorHeap()->GetGPUDescriptorHandleAt(4);
		CameraBufferCPUHandle = Device->GetGlobalResourceDescriptorHeap()->GetCPUDescriptorHandleAt(4);
		Device->GetDevice()->CreateConstantBufferView(&CameraViewDesc, CameraBufferCPUHandle);
	}
	else
	{
		return false;
	}

	// Create vertexbuffer
	BufferProps.SizeInBytes = sizeof(Vertex) * static_cast<Uint64>(Mesh.Vertices.size());
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

	//return true;

	// Build Acceleration Structures
	CommandAllocators[0]->Reset();
	CommandList->Reset(CommandAllocators[0].get());

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

	VertexBufferCPUHandle = Device->GetGlobalResourceDescriptorHeap()->GetCPUDescriptorHandleAt(2);
	Device->GetDevice()->CreateShaderResourceView(MeshVertexBuffer->GetResource(), &SrvDesc, VertexBufferCPUHandle);

	// IndexBuffer
	SrvDesc.Format						= DXGI_FORMAT_R32_TYPELESS;
	SrvDesc.Buffer.Flags				= D3D12_BUFFER_SRV_FLAG_RAW;
	SrvDesc.Buffer.NumElements			= static_cast<Uint32>(Mesh.Indices.size());
	SrvDesc.Buffer.StructureByteStride	= 0;

	IndexBufferCPUHandle = Device->GetGlobalResourceDescriptorHeap()->GetCPUDescriptorHandleAt(3);
	Device->GetDevice()->CreateShaderResourceView(MeshIndexBuffer->GetResource(), &SrvDesc, IndexBufferCPUHandle);

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
	OutputProperties.HeapProperties	= HeapProps::DefaultHeap();

	ResultTexture = new D3D12Texture(Device.get());
	if (!ResultTexture->Initialize(OutputProperties))
	{
		return false;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC UavView = { };
	UavView.Format					= OutputProperties.Format;
	UavView.ViewDimension			= D3D12_UAV_DIMENSION_TEXTURE2D;
	UavView.Texture2D.MipSlice		= 0;
	UavView.Texture2D.PlaneSlice	= 0;

	OutputCPUHandle = Device->GetGlobalResourceDescriptorHeap()->GetCPUDescriptorHandleAt(0);
	Device->GetDevice()->CreateUnorderedAccessView(ResultTexture->GetResource(), nullptr, &UavView, OutputCPUHandle);
	
	return true;
}

void Renderer::CreateRenderTargetViews()
{
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> BackBufferHandles(SwapChain->GetSurfaceCount());
	for (Uint32 i = 0; i < SwapChain->GetSurfaceCount(); i++)
	{
		D3D12_RENDER_TARGET_VIEW_DESC RtvDesc = { };
		RtvDesc.ViewDimension			= D3D12_RTV_DIMENSION_TEXTURE2D;
		RtvDesc.Format					= SwapChain->GetSurfaceFormat();
		RtvDesc.Texture2D.MipSlice		= 0;
		RtvDesc.Texture2D.PlaneSlice	= 0;

		D3D12_CPU_DESCRIPTOR_HANDLE Handle = RenderTargetHeap->GetCPUDescriptorHandleAt(i);
		Device->GetDevice()->CreateRenderTargetView(SwapChain->GetSurfaceResource(i), &RtvDesc, Handle);

		BackBufferHandles[i] = Handle;
	}
}

void Renderer::WaitForPendingFrames()
{
	const Uint32 BackBufferIndex	= SwapChain->GetCurrentBackBufferIndex();
	const Uint64 CurrentFenceValue	= FenceValues[BackBufferIndex];
	Queue->SignalFence(Fence.get(), CurrentFenceValue);
	if (Fence->WaitForValue(CurrentFenceValue))
	{
		FenceValues[BackBufferIndex]++;
	}
}
