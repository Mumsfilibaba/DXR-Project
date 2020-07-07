#include "Renderer.h"
#include "MeshFactory.h"
#include "TextureFactory.h"
#include "GuiContext.h"

#include "D3D12/D3D12Texture.h"
#include "D3D12/D3D12Views.h"
#include "D3D12/D3D12RootSignature.h"

#include "Application/InputManager.h"

std::unique_ptr<Renderer> Renderer::RendererInstance = nullptr;

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
}

void Renderer::Tick()
{
	Frameclock.Tick();

	const Float32 Delta = Frameclock.GetDeltaTime().AsSeconds();
	const Float32 Speed			= 1.0f;
	const Float32 RotationSpeed = 45.0f;
	if (InputManager::Get().IsKeyDown(EKey::KEY_RIGHT))
	{
		SceneCamera.Rotate(0.0f, XMConvertToRadians(RotationSpeed * Delta), 0.0f);
	}
	else if (InputManager::Get().IsKeyDown(EKey::KEY_LEFT))
	{
		SceneCamera.Rotate(0.0f, XMConvertToRadians(-RotationSpeed * Delta), 0.0f);
	}

	if (InputManager::Get().IsKeyDown(EKey::KEY_UP))
	{
		SceneCamera.Rotate(XMConvertToRadians(-RotationSpeed * Delta), 0.0f, 0.0f);
	}
	else if (InputManager::Get().IsKeyDown(EKey::KEY_DOWN))
	{
		SceneCamera.Rotate(XMConvertToRadians(RotationSpeed * Delta), 0.0f, 0.0f);
	}

	if (InputManager::Get().IsKeyDown(EKey::KEY_W))
	{
		SceneCamera.Move(0.0f, 0.0f, Speed * Delta);
	}
	else if (InputManager::Get().IsKeyDown(EKey::KEY_S))
	{
		SceneCamera.Move(0.0f, 0.0f, -Speed * Delta);
	}
	
	if (InputManager::Get().IsKeyDown(EKey::KEY_A))
	{
		SceneCamera.Move(Speed * Delta, 0.0f, 0.0f);
	}
	else if (InputManager::Get().IsKeyDown(EKey::KEY_D))
	{
		SceneCamera.Move(-Speed * Delta, 0.0f, 0.0f);
	}
	
	if (InputManager::Get().IsKeyDown(EKey::KEY_Q))
	{
		SceneCamera.Move(0.0f, Speed * Delta, 0.0f);
	}
	else if (InputManager::Get().IsKeyDown(EKey::KEY_E))
	{
		SceneCamera.Move(0.0f, -Speed * Delta, 0.0f);
	}

	SceneCamera.UpdateMatrices();

	D3D12Texture* BackBuffer = SwapChain->GetSurfaceResource(CurrentBackBufferIndex);

	CommandAllocators[CurrentBackBufferIndex]->Reset();
	CommandList->Reset(CommandAllocators[CurrentBackBufferIndex].get());
	UploadBuffers[CurrentBackBufferIndex]->Reset();

	// Set constant buffer descriptor heap
	ID3D12DescriptorHeap* DescriptorHeaps[] = { Device->GetGlobalOnlineResourceHeap()->GetHeap() };
	CommandList->SetDescriptorHeaps(DescriptorHeaps, ARRAYSIZE(DescriptorHeaps));

	struct CameraBufferDesc
	{
		XMFLOAT4X4	ViewProjection;
		XMFLOAT4X4	ViewProjectionInv;
		XMFLOAT3	Position;
	} CamBuff;

	CamBuff.ViewProjection		= SceneCamera.GetViewProjection();
	CamBuff.ViewProjectionInv	= SceneCamera.GetViewProjectionInverse();
	CamBuff.Position			= SceneCamera.GetPosition();

	Uint64 Offset = UploadBuffers[CurrentBackBufferIndex]->GetOffset();
	void* CameraMemory = UploadBuffers[CurrentBackBufferIndex]->Allocate(sizeof(CameraBufferDesc));
	memcpy(CameraMemory, &CamBuff, sizeof(CameraBufferDesc));
	
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

void Renderer::TraceRays(D3D12Texture* BackBuffer, D3D12CommandList* InCommandList)
{
	InCommandList->TransitionBarrier(ResultTexture.get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	D3D12_DISPATCH_RAYS_DESC raytraceDesc = {};
	raytraceDesc.Width	= SwapChain->GetWidth();
	raytraceDesc.Height = SwapChain->GetHeight();
	raytraceDesc.Depth	= 1;

	// Set shader tables
	raytraceDesc.RayGenerationShaderRecord	= RayTracingScene->GetRayGenerationShaderRecord();
	raytraceDesc.MissShaderTable			= RayTracingScene->GetMissShaderTable();
	raytraceDesc.HitGroupTable				= RayTracingScene->GetHitGroupTable();

	// Bind the empty root signature
	InCommandList->SetComputeRootSignature(GlobalRootSignature->GetRootSignature());
	InCommandList->SetComputeRootDescriptorTable(GlobalDescriptorTable->GetGPUTableStartHandle(), 0);

	// Dispatch
	InCommandList->SetStateObject(PipelineState->GetStateObject());
	InCommandList->DispatchRays(&raytraceDesc);

	// Copy the results to the back-buffer
	InCommandList->TransitionBarrier(ResultTexture.get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	InCommandList->TransitionBarrier(BackBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);

	InCommandList->CopyResource(BackBuffer, ResultTexture.get());

	// Indicate that the back buffer will now be used to present.
	InCommandList->TransitionBarrier(BackBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
}

void Renderer::OnResize(Int32 Width, Int32 Height)
{
	WaitForPendingFrames();

	SwapChain->Resize(Width, Height);

	ResultTexture.reset();

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

		CameraBuffer->SetConstantBufferView(std::make_shared<D3D12ConstantBufferView>(Device.get(), CameraBuffer->GetResource(), &CameraViewDesc));
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
	std::unique_ptr<D3D12Texture> Panorama = std::unique_ptr<D3D12Texture>(TextureFactory::LoadFromFile(Device.get(), "../Assets/Textures/arches.hdr", 0));
	if (!Panorama)
	{
		return false;	
	}

	Skybox = std::shared_ptr<D3D12Texture>(TextureFactory::CreateTextureCubeFromPanorma(Device.get(), Panorama.get(), 512));
	if (!Skybox)
	{
		return false;
	}

	Albedo = std::shared_ptr<D3D12Texture>(TextureFactory::LoadFromFile(Device.get(), "../Assets/Textures/RockySoil_Albedo.png", TEXTURE_FACTORY_FLAGS_GENERATE_MIPS));
	if (!Albedo)
	{
		return false;
	}

	Normal = std::shared_ptr<D3D12Texture>(TextureFactory::LoadFromFile(Device.get(), "../Assets/Textures/RockySoil_Normal.png", TEXTURE_FACTORY_FLAGS_GENERATE_MIPS));
	if (!Normal)
	{
		return false;
	}

	// Return before createing accelerationstructure
	if (!Device->IsRayTracingSupported())
	{
		return true;
	}

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
			RayGenLocalRoot->SetName("RayGen Local RootSignature");
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
			HitLocalRoot->SetName("Closest Hit Local RootSignature");
		}
		else
		{
			return false;
		}
	}

	std::unique_ptr<D3D12RootSignature> MissLocalRoot;
	{
		D3D12_ROOT_SIGNATURE_DESC MissLocalRootDesc = {};
		MissLocalRootDesc.NumParameters = 0;
		MissLocalRootDesc.pParameters	= nullptr;
		MissLocalRootDesc.Flags			= D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

		MissLocalRoot = std::make_unique<D3D12RootSignature>(Device.get());
		if (MissLocalRoot->Initialize(MissLocalRootDesc))
		{
			MissLocalRoot->SetName("Miss Local RootSignature");
		}
		else
		{
			return false;
		}
	}

	// Global RootSignature
	{
		D3D12_DESCRIPTOR_RANGE Ranges[5] = {};
		// AccelerationStructure
		Ranges[0].BaseShaderRegister				= 0;
		Ranges[0].NumDescriptors					= 1;
		Ranges[0].RegisterSpace						= 0;
		Ranges[0].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		Ranges[0].OffsetInDescriptorsFromTableStart = 0;

		// Camera Buffer
		Ranges[1].BaseShaderRegister				= 0;
		Ranges[1].NumDescriptors					= 1;
		Ranges[1].RegisterSpace						= 0;
		Ranges[1].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		Ranges[1].OffsetInDescriptorsFromTableStart = 1;

		// Skybox
		Ranges[2].BaseShaderRegister				= 1;
		Ranges[2].NumDescriptors					= 1;
		Ranges[2].RegisterSpace						= 0;
		Ranges[2].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		Ranges[2].OffsetInDescriptorsFromTableStart = 2;

		// Albedo
		Ranges[3].BaseShaderRegister				= 4;
		Ranges[3].NumDescriptors					= 1;
		Ranges[3].RegisterSpace						= 0;
		Ranges[3].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		Ranges[3].OffsetInDescriptorsFromTableStart = 3;

		// Normal
		Ranges[4].BaseShaderRegister				= 5;
		Ranges[4].NumDescriptors					= 1;
		Ranges[4].RegisterSpace						= 0;
		Ranges[4].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		Ranges[4].OffsetInDescriptorsFromTableStart = 4;

		D3D12_ROOT_PARAMETER RootParams = { };
		RootParams.ParameterType						= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		RootParams.DescriptorTable.NumDescriptorRanges	= 5;
		RootParams.DescriptorTable.pDescriptorRanges	= Ranges;

		D3D12_STATIC_SAMPLER_DESC Sampler = { };
		Sampler.ShaderVisibility	= D3D12_SHADER_VISIBILITY_ALL;
		Sampler.AddressU			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		Sampler.AddressV			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		Sampler.AddressW			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		Sampler.Filter				= D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		Sampler.ShaderRegister		= 0;
		Sampler.RegisterSpace		= 0;
		Sampler.MinLOD				= 0.0f;
		Sampler.MaxLOD				= FLT_MAX;

		D3D12_ROOT_SIGNATURE_DESC GlobalRootDesc = {};
		GlobalRootDesc.NumStaticSamplers	= 1;
		GlobalRootDesc.pStaticSamplers		= &Sampler;
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

	PipelineState = std::make_shared<D3D12RayTracingPipelineState>(Device.get());
	if (!PipelineState->Initialize(PipelineProperties))
	{
		return false;
	}


	// Build Acceleration Structures
	CommandAllocators[0]->Reset();
	CommandList->Reset(CommandAllocators[0].get());

	CommandList->TransitionBarrier(CameraBuffer.get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	// Create BLAS
	std::shared_ptr<D3D12RayTracingGeometry> MeshGeometry = std::make_shared<D3D12RayTracingGeometry>(Device.get());
	MeshGeometry->BuildAccelerationStructure(CommandList.get(), MeshVertexBuffer, static_cast<Uint32>(Mesh.Vertices.size()), MeshIndexBuffer, static_cast<Uint64>(Mesh.Indices.size()));

	std::shared_ptr<D3D12RayTracingGeometry> CubeGeometry = std::make_shared<D3D12RayTracingGeometry>(Device.get());
	CubeGeometry->BuildAccelerationStructure(CommandList.get(), CubeVertexBuffer, static_cast<Uint32>(Cube.Vertices.size()), CubeIndexBuffer, static_cast<Uint64>(Cube.Indices.size()));

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
			Instances.emplace_back(MeshGeometry, Matrix, 0, 0);
		}
	}

	XMStoreFloat3x4(&Matrix, XMMatrixTranslation(0.0f, 0.0f, -3.0f));
	Instances.emplace_back(CubeGeometry, Matrix, 1, 1);

	// Create DescriptorTables
	std::shared_ptr<D3D12DescriptorTable> RayGenDescriptorTable	= std::make_shared<D3D12DescriptorTable>(Device.get(), 1);
	std::shared_ptr<D3D12DescriptorTable> SphereDescriptorTable	= std::make_shared<D3D12DescriptorTable>(Device.get(), 2);
	std::shared_ptr<D3D12DescriptorTable> CubeDescriptorTable	= std::make_shared<D3D12DescriptorTable>(Device.get(), 2);
	GlobalDescriptorTable = std::make_shared<D3D12DescriptorTable>(Device.get(), 5);

	// Create TLAS
	std::vector<BindingTableEntry> BindingTableEntries;
	BindingTableEntries.emplace_back("RayGen", RayGenDescriptorTable);
	BindingTableEntries.emplace_back("HitGroup", SphereDescriptorTable);
	BindingTableEntries.emplace_back("HitGroup", CubeDescriptorTable);
	BindingTableEntries.emplace_back("Miss", nullptr);

	RayTracingScene = std::make_shared<D3D12RayTracingScene>(Device.get());
	if (!RayTracingScene->Initialize(PipelineState.get(), BindingTableEntries, 2))
	{
		return false;
	}

	RayTracingScene->BuildAccelerationStructure(CommandList.get(), Instances);

	CommandList->Close();
	Queue->ExecuteCommandList(CommandList.get());
	Queue->WaitForCompletion();

	// VertexBuffer
	D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = { };
	SrvDesc.Format						= DXGI_FORMAT_UNKNOWN;
	SrvDesc.ViewDimension				= D3D12_SRV_DIMENSION_BUFFER;
	SrvDesc.Shader4ComponentMapping		= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SrvDesc.Buffer.FirstElement			= 0;
	SrvDesc.Buffer.Flags				= D3D12_BUFFER_SRV_FLAG_NONE;
	SrvDesc.Buffer.NumElements			= static_cast<Uint32>(Mesh.Vertices.size());
	SrvDesc.Buffer.StructureByteStride	= sizeof(Vertex);
	
	MeshVertexBuffer->SetShaderResourceView(std::make_shared<D3D12ShaderResourceView>(Device.get(), MeshVertexBuffer->GetResource(), &SrvDesc), 0);
	
	SrvDesc.Buffer.NumElements = static_cast<Uint32>(Cube.Vertices.size());
	CubeVertexBuffer->SetShaderResourceView(std::make_shared<D3D12ShaderResourceView>(Device.get(), CubeVertexBuffer->GetResource(), &SrvDesc), 0);

	// IndexBuffer
	SrvDesc.Format						= DXGI_FORMAT_R32_TYPELESS;
	SrvDesc.Buffer.Flags				= D3D12_BUFFER_SRV_FLAG_RAW;
	SrvDesc.Buffer.NumElements			= static_cast<Uint32>(Mesh.Indices.size());
	SrvDesc.Buffer.StructureByteStride	= 0;

	MeshIndexBuffer->SetShaderResourceView(std::make_shared<D3D12ShaderResourceView>(Device.get(), MeshIndexBuffer->GetResource(), &SrvDesc), 0);
	
	SrvDesc.Buffer.NumElements = static_cast<Uint32>(Cube.Indices.size());
	CubeIndexBuffer->SetShaderResourceView(std::make_shared<D3D12ShaderResourceView>(Device.get(), CubeIndexBuffer->GetResource(), &SrvDesc), 0);

	// Populate descriptors
	RayGenDescriptorTable->SetUnorderedAccessView(ResultTexture->GetUnorderedAccessView(0).get(), 0);
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
	GlobalDescriptorTable->CopyDescriptors();

	return true;
}

bool Renderer::CreateResultTexture()
{
	TextureProperties OutputProperties = { };
	OutputProperties.DebugName	= "RayTracing Output";
	OutputProperties.Flags		= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	OutputProperties.Width		= SwapChain->GetWidth();
	OutputProperties.Height		= SwapChain->GetHeight();
	OutputProperties.MipLevels	= 1;
	OutputProperties.ArrayCount	= 1;
	OutputProperties.Format		= DXGI_FORMAT_R8G8B8A8_UNORM;
	OutputProperties.MemoryType	= EMemoryType::MEMORY_TYPE_DEFAULT;

	ResultTexture = std::shared_ptr<D3D12Texture>(new D3D12Texture(Device.get()));
	if (!ResultTexture->Initialize(OutputProperties))
	{
		return false;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVView = { };
	UAVView.Format					= OutputProperties.Format;
	UAVView.ViewDimension			= D3D12_UAV_DIMENSION_TEXTURE2D;
	UAVView.Texture2D.MipSlice		= 0;
	UAVView.Texture2D.PlaneSlice	= 0;

	ResultTexture->SetUnorderedAccessView(std::make_shared<D3D12UnorderedAccessView>(Device.get(), nullptr, ResultTexture->GetResource(), &UAVView), 0);
	return true;
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