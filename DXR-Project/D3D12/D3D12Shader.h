#pragma once
#include "RenderingCore/Shader.h"

#include "D3D12DeviceChild.h"

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
	{
	}

	~D3D12ComputeShader() = default;
};