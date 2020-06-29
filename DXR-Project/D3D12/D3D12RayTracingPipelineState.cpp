#include "D3D12RayTracingPipelineState.h"
#include "D3D12ShaderCompiler.h"
#include "D3D12Device.h"
#include "D3D12DescriptorHeap.h"

const LPCWSTR RayGenShaderName		= L"RayGen";
const LPCWSTR MissShaderName		= L"Miss";
const LPCWSTR ClosestHitShaderName	= L"ClosestHit";
const LPCWSTR HitGroupName			= L"HitGroup";

D3D12RayTracingPipelineState::D3D12RayTracingPipelineState(D3D12Device* InDevice)
	: D3D12DeviceChild(InDevice)
	, GlobalRootSignature(nullptr)
	, DXRStateObject(nullptr)
	, RayGenTable()
	, MissTable()
	, HitTable()
{
}

D3D12RayTracingPipelineState::~D3D12RayTracingPipelineState()
{
}

bool D3D12RayTracingPipelineState::Initialize()
{
	if (!CreatePipeline())
	{
		return false;
	}

	if (!CreateBindingTable())
	{
		return false;
	}

	return true;
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE D3D12RayTracingPipelineState::GetRayGenerationShaderRecord() const
{
	return { RayGenTable.Resource->GetGPUVirtualAddress(), RayGenTable.SizeInBytes };
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE D3D12RayTracingPipelineState::GetMissShaderTable() const
{
	return { MissTable.Resource->GetGPUVirtualAddress(), MissTable.SizeInBytes, MissTable.StrideInBytes };
}

D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE D3D12RayTracingPipelineState::GetHitGroupTable() const
{
	return { HitTable.Resource->GetGPUVirtualAddress(), HitTable.SizeInBytes, HitTable.StrideInBytes };
}

void D3D12RayTracingPipelineState::SetName(const std::string& InName)
{
	DXRStateObject->SetName(ConvertToWide(InName).c_str());
}

bool D3D12RayTracingPipelineState::CreatePipeline()
{
	using namespace Microsoft::WRL;

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
	HitGroupDesc.ClosestHitShaderImport		= ClosestHitShaderName;
	HitGroupDesc.HitGroupExport				= HitGroupName;
	HitGroupDesc.IntersectionShaderImport	= nullptr;

	{
		D3D12_STATE_SUBOBJECT HitGroupSubObject = { };
		HitGroupSubObject.Type	= D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
		HitGroupSubObject.pDesc	= &HitGroupDesc;
		SubObjects.push_back(HitGroupSubObject);
	}

	// Init RayGen local root signature
	ComPtr<ID3D12RootSignature> RayGenLocalRoot;
	{
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

		D3D12_ROOT_PARAMETER RootParams = { };
		RootParams.ParameterType						= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		RootParams.DescriptorTable.NumDescriptorRanges	= _countof(Ranges);
		RootParams.DescriptorTable.pDescriptorRanges	= Ranges;

		D3D12_ROOT_SIGNATURE_DESC RayGenLocalRootDesc = {};
		RayGenLocalRootDesc.NumParameters	= 1;
		RayGenLocalRootDesc.pParameters		= &RootParams;
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

		hResult = Device->GetDevice()->CreateRootSignature(0, SignatureBlob->GetBufferPointer(), SignatureBlob->GetBufferSize(), IID_PPV_ARGS(&RayGenLocalRoot));
		if (FAILED(hResult))
		{
			::OutputDebugString("[D3D12RayTracer]: FAILED to create local RootSignature\n");
			return false;
		}
	}

	RayGenLocalRoot->SetName(L"RayGen Local RootSignature");

	{
		D3D12_STATE_SUBOBJECT RayGenLocalRootSubObject = { };
		RayGenLocalRootSubObject.Type	= D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
		RayGenLocalRootSubObject.pDesc	= RayGenLocalRoot.GetAddressOf();
		SubObjects.push_back(RayGenLocalRootSubObject);
	}

	// Bind local root signature to rayGen shader
	LPCWSTR RayGenLocalRootAssociationShaderNames[] = { RayGenShaderName };

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
	{
		D3D12_DESCRIPTOR_RANGE Ranges[3] = {};

		// AccelerationStructure
		Ranges[0].BaseShaderRegister				= 0;
		Ranges[0].NumDescriptors					= 1;
		Ranges[0].RegisterSpace						= 0;
		Ranges[0].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		Ranges[0].OffsetInDescriptorsFromTableStart = 0;

		// VertexBuffer
		Ranges[1].BaseShaderRegister				= 1;
		Ranges[1].NumDescriptors					= 1;
		Ranges[1].RegisterSpace						= 0;
		Ranges[1].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		Ranges[1].OffsetInDescriptorsFromTableStart = 1;

		// IndexBuffer
		Ranges[2].BaseShaderRegister				= 2;
		Ranges[2].NumDescriptors					= 1;
		Ranges[2].RegisterSpace						= 0;
		Ranges[2].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		Ranges[2].OffsetInDescriptorsFromTableStart = 2;

		D3D12_ROOT_PARAMETER RootParams = { };
		RootParams.ParameterType						= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		RootParams.DescriptorTable.NumDescriptorRanges	= _countof(Ranges);
		RootParams.DescriptorTable.pDescriptorRanges	= Ranges;

		D3D12_ROOT_SIGNATURE_DESC HitGroupRootDesc = {};
		HitGroupRootDesc.NumParameters	= 1;
		HitGroupRootDesc.pParameters	= &RootParams;
		HitGroupRootDesc.Flags			= D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

		ComPtr<ID3DBlob> SignatureBlob;
		ComPtr<ID3DBlob> ErrorBlob;
		HRESULT hResult = D3D12SerializeRootSignature(&HitGroupRootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &SignatureBlob, &ErrorBlob);
		if (FAILED(hResult))
		{
			::OutputDebugString("[D3D12RayTracer]: D3D12SerializeRootSignature failed with following error:\n");
			::OutputDebugString(reinterpret_cast<LPCSTR>(ErrorBlob->GetBufferPointer()));
			return false;
		}

		hResult = Device->GetDevice()->CreateRootSignature(0, SignatureBlob->GetBufferPointer(), SignatureBlob->GetBufferSize(), IID_PPV_ARGS(&HitGroupLocalRoot));
		if (FAILED(hResult))
		{
			::OutputDebugString("[D3D12RayTracer]: FAILED to create local RootSignature\n");
			return false;
		}
	}

	HitGroupLocalRoot->SetName(L"HitGroup Local RootSignature");

	{
		D3D12_STATE_SUBOBJECT HitGroupLocalRootSubObject = { };
		HitGroupLocalRootSubObject.Type		= D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
		HitGroupLocalRootSubObject.pDesc	= HitGroupLocalRoot.GetAddressOf();
		SubObjects.push_back(HitGroupLocalRootSubObject);
	}

	// Bind local root signature to hit group shaders
	LPCWSTR HitGroupLocalRootAssociationShaderNames[] = { ClosestHitShaderName };

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
	{
		D3D12_ROOT_SIGNATURE_DESC MissLocalRootDesc = {};
		MissLocalRootDesc.NumParameters = 0;
		MissLocalRootDesc.pParameters	= nullptr;
		MissLocalRootDesc.Flags			= D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

		ComPtr<ID3DBlob> SignatureBlob;
		ComPtr<ID3DBlob> ErrorBlob;
		HRESULT hResult = D3D12SerializeRootSignature(&MissLocalRootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &SignatureBlob, &ErrorBlob);
		if (FAILED(hResult))
		{
			::OutputDebugString("[D3D12RayTracer]: D3D12SerializeRootSignature failed with following error:\n");
			::OutputDebugString(reinterpret_cast<LPCSTR>(ErrorBlob->GetBufferPointer()));
			return false;
		}

		hResult = Device->GetDevice()->CreateRootSignature(0, SignatureBlob->GetBufferPointer(), SignatureBlob->GetBufferSize(), IID_PPV_ARGS(&MissLocalRoot));
		if (FAILED(hResult))
		{
			::OutputDebugString("[D3D12RayTracer]: FAILED to create local RootSignature\n");
			return false;
		}
	}

	MissLocalRoot->SetName(L"Miss Local RootSignature");

	{
		D3D12_STATE_SUBOBJECT MissLocalRootSubObject = { };
		MissLocalRootSubObject.Type		= D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
		MissLocalRootSubObject.pDesc	= MissLocalRoot.GetAddressOf();
		SubObjects.push_back(MissLocalRootSubObject);
	}

	// Bind local root signature to miss shader
	LPCWSTR missLocalRootAssociationShaderNames[] = { MissShaderName };

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
	ShaderConfig.MaxPayloadSizeInBytes		= sizeof(Float32) * 3 + sizeof(Uint32);

	{
		D3D12_STATE_SUBOBJECT ShaderConfigSubObject = { };
		ShaderConfigSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
		ShaderConfigSubObject.pDesc = &ShaderConfig;
		SubObjects.push_back(ShaderConfigSubObject);
	}

	// Bind the payload size to the programs
	const WCHAR* ShaderNamesToConfig[] = { MissShaderName, ClosestHitShaderName, RayGenShaderName };

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION ShaderConfigAssociation;
	ShaderConfigAssociation.pExports				= ShaderNamesToConfig;
	ShaderConfigAssociation.NumExports				= _countof(ShaderNamesToConfig);
	ShaderConfigAssociation.pSubobjectToAssociate	= &SubObjects.back();

	{
		D3D12_STATE_SUBOBJECT ShaderConfigAssociationSubObject = { };
		ShaderConfigAssociationSubObject.Type	= D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
		ShaderConfigAssociationSubObject.pDesc	= &ShaderConfigAssociation;
		SubObjects.push_back(ShaderConfigAssociationSubObject);
	}

	// Init pipeline config
	D3D12_RAYTRACING_PIPELINE_CONFIG PipelineConfig;
	PipelineConfig.MaxTraceRecursionDepth = 2;

	{
		D3D12_STATE_SUBOBJECT PipelineConfigSubObject = { };
		PipelineConfigSubObject.Type	= D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
		PipelineConfigSubObject.pDesc	= &PipelineConfig;
		SubObjects.push_back(PipelineConfigSubObject);
	}

	// Init global root signature
	{
		D3D12_DESCRIPTOR_RANGE Ranges[1] = {};

		// Camera Buffer
		Ranges[0].BaseShaderRegister				= 0;
		Ranges[0].NumDescriptors					= 1;
		Ranges[0].RegisterSpace						= 0;
		Ranges[0].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		Ranges[0].OffsetInDescriptorsFromTableStart = 0;

		D3D12_ROOT_PARAMETER RootParams = { };
		RootParams.ParameterType						= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		RootParams.DescriptorTable.NumDescriptorRanges	= _countof(Ranges);
		RootParams.DescriptorTable.pDescriptorRanges	= Ranges;

		D3D12_ROOT_SIGNATURE_DESC GlobalRootDesc = {};
		GlobalRootDesc.NumParameters	= 1;
		GlobalRootDesc.pParameters		= &RootParams;
		GlobalRootDesc.Flags			= D3D12_ROOT_SIGNATURE_FLAG_NONE;

		ComPtr<ID3DBlob> SignatureBlob;
		ComPtr<ID3DBlob> ErrorBlob;
		HRESULT hResult = D3D12SerializeRootSignature(&GlobalRootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &SignatureBlob, &ErrorBlob);
		if (FAILED(hResult))
		{
			::OutputDebugString("[D3D12RayTracer]: D3D12SerializeRootSignature failed with following error:\n");
			::OutputDebugString(reinterpret_cast<LPCSTR>(ErrorBlob->GetBufferPointer()));
			return false;
		}

		hResult = Device->GetDevice()->CreateRootSignature(0, SignatureBlob->GetBufferPointer(), SignatureBlob->GetBufferSize(), IID_PPV_ARGS(&GlobalRootSignature));
		if (FAILED(hResult))
		{
			::OutputDebugString("[D3D12RayTracer]: FAILED to create global RootSignature\n");
			return false;
		}
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

	HRESULT hResult = Device->GetDXRDevice()->CreateStateObject(&RayTracingPipeline, IID_PPV_ARGS(&DXRStateObject));
	if (SUCCEEDED(hResult))
	{
		::OutputDebugString("[D3D12RayTracer]: Created RayTracing PipelineState\n");
	}
	else
	{
		::OutputDebugString("[D3D12RayTracer]: FAILED to create RayTracing PipelineState\n");
		return false;
	}

	return true;
}

bool D3D12RayTracingPipelineState::CreateBindingTable()
{
	// Create Shader Binding Table
	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> StateProperties;
	HRESULT hResult = DXRStateObject.As<ID3D12StateObjectProperties>(&StateProperties);
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
			Uint8	ShaderIdentifier[D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES];
			UINT64	RTVDescriptor;
		} TableData;

		memcpy(TableData.ShaderIdentifier, StateProperties->GetShaderIdentifier(RayGenShaderName), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		// TableData.RTVDescriptor = Device->GetGlobalResourceDescriptorHeap()->GetGPUDescriptorHandleAt(0).ptr;

		// How big is the biggest?
		union MaxSize
		{
			RAY_GEN_SHADER_TABLE_DATA Data0;
		};

		RayGenTable.StrideInBytes	= sizeof(MaxSize);
		RayGenTable.SizeInBytes		= RayGenTable.StrideInBytes * 1; //<-- only one for now...

		BufferProperties BufferProps = { };
		BufferProps.SizeInBytes	= RayGenTable.SizeInBytes;
		BufferProps.Flags		= D3D12_RESOURCE_FLAG_NONE;
		BufferProps.InitalState	= D3D12_RESOURCE_STATE_GENERIC_READ;
		BufferProps.MemoryType	= EMemoryType::MEMORY_TYPE_UPLOAD;

		RayGenTable.Resource = new D3D12Buffer(Device);
		RayGenTable.Resource->Initialize(BufferProps);

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

		memcpy(TableData.ShaderIdentifier, StateProperties->GetShaderIdentifier(MissShaderName), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

		// How big is the biggest?
		union MaxSize
		{
			MISS_SHADER_TABLE_DATA Data0;
		};

		MissTable.StrideInBytes = sizeof(MaxSize);
		MissTable.SizeInBytes	= MissTable.StrideInBytes * 1; //<-- only one for now...

		BufferProperties BufferProps = { };
		BufferProps.SizeInBytes	= MissTable.SizeInBytes;
		BufferProps.Flags		= D3D12_RESOURCE_FLAG_NONE;
		BufferProps.InitalState	= D3D12_RESOURCE_STATE_GENERIC_READ;
		BufferProps.MemoryType	= EMemoryType::MEMORY_TYPE_UPLOAD;

		MissTable.Resource = new D3D12Buffer(Device);
		MissTable.Resource->Initialize(BufferProps);

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
			UINT64 ASDescriptor;
		} TableData;

		//TableData.ASDescriptor = Device->GetGlobalResourceDescriptorHeap()->GetGPUDescriptorHandleAt(1).ptr;
		memcpy(TableData.ShaderIdentifier, StateProperties->GetShaderIdentifier(HitGroupName), D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);

		// How big is the biggest?
		union MaxSize
		{
			HIT_GROUP_SHADER_TABLE_DATA Data0;
		};

		HitTable.StrideInBytes = sizeof(MaxSize);
		HitTable.SizeInBytes = HitTable.StrideInBytes * 1; //<-- only one for now...

		BufferProperties BufferProps = { };
		BufferProps.SizeInBytes	= HitTable.SizeInBytes;
		BufferProps.Flags		= D3D12_RESOURCE_FLAG_NONE;
		BufferProps.InitalState	= D3D12_RESOURCE_STATE_GENERIC_READ;
		BufferProps.MemoryType	= EMemoryType::MEMORY_TYPE_UPLOAD;

		HitTable.Resource = new D3D12Buffer(Device);
		HitTable.Resource->Initialize(BufferProps);

		// Map the buffer
		void* Data = HitTable.Resource->Map();
		memcpy(Data, &TableData, sizeof(TableData));
		HitTable.Resource->Unmap();
	}

	return true;
}
