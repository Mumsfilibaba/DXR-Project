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
	TArray<D3D12_STATE_SUBOBJECT> SubObjects;
	SubObjects.Reserve(32);

	// Init DXIL subobject
	ComPtr<IDxcBlob> RayTracingShaders = nullptr;// D3D12ShaderCompiler::CompileFromFile("Shaders/RayTracingShaders.hlsl", "", "lib_6_3");
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
		SubObjects.PushBack(LibrarySubObject);
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
		SubObjects.PushBack(HitGroupSubObject);
	}

	// Init RayGen local root signature
	{
		D3D12_STATE_SUBOBJECT RayGenLocalRootSubObject = { };
		RayGenLocalRootSubObject.Type	= D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
		RayGenLocalRootSubObject.pDesc	= Properties.RayGenRootSignature->GetAddressOfRootSignature();
		SubObjects.PushBack(RayGenLocalRootSubObject);
	}

	// Bind local root signature to rayGen shader
	LPCWSTR RayGenLocalRootAssociationShaderNames[] = { RayGenShaderName };

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION RayGenLocalRootAssociation;
	RayGenLocalRootAssociation.pExports		= RayGenLocalRootAssociationShaderNames;
	RayGenLocalRootAssociation.NumExports	= _countof(RayGenLocalRootAssociationShaderNames);
	RayGenLocalRootAssociation.pSubobjectToAssociate = &SubObjects.Back();

	{
		D3D12_STATE_SUBOBJECT RayGenLocalRootAssociationSubObject = { };
		RayGenLocalRootAssociationSubObject.Type	= D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
		RayGenLocalRootAssociationSubObject.pDesc	= &RayGenLocalRootAssociation;
		SubObjects.PushBack(RayGenLocalRootAssociationSubObject);
	}

	// Init hit group local root signature
	{
		D3D12_STATE_SUBOBJECT HitGroupLocalRootSubObject = { };
		HitGroupLocalRootSubObject.Type		= D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
		HitGroupLocalRootSubObject.pDesc	= Properties.HitGroupRootSignature->GetAddressOfRootSignature();
		SubObjects.PushBack(HitGroupLocalRootSubObject);
	}

	// Bind local root signature to hit group shaders
	LPCWSTR HitGroupLocalRootAssociationShaderNames[] = { ClosestHitShaderName };

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION HitGroupLocalRootAssociation;
	HitGroupLocalRootAssociation.pExports	= HitGroupLocalRootAssociationShaderNames;
	HitGroupLocalRootAssociation.NumExports = _countof(HitGroupLocalRootAssociationShaderNames);
	HitGroupLocalRootAssociation.pSubobjectToAssociate = &SubObjects.Back();

	{
		D3D12_STATE_SUBOBJECT HitGroupLocalRootAssociationSubObject = { };
		HitGroupLocalRootAssociationSubObject.Type	= D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
		HitGroupLocalRootAssociationSubObject.pDesc = &HitGroupLocalRootAssociation;
		SubObjects.PushBack(HitGroupLocalRootAssociationSubObject);
	}

	// Init miss local root signature
	{
		D3D12_STATE_SUBOBJECT MissLocalRootSubObject = { };
		MissLocalRootSubObject.Type		= D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
		MissLocalRootSubObject.pDesc	= Properties.MissRootSignature->GetAddressOfRootSignature();
		SubObjects.PushBack(MissLocalRootSubObject);
	}

	// Bind local root signature to miss shader
	LPCWSTR missLocalRootAssociationShaderNames[] = { MissShaderName };

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION MissLocalRootAssociation;
	MissLocalRootAssociation.pExports	= missLocalRootAssociationShaderNames;
	MissLocalRootAssociation.NumExports = _countof(missLocalRootAssociationShaderNames);
	MissLocalRootAssociation.pSubobjectToAssociate = &SubObjects.Back();

	{
		D3D12_STATE_SUBOBJECT MissLocalRootAssociationSubObject = { };
		MissLocalRootAssociationSubObject.Type	= D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
		MissLocalRootAssociationSubObject.pDesc = &MissLocalRootAssociation;
		SubObjects.PushBack(MissLocalRootAssociationSubObject);
	}

	// Init shader config
	D3D12_RAYTRACING_SHADER_CONFIG ShaderConfig = {};
	ShaderConfig.MaxAttributeSizeInBytes	= sizeof(Float32) * 2;
	ShaderConfig.MaxPayloadSizeInBytes		= sizeof(Float32) * 3 + sizeof(Uint32);

	{
		D3D12_STATE_SUBOBJECT ShaderConfigSubObject = { };
		ShaderConfigSubObject.Type	= D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
		ShaderConfigSubObject.pDesc = &ShaderConfig;
		SubObjects.PushBack(ShaderConfigSubObject);
	}

	// Bind the payload size to the programs
	const WCHAR* ShaderNamesToConfig[] = { MissShaderName, ClosestHitShaderName, RayGenShaderName };

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION ShaderConfigAssociation;
	ShaderConfigAssociation.pExports	= ShaderNamesToConfig;
	ShaderConfigAssociation.NumExports	= _countof(ShaderNamesToConfig);
	ShaderConfigAssociation.pSubobjectToAssociate = &SubObjects.Back();

	{
		D3D12_STATE_SUBOBJECT ShaderConfigAssociationSubObject = { };
		ShaderConfigAssociationSubObject.Type	= D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
		ShaderConfigAssociationSubObject.pDesc	= &ShaderConfigAssociation;
		SubObjects.PushBack(ShaderConfigAssociationSubObject);
	}

	// Init pipeline config
	D3D12_RAYTRACING_PIPELINE_CONFIG PipelineConfig;
	PipelineConfig.MaxTraceRecursionDepth = Properties.MaxRecursions;

	{
		D3D12_STATE_SUBOBJECT PipelineConfigSubObject = { };
		PipelineConfigSubObject.Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
		PipelineConfigSubObject.pDesc = &PipelineConfig;
		SubObjects.PushBack(PipelineConfigSubObject);
	}

	// Init global root signature
	{
		D3D12_STATE_SUBOBJECT GlobalRootSubObject = { };
		GlobalRootSubObject.Type	= D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
		GlobalRootSubObject.pDesc	= Properties.GlobalRootSignature->GetAddressOfRootSignature();
		SubObjects.PushBack(GlobalRootSubObject);
	}

	// Create state object
	D3D12_STATE_OBJECT_DESC RayTracingPipeline = { };
	RayTracingPipeline.Type				= D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	RayTracingPipeline.NumSubobjects	= static_cast<Uint32>(SubObjects.Size());
	RayTracingPipeline.pSubobjects		= SubObjects.Data();

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