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

bool D3D12RootSignature::Initialize()
{
	using namespace Microsoft::WRL;

	D3D12_DESCRIPTOR_RANGE DescRange = {};
	DescRange.RangeType							= D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	DescRange.NumDescriptors					= 1;
	DescRange.BaseShaderRegister				= 0;
	DescRange.RegisterSpace						= 0;
	DescRange.OffsetInDescriptorsFromTableStart	= 0;

	D3D12_ROOT_PARAMETER Params[2] = {};
	Params[0].ParameterType				= D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	Params[0].Constants.ShaderRegister	= 0;
	Params[0].Constants.RegisterSpace	= 0;
	Params[0].Constants.Num32BitValues	= 16;
	Params[0].ShaderVisibility			= D3D12_SHADER_VISIBILITY_VERTEX;

	Params[1].ParameterType							= D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	Params[1].DescriptorTable.NumDescriptorRanges	= 1;
	Params[1].DescriptorTable.pDescriptorRanges		= &DescRange;
	Params[1].ShaderVisibility						= D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_STATIC_SAMPLER_DESC StaticSampler = {};
	StaticSampler.Filter			= D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	StaticSampler.AddressU			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	StaticSampler.AddressV			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	StaticSampler.AddressW			= D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	StaticSampler.MipLODBias		= 0.0f;
	StaticSampler.MaxAnisotropy		= 0;
	StaticSampler.ComparisonFunc	= D3D12_COMPARISON_FUNC_ALWAYS;
	StaticSampler.BorderColor		= D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	StaticSampler.MinLOD			= 0.f;
	StaticSampler.MaxLOD			= 0.f;
	StaticSampler.ShaderRegister	= 0;
	StaticSampler.RegisterSpace		= 0;
	StaticSampler.ShaderVisibility	= D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC desc = {};
	desc.NumParameters		= _countof(Params);
	desc.pParameters		= Params;
	desc.NumStaticSamplers	= 1;
	desc.pStaticSamplers	= &StaticSampler;
	desc.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	ComPtr<ID3DBlob> ErrorBlob;
	ComPtr<ID3DBlob> SignatureBlob;
	HRESULT hResult = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &SignatureBlob, &ErrorBlob);
	if (FAILED(hResult))
	{
		::OutputDebugString("[D3D12RootSignature]: Failed to Serialize RootSignature\n");
		::OutputDebugString(reinterpret_cast<const Char*>(ErrorBlob->GetBufferPointer()));
		return false;
	}

	hResult = Device->GetDevice()->CreateRootSignature(0, SignatureBlob->GetBufferPointer(), SignatureBlob->GetBufferSize(), IID_PPV_ARGS(&RootSignature));
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

void D3D12RootSignature::SetName(const std::string& Name)
{
	RootSignature->SetName(ConvertToWide(Name).c_str());
}
