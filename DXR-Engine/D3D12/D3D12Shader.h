#pragma once
#include "RenderLayer/Shader.h"

#include "D3D12RootSignature.h"
#include "D3D12Device.h"

class D3D12Shader : public D3D12DeviceChild
{
public:
    inline D3D12Shader(D3D12Device* InDevice, const TArray<UInt8>& InCode)
        : D3D12DeviceChild(InDevice)
        , Code(InCode)
        , ByteCode()
    {
        ByteCode.BytecodeLength  = Code.Size();
        ByteCode.pShaderBytecode = reinterpret_cast<const void*>(Code.Data());
    }

    FORCEINLINE D3D12_SHADER_BYTECODE GetShaderByteCode() const
    {
        return ByteCode;
    }

    FORCEINLINE const void* GetCodeData() const
    {
        return reinterpret_cast<const void*>(Code.Data());
    }

    FORCEINLINE UInt32 GetCodeSize() const
    {
        return Code.Size();
    }

protected:
    D3D12_SHADER_BYTECODE ByteCode;
    TArray<UInt8> Code;
};

class D3D12VertexShader : public VertexShader, public D3D12Shader
{
public:
    inline D3D12VertexShader(D3D12Device* InDevice, const TArray<UInt8>& InCode)
        : VertexShader()
        , D3D12Shader(InDevice, InCode)
    {
    }
};

class D3D12PixelShader : public PixelShader, public D3D12Shader
{
public:
    inline D3D12PixelShader(D3D12Device* InDevice, const TArray<UInt8>& InCode)
        : PixelShader()
        , D3D12Shader(InDevice, InCode)
    {
    }
};

class D3D12ComputeShader : public ComputeShader, public D3D12Shader
{
public:
    inline D3D12ComputeShader(D3D12Device* InDevice, const TArray<UInt8>& InCode)
        : ComputeShader()
        , D3D12Shader(InDevice, InCode)
        , RootSignature(nullptr)
    {
    }

    FORCEINLINE Bool CreateRootSignature()
    {
        // Create RootSignature if the shader contains one
        TComPtr<ID3D12RootSignatureDeserializer> Deserializer;
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
        return RootSignature.Get();
    }

protected:
    TSharedRef<D3D12RootSignature> RootSignature;
};
