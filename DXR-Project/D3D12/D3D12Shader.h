#pragma once
#include "RenderingCore/Shader.h"

#include "D3D12RootSignature.h"

/*
* D3D12Shader
*/

class D3D12Shader : public D3D12DeviceChild
{
public:
	inline D3D12Shader(D3D12Device* InDevice, const TArray<Uint8>& InCode)
		: D3D12DeviceChild(InDevice)
		, Code(InCode)
		, ByteCode()
	{
		ByteCode.BytecodeLength		= Code.Size();
		ByteCode.pShaderBytecode	= reinterpret_cast<const void*>(Code.Data());
	}

	~D3D12Shader() = default;

	FORCEINLINE D3D12_SHADER_BYTECODE GetShaderByteCode() const
	{
		return ByteCode;
	}

	FORCEINLINE const void* GetCodeData() const
	{
		return reinterpret_cast<const void*>(Code.Data());
	}

	FORCEINLINE Uint32 GetCodeSize() const
	{
		return Code.Size();
	}

protected:
	D3D12_SHADER_BYTECODE ByteCode;
	TArray<Uint8> Code;
};

/*
* D3D12VertexShader
*/

class D3D12VertexShader : public VertexShader, public D3D12Shader
{
public:
	inline D3D12VertexShader(D3D12Device* InDevice, const TArray<Uint8>& InCode)
		: VertexShader()
		, D3D12Shader(InDevice, InCode)
	{
	}

	~D3D12VertexShader() = default;
};

/*
* D3D12PixelShader
*/

class D3D12PixelShader : public PixelShader, public D3D12Shader
{
public:
	inline D3D12PixelShader(D3D12Device* InDevice, const TArray<Uint8>& InCode)
		: PixelShader()
		, D3D12Shader(InDevice, InCode)
	{
	}

	~D3D12PixelShader() = default;
};

/*
* D3D12ComputeShader
*/

class D3D12ComputeShader : public ComputeShader, public D3D12Shader
{
public:
	inline D3D12ComputeShader(D3D12Device* InDevice, const TArray<Uint8>& InCode)
		: ComputeShader()
		, D3D12Shader(InDevice, InCode)
		, RootSignature(nullptr)
	{
	}

	~D3D12ComputeShader() = default;

	/*
	* Creates a rootsignature if the shader contains one, this is only supported for Compute for now
	*/
	FORCEINLINE bool CreateRootSignature()
	{
		using namespace Microsoft::WRL;

		// Create RootSignature if the shader contains one
		ComPtr<ID3D12RootSignatureDeserializer> Deserializer;
		HRESULT hResult = Device->CreateRootSignatureDeserializer(
			ByteCode.pShaderBytecode,
			ByteCode.BytecodeLength,
			IID_PPV_ARGS(&Deserializer));
		if (SUCCEEDED(hResult))
		{
			LOG_INFO("[D3D12Shader]: Created ID3D12RootSignatureDeserializer");

			const D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = *Deserializer->GetRootSignatureDesc();
			RootSignature = Device->CreateRootSignature(RootSignatureDesc);
			if (!RootSignature)
			{
				LOG_INFO("[D3D12Shader]: FAILED to create RootSignature");
				return false;
			}
			else
			{
				LOG_INFO("[D3D12Shader]: Created RootSignature");
				return true;
			}
		}
		else
		{
			return false;
		}
	}

	FORCEINLINE D3D12RootSignature* GetRootSignature() const
	{
		return RootSignature;
	}

protected:
	D3D12RootSignature* RootSignature;
};
