#include "D3D12RayTracer.h"
#include "D3D12Device.h"
#include "D3D12CommandList.h"
#include "D3D12ShaderCompiler.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Texture.h"
#include "D3D12CommandAllocator.h"
#include "D3D12CommandQueue.h"
#include "D3D12AccelerationStructure.h"
#include "HeapProps.h"

#include "Meshes/MeshFactory.h"

#include <DirectXMath.h>

D3D12RayTracer::D3D12RayTracer(D3D12Device* Device)
	: D3D12DeviceChild(Device)
{
}

D3D12RayTracer::~D3D12RayTracer()
{
	SAFEDELETE(ResourceHeap);
	SAFEDELETE(ResultTexture);
	SAFEDELETE(VertexBuffer);
	SAFEDELETE(IndexBuffer);
	SAFEDELETE(TopLevelAS);
	SAFEDELETE(BottomLevelAS);
}

bool D3D12RayTracer::Init(D3D12CommandList* CommandList, D3D12CommandQueue* CommandQueue)
{
	using namespace Microsoft::WRL;

	// Get DXR Interfaces 
	ComPtr<ID3D12Device> MainDevice(Device->GetDevice());
	if (FAILED(MainDevice.As<ID3D12Device5>(&DXRDevice)))
	{
		::OutputDebugString("[D3D12RayTracer]: Failed to retrive DXR-Device\n");
		return false;
	}

	ComPtr<ID3D12CommandList> MainCommandList(CommandList->GetCommandList());
	if (FAILED(MainCommandList.As<ID3D12GraphicsCommandList4>(&DXRCommandList)))
	{
		::OutputDebugString("[D3D12RayTracer]: Failed to retrive DXR-CommandList\n");
		return false;
	}

	// Create DescriptorHeap
	ResourceHeap = new D3D12DescriptorHeap(Device);
	if (!ResourceHeap->Init(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 8, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE))
	{
		return false;
	}

	// Create image
	DXGI_FORMAT ImageFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	ResultTexture = new D3D12Texture(Device);
	if (!ResultTexture->Init(ImageFormat, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, 1920, 1080, HeapProps::DefaultHeap()))
	{
		return false;
	}

	D3D12_UNORDERED_ACCESS_VIEW_DESC UavView = { };
	UavView.Format					= ImageFormat;
	UavView.ViewDimension			= D3D12_UAV_DIMENSION_TEXTURE2D;
	UavView.Texture2D.MipSlice		= 0;
	UavView.Texture2D.PlaneSlice	= 0;

	OutputCPUHandle = ResourceHeap->GetCPUDescriptorHandleAt(0);
	DXRDevice->CreateUnorderedAccessView(ResultTexture->GetResource(), nullptr, &UavView, OutputCPUHandle);

	// Create mesh
	MeshData Mesh = MeshFactory::CreateSphere(3);

	// Create vertexbuffer
	Uint64 BufferSizeInBytes = sizeof(Vertex) * static_cast<Uint64>(Mesh.Vertices.size());
	VertexBuffer = new D3D12Buffer(Device);
	if (VertexBuffer->Init(D3D12_RESOURCE_FLAG_NONE, BufferSizeInBytes, D3D12_RESOURCE_STATE_GENERIC_READ, HeapProps::UploadHeap()))
	{
		void* BufferMemory = VertexBuffer->Map();
		memcpy(BufferMemory, Mesh.Vertices.data(), BufferSizeInBytes);
		VertexBuffer->Unmap();
	}
	else
	{
		return false;
	}

	// Create indexbuffer
	BufferSizeInBytes = sizeof(Uint32) * static_cast<Uint64>(Mesh.Indices.size());
	IndexBuffer = new D3D12Buffer(Device);
	if (IndexBuffer->Init(D3D12_RESOURCE_FLAG_NONE, BufferSizeInBytes, D3D12_RESOURCE_STATE_GENERIC_READ, HeapProps::UploadHeap()))
	{
		void* BufferMemory = IndexBuffer->Map();
		memcpy(BufferMemory, Mesh.Indices.data(), BufferSizeInBytes);
		IndexBuffer->Unmap();
	}
	else
	{
		return false;
	}

	// Build Acceleration Structures
	D3D12CommandAllocator* CommandAllocator = new D3D12CommandAllocator(Device);
	CommandAllocator->Init(D3D12_COMMAND_LIST_TYPE_DIRECT);
	CommandAllocator->Reset();

	DXRCommandList->Reset(CommandAllocator->GetAllocator(), nullptr);

	// Bottom Level
	{
		D3D12_RAYTRACING_GEOMETRY_DESC GeometryDesc = {};
		GeometryDesc.Type									= D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		GeometryDesc.Triangles.VertexBuffer.StartAddress	= VertexBuffer->GetVirtualAddress();
		GeometryDesc.Triangles.VertexBuffer.StrideInBytes	= sizeof(Vertex);
		GeometryDesc.Triangles.VertexFormat					= DXGI_FORMAT_R32G32B32_FLOAT;
		GeometryDesc.Triangles.VertexCount					= static_cast<Uint32>(Mesh.Vertices.size());
		GeometryDesc.Triangles.IndexFormat					= DXGI_FORMAT_R32_UINT;
		GeometryDesc.Triangles.IndexBuffer					= IndexBuffer->GetVirtualAddress();
		GeometryDesc.Triangles.IndexCount					= static_cast<Uint32>(Mesh.Indices.size());
		GeometryDesc.Flags									= D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

		// Get the size requirements for the scratch and AS buffers
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Inputs = {};
		Inputs.DescsLayout		= D3D12_ELEMENTS_LAYOUT_ARRAY;
		Inputs.Flags			= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
		Inputs.NumDescs			= 1;
		Inputs.pGeometryDescs	= &GeometryDesc;
		Inputs.Type				= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO Info = { };
		DXRDevice->GetRaytracingAccelerationStructurePrebuildInfo(&Inputs, &Info);

		// Create the buffers. They need to support UAV, and since we are going to immediately use them, we create them with an unordered-access state
		BottomLevelAS = new D3D12AccelerationStructure(Device);
		BottomLevelAS->ScratchBuffer = new D3D12Buffer(Device);
		BottomLevelAS->ScratchBuffer->Init(D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, Info.ScratchDataSizeInBytes, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, HeapProps::DefaultHeap());

		BottomLevelAS->ResultBuffer = new D3D12Buffer(Device);
		BottomLevelAS->ResultBuffer->Init(D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, Info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, HeapProps::DefaultHeap());

		// Create the bottom-level AS
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC AccelerationStructureDesc = {};
		AccelerationStructureDesc.Inputs							= Inputs;
		AccelerationStructureDesc.DestAccelerationStructureData		= BottomLevelAS->ResultBuffer->GetVirtualAddress();
		AccelerationStructureDesc.ScratchAccelerationStructureData	= BottomLevelAS->ScratchBuffer->GetVirtualAddress();

		DXRCommandList->BuildRaytracingAccelerationStructure(&AccelerationStructureDesc, 0, nullptr);

		// We need to insert a UAV barrier before using the acceleration structures in a raytracing operation
		D3D12_RESOURCE_BARRIER UavBarrier = {};
		UavBarrier.Type				= D3D12_RESOURCE_BARRIER_TYPE_UAV;
		UavBarrier.UAV.pResource	= BottomLevelAS->ResultBuffer->GetResource();
		DXRCommandList->ResourceBarrier(1, &UavBarrier);
	}

	// Top Level
	{
		Int32 InstanceCount = 3;

		// First get the size of the TLAS buffers and create them
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Inputs = {};
		Inputs.DescsLayout	= D3D12_ELEMENTS_LAYOUT_ARRAY;
		Inputs.Flags		= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
		Inputs.NumDescs		= InstanceCount;
		Inputs.Type			= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO Info;
		DXRDevice->GetRaytracingAccelerationStructurePrebuildInfo(&Inputs, &Info);

		// Create the buffers
		TopLevelAS = new D3D12AccelerationStructure(Device);
		TopLevelAS->ScratchBuffer = new D3D12Buffer(Device);
		TopLevelAS->ScratchBuffer->Init(D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, Info.ScratchDataSizeInBytes, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, HeapProps::DefaultHeap());

		TopLevelAS->ResultBuffer = new D3D12Buffer(Device); 
		TopLevelAS->ResultBuffer->Init(D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, Info.ScratchDataSizeInBytes, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, HeapProps::DefaultHeap());

		TopLevelAS->InstanceBuffer = new D3D12Buffer(Device);
		TopLevelAS->InstanceBuffer->Init(D3D12_RESOURCE_FLAG_NONE, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * InstanceCount, D3D12_RESOURCE_STATE_GENERIC_READ, HeapProps::UploadHeap());

		D3D12_RAYTRACING_INSTANCE_DESC* InstanceDesc = reinterpret_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(TopLevelAS->InstanceBuffer->Map());

		//static float rotY = 0;
		//rotY += 0.001f;
		for (Int32 i = 0; i < InstanceCount; i++)
		{
			InstanceDesc->InstanceID							= i;	// Exposed to the shader via InstanceID()
			InstanceDesc->InstanceContributionToHitGroupIndex	= 0;	// Offset inside the shader-table. we only have a single geometry, so the offset 0
			InstanceDesc->Flags									= D3D12_RAYTRACING_INSTANCE_FLAG_NONE;


			// Apply transform
			XMFLOAT3X4 m
			(
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f
			);
			XMStoreFloat3x4(&m, XMMatrixTranslation(-2.0f + (i * 2.0f), 0, 0));
			memcpy(InstanceDesc->Transform, &m, sizeof(InstanceDesc->Transform));

			InstanceDesc->AccelerationStructure		= BottomLevelAS->ResultBuffer->GetVirtualAddress();
			InstanceDesc->InstanceMask				= 0xFF;

			InstanceDesc++;
		}

		// Unmap
		TopLevelAS->InstanceBuffer->Unmap();

		// Create the TLAS
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC AccelerationStructureDesc = {};
		AccelerationStructureDesc.Inputs							= Inputs;
		AccelerationStructureDesc.Inputs.InstanceDescs				= TopLevelAS->InstanceBuffer->GetVirtualAddress();
		AccelerationStructureDesc.DestAccelerationStructureData		= TopLevelAS->ResultBuffer->GetVirtualAddress();
		AccelerationStructureDesc.ScratchAccelerationStructureData	= TopLevelAS->ScratchBuffer->GetVirtualAddress();

		DXRCommandList->BuildRaytracingAccelerationStructure(&AccelerationStructureDesc, 0, nullptr);

		// UAV barrier needed before using the acceleration structures in a raytracing operation
		D3D12_RESOURCE_BARRIER UavBarrier = {};
		UavBarrier.Type				= D3D12_RESOURCE_BARRIER_TYPE_UAV;
		UavBarrier.UAV.pResource	= TopLevelAS->ResultBuffer->GetResource();
		DXRCommandList->ResourceBarrier(1, &UavBarrier);
	}

	DXRCommandList->Close();

	ID3D12CommandList* CommandLists[] = { DXRCommandList.Get() };
	CommandQueue->GetQueue()->ExecuteCommandLists(1, CommandLists);

	//delete CommandAllocator;

	D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = { };
	SrvDesc.ViewDimension								= D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
	SrvDesc.Shader4ComponentMapping						= D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SrvDesc.RaytracingAccelerationStructure.Location	= TopLevelAS->ResultBuffer->GetVirtualAddress();

	AccelerationStructureCPUHandle = ResourceHeap->GetCPUDescriptorHandleAt(1);
	DXRDevice->CreateShaderResourceView(nullptr, &SrvDesc, AccelerationStructureCPUHandle);

	// Vector for all subobjects
	std::vector<D3D12_STATE_SUBOBJECT> SubObjects;
	SubObjects.reserve(32);

	// Init DXIL subobject
	ComPtr<IDxcBlob> RayTracingShaders = D3D12ShaderCompiler::Get()->CompileFromFile("Shaders/RayTracingShaders.hlsl", "", "lib_6_3");
	D3D12_EXPORT_DESC DxilExports[] = 
	{
		L"RayGen",		nullptr, D3D12_EXPORT_FLAG_NONE,
		L"Miss",		nullptr, D3D12_EXPORT_FLAG_NONE,
		L"ClosestHit",	nullptr, D3D12_EXPORT_FLAG_NONE,
	};

	D3D12_DXIL_LIBRARY_DESC DxilLibraryDesc = { };
	DxilLibraryDesc.DXILLibrary.pShaderBytecode = RayTracingShaders->GetBufferPointer();
	DxilLibraryDesc.DXILLibrary.BytecodeLength	= RayTracingShaders->GetBufferSize();
	DxilLibraryDesc.pExports					= DxilExports;
	DxilLibraryDesc.NumExports					= _countof(DxilExports);

	{
		D3D12_STATE_SUBOBJECT LibrarySubObject = { };
		LibrarySubObject.Type	= D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
		LibrarySubObject.pDesc	= &DxilLibraryDesc;
		SubObjects.push_back(LibrarySubObject);
	}

	// Init hit group
	D3D12_HIT_GROUP_DESC HitGroupDesc;
	HitGroupDesc.Type						= D3D12_HIT_GROUP_TYPE_TRIANGLES;
	HitGroupDesc.AnyHitShaderImport			= nullptr;
	HitGroupDesc.ClosestHitShaderImport		= L"ClosestHit";
	HitGroupDesc.HitGroupExport				= L"HitGroup";
	HitGroupDesc.IntersectionShaderImport	= nullptr;

	{
		D3D12_STATE_SUBOBJECT HitGroupSubObject = { };
		HitGroupSubObject.Type	= D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
		HitGroupSubObject.pDesc = &HitGroupDesc;
		SubObjects.push_back(HitGroupSubObject);
	}

	// Init RayGen local root signature
	ComPtr<ID3D12RootSignature> RayGenLocalRoot;

	D3D12_DESCRIPTOR_RANGE Ranges[2] = {};
	
	// Output
	Ranges[0].BaseShaderRegister				= 0;
	Ranges[0].NumDescriptors					= 1;
	Ranges[0].RegisterSpace						= 0;
	Ranges[0].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	Ranges[0].OffsetInDescriptorsFromTableStart = 0;

	// AccelerationStructure
	Ranges[1].BaseShaderRegister				= 0;
	Ranges[1].NumDescriptors					= 1;
	Ranges[1].RegisterSpace						= 0;
	Ranges[1].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	Ranges[1].OffsetInDescriptorsFromTableStart = 1;

	D3D12_ROOT_PARAMETER RootParams[1] = { };
	RootParams[0].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	RootParams[0].DescriptorTable.NumDescriptorRanges	= _countof(Ranges);
	RootParams[0].DescriptorTable.pDescriptorRanges		= Ranges;

	D3D12_ROOT_SIGNATURE_DESC RayGenLocalRootDesc = {};
	RayGenLocalRootDesc.NumParameters	= _countof(RootParams);
	RayGenLocalRootDesc.pParameters		= RootParams;
	RayGenLocalRootDesc.Flags			= D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	ComPtr<ID3DBlob> SignatureBlob;
	ComPtr<ID3DBlob> ErrorBlob;
	HRESULT hResult = D3D12SerializeRootSignature(&RayGenLocalRootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &SignatureBlob, &ErrorBlob);
	if (FAILED(hResult))
	{
		::OutputDebugString("[D3D12RayTracer]: D3D12SerializeRootSignature failed with following error:\n");
		::OutputDebugString(reinterpret_cast<LPCSTR>(ErrorBlob->GetBufferPointer()));
		return false;
	}

	hResult = DXRDevice->CreateRootSignature(0, SignatureBlob->GetBufferPointer(), SignatureBlob->GetBufferSize(), IID_PPV_ARGS(&RayGenLocalRoot));
	if (FAILED(hResult))
	{
		::OutputDebugString("[D3D12RayTracer]: FAILED to create local RootSignature\n");
		return false;
	}

	RayGenLocalRoot->SetName(L"RayGen Local RootSignature");

	{
		D3D12_STATE_SUBOBJECT RayGenLocalRootSubObject = { };
		RayGenLocalRootSubObject.Type	= D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
		RayGenLocalRootSubObject.pDesc	= RayGenLocalRoot.GetAddressOf();
		SubObjects.push_back(RayGenLocalRootSubObject);
	}

	// Bind local root signature to rayGen shader
	LPCWSTR RayGenLocalRootAssociationShaderNames[] = { L"RayGen" };

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION RayGenLocalRootAssociation;
	RayGenLocalRootAssociation.pExports					= RayGenLocalRootAssociationShaderNames;
	RayGenLocalRootAssociation.NumExports				= _countof(RayGenLocalRootAssociationShaderNames);
	RayGenLocalRootAssociation.pSubobjectToAssociate	= &SubObjects.back();

	{
		D3D12_STATE_SUBOBJECT RayGenLocalRootAssociationSubObject = { };
		RayGenLocalRootAssociationSubObject.Type	= D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
		RayGenLocalRootAssociationSubObject.pDesc	= &RayGenLocalRootAssociation;
		SubObjects.push_back(RayGenLocalRootAssociationSubObject);
	}

	// Init hit group local root signature
	ComPtr<ID3D12RootSignature> HitGroupLocalRoot;
	
	D3D12_ROOT_SIGNATURE_DESC HitGroupRootDesc = {};
	HitGroupRootDesc.NumParameters	= 0;
	HitGroupRootDesc.pParameters	= nullptr;
	HitGroupRootDesc.Flags			= D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	hResult = D3D12SerializeRootSignature(&HitGroupRootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &SignatureBlob, &ErrorBlob);
	if (FAILED(hResult))
	{
		::OutputDebugString("[D3D12RayTracer]: D3D12SerializeRootSignature failed with following error:\n");
		::OutputDebugString(reinterpret_cast<LPCSTR>(ErrorBlob->GetBufferPointer()));
		return false;
	}

	hResult = DXRDevice->CreateRootSignature(0, SignatureBlob->GetBufferPointer(), SignatureBlob->GetBufferSize(), IID_PPV_ARGS(&HitGroupLocalRoot));
	if (FAILED(hResult))
	{
		::OutputDebugString("[D3D12RayTracer]: FAILED to create local RootSignature\n");
		return false;
	}

	HitGroupLocalRoot->SetName(L"HitGroup Local RootSignature");

	{
		D3D12_STATE_SUBOBJECT HitGroupLocalRootSubObject = { };
		HitGroupLocalRootSubObject.Type		= D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
		HitGroupLocalRootSubObject.pDesc	= HitGroupLocalRoot.GetAddressOf();
		SubObjects.push_back(HitGroupLocalRootSubObject);
	}

	// Bind local root signature to hit group shaders
	LPCWSTR HitGroupLocalRootAssociationShaderNames[] = { L"ClosestHit" };
	
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION HitGroupLocalRootAssociation;
	HitGroupLocalRootAssociation.pExports				= HitGroupLocalRootAssociationShaderNames;
	HitGroupLocalRootAssociation.NumExports				= _countof(HitGroupLocalRootAssociationShaderNames);
	HitGroupLocalRootAssociation.pSubobjectToAssociate	= &SubObjects.back();

	{
		D3D12_STATE_SUBOBJECT HitGroupLocalRootAssociationSubObject = { };
		HitGroupLocalRootAssociationSubObject.Type	= D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
		HitGroupLocalRootAssociationSubObject.pDesc = &HitGroupLocalRootAssociation;
		SubObjects.push_back(HitGroupLocalRootAssociationSubObject);
	}

	// Init miss local root signature
	ComPtr<ID3D12RootSignature> MissLocalRoot;

	D3D12_ROOT_SIGNATURE_DESC MissLocalRootDesc = {};
	MissLocalRootDesc.NumParameters	= 0;
	MissLocalRootDesc.pParameters	= nullptr;
	MissLocalRootDesc.Flags			= D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

	hResult = D3D12SerializeRootSignature(&MissLocalRootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &SignatureBlob, &ErrorBlob);
	if (FAILED(hResult))
	{
		::OutputDebugString("[D3D12RayTracer]: D3D12SerializeRootSignature failed with following error:\n");
		::OutputDebugString(reinterpret_cast<LPCSTR>(ErrorBlob->GetBufferPointer()));
		return false;
	}

	hResult = DXRDevice->CreateRootSignature(0, SignatureBlob->GetBufferPointer(), SignatureBlob->GetBufferSize(), IID_PPV_ARGS(&MissLocalRoot));
	if (FAILED(hResult))
	{
		::OutputDebugString("[D3D12RayTracer]: FAILED to create local RootSignature\n");
		return false;
	}

	MissLocalRoot->SetName(L"Miss Local RootSignature");

	{
		D3D12_STATE_SUBOBJECT MissLocalRootSubObject = { };
		MissLocalRootSubObject.Type		= D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
		MissLocalRootSubObject.pDesc	= MissLocalRoot.GetAddressOf();
		SubObjects.push_back(MissLocalRootSubObject);
	}

	// Bind local root signature to miss shader
	LPCWSTR missLocalRootAssociationShaderNames[] = { L"Miss" };
	
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION MissLocalRootAssociation;
	MissLocalRootAssociation.pExports				= missLocalRootAssociationShaderNames;
	MissLocalRootAssociation.NumExports				= _countof(missLocalRootAssociationShaderNames);
	MissLocalRootAssociation.pSubobjectToAssociate	= &SubObjects.back();

	{
		D3D12_STATE_SUBOBJECT MissLocalRootAssociationSubObject = { };
		MissLocalRootAssociationSubObject.Type	= D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
		MissLocalRootAssociationSubObject.pDesc = &MissLocalRootAssociation;
		SubObjects.push_back(MissLocalRootAssociationSubObject);
	}

	// Init shader config
	D3D12_RAYTRACING_SHADER_CONFIG ShaderConfig = {};
	ShaderConfig.MaxAttributeSizeInBytes	= sizeof(Float32) * 2;
	ShaderConfig.MaxPayloadSizeInBytes		= sizeof(Float32) * 3;

	{
		D3D12_STATE_SUBOBJECT ShaderConfigSubObject = { };
		ShaderConfigSubObject.Type	= D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
		ShaderConfigSubObject.pDesc = &ShaderConfig;
		SubObjects.push_back(ShaderConfigSubObject);
	}

	// Bind the payload size to the programs
	const WCHAR* ShaderNamesToConfig[] = { L"Miss", L"ClosestHit", L"RayGen" };

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION ShaderConfigAssociation;
	ShaderConfigAssociation.pExports				= ShaderNamesToConfig;
	ShaderConfigAssociation.NumExports				= _countof(ShaderNamesToConfig);
	ShaderConfigAssociation.pSubobjectToAssociate	= &SubObjects.back();

	{
		D3D12_STATE_SUBOBJECT ShaderConfigAssociationSubObject = { };
		ShaderConfigAssociationSubObject.Type		= D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
		ShaderConfigAssociationSubObject.pDesc		= &ShaderConfigAssociation;
		SubObjects.push_back(ShaderConfigAssociationSubObject);
	}

	// Init pipeline config
	D3D12_RAYTRACING_PIPELINE_CONFIG PipelineConfig;
	PipelineConfig.MaxTraceRecursionDepth = 1;

	{
		D3D12_STATE_SUBOBJECT PipelineConfigSubObject = { };
		PipelineConfigSubObject.Type	= D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
		PipelineConfigSubObject.pDesc	= &PipelineConfig;
		SubObjects.push_back(PipelineConfigSubObject);
	}

	// Init global root signature
	D3D12_ROOT_SIGNATURE_DESC GlobalRootDesc = {};
	GlobalRootDesc.NumParameters	= 0;
	GlobalRootDesc.pParameters		= nullptr;
	GlobalRootDesc.Flags			= D3D12_ROOT_SIGNATURE_FLAG_NONE;

	hResult = D3D12SerializeRootSignature(&GlobalRootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &SignatureBlob, &ErrorBlob);
	if (FAILED(hResult))
	{
		::OutputDebugString("[D3D12RayTracer]: D3D12SerializeRootSignature failed with following error:\n");
		::OutputDebugString(reinterpret_cast<LPCSTR>(ErrorBlob->GetBufferPointer()));
		return false;
	}

	hResult = DXRDevice->CreateRootSignature(0, SignatureBlob->GetBufferPointer(), SignatureBlob->GetBufferSize(), IID_PPV_ARGS(&GlobalRootSignature));
	if (FAILED(hResult))
	{
		::OutputDebugString("[D3D12RayTracer]: FAILED to create global RootSignature\n");
		return false;
	}

	GlobalRootSignature->SetName(L"Global RootSignature");

	{
		D3D12_STATE_SUBOBJECT GlobalRootSubObject = { };
		GlobalRootSubObject.Type	= D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
		GlobalRootSubObject.pDesc	= GlobalRootSignature.GetAddressOf();
		SubObjects.push_back(GlobalRootSubObject);
	}

	// Create state object
	D3D12_STATE_OBJECT_DESC RayTracingPipeline = { };
	RayTracingPipeline.Type				= D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	RayTracingPipeline.NumSubobjects	= static_cast<Uint32>(SubObjects.size());
	RayTracingPipeline.pSubobjects		= SubObjects.data();
	
	hResult = DXRDevice->CreateStateObject(&RayTracingPipeline, IID_PPV_ARGS(&DXRStateObject));
	if (SUCCEEDED(hResult))
	{
		::OutputDebugString("[D3D12RayTracer]: Created RayTracing PipelineState\n");
	}
	else
	{
		::OutputDebugString("[D3D12RayTracer]: FAILED to create RayTracing PipelineState\n");
		return false;
	}

	// Create Shader Binding Table
	hResult = DXRStateObject.As<ID3D12StateObjectProperties>(&DXRStateProperties);
	if (SUCCEEDED(hResult))
	{
		::OutputDebugString("[D3D12RayTracer]: Retrived PipelineState Properties\n");
	}
	else
	{
		::OutputDebugString("[D3D12RayTracer]: FAILED to retrive PipelineState Properties\n");
		return false;
	}
	
	// RayGen
	{
		struct alignas(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) RAY_GEN_SHADER_TABLE_DATA
		{
			Uint8 ShaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
			UINT64 RTVDescriptor;
		} TableData;

		memcpy(TableData.ShaderIdentifier, DXRStateProperties->GetShaderIdentifier(L"RayGen"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		TableData.RTVDescriptor = ResourceHeap->GetCPUDescriptorHandleForHeapStart().ptr;

		// How big is the biggest?
		union MaxSize
		{
			RAY_GEN_SHADER_TABLE_DATA data0;
		};

		RayGenTable.StrideInBytes	= sizeof(MaxSize);
		RayGenTable.SizeInBytes		= RayGenTable.StrideInBytes * 1; //<-- only one for now...
		RayGenTable.Resource		= new D3D12Buffer(Device); 
		RayGenTable.Resource->Init(D3D12_RESOURCE_FLAG_NONE, RayGenTable.SizeInBytes, D3D12_RESOURCE_STATE_GENERIC_READ, HeapProps::UploadHeap());

		// Map the buffer
		void* Data = RayGenTable.Resource->Map();
		memcpy(Data, &TableData, sizeof(TableData));
		RayGenTable.Resource->Unmap();
	}

	// Miss
	{
		struct alignas(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) MISS_SHADER_TABLE_DATA
		{
			Uint8 ShaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
		} TableData;

		memcpy(TableData.ShaderIdentifier, DXRStateProperties->GetShaderIdentifier(L"Miss"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

		// How big is the biggest?
		union MaxSize
		{
			MISS_SHADER_TABLE_DATA data0;
		};

		MissTable.StrideInBytes = sizeof(MaxSize);
		MissTable.SizeInBytes	= MissTable.StrideInBytes * 1; //<-- only one for now...
		MissTable.Resource		= new D3D12Buffer(Device); 
		MissTable.Resource->Init(D3D12_RESOURCE_FLAG_NONE, MissTable.SizeInBytes, D3D12_RESOURCE_STATE_GENERIC_READ, HeapProps::UploadHeap());

		// Map the buffer
		void* Data = MissTable.Resource->Map();
		memcpy(Data, &TableData, sizeof(TableData));
		MissTable.Resource->Unmap();
	}

	// Hit
	{
		struct alignas(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT) HIT_GROUP_SHADER_TABLE_DATA
		{
			Uint8 ShaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
		} TableData;

		memcpy(TableData.ShaderIdentifier, DXRStateProperties->GetShaderIdentifier(L"HitGroup"), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

		// How big is the biggest?
		union MaxSize
		{
			HIT_GROUP_SHADER_TABLE_DATA data0;
		};

		HitTable.StrideInBytes	= sizeof(MaxSize);
		HitTable.SizeInBytes	= HitTable.StrideInBytes * 1; //<-- only one for now...
		HitTable.Resource		= new D3D12Buffer(Device);
		HitTable.Resource->Init(D3D12_RESOURCE_FLAG_NONE, HitTable.SizeInBytes, D3D12_RESOURCE_STATE_GENERIC_READ, HeapProps::UploadHeap());

		// Map the buffer
		void* Data = HitTable.Resource->Map();
		memcpy(Data, &TableData, sizeof(TableData));
		HitTable.Resource->Unmap();
	}

	return true;
}

static void TransitionResourceState(ID3D12GraphicsCommandList4* CommandList, ID3D12Resource* Resource, D3D12_RESOURCE_STATES BeforeState, D3D12_RESOURCE_STATES AfterState)
{
	D3D12_RESOURCE_BARRIER Barrier = { };
	Barrier.Type					= D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	Barrier.Flags					= D3D12_RESOURCE_BARRIER_FLAG_NONE;
	Barrier.Transition.pResource	= Resource;
	Barrier.Transition.Subresource	= D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	Barrier.Transition.StateBefore	= BeforeState;
	Barrier.Transition.StateAfter	= AfterState;

	CommandList->ResourceBarrier(1, &Barrier);
}


void D3D12RayTracer::Render(ID3D12Resource* BackBuffer)
{
	//Set constant buffer descriptor heap
	ID3D12DescriptorHeap* descriptorHeaps[] = { ResourceHeap->GetHeap() };
	DXRCommandList->SetDescriptorHeaps(ARRAYSIZE(descriptorHeaps), descriptorHeaps);

	// Let's raytrace
	TransitionResourceState(DXRCommandList.Get(), ResultTexture->GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	
	D3D12_DISPATCH_RAYS_DESC raytraceDesc = {};
	raytraceDesc.Width	= 1920;
	raytraceDesc.Height = 1080;
	raytraceDesc.Depth	= 1;

	// Set shader tables
	raytraceDesc.RayGenerationShaderRecord.StartAddress = RayGenTable.Resource->GetVirtualAddress();
	raytraceDesc.RayGenerationShaderRecord.SizeInBytes	= RayGenTable.SizeInBytes;

	raytraceDesc.MissShaderTable.StartAddress	= MissTable.Resource->GetVirtualAddress();
	raytraceDesc.MissShaderTable.StrideInBytes	= MissTable.StrideInBytes;
	raytraceDesc.MissShaderTable.SizeInBytes	= MissTable.SizeInBytes;

	raytraceDesc.HitGroupTable.StartAddress		= HitTable.Resource->GetVirtualAddress();
	raytraceDesc.HitGroupTable.StrideInBytes	= HitTable.StrideInBytes;
	raytraceDesc.HitGroupTable.SizeInBytes		= HitTable.SizeInBytes;

	// Bind the empty root signature
	DXRCommandList->SetComputeRootSignature(GlobalRootSignature.Get());

	// Dispatch
	DXRCommandList->SetPipelineState1(DXRStateObject.Get());
	DXRCommandList->DispatchRays(&raytraceDesc);

	// Copy the results to the back-buffer
	TransitionResourceState(DXRCommandList.Get(), ResultTexture->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE);
	TransitionResourceState(DXRCommandList.Get(), BackBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
	
	DXRCommandList->CopyResource(BackBuffer, ResultTexture->GetResource());

	// Indicate that the back buffer will now be used to present.
	TransitionResourceState(DXRCommandList.Get(), BackBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
}
