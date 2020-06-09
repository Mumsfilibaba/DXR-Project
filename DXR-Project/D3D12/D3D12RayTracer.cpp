#include "D3D12RayTracer.h"
#include "D3D12Device.h"
#include "D3D12CommandList.h"
#include "D3D12ShaderCompiler.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12Texture.h"

D3D12RayTracer::D3D12RayTracer(D3D12Device* Device)
	: D3D12DeviceChild(Device)
{
}

D3D12RayTracer::~D3D12RayTracer()
{
	delete ResourceHeap;
	delete ResultTexture;
}

bool D3D12RayTracer::Init(D3D12CommandList* CommandList)
{
	using namespace Microsoft::WRL;

	// Get DXR Interfaces 
	ComPtr<ID3D12Device> MainDevice(Device->GetDevice());
	if (FAILED(MainDevice.As<ID3D12Device5>(&DXRDevice)))
	{
		::OutputDebugString("[D3D12RayTracer]: Failed to retrive DXR-Device");
		return false;
	}

	ComPtr<ID3D12CommandList> MainCommandList(CommandList->GetCommandList());
	if (FAILED(MainCommandList.As<ID3D12GraphicsCommandList4>(&DXRCommandList)))
	{
		::OutputDebugString("[D3D12RayTracer]: Failed to retrive DXR-CommandList");
		return false;
	}

	// Create DescriptorHeap
	ResourceHeap = new D3D12DescriptorHeap(Device);
	if (!ResourceHeap->Init(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 8, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE))
	{
		return false;
	}

	// Create image
	ResultTexture = new D3D12Texture(Device);
	if (!ResultTexture->Init(DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, 1280, 720))
	{
		return false;
	}

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
		::OutputDebugString("[D3D12RayTracer]: FAILED to create local RootSignature");
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
		RayGenLocalRootAssociationSubObject.pDesc =	 &RayGenLocalRootAssociation;
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
		::OutputDebugString("[D3D12RayTracer]: FAILED to create local RootSignature");
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
		::OutputDebugString("[D3D12RayTracer]: FAILED to create local RootSignature");
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
		::OutputDebugString("[D3D12RayTracer]: FAILED to create global RootSignature");
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
		::OutputDebugString("[D3D12RayTracer]: Created RayTracing PipelineState");
		return true;
	}
	else
	{
		::OutputDebugString("[D3D12RayTracer]: FAILED to create RayTracing PipelineState");
		return false;
	}
}
