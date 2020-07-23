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
#include "D3D12/D3D12UploadStack.h"

#include "Windows/WindowsWindow.h"

#include "Application/Clock.h"
#include "Application/InputCodes.h"

#include "Scene/Actor.h"
#include "Scene/Scene.h"

#include <memory>
#include <vector>

#include "Camera.h"
#include "MeshFactory.h"
#include "Material.h"
#include "Mesh.h"

class D3D12Texture;
class D3D12GraphicsPipelineState;
class D3D12RayTracingPipelineState;

/*
* RenderComponent
*/

class RenderComponent : public Component
{
public:
	RenderComponent(Actor* InOwningActor)
		: Component(InOwningActor)
		, Material(nullptr)
		, Mesh(nullptr)
	{
	}

	~RenderComponent() = default;

public:
	std::shared_ptr<Material>	Material;
	std::shared_ptr<Mesh>		Mesh;
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
	void OnMouseMove(Int32 X, Int32 Y);
	void OnKeyPressed(EKey KeyCode);

	FORCEINLINE std::shared_ptr<D3D12Device> GetDevice() const
	{
		return Device;
	}

	static Renderer* Make(std::shared_ptr<WindowsWindow> RendererWindow);
	static Renderer* Get();
	
private:
	bool Initialize(std::shared_ptr<WindowsWindow> RendererWindow);

	bool InitRayTracing();
	bool InitDeferred();
	bool InitGBuffer();
	bool InitIntegrationLUT();
	bool InitRayTracingTexture();

	void WaitForPendingFrames();

	void TraceRays(D3D12Texture* BackBuffer, D3D12CommandList* CommandList);

private:
	std::shared_ptr<D3D12Device>		Device;
	std::shared_ptr<D3D12CommandQueue>	Queue;
	std::shared_ptr<D3D12CommandQueue>	ComputeQueue;
	std::shared_ptr<D3D12CommandList>	CommandList;
	std::shared_ptr<D3D12Fence>			Fence;
	std::shared_ptr<D3D12SwapChain>		SwapChain;

	std::vector<std::shared_ptr<D3D12CommandAllocator>> CommandAllocators;

	MeshData Sphere;
	MeshData SkyboxMesh;
	MeshData Cube;

	std::shared_ptr<D3D12Buffer> CameraBuffer;
	std::shared_ptr<D3D12Buffer> MeshVertexBuffer;
	std::shared_ptr<D3D12Buffer> MeshIndexBuffer;
	std::shared_ptr<D3D12Buffer> SkyboxVertexBuffer;
	std::shared_ptr<D3D12Buffer> SkyboxIndexBuffer;
	std::shared_ptr<D3D12Buffer> CubeVertexBuffer;
	std::shared_ptr<D3D12Buffer> CubeIndexBuffer;

	std::shared_ptr<D3D12Texture> Skybox;
	std::shared_ptr<D3D12Texture> Albedo;
	std::shared_ptr<D3D12Texture> Normal;
	std::shared_ptr<D3D12Texture> ReflectionTexture;
	std::shared_ptr<D3D12Texture> IntegrationLUT;

	std::shared_ptr<D3D12Texture> GBuffer[4];
	
	std::shared_ptr<D3D12RootSignature>		GeometryRootSignature;
	std::shared_ptr<D3D12RootSignature>		LightRootSignature;
	std::shared_ptr<D3D12RootSignature>		SkyboxRootSignature;
	std::shared_ptr<D3D12RootSignature>		GlobalRootSignature;
	std::shared_ptr<D3D12DescriptorTable>	RayGenDescriptorTable;
	std::shared_ptr<D3D12DescriptorTable>	GlobalDescriptorTable;
	std::shared_ptr<D3D12DescriptorTable>	GeometryDescriptorTable;
	std::shared_ptr<D3D12DescriptorTable>	LightDescriptorTable;
	std::shared_ptr<D3D12DescriptorTable>	SkyboxDescriptorTable;
	std::shared_ptr<D3D12RayTracingScene>	RayTracingScene;

	std::shared_ptr<D3D12GraphicsPipelineState>		GeometryPSO;
	std::shared_ptr<D3D12GraphicsPipelineState>		LightPassPSO;
	std::shared_ptr<D3D12GraphicsPipelineState>		SkyboxPSO;
	std::shared_ptr<D3D12RayTracingPipelineState>	RaytracingPSO;

	std::vector<Uint64>	FenceValues;
	Uint32				CurrentBackBufferIndex = 0;

	std::vector<std::shared_ptr<D3D12UploadStack>> UploadBuffers;

	Clock Frameclock;
	Camera SceneCamera;

	bool IsCameraAcive = false;

	static std::unique_ptr<Renderer> RendererInstance;
};