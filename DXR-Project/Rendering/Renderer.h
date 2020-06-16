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
#include "D3D12/D3D12RayTracingPipelineState.h"

#include "Windows/WindowsWindow.h"

#include <memory>
#include <vector>

#include "Camera.h"

class Renderer
{
public:
	Renderer();
	~Renderer();
	
	void Tick();
	
	void OnResize(Int32 NewWidth, Int32 NewHeight);
	void OnKeyDown(Uint32 KeyCode);

	static Renderer* Create(std::shared_ptr<WindowsWindow> RendererWindow);
	static Renderer* Get();
	
private:
	bool Init(std::shared_ptr<WindowsWindow> RendererWindow);

private:
	std::shared_ptr<D3D12Device>			Device				= nullptr;
	std::shared_ptr<D3D12CommandQueue>		Queue				= nullptr;
	std::shared_ptr<D3D12CommandList>		CommandList			= nullptr;
	std::shared_ptr<D3D12DescriptorHeap>	RenderTargetHeap	= nullptr;
	std::shared_ptr<D3D12DescriptorHeap>	DepthStencilHeap	= nullptr;
	std::shared_ptr<D3D12Fence>				Fence				= nullptr;
	std::shared_ptr<D3D12SwapChain>			SwapChain			= nullptr;

	std::vector<std::shared_ptr<D3D12CommandAllocator>> CommandAllocators;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>			BackBufferHandles;
	std::vector<Uint64>									FenceValues;

	class D3D12Texture* ResultTexture = nullptr;

	D3D12_CPU_DESCRIPTOR_HANDLE VertexBufferCPUHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE IndexBufferCPUHandle;

	std::shared_ptr<D3D12Buffer>					CameraBuffer	= nullptr;
	std::shared_ptr<D3D12RayTracingScene>			Scene			= nullptr;
	std::shared_ptr<D3D12RayTracingPipelineState>	PipelineState	= nullptr;

	Camera SceneCamera;

	D3D12_GPU_DESCRIPTOR_HANDLE CameraBufferGPUHandle;

	D3D12_CPU_DESCRIPTOR_HANDLE CameraBufferCPUHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE OutputCPUHandle;

	bool IsCameraAcive = false;

	static std::unique_ptr<Renderer> RendererInstance;
};


//
//void D3D12RayTracer::OnMouseMove(Int32 x, Int32 y)
//{
//	if (IsCameraAcive)
//	{
//		static Int32 OldX = x;
//		static Int32 OldY = y;
//
//		const Int32 DeltaX = OldX - x;
//		const Int32 DeltaY = y - OldY;
//
//		SceneCamera.Rotate(XMConvertToRadians(static_cast<Float32>(DeltaY)), XMConvertToRadians(static_cast<Float32>(DeltaX)), 0.0f);
//		SceneCamera.UpdateMatrices();
//
//		OldX = x;
//		OldY = y;
//	}
//}