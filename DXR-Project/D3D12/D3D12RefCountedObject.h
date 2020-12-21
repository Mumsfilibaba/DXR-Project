#pragma once
#include "Core/RefCountedObject.h"

#include "D3D12DeviceChild.h"

/*
* D3D12RefCountedObject
*/

class D3D12RefCountedObject : public D3D12DeviceChild, public RefCountedObject
{
public:
	inline D3D12RefCountedObject(D3D12Device* InDevice)
		: D3D12DeviceChild(InDevice)
		, RefCountedObject()
	{
	}
};