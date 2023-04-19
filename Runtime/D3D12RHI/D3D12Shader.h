#pragma once
#include "D3D12DeviceChild.h"
#include "D3D12Constants.h"
#include "RHI/RHIShader.h"
#include "RHI/RHIResources.h"

#include <d3d12shader.h>

enum EShaderVisibility : int32
{
    ShaderVisibility_All      = 0,
    ShaderVisibility_Vertex   = 1,
    ShaderVisibility_Hull     = 2,
    ShaderVisibility_Domain   = 3,
    ShaderVisibility_Geometry = 4,
    ShaderVisibility_Pixel    = 5,
    ShaderVisibility_Count    = ShaderVisibility_Pixel + 1
};


enum EResourceType : int32
{
    ResourceType_CBV     = 0,
    ResourceType_SRV     = 1,
    ResourceType_UAV     = 2,
    ResourceType_Sampler = 3,
    ResourceType_Count   = ResourceType_Sampler + 1,
    ResourceType_Unknown = 5,
};


struct FShaderResourceRange
{
    uint32 NumCBVs     = 0;
    uint32 NumSRVs     = 0;
    uint32 NumUAVs     = 0;
    uint32 NumSamplers = 0;
};


struct FShaderResourceCount
{
    void Combine(const FShaderResourceCount& Other);
    bool IsCompatible(const FShaderResourceCount& Other) const;

    FShaderResourceRange Ranges;
    uint32               Num32BitConstants = 0;
};

struct FD3D12ShaderParameter
{
    FD3D12ShaderParameter() = default;

    FORCEINLINE FD3D12ShaderParameter(const FString& InName, uint32 InRegister, uint32 InSpace, uint32 InNumDescriptors, uint32 InSizeInBytes)
        : Name(InName)
        , Register(InRegister)
        , Space(InSpace)
        , NumDescriptors(InNumDescriptors)
        , SizeInBytes(InSizeInBytes)
    { }

    FString Name;
    uint32 Register       = 0;
    uint32 Space          = 0;
    uint32 NumDescriptors = 0;
    uint32 SizeInBytes    = 0;
};


class FD3D12Shader 
    : public FD3D12DeviceChild
{
public:
    FD3D12Shader(FD3D12Device* InDevice, const TArray<uint8>& InCode, EShaderVisibility ShaderVisibility);
    ~FD3D12Shader();

public:
    static bool GetShaderReflection(class FD3D12Shader* Shader);

    EShaderVisibility GetShaderVisibility() const { return Visibility; }

    const FShaderResourceCount& GetResourceCount()        const { return ResourceCount; }
    const FShaderResourceCount& GetRTLocalResourceCount() const { return RTLocalResourceCount; }

    D3D12_SHADER_BYTECODE GetByteCode() const { return ByteCode; }

    FORCEINLINE bool HasRootSignature() const { return bContainsRootSignature; }
    
    FORCEINLINE const void* GetCode() const { return ByteCode.pShaderBytecode; }

    FORCEINLINE uint64 GetCodeSize() const { return static_cast<uint64>(ByteCode.BytecodeLength); }

    FORCEINLINE FD3D12ShaderParameter GetConstantBufferParameter(uint32 ParameterIndex)
    {
        D3D12_ERROR_COND(
            ParameterIndex < static_cast<uint32>(ConstantBufferParameters.Size()),
            "Trying to access ParameterIndex=%u, but the shader only has %u slots",
            ParameterIndex, 
            ConstantBufferParameters.Size());
        return ConstantBufferParameters[ParameterIndex];
    }

    FORCEINLINE uint32 GetNumConstantBufferParameters() { return ConstantBufferParameters.Size(); }

    FORCEINLINE FD3D12ShaderParameter GetShaderResourceParameter(uint32 ParameterIndex)
    {
        D3D12_ERROR_COND(
            ParameterIndex < static_cast<uint32>(ShaderResourceParameters.Size()),
            "Trying to access ParameterIndex=%u, but the shader only has %u slots",
            ParameterIndex, 
            ShaderResourceParameters.Size());
        return ShaderResourceParameters[ParameterIndex];
    }

    FORCEINLINE uint32 GetNumShaderResourceParameters() { return ShaderResourceParameters.Size(); }

    FORCEINLINE FD3D12ShaderParameter GetUnorderedAccessParameter(uint32 ParameterIndex)
    {
        D3D12_ERROR_COND(
            ParameterIndex < static_cast<uint32>(UnorderedAccessParameters.Size()),
            "Trying to access ParameterIndex=%u, but the shader only has %u slots",
            ParameterIndex,
            UnorderedAccessParameters.Size());
        return UnorderedAccessParameters[ParameterIndex];
    }

    FORCEINLINE uint32 GetNumUnorderedAccessParameters() { return UnorderedAccessParameters.Size(); }

    FORCEINLINE FD3D12ShaderParameter GetSamplerStateParameter(uint32 ParameterIndex)
    {
        D3D12_ERROR_COND(
            ParameterIndex < static_cast<uint32>(SamplerParameters.Size()),
            "Trying to access ParameterIndex=%u, but the shader only has %u slots",
            ParameterIndex,
            SamplerParameters.Size());
        return SamplerParameters[ParameterIndex];
    }

    FORCEINLINE uint32 GetNumSamplerStateParameters() { return SamplerParameters.Size(); }

protected:
    template<typename TD3D12ReflectionInterface>
    static bool GetShaderResourceBindings(TD3D12ReflectionInterface* Reflection, FD3D12Shader* Shader, uint32 NumBoundResources);

    D3D12_SHADER_BYTECODE         ByteCode;

    TArray<FD3D12ShaderParameter> ConstantBufferParameters;
    TArray<FD3D12ShaderParameter> ShaderResourceParameters;
    TArray<FD3D12ShaderParameter> UnorderedAccessParameters;
    TArray<FD3D12ShaderParameter> SamplerParameters;

    EShaderVisibility             Visibility;
    FShaderResourceCount          ResourceCount;
    FShaderResourceCount          RTLocalResourceCount;

    bool bContainsRootSignature = false;
};


class FD3D12VertexShader 
    : public FRHIVertexShader
    , public FD3D12Shader
{
public:
    FD3D12VertexShader(FD3D12Device* InDevice, const TArray<uint8>& InCode)
        : FRHIVertexShader()
        , FD3D12Shader(InDevice, InCode, ShaderVisibility_Vertex)
    { }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(&ByteCode); }
    virtual void* GetRHIBaseShader()   override final { return reinterpret_cast<void*>(static_cast<FD3D12Shader*>(this)); }
};


class FD3D12PixelShader 
    : public FRHIPixelShader
    , public FD3D12Shader
{
public:
    FD3D12PixelShader(FD3D12Device* InDevice, const TArray<uint8>& InCode)
        : FRHIPixelShader()
        , FD3D12Shader(InDevice, InCode, ShaderVisibility_Pixel)
    { }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(&ByteCode); }
    virtual void* GetRHIBaseShader()   override final { return reinterpret_cast<void*>(static_cast<FD3D12Shader*>(this)); }
};


class FD3D12RayTracingShader 
    : public FD3D12Shader
{
public:
    FD3D12RayTracingShader(FD3D12Device* InDevice, const TArray<uint8>& InCode)
        : FD3D12Shader(InDevice, InCode, ShaderVisibility_All)
    { }

    static bool GetRayTracingShaderReflection(class FD3D12RayTracingShader* Shader);
    
    FORCEINLINE const FString& GetIdentifier() const { return Identifier; }

protected:
    FString Identifier;
};


class FD3D12RayGenShader 
    : public FRHIRayGenShader
    , public FD3D12RayTracingShader
{
public:
    FD3D12RayGenShader(FD3D12Device* InDevice, const TArray<uint8>& InCode)
        : FRHIRayGenShader()
        , FD3D12RayTracingShader(InDevice, InCode)
    { }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(&ByteCode); }
    virtual void* GetRHIBaseShader()   override final { return reinterpret_cast<void*>(static_cast<FD3D12RayTracingShader*>(this)); }
};


class FD3D12RayAnyHitShader 
    : public FRHIRayAnyHitShader
    , public FD3D12RayTracingShader
{
public:
    FD3D12RayAnyHitShader(FD3D12Device* InDevice, const TArray<uint8>& InCode)
        : FRHIRayAnyHitShader()
        , FD3D12RayTracingShader(InDevice, InCode)
    { }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(&ByteCode); }
    virtual void* GetRHIBaseShader()   override final { return reinterpret_cast<void*>(static_cast<FD3D12RayTracingShader*>(this)); }
};


class FD3D12RayClosestHitShader 
    : public FRHIRayClosestHitShader
    , public FD3D12RayTracingShader
{
public:
    FD3D12RayClosestHitShader(FD3D12Device* InDevice, const TArray<uint8>& InCode)
        : FRHIRayClosestHitShader()
        , FD3D12RayTracingShader(InDevice, InCode)
    { }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(&ByteCode); }
    virtual void* GetRHIBaseShader()   override final { return reinterpret_cast<void*>(static_cast<FD3D12RayTracingShader*>(this)); }
};


class FD3D12RayMissShader 
    : public FRHIRayMissShader
    , public FD3D12RayTracingShader
{
public:
    FD3D12RayMissShader(FD3D12Device* InDevice, const TArray<uint8>& InCode)
        : FRHIRayMissShader()
        , FD3D12RayTracingShader(InDevice, InCode)
    { }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(&ByteCode); }
    virtual void* GetRHIBaseShader()   override final { return reinterpret_cast<void*>(static_cast<FD3D12RayTracingShader*>(this)); }
};


class FD3D12ComputeShader : public FRHIComputeShader, public FD3D12Shader
{
public:
    FD3D12ComputeShader(FD3D12Device* InDevice, const TArray<uint8>& InCode)
        : FRHIComputeShader()
        , FD3D12Shader(InDevice, InCode, ShaderVisibility_All)
        , ThreadGroupXYZ(0, 0, 0)
    { }

    bool Initialize();

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(&ByteCode); }
    virtual void* GetRHIBaseShader()   override final { return reinterpret_cast<void*>(static_cast<FD3D12Shader*>(this)); }

    virtual FIntVector3 GetThreadGroupXYZ() const override final { return ThreadGroupXYZ; }

protected:
    FIntVector3 ThreadGroupXYZ;
};


inline FD3D12Shader* GetD3D12Shader(FRHIShader* Shader)
{
    return Shader ? reinterpret_cast<FD3D12Shader*>(Shader->GetRHIBaseShader()) : nullptr;
}

inline FD3D12RayTracingShader* GetD3D12RayTracingShader(FRHIRayTracingShader* Shader)
{
    return Shader ? reinterpret_cast<FD3D12RayTracingShader*>(Shader->GetRHIBaseShader()) : nullptr;
}
