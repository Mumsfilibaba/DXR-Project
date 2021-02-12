#pragma once
#include "RenderLayer/Resources.h"

#include "D3D12RootSignature.h"
#include "D3D12Device.h"

class D3D12BaseShader : public D3D12DeviceChild
{
public:
    D3D12BaseShader(D3D12Device* InDevice, const TArray<UInt8>& InCode)
        : D3D12DeviceChild(InDevice)
        , Code(InCode)
        , ByteCode()
    {
        ByteCode.BytecodeLength  = Code.Size();
        ByteCode.pShaderBytecode = reinterpret_cast<const void*>(Code.Data());
    }

    const void* GetCodeData() const { return reinterpret_cast<const void*>(Code.Data()); }
    UInt32 GetCodeSize() const { return Code.Size(); }

    D3D12_SHADER_BYTECODE GetShaderByteCode() const { return ByteCode; }

protected:
    D3D12_SHADER_BYTECODE ByteCode;
    TArray<UInt8> Code;
};

class D3D12BaseVertexShader : public VertexShader, public D3D12BaseShader
{
public:
    D3D12BaseVertexShader(D3D12Device* InDevice, const TArray<UInt8>& InCode)
        : VertexShader()
        , D3D12BaseShader(InDevice, InCode)
    {
    }
};

class D3D12BasePixelShader : public PixelShader, public D3D12BaseShader
{
public:
    D3D12BasePixelShader(D3D12Device* InDevice, const TArray<UInt8>& InCode)
        : PixelShader()
        , D3D12BaseShader(InDevice, InCode)
    {
    }
};

class D3D12BaseRayTracingShader : public D3D12BaseShader
{
public:
    D3D12BaseRayTracingShader(D3D12Device* InDevice, const TArray<UInt8>& InCode)
        : D3D12BaseShader(InDevice, InCode)
    {
    }

    std::string Identifier;
};

class D3D12BaseRayGenShader : public RayGenShader, public D3D12BaseRayTracingShader
{
public:
    D3D12BaseRayGenShader(D3D12Device* InDevice, const TArray<UInt8>& InCode)
        : RayGenShader()
        , D3D12BaseRayTracingShader(InDevice, InCode)
    {
    }
};

class D3D12BaseRayAnyhitShader : public RayAnyHitShader, public D3D12BaseRayTracingShader
{
public:
    D3D12BaseRayAnyhitShader(D3D12Device* InDevice, const TArray<UInt8>& InCode)
        : RayAnyHitShader()
        , D3D12BaseRayTracingShader(InDevice, InCode)
    {
    }
};

class D3D12BaseRayClosestHitShader : public RayClosestHitShader, public D3D12BaseRayTracingShader
{
public:
    D3D12BaseRayClosestHitShader(D3D12Device* InDevice, const TArray<UInt8>& InCode)
        : RayClosestHitShader()
        , D3D12BaseRayTracingShader(InDevice, InCode)
    {
    }
};

class D3D12BaseRayMissShader : public RayMissShader, public D3D12BaseRayTracingShader
{
public:
    D3D12BaseRayMissShader(D3D12Device* InDevice, const TArray<UInt8>& InCode)
        : RayMissShader()
        , D3D12BaseRayTracingShader(InDevice, InCode)
    {
    }
};

class D3D12BaseComputeShader : public ComputeShader, public D3D12BaseShader
{
public:
    D3D12BaseComputeShader(D3D12Device* InDevice, const TArray<UInt8>& InCode)
        : ComputeShader()
        , D3D12BaseShader(InDevice, InCode)
        , RootSignature(nullptr)
    {
    }

    Bool CreateRootSignature()
    {
        // Create RootSignature if the shader contains one
        TComPtr<ID3D12RootSignatureDeserializer> Deserializer;
        HRESULT hResult = Device->CreateRootSignatureDeserializer(
            ByteCode.pShaderBytecode,
            ByteCode.BytecodeLength,
            IID_PPV_ARGS(&Deserializer));
        if (SUCCEEDED(hResult))
        {
            LOG_INFO("[D3D12BaseShader]: Created ID3D12RootSignatureDeserializer");

            const D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = *Deserializer->GetRootSignatureDesc();
            RootSignature = Device->CreateRootSignature(RootSignatureDesc);
            if (!RootSignature)
            {
                LOG_INFO("[D3D12BaseShader]: FAILED to create RootSignature");
                return false;
            }
            else
            {
                LOG_INFO("[D3D12BaseShader]: Created RootSignature");
                return true;
            }
        }
        else
        {
            return false;
        }
    }

    D3D12RootSignature* GetRootSignature() const { return RootSignature.Get(); }

protected:
    TRef<D3D12RootSignature> RootSignature;
};

template<typename TBaseShader>
class TD3D12Shader : public TBaseShader
{
public:
    TD3D12Shader(D3D12Device* InDevice, const TArray<UInt8>& InCode)
        : TBaseShader(InDevice, InCode)
    {
    }

    virtual Bool IsValid() const override
    {
        return Code.Data() != nullptr && Code.Size() > 0;
    }
};

using D3D12VertexShader = TD3D12Shader<D3D12BaseVertexShader>;
using D3D12PixelShader  = TD3D12Shader<D3D12BasePixelShader>;

using D3D12ComputeShader = TD3D12Shader<D3D12BaseComputeShader>;

using D3D12RayGenShader        = TD3D12Shader<D3D12BaseRayGenShader>;
using D3D12RayAnyhitShader     = TD3D12Shader<D3D12BaseRayAnyhitShader>;
using D3D12RayClosestHitShader = TD3D12Shader<D3D12BaseRayClosestHitShader>;
using D3D12RayMissShader       = TD3D12Shader<D3D12BaseRayMissShader>;