#pragma once
#include "D3D12DeviceChild.h"
#include "D3D12Buffer.h"
#include "D3D12Views.h"

#include "Defines.h"

#include <memory>

class D3D12CommandList;

/*
* D3D12RayTracingGeometry - Equal to the Bottom-Level AccelerationStructure
*/

class D3D12RayTracingGeometry : public D3D12DeviceChild
{
public:
	D3D12RayTracingGeometry(D3D12Device* InDevice);
	~D3D12RayTracingGeometry();

	bool Initialize(D3D12CommandList* CommandList, std::shared_ptr<D3D12Buffer>& InVertexBuffer, Uint32 InVertexCount, std::shared_ptr<D3D12Buffer>& IndexBuffer, Uint32 InIndexCount);

	D3D12_GPU_VIRTUAL_ADDRESS GetVirtualAddress() const;

public:
	// DeviceChild Interface
	virtual void SetName(const std::string& Name) override;

private:
	std::shared_ptr<D3D12Buffer> VertexBuffer	= nullptr;
	std::shared_ptr<D3D12Buffer> IndexBuffer	= nullptr;
	
	D3D12Buffer* ResultBuffer	= nullptr;
	D3D12Buffer* ScratchBuffer	= nullptr;
	
	Uint32 VertexCount	= 0;
	Uint32 IndexCount	= 0;
};

/*
* D3D12RayTracingGeometryInstance
*/

class D3D12RayTracingGeometryInstance
{
public:
	D3D12RayTracingGeometryInstance(std::shared_ptr<D3D12RayTracingGeometry>& InGeometry, XMFLOAT3X4 InTransform)
		: Geometry(InGeometry)
		, Transform(InTransform)
	{
	}

	~D3D12RayTracingGeometryInstance()
	{
	}

public:
	std::shared_ptr<D3D12RayTracingGeometry>	Geometry;
	XMFLOAT3X4									Transform;
};

/*
* D3D12RayTracingScene - Equal to the Top-Level AccelerationStructure
*/

class D3D12RayTracingScene : public D3D12DeviceChild
{
public:
	D3D12RayTracingScene(D3D12Device* InDevice);
	~D3D12RayTracingScene();

	bool Initialize(D3D12CommandList* CommandList, std::vector<D3D12RayTracingGeometryInstance>& InInstances);

	D3D12_GPU_VIRTUAL_ADDRESS GetVirtualAddress() const;

public:
	// DeviceChild Interface
	virtual void SetName(const std::string& Name) override;

private:
	D3D12Buffer*	ResultBuffer	= nullptr;
	D3D12Buffer*	ScratchBuffer	= nullptr;
	D3D12Buffer*	InstanceBuffer	= nullptr;

	std::shared_ptr<D3D12ShaderResourceView>		View;
	std::vector<D3D12RayTracingGeometryInstance>	Instances;
};