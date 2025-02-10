#pragma once
#include "RHI/RHIShader.h"
#include "RHI/RHIResources.h"
#include "D3D12RHI/D3D12DeviceChild.h"
#include "D3D12RHI/D3D12Constants.h"

#include <d3d12shader.h>

enum EShaderVisibility : int32
{
    ShaderVisibility_All = 0,
    ShaderVisibility_Vertex,
    ShaderVisibility_Hull,
    ShaderVisibility_Domain,
    ShaderVisibility_Geometry,
    ShaderVisibility_Pixel,
    ShaderVisibility_Count = ShaderVisibility_Pixel + 1
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
    FShaderResourceRange()
        : NumCBVs(0)
        , NumSRVs(0)
        , NumUAVs(0)
        , NumSamplers(0)
    {
    }

    uint8 NumCBVs;
    uint8 NumSRVs;
    uint8 NumUAVs;
    uint8 NumSamplers;
};

struct FShaderResourceCount
{
    FShaderResourceCount()
        : Ranges()
        , Num32BitConstants(0)
    {
    }

    void Combine(const FShaderResourceCount& Other);
    bool IsCompatible(const FShaderResourceCount& Other) const;

    FShaderResourceRange Ranges;
    uint8                Num32BitConstants;
};

struct FD3D12ShaderHash
{
    // Hash retrieved from the shader ByteCode
    uint64 Hash[2] = { 0, 0 };

    friend uint64 GetHashForType(const FD3D12ShaderHash& Value)
    {
        uint64 Hash = Value.Hash[0];
        HashCombine(Hash, Value.Hash[1]);
        return Hash;
    }

    bool operator==(const FD3D12ShaderHash& Other) const
    {
        return Hash[0] == Other.Hash[0] && Hash[1] == Other.Hash[1];
    }

    bool operator!=(const FD3D12ShaderHash& Other) const
    {
        return Hash[0] != Other.Hash[0] || Hash[1] != Other.Hash[1];
    }
};

class FD3D12Shader : public FD3D12DeviceChild
{
public:
    FD3D12Shader(FD3D12Device* InDevice, const TArray<uint8>& InCode, EShaderVisibility InShaderVisibility);
    ~FD3D12Shader();

    static bool GetShaderReflection(class FD3D12Shader* Shader);

    EShaderVisibility GetShaderVisibility() const { return ShaderVisibility; }
    const FShaderResourceCount& GetResourceCount() const { return ResourceCount; }
    const FShaderResourceCount& GetRTLocalResourceCount() const { return RTLocalResourceCount; }
    D3D12_SHADER_BYTECODE GetByteCode() const { return ByteCode; }
    bool HasRootSignature() const { return bContainsRootSignature; }
    
    FORCEINLINE const void* GetCode() const
    {
        return ByteCode.pShaderBytecode;
    }

    FORCEINLINE uint64 GetCodeSize() const
    {
        return static_cast<uint64>(ByteCode.BytecodeLength);
    }

    FORCEINLINE FD3D12ShaderHash GetHash() const
    {
        return ByteCodeHash;
    }

protected:
    template<typename TD3D12ReflectionInterface>
    static bool GetShaderResourceBindings(TD3D12ReflectionInterface* Reflection, FD3D12Shader* Shader, uint32 NumBoundResources);

    D3D12_SHADER_BYTECODE ByteCode;
    FD3D12ShaderHash      ByteCodeHash;
    EShaderVisibility     ShaderVisibility;
    FShaderResourceCount  ResourceCount;
    FShaderResourceCount  RTLocalResourceCount;
    bool                  bContainsRootSignature = false;
};

class FD3D12VertexShader : public FRHIVertexShader, public FD3D12Shader
{
public:
    FD3D12VertexShader(FD3D12Device* InDevice, const TArray<uint8>& InCode)
        : FRHIVertexShader()
        , FD3D12Shader(InDevice, InCode, ShaderVisibility_Vertex)
    {
    }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(&ByteCode); }
    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<FD3D12Shader*>(this)); }
};

class FD3D12HullShader : public FRHIHullShader, public FD3D12Shader
{
public:
    FD3D12HullShader(FD3D12Device* InDevice, const TArray<uint8>& InCode)
        : FRHIHullShader()
        , FD3D12Shader(InDevice, InCode, ShaderVisibility_Hull)
    {
    }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(&ByteCode); }
    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<FD3D12Shader*>(this)); }
};

class FD3D12DomainShader : public FRHIDomainShader, public FD3D12Shader
{
public:
    FD3D12DomainShader(FD3D12Device* InDevice, const TArray<uint8>& InCode)
        : FRHIDomainShader()
        , FD3D12Shader(InDevice, InCode, ShaderVisibility_Domain)
    {
    }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(&ByteCode); }
    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<FD3D12Shader*>(this)); }
};

class FD3D12GeometryShader : public FRHIGeometryShader, public FD3D12Shader
{
public:
    FD3D12GeometryShader(FD3D12Device* InDevice, const TArray<uint8>& InCode)
        : FRHIGeometryShader()
        , FD3D12Shader(InDevice, InCode, ShaderVisibility_Geometry)
    {
    }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(&ByteCode); }
    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<FD3D12Shader*>(this)); }
};

class FD3D12PixelShader : public FRHIPixelShader, public FD3D12Shader
{
public:
    FD3D12PixelShader(FD3D12Device* InDevice, const TArray<uint8>& InCode)
        : FRHIPixelShader()
        , FD3D12Shader(InDevice, InCode, ShaderVisibility_Pixel)
    {
    }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(&ByteCode); }
    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<FD3D12Shader*>(this)); }
};

class FD3D12RayTracingShader : public FD3D12Shader
{
public:
    FD3D12RayTracingShader(FD3D12Device* InDevice, const TArray<uint8>& InCode)
        : FD3D12Shader(InDevice, InCode, ShaderVisibility_All)
    {
    }

    static bool GetRayTracingShaderReflection(class FD3D12RayTracingShader* Shader);
    
    FORCEINLINE const FString& GetIdentifier() const
    {
        return Identifier;
    }

protected:
    FString Identifier;
};

class FD3D12RayGenShader : public FRHIRayGenShader, public FD3D12RayTracingShader
{
public:
    FD3D12RayGenShader(FD3D12Device* InDevice, const TArray<uint8>& InCode)
        : FRHIRayGenShader()
        , FD3D12RayTracingShader(InDevice, InCode)
    {
    }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(&ByteCode); }
    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<FD3D12RayTracingShader*>(this)); }
};

class FD3D12RayAnyHitShader : public FRHIRayAnyHitShader, public FD3D12RayTracingShader
{
public:
    FD3D12RayAnyHitShader(FD3D12Device* InDevice, const TArray<uint8>& InCode)
        : FRHIRayAnyHitShader()
        , FD3D12RayTracingShader(InDevice, InCode)
    {
    }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(&ByteCode); }
    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<FD3D12RayTracingShader*>(this)); }
};

class FD3D12RayClosestHitShader : public FRHIRayClosestHitShader, public FD3D12RayTracingShader
{
public:
    FD3D12RayClosestHitShader(FD3D12Device* InDevice, const TArray<uint8>& InCode)
        : FRHIRayClosestHitShader()
        , FD3D12RayTracingShader(InDevice, InCode)
    {
    }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(&ByteCode); }
    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<FD3D12RayTracingShader*>(this)); }
};

class FD3D12RayMissShader : public FRHIRayMissShader, public FD3D12RayTracingShader
{
public:
    FD3D12RayMissShader(FD3D12Device* InDevice, const TArray<uint8>& InCode)
        : FRHIRayMissShader()
        , FD3D12RayTracingShader(InDevice, InCode)
    {
    }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(&ByteCode); }
    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<FD3D12RayTracingShader*>(this)); }
};

class FD3D12ComputeShader : public FRHIComputeShader, public FD3D12Shader
{
public:
    FD3D12ComputeShader(FD3D12Device* InDevice, const TArray<uint8>& InCode)
        : FRHIComputeShader()
        , FD3D12Shader(InDevice, InCode, ShaderVisibility_All)
        , ThreadGroupXYZ(0, 0, 0)
    {
    }

    bool Initialize();

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(&ByteCode); }
    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<FD3D12Shader*>(this)); }

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
