#pragma once
#include "D3D12DeviceChild.h"
#include "D3D12Buffer.h"

#include "Defines.h"

#include <memory>

class D3D12CommandList;

/*
* Geometry - Equal to the Bottom-Level AccelerationStructure
*/

class D3D12RayTracingGeometry : public D3D12DeviceChild
{
public:
	D3D12RayTracingGeometry(D3D12Device* Device);
	~D3D12RayTracingGeometry();

	bool Initialize(D3D12CommandList* CommandList, std::shared_ptr<D3D12Buffer> VertexBuffer, Uint32 VertexCount, std::shared_ptr<D3D12Buffer> IndexBuffer, Uint32 IndexCount);

	D3D12_GPU_VIRTUAL_ADDRESS GetVirtualAddress() const;

public:
	// DeviceChild Interface
	virtual void SetName(const std::string& InName) override;

private:
	std::shared_ptr<D3D12Buffer> VertexBuffer	= nullptr;
	std::shared_ptr<D3D12Buffer> IndexBuffer	= nullptr;
	
	D3D12Buffer* ResultBuffer	= nullptr;
	D3D12Buffer* ScratchBuffer	= nullptr;
};

class D3D12RayTracingGeometryInstance
{
public:
	D3D12RayTracingGeometryInstance(std::shared_ptr<D3D12RayTracingGeometry> Geometry, XMFLOAT3X4 Transform)
		: Geometry(Geometry),
		Transform(Transform)
	{
	}

public:
	std::shared_ptr<D3D12RayTracingGeometry>	Geometry;
	XMFLOAT3X4									Transform;
};

/*
* Scene - Equal to the Top-Level AccelerationStructure
*/

class D3D12RayTracingScene : public D3D12DeviceChild
{
public:
	D3D12RayTracingScene(D3D12Device* Device);
	~D3D12RayTracingScene();

	bool Initialize(D3D12CommandList* CommandList, std::vector<D3D12RayTracingGeometryInstance>& Instances);

	D3D12_GPU_VIRTUAL_ADDRESS GetVirtualAddress()		const;

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle() const
	{
		return CPUHandle;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle() const
	{
		return GPUHandle;
	}

public:
	// DeviceChild Interface
	virtual void SetName(const std::string& InName) override;

private:
	D3D12Buffer* ResultBuffer	= nullptr;
	D3D12Buffer* ScratchBuffer	= nullptr;
	D3D12Buffer* InstanceBuffer	= nullptr;

	std::vector<D3D12RayTracingGeometryInstance> Instances;

	D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle;
};