#include "D3D12GraphicsDevice.h"

/*
* Members
*/

D3D12GraphicsDevice::D3D12GraphicsDevice()
{
}

D3D12GraphicsDevice::~D3D12GraphicsDevice()
{
}

bool D3D12GraphicsDevice::Init()
{


	return false;
}

/*
* Static
*/

std::unique_ptr<D3D12GraphicsDevice> D3D12GraphicsDevice::D3D12Device = nullptr;

D3D12GraphicsDevice* D3D12GraphicsDevice::Create()
{
	D3D12Device.reset(new D3D12GraphicsDevice());
	if (D3D12Device->Init())
	{
		return D3D12Device.get();
	}
	else
	{
		return nullptr;
	}
}

D3D12GraphicsDevice* D3D12GraphicsDevice::Get()
{
	return D3D12Device.get();
}

bool D3D12GraphicsDevice::CreateFactory()
{
	return false;
}

bool D3D12GraphicsDevice::ChooseAdapter()
{
	return false;
}
