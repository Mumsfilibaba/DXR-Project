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

bool D3D12RootSignature::Initialize(const D3D12_ROOT_SIGNATURE_DESC& Desc)
{
	using namespace Microsoft::WRL;

	ComPtr<ID3DBlob> ErrorBlob;
	ComPtr<ID3DBlob> SignatureBlob;
	HRESULT hResult = D3D12SerializeRootSignature(&Desc, D3D_ROOT_SIGNATURE_VERSION_1, &SignatureBlob, &ErrorBlob);
	if (FAILED(hResult))
	{
		LOG_ERROR("[D3D12RootSignature]: FAILED to Serialize RootSignature");
		LOG_ERROR(reinterpret_cast<const Char*>(ErrorBlob->GetBufferPointer()));
		__debugbreak();

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
	std::wstring WideName = ConvertToWide(Name);
	RootSignature->SetName(WideName.c_str());
}

bool D3D12RootSignature::Initialize(const void* RootSignatureBlob, Uint32 BlobSizeInBytes)
{
	HRESULT hResult = Device->GetDevice()->CreateRootSignature(0, RootSignatureBlob, BlobSizeInBytes, IID_PPV_ARGS(&RootSignature));
	if (SUCCEEDED(hResult))
	{
		LOG_INFO("[D3D12RootSignature]: Created RootSignature");
		return true;
	}
	else
	{
		LOG_ERROR("[D3D12RootSignature]: FAILED to Create RootSignature");
		__debugbreak();

		return false;
	}
}
