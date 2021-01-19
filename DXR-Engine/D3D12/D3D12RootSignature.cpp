#include "D3D12RootSignature.h"
#include "D3D12Device.h"

/*
* D3D12DefaultRootSignatures
*/

Bool D3D12DefaultRootSignatures::CreateRootSignatures(D3D12Device* Device)
{
	constexpr UInt32 ShaderRegisterOffset32BitConstants = 1;

	// Ranges for resources
	D3D12_DESCRIPTOR_RANGE CBVRanges[D3D12_DEFAULT_DESCRIPTOR_TABLE_HANDLE_COUNT];
	Memory::Memzero(CBVRanges, sizeof(CBVRanges));

	D3D12_DESCRIPTOR_RANGE SRVRanges[D3D12_DEFAULT_DESCRIPTOR_TABLE_HANDLE_COUNT];
	Memory::Memzero(SRVRanges, sizeof(SRVRanges));

	D3D12_DESCRIPTOR_RANGE UAVRanges[D3D12_DEFAULT_DESCRIPTOR_TABLE_HANDLE_COUNT];
	Memory::Memzero(UAVRanges, sizeof(UAVRanges));

	D3D12_DESCRIPTOR_RANGE SamplerRanges[D3D12_DEFAULT_DESCRIPTOR_TABLE_HANDLE_COUNT];
	Memory::Memzero(SamplerRanges, sizeof(SamplerRanges));
	
	for (UInt32 i = 0; i < D3D12_DEFAULT_DESCRIPTOR_TABLE_HANDLE_COUNT; i++)
	{
		CBVRanges[i].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		CBVRanges[i].BaseShaderRegister					= ShaderRegisterOffset32BitConstants + i;
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

		SamplerRanges[i].RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
		SamplerRanges[i].BaseShaderRegister					= i;
		SamplerRanges[i].NumDescriptors						= 1;
		SamplerRanges[i].RegisterSpace						= 0;
		SamplerRanges[i].OffsetInDescriptorsFromTableStart	= i;
	}

	// Graphics
	D3D12_ROOT_SIGNATURE_DESC GraphicsRootDesc;
	Memory::Memzero(&GraphicsRootDesc);

	// 1 For 32 bit constants and 4, one for each type of resource, CBV, SRV, UAV, Samplers
	constexpr UInt32 NumParameters = 1 + 4;

	D3D12_ROOT_PARAMETER GraphicsRootParameters[NumParameters];
	Memory::Memzero(GraphicsRootParameters, sizeof(GraphicsRootParameters));

	// Constants
	GraphicsRootParameters[D3D12_DEFAULT_SHADER_32BIT_CONSTANTS_ROOT_PARAMETER].Constants.Num32BitValues	= D3D12_DEFAULT_SHADER_32BIT_CONSTANTS_COUNT;
	GraphicsRootParameters[D3D12_DEFAULT_SHADER_32BIT_CONSTANTS_ROOT_PARAMETER].Constants.RegisterSpace		= 0;
	GraphicsRootParameters[D3D12_DEFAULT_SHADER_32BIT_CONSTANTS_ROOT_PARAMETER].Constants.ShaderRegister	= 0;
	GraphicsRootParameters[D3D12_DEFAULT_SHADER_32BIT_CONSTANTS_ROOT_PARAMETER].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	GraphicsRootParameters[D3D12_DEFAULT_SHADER_32BIT_CONSTANTS_ROOT_PARAMETER].ShaderVisibility			= D3D12_SHADER_VISIBILITY_ALL;

	// Start at 1, and skip the constants
	for (UInt32 i = 1; i < NumParameters; i++)
	{
		GraphicsRootParameters[i].ParameterType		= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		GraphicsRootParameters[i].ShaderVisibility	= D3D12_SHADER_VISIBILITY_ALL;
		GraphicsRootParameters[i].DescriptorTable.NumDescriptorRanges = D3D12_DEFAULT_DESCRIPTOR_TABLE_HANDLE_COUNT;
	}

	GraphicsRootParameters[D3D12_DEFAULT_CONSTANT_BUFFER_ROOT_PARAMETER].DescriptorTable.pDescriptorRanges			= CBVRanges;
	GraphicsRootParameters[D3D12_DEFAULT_SHADER_RESOURCE_VIEW_ROOT_PARAMETER].DescriptorTable.pDescriptorRanges		= SRVRanges;
	GraphicsRootParameters[D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_ROOT_PARAMETER].DescriptorTable.pDescriptorRanges	= UAVRanges;
	GraphicsRootParameters[D3D12_DEFAULT_SAMPLER_STATE_ROOT_PARAMETER].DescriptorTable.pDescriptorRanges			= SamplerRanges;

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
	Memory::Memzero(&ComputeRootDesc);

	D3D12_ROOT_PARAMETER ComputeRootParameters[NumParameters];
	Memory::Memzero(ComputeRootParameters, sizeof(ComputeRootParameters));

	// Constants
	ComputeRootParameters[D3D12_DEFAULT_SHADER_32BIT_CONSTANTS_ROOT_PARAMETER].Constants.Num32BitValues	= D3D12_DEFAULT_SHADER_32BIT_CONSTANTS_COUNT;
	ComputeRootParameters[D3D12_DEFAULT_SHADER_32BIT_CONSTANTS_ROOT_PARAMETER].Constants.RegisterSpace	= 0;
	ComputeRootParameters[D3D12_DEFAULT_SHADER_32BIT_CONSTANTS_ROOT_PARAMETER].Constants.ShaderRegister	= 0;
	ComputeRootParameters[D3D12_DEFAULT_SHADER_32BIT_CONSTANTS_ROOT_PARAMETER].ParameterType			= D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	ComputeRootParameters[D3D12_DEFAULT_SHADER_32BIT_CONSTANTS_ROOT_PARAMETER].ShaderVisibility			= D3D12_SHADER_VISIBILITY_ALL;

	// Start at 1, and skip the constants
	for (UInt32 i = 1; i < NumParameters; i++)
	{
		ComputeRootParameters[i].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		ComputeRootParameters[i].ShaderVisibility						= D3D12_SHADER_VISIBILITY_ALL;
		ComputeRootParameters[i].DescriptorTable.NumDescriptorRanges	= D3D12_DEFAULT_DESCRIPTOR_TABLE_HANDLE_COUNT;
	}

	ComputeRootParameters[D3D12_DEFAULT_CONSTANT_BUFFER_ROOT_PARAMETER].DescriptorTable.pDescriptorRanges		= CBVRanges;
	ComputeRootParameters[D3D12_DEFAULT_SHADER_RESOURCE_VIEW_ROOT_PARAMETER].DescriptorTable.pDescriptorRanges	= SRVRanges;
	ComputeRootParameters[D3D12_DEFAULT_UNORDERED_ACCESS_VIEW_ROOT_PARAMETER].DescriptorTable.pDescriptorRanges = UAVRanges;
	ComputeRootParameters[D3D12_DEFAULT_SAMPLER_STATE_ROOT_PARAMETER].DescriptorTable.pDescriptorRanges			= SamplerRanges;

	ComputeRootDesc.NumParameters	= NumParameters;
	ComputeRootDesc.pParameters		= ComputeRootParameters;
	ComputeRootDesc.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS	|
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS		|
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS	|
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS	|
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
