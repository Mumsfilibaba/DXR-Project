#include "Windows/WindowsApplication.h"
#include "Windows/WindowsWindow.h"

#include "D3D12/D3D12Device.h"
#include "D3D12/D3D12CommandQueue.h"
#include "D3D12/D3D12DescriptorHeap.h"
#include "D3D12/D3D12SwapChain.h"
#include "D3D12/D3D12CommandAllocator.h"
#include "D3D12/D3D12CommandList.h"
#include "D3D12/D3D12Fence.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow)
{
	WindowsApplication* App = WindowsApplication::Create(hInstance);

	WindowsWindow* Window = App->CreateWindow(1280, 720);
	Window->Show();

	D3D12Device* Device = D3D12Device::Create(true);

	D3D12CommandQueue* Queue = new D3D12CommandQueue(Device);
	Queue->Init();

	D3D12DescriptorHeap* RtvHeap = new D3D12DescriptorHeap(Device);
	RtvHeap->Init(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 3, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	D3D12DescriptorHeap* DsvHeap = new D3D12DescriptorHeap(Device);
	DsvHeap->Init(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	D3D12SwapChain* SwapChain = new D3D12SwapChain(Device);
	SwapChain->Init(Window, Queue);

	Uint32 BackBufferCount = SwapChain->GetSurfaceCount();
	std::vector<D3D12CommandAllocator*> Allocators(BackBufferCount);
	
	for (Uint32 i = 0; i < BackBufferCount; i++)
	{
		Allocators[i] = new D3D12CommandAllocator(Device);
		Allocators[i]->Init(D3D12_COMMAND_LIST_TYPE_DIRECT);
	}

	D3D12CommandList* CommandList = new D3D12CommandList(Device);
	CommandList->Init(D3D12_COMMAND_LIST_TYPE_DIRECT, Allocators[0], nullptr);

	D3D12Fence* Fence = new D3D12Fence(Device);
	Fence->Init(0);

	bool IsRunning = true;
	while (IsRunning)
	{
		IsRunning = App->Tick();
	}

	delete Fence;
	delete CommandList;
	delete SwapChain;
	delete DsvHeap;
	delete RtvHeap;
	delete Queue;

	return 0;
}