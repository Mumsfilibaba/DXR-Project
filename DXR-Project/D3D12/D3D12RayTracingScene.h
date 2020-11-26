#pragma once
#include "D3D12DeviceChild.h"
#include "D3D12Buffer.h"
#include "D3D12Views.h"

#include "Defines.h"

class D3D12CommandList;
class D3D12DescriptorTable;
class Material;

/*
* D3D12RayTracingGeometry - Equal to the Bottom-Level AccelerationStructure
*/

class D3D12RayTracingGeometry : public D3D12DeviceChild
{
public:
	D3D12RayTracingGeometry(D3D12Device* InDevice);
	~D3D12RayTracingGeometry();

	bool BuildAccelerationStructure(D3D12CommandList* CommandList, TSharedPtr<D3D12Buffer>& InVertexBuffer, uint32 InVertexCount, TSharedPtr<D3D12Buffer>& IndexBuffer, uint32 InIndexCount);

	// DeviceChild Interface
	virtual void SetDebugName(const std::string& Name) override;

	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const;

	FORCEINLINE TSharedPtr<D3D12DescriptorTable> GetDescriptorTable() const
	{
		return DescriptorTable;
	}

private:
	TSharedPtr<D3D12Buffer> VertexBuffer;
	TSharedPtr<D3D12Buffer> IndexBuffer;
	TSharedPtr<D3D12DescriptorTable> DescriptorTable;

	D3D12Buffer* ResultBuffer	= nullptr;
	D3D12Buffer* ScratchBuffer	= nullptr;
	
	uint32 VertexCount	= 0;
	uint32 IndexCount	= 0;

	bool IsDirty = true;
};

/*
* D3D12RayTracingGeometryInstance
*/

class D3D12RayTracingGeometryInstance
{
public:
	D3D12RayTracingGeometryInstance(TSharedPtr<D3D12RayTracingGeometry>& InGeometry, Material* InMaterial, XMFLOAT3X4 InTransform, uint32 InHitGroupIndex, uint32 InInstanceID)
		: Geometry(InGeometry)
		, Material(InMaterial)
		, Transform(InTransform)
		, HitGroupIndex(InHitGroupIndex)
		, InstanceID(InInstanceID)
	{
	}

	~D3D12RayTracingGeometryInstance()
	{
	}

public:
	TSharedPtr<D3D12RayTracingGeometry> Geometry;
	Material* Material;

	XMFLOAT3X4	Transform;
	uint32		HitGroupIndex;
	uint32		InstanceID;
};

/*
* BindingTableEntry
*/

struct BindingTableEntry
{
public:
	BindingTableEntry()
		: ShaderExportName()
		, DescriptorTable0(nullptr)
		, DescriptorTable1(nullptr)
	{
	}

	BindingTableEntry(std::string InShaderExportName, TSharedPtr<D3D12DescriptorTable> InDescriptorTable0, TSharedPtr<D3D12DescriptorTable> InDescriptorTable1)
		: ShaderExportName(InShaderExportName)
		, DescriptorTable0(InDescriptorTable0)
		, DescriptorTable1(InDescriptorTable1)
	{
	}

	std::string ShaderExportName;

	TSharedPtr<D3D12DescriptorTable> DescriptorTable0;
	TSharedPtr<D3D12DescriptorTable> DescriptorTable1;
};

/*
* D3D12RayTracingScene - Equal to the Top-Level AccelerationStructure
*/

class D3D12RayTracingScene : public D3D12DeviceChild
{
public:
	D3D12RayTracingScene(D3D12Device* InDevice);
	~D3D12RayTracingScene();

	bool Initialize(class D3D12RayTracingPipelineState* PipelineState);

	bool BuildAccelerationStructure(D3D12CommandList* CommandList,
		TArray<D3D12RayTracingGeometryInstance>& InInstances,
		TArray<BindingTableEntry>& InBindingTableEntries,
		uint32 InNumHitGroups);

	// DeviceChild Interface
	virtual void SetDebugName(const std::string& Name) override;

	D3D12_GPU_VIRTUAL_ADDRESS_RANGE				GetRayGenerationShaderRecord()	const;
	D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE	GetMissShaderTable()			const;
	D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE	GetHitGroupTable()				const;
	
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const;

	FORCEINLINE D3D12ShaderResourceView* GetShaderResourceView() const
	{
		return View.Get();
	}

	FORCEINLINE bool NeedsBuild() const
	{
		return IsDirty;
	}

private:
	D3D12Buffer* ResultBuffer	= nullptr;
	D3D12Buffer* ScratchBuffer	= nullptr;
	D3D12Buffer* InstanceBuffer	= nullptr;

	D3D12Buffer*	BindingTable		= nullptr;
	uint32			BindingTableStride	= 0;
	uint32			NumHitGroups		= 0;

	TSharedPtr<D3D12ShaderResourceView>	View;
	
	TArray<D3D12RayTracingGeometryInstance>	Instances;
	TArray<BindingTableEntry>				BindingTableEntries;

	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> PipelineStateProperties;

	bool IsDirty = true;
};