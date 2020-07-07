#include "D3D12RootSignature.h"
#include "D3D12Device.h"

D3D12RootSignature::D3D12RootSignature(D3D12Device* InDevice)
	: D3D12DeviceChild(InDevice)
	, RootSignature(nullptr)
{
}

D3D12RootSignature::~D3D12RootSignature()
{
}

bool D3D12RootSignature::Initialize(const RootSignatureDesc& RootSignatureDesc)
{
	using namespace Microsoft::WRL;

	const Uint32 NumRootParameters = static_cast<Uint32>(RootSignatureDesc.DescriptorTables.size()) + 1;
	std::vector<D3D12_ROOT_PARAMETER> Params(NumRootParameters);
	Params[0].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	Params[0].Constants.ShaderRegister	= 0;
	Params[0].Constants.RegisterSpace	= 0;
	Params[0].Constants.Num32BitValues	= 16;
	Params[0].ShaderVisibility			= D3D12_SHADER_VISIBILITY_VERTEX;

	// DescriptorTables;
	Uint32 ParameterOffset = 1;
	for (const DescriptorTableDesc& Table : RootSignatureDesc.DescriptorTables)
	{
		Params[ParameterOffset].ParameterType						= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		Params[ParameterOffset].DescriptorTable.NumDescriptorRanges	= static_cast<Uint32>(Table.Ranges.size());
		Params[ParameterOffset].DescriptorTable.pDescriptorRanges	= Table.Ranges.data();
		Params[ParameterOffset].ShaderVisibility					= Table.Visibility;
		ParameterOffset++;
	}

	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters		= static_cast<Uint32>(Params.size());
	desc.pParameters		= Params.data();
	desc.NumStaticSamplers	= static_cast<Uint32>(RootSignatureDesc.StaticSamplers.size());
	desc.pStaticSamplers	= RootSignatureDesc.StaticSamplers.data();
	desc.Flags				= RootSignatureDesc.Flags;

	ComPtr<ID3DBlob> ErrorBlob;
	ComPtr<ID3DBlob> SignatureBlob;
	HRESULT hResult = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &SignatureBlob, &ErrorBlob);
	if (FAILED(hResult))
	{
		::OutputDebugString("[D3D12RootSignature]: Failed to Serialize RootSignature\n");
		::OutputDebugString(reinterpret_cast<const Char*>(ErrorBlob->GetBufferPointer()));
		return false;
	}
	else
	{
		return Initialize(SignatureBlob->GetBufferPointer(), SignatureBlob->GetBufferSize());
	}
}

bool D3D12RootSignature::Initialize(IDxcBlob* ShaderBlob)
{
	return Initialize(ShaderBlob->GetBufferPointer(), ShaderBlob->GetBufferSize());
}

void D3D12RootSignature::SetName(const std::string& Name)
{
	RootSignature->SetName(ConvertToWide(Name).c_str());
}

bool D3D12RootSignature::Initialize(const void* RootSignatureBlob, Uint32 BlobSizeInBytes)
{
	HRESULT hResult = Device->GetDevice()->CreateRootSignature(0, RootSignatureBlob, BlobSizeInBytes, IID_PPV_ARGS(&RootSignature));
	if (SUCCEEDED(hResult))
	{
		::OutputDebugString("[D3D12RootSignature]: Created RootSignature\n");
		return true;
	}
	else
	{
		::OutputDebugString("[D3D12RootSignature]: Failed to Create RootSignature\n");
		return false;
	}
}
