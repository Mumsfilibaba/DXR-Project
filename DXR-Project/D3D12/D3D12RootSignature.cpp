#include "D3D12RootSignature.h"
#include "D3D12Device.h"

/*
* D3D12DefaultRootSignatures
*/

bool D3D12DefaultRootSignatures::Init(D3D12Device* Device)
{
	// Ranges for resources
	constexpr Uint32 NumRanges = 8;
	D3D12_DESCRIPTOR_RANGE CBVRanges[NumRanges];
	Memory::Memzero(CBVRanges, sizeof(CBVRanges));

	D3D12_DESCRIPTOR_RANGE SRVRanges[NumRanges];
	Memory::Memzero(SRVRanges, sizeof(SRVRanges));

	D3D12_DESCRIPTOR_RANGE UAVRanges[NumRanges];
	Memory::Memzero(UAVRanges, sizeof(UAVRanges));
	
	for (Uint32 i = 0; i < NumRanges; i++)
	{
		CBVRanges[i].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		CBVRanges[i].BaseShaderRegister					= i;
		CBVRanges[i].NumDescriptors						= 1;
		CBVRanges[i].RegisterSpace						= 0;
		CBVRanges[i].OffsetInDescriptorsFromTableStart	= i;

		SRVRanges[i].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		SRVRanges[i].BaseShaderRegister					= i;
		SRVRanges[i].NumDescriptors						= 1;
		SRVRanges[i].RegisterSpace						= 0;
		SRVRanges[i].OffsetInDescriptorsFromTableStart	= i;

		UAVRanges[i].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		UAVRanges[i].BaseShaderRegister					= i;
		UAVRanges[i].NumDescriptors						= 1;
		UAVRanges[i].RegisterSpace						= 0;
		UAVRanges[i].OffsetInDescriptorsFromTableStart	= i;
	}

	// Graphics
	D3D12_ROOT_SIGNATURE_DESC GraphicsRootDesc;
	Memory::Memzero(&GraphicsRootDesc, sizeof(D3D12_ROOT_SIGNATURE_DESC));

	constexpr Uint32 NumParametersPerStage	= 3;
	constexpr Uint32 NumStages				= 2;
	constexpr Uint32 NumParameters			= NumParametersPerStage * NumStages;
	D3D12_ROOT_PARAMETER GraphicsRootParameters[NumParameters];
	Memory::Memzero(GraphicsRootParameters, sizeof(GraphicsRootParameters));

	const D3D12_SHADER_VISIBILITY Stages[NumStages] = 
	{ 
		D3D12_SHADER_VISIBILITY_VERTEX, 
		D3D12_SHADER_VISIBILITY_PIXEL 
	};

	for (Uint32 Stage = 0; Stage < NumStages; Stage++)
	{
		const Uint32 Index = Stage * NumParametersPerStage;
		for (Uint32 i = 0; i < NumParametersPerStage; i++)
		{
			GraphicsRootParameters[Index + i].ParameterType		= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			GraphicsRootParameters[Index + i].ShaderVisibility	= Stages[Stage];
			GraphicsRootParameters[Index + i].DescriptorTable.NumDescriptorRanges = NumRanges;
		}

		GraphicsRootParameters[Index + 0].DescriptorTable.pDescriptorRanges = CBVRanges;
		GraphicsRootParameters[Index + 1].DescriptorTable.pDescriptorRanges = SRVRanges;
		GraphicsRootParameters[Index + 2].DescriptorTable.pDescriptorRanges = UAVRanges;
	}

	GraphicsRootDesc.NumParameters	= NumParameters;
	GraphicsRootDesc.pParameters	= GraphicsRootParameters;
	GraphicsRootDesc.Flags = 
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT	|
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS			|
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS		|
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	Graphics = Device->CreateRootSignature(GraphicsRootDesc);
	if (!Graphics)
	{
		return false;
	}

	// Compute
	D3D12_ROOT_SIGNATURE_DESC ComputeRootDesc;
	Memory::Memzero(&ComputeRootDesc, sizeof(D3D12_ROOT_SIGNATURE_DESC));

	D3D12_ROOT_PARAMETER ComputeRootParameters[NumParametersPerStage];
	Memory::Memzero(ComputeRootParameters, sizeof(ComputeRootParameters));

	for (Uint32 i = 0; i < NumParametersPerStage; i++)
	{
		ComputeRootParameters[i].ParameterType		= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		ComputeRootParameters[i].ShaderVisibility	= D3D12_SHADER_VISIBILITY_ALL;
		ComputeRootParameters[i].DescriptorTable.NumDescriptorRanges = NumRanges;
	}

	ComputeRootParameters[0].DescriptorTable.pDescriptorRanges = CBVRanges;
	ComputeRootParameters[1].DescriptorTable.pDescriptorRanges = SRVRanges;
	ComputeRootParameters[2].DescriptorTable.pDescriptorRanges = UAVRanges;

	ComputeRootDesc.NumParameters	= NumParametersPerStage;
	ComputeRootDesc.pParameters		= ComputeRootParameters;
	ComputeRootDesc.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;

	if (Device->IsMeshShadersSupported())
	{
		ComputeRootDesc.Flags |=
			D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS;
	}

	Compute = Device->CreateRootSignature(ComputeRootDesc);
	if (Compute)
	{
		LOG_INFO("Created Default RootSignatures");
		return true;
	}
	else
	{
		return false;
	}
}
