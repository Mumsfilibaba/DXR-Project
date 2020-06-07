#include "Windows/WindowsApplication.h"
#include "Windows/WindowsWindow.h"

#include "D3D12/D3D12GraphicsDevice.h"
#include "D3D12/D3D12CommandQueue.h"
#include "D3D12/D3D12DescriptorHeap.h"
#include "D3D12/D3D12SwapChain.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR cmdLine, int cmdShow)
{
	WindowsApplication* App = WindowsApplication::Create(hInstance);

	WindowsWindow* Window = App->CreateWindow(1280, 720);
	Window->Show();

	D3D12GraphicsDevice* Device = D3D12GraphicsDevice::Create(true);

	D3D12CommandQueue* Queue = new D3D12CommandQueue(Device);
	Queue->Init();

	D3D12DescriptorHeap* RtvHeap = new D3D12DescriptorHeap(Device);
	RtvHeap->Init(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 3, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	D3D12DescriptorHeap* DsvHeap = new D3D12DescriptorHeap(Device);
	DsvHeap->Init(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	D3D12SwapChain* SwapChain = new D3D12SwapChain(Device);
	SwapChain->Init(Window, Queue);

	bool IsRunning = true;
	while (IsRunning)
	{
		IsRunning = App->Tick();
	}

	delete SwapChain;
	delete DsvHeap;
	delete RtvHeap;
	delete Queue;

	return 0;
}