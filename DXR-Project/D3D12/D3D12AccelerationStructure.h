#pragma once
#include "D3D12DeviceChild.h"

#include "Defines.h"

class D3D12Buffer;

class D3D12AccelerationStructure : public D3D12DeviceChild
{
public:
	D3D12AccelerationStructure(D3D12Device* Device)
		: D3D12DeviceChild(Device)
	{
	}

	~D3D12AccelerationStructure()
	{
		SAFEDELETE(ScratchBuffer);
		SAFEDELETE(ResultBuffer);
		SAFEDELETE(InstanceBuffer);
	}

public:
	D3D12Buffer* ScratchBuffer	= nullptr;
	D3D12Buffer* ResultBuffer	= nullptr;
	D3D12Buffer* InstanceBuffer = nullptr;
};