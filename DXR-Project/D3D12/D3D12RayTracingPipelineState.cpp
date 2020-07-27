#include "D3D12RayTracingPipelineState.h"
#include "D3D12ShaderCompiler.h"
#include "D3D12Device.h"
#include "D3D12DescriptorHeap.h"
#include "D3D12RootSignature.h"

const LPCWSTR RayGenShaderName		= L"RayGen";
const LPCWSTR MissShaderName		= L"Miss";
const LPCWSTR ClosestHitShaderName	= L"ClosestHit";
const LPCWSTR HitGroupName			= L"HitGroup";

D3D12RayTracingPipelineState::D3D12RayTracingPipelineState(D3D12Device* InDevice)
	: D3D12DeviceChild(InDevice)
	, StateObject(nullptr)
{
}

D3D12RayTracingPipelineState::~D3D12RayTracingPipelineState()
{
}

bool D3D12RayTracingPipelineState::Initialize(const RayTracingPipelineStateProperties& Properties)
{
	using namespace Microsoft::WRL;

	// Vector for all subobjects
	std::vector<D3D12_STATE_SUBOBJECT> SubObjects;
	SubObjects.reserve(32);

	// Init DXIL subobject
	ComPtr<IDxcBlob> RayTracingShaders = D3D12ShaderCompiler::Get()->CompileFromFile("Shaders/RayTracingShaders.hlsl", "", "lib_6_3");
	D3D12_EXPORT_DESC DxilExports[] =
	{
		RayGenShaderName,		nullptr, D3D12_EXPORT_FLAG_NONE,
		MissShaderName,			nullptr, D3D12_EXPORT_FLAG_NONE,
		ClosestHitShaderName,	nullptr, D3D12_EXPORT_FLAG_NONE,
	};

	VALIDATE(RayTracingShaders);

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
		HitGroupSubObject.pDesc = &HitGroupDesc;
		SubObjects.push_back(HitGroupSubObject);
	}

	// Init RayGen local root signature
	{
		D3D12_STATE_SUBOBJECT RayGenLocalRootSubObject = { };
		RayGenLocalRootSubObject.Type	= D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
		RayGenLocalRootSubObject.pDesc	= Properties.RayGenRootSignature->GetRootSignatureAddress();
		SubObjects.push_back(RayGenLocalRootSubObject);
	}

	// Bind local root signature to rayGen shader
	LPCWSTR RayGenLocalRootAssociationShaderNames[] = { RayGenShaderName };

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION RayGenLocalRootAssociation;
	RayGenLocalRootAssociation.pExports		= RayGenLocalRootAssociationShaderNames;
	RayGenLocalRootAssociation.NumExports	= _countof(RayGenLocalRootAssociationShaderNames);
	RayGenLocalRootAssociation.pSubobjectToAssociate = &SubObjects.back();

	{
		D3D12_STATE_SUBOBJECT RayGenLocalRootAssociationSubObject = { };
		RayGenLocalRootAssociationSubObject.Type	= D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
		RayGenLocalRootAssociationSubObject.pDesc	= &RayGenLocalRootAssociation;
		SubObjects.push_back(RayGenLocalRootAssociationSubObject);
	}

	// Init hit group local root signature
	{
		D3D12_STATE_SUBOBJECT HitGroupLocalRootSubObject = { };
		HitGroupLocalRootSubObject.Type		= D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
		HitGroupLocalRootSubObject.pDesc	= Properties.HitGroupRootSignature->GetRootSignatureAddress();
		SubObjects.push_back(HitGroupLocalRootSubObject);
	}

	// Bind local root signature to hit group shaders
	LPCWSTR HitGroupLocalRootAssociationShaderNames[] = { ClosestHitShaderName };

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION HitGroupLocalRootAssociation;
	HitGroupLocalRootAssociation.pExports	= HitGroupLocalRootAssociationShaderNames;
	HitGroupLocalRootAssociation.NumExports = _countof(HitGroupLocalRootAssociationShaderNames);
	HitGroupLocalRootAssociation.pSubobjectToAssociate = &SubObjects.back();

	{
		D3D12_STATE_SUBOBJECT HitGroupLocalRootAssociationSubObject = { };
		HitGroupLocalRootAssociationSubObject.Type	= D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
		HitGroupLocalRootAssociationSubObject.pDesc = &HitGroupLocalRootAssociation;
		SubObjects.push_back(HitGroupLocalRootAssociationSubObject);
	}

	// Init miss local root signature
	{
		D3D12_STATE_SUBOBJECT MissLocalRootSubObject = { };
		MissLocalRootSubObject.Type		= D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
		MissLocalRootSubObject.pDesc	= Properties.MissRootSignature->GetRootSignatureAddress();
		SubObjects.push_back(MissLocalRootSubObject);
	}

	// Bind local root signature to miss shader
	LPCWSTR missLocalRootAssociationShaderNames[] = { MissShaderName };

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION MissLocalRootAssociation;
	MissLocalRootAssociation.pExports	= missLocalRootAssociationShaderNames;
	MissLocalRootAssociation.NumExports = _countof(missLocalRootAssociationShaderNames);
	MissLocalRootAssociation.pSubobjectToAssociate = &SubObjects.back();

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
		ShaderConfigSubObject.Type	= D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
		ShaderConfigSubObject.pDesc = &ShaderConfig;
		SubObjects.push_back(ShaderConfigSubObject);
	}

	// Bind the payload size to the programs
	const WCHAR* ShaderNamesToConfig[] = { MissShaderName, ClosestHitShaderName, RayGenShaderName };

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION ShaderConfigAssociation;
	ShaderConfigAssociation.pExports	= ShaderNamesToConfig;
	ShaderConfigAssociation.NumExports	= _countof(ShaderNamesToConfig);
	ShaderConfigAssociation.pSubobjectToAssociate = &SubObjects.back();

	{
		D3D12_STATE_SUBOBJECT ShaderConfigAssociationSubObject = { };
		ShaderConfigAssociationSubObject.Type	= D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
		ShaderConfigAssociationSubObject.pDesc	= &ShaderConfigAssociation;
		SubObjects.push_back(ShaderConfigAssociationSubObject);
	}

	// Init pipeline config
	D3D12_RAYTRACING_PIPELINE_CONFIG PipelineConfig;
	PipelineConfig.MaxTraceRecursionDepth = Properties.MaxRecursions;

	{
		D3D12_STATE_SUBOBJECT PipelineConfigSubObject = { };
		PipelineConfigSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
		PipelineConfigSubObject.pDesc = &PipelineConfig;
		SubObjects.push_back(PipelineConfigSubObject);
	}

	// Init global root signature
	{
		D3D12_STATE_SUBOBJECT GlobalRootSubObject = { };
		GlobalRootSubObject.Type	= D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
		GlobalRootSubObject.pDesc	= Properties.GlobalRootSignature->GetRootSignatureAddress();
		SubObjects.push_back(GlobalRootSubObject);
	}

	// Create state object
	D3D12_STATE_OBJECT_DESC RayTracingPipeline = { };
	RayTracingPipeline.Type				= D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	RayTracingPipeline.NumSubobjects	= static_cast<Uint32>(SubObjects.size());
	RayTracingPipeline.pSubobjects		= SubObjects.data();

	HRESULT hResult = Device->GetDXRDevice()->CreateStateObject(&RayTracingPipeline, IID_PPV_ARGS(&StateObject));
	if (SUCCEEDED(hResult))
	{
		SetName(Properties.DebugName);

		LOG_INFO("[D3D12RayTracingPipelineState]: Created RayTracing PipelineState");
		return true;
	}
	else
	{
		LOG_ERROR("[D3D12RayTracingPipelineState]: FAILED to create RayTracing PipelineState");
		return false;
	}
}

void D3D12RayTracingPipelineState::SetName(const std::string& InName)
{
	std::wstring WideName = ConvertToWide(InName);
	StateObject->SetName(WideName.c_str());
}