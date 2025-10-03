#pragma once
#include "RHI/RHIShader.h"
#include "RHI/RHIResources.h"
#include "RHI/ShaderCompilerInclude.h"
#include "D3D12RHI/D3D12DeviceChild.h"
#include "D3D12RHI/D3D12Constants.h"
#include <d3d12shader.h>

typedef TSharedRef<class FD3D12Shader>              FD3D12ShaderRef;
typedef TSharedRef<class FD3D12VertexShader>        FD3D12VertexShaderRef;
typedef TSharedRef<class FD3D12HullShader>          FD3D12HullShaderRef;
typedef TSharedRef<class FD3D12DomainShader>        FD3D12DomainShaderRef;
typedef TSharedRef<class FD3D12GeometryShader>      FD3D12GeometryShaderRef;
typedef TSharedRef<class FD3D12PixelShader>         FD3D12PixelShaderRef;
typedef TSharedRef<class FD3D12ComputeShader>       FD3D12ComputeShaderRef;
typedef TSharedRef<class FD3D12RayTracingShader>    FD3D12RayTracingShaderRef;
typedef TSharedRef<class FD3D12RayGenShader>        FD3D12RayGenShaderRef;
typedef TSharedRef<class FD3D12RayAnyHitShader>     FD3D12RayAnyHitShaderRef;
typedef TSharedRef<class FD3D12RayClosestHitShader> FD3D12RayClosestHitShaderRef;
typedef TSharedRef<class FD3D12RayMissShader>       FD3D12RayMissShaderRef;

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

    bool operator==(const FD3D12ShaderHash& Other) const
    {
        return Hash[0] == Other.Hash[0] && Hash[1] == Other.Hash[1];
    }

    bool operator!=(const FD3D12ShaderHash& Other) const
    {
        return Hash[0] != Other.Hash[0] || Hash[1] != Other.Hash[1];
    }

    friend uint64 GetHashForType(const FD3D12ShaderHash& Value)
    {
        uint64 Hash = Value.Hash[0];
        HashCombine(Hash, Value.Hash[1]);
        return Hash;
    }
};

enum ED3D12BindingType : uint8
{
    D3D12BindingType_ConstantBuffer = 0,
    D3D12BindingType_SRV,
    D3D12BindingType_UAV,
    D3D12BindingType_Sampler,
    D3D12BindingType_Count = D3D12BindingType_Sampler + 1,
};

struct FD3D12ShaderInfo
{   
    struct FResourceBinding
    {
        FString DebugName;
        ED3D12BindingType BindingType;
        uint8 BindingIndex;
        uint8 OriginalBindingIndex;
    };
    
    TArray<FResourceBinding> ResourceBindings;
    uint32 NumPushConstants;
};

class FD3D12Shader : public FD3D12DeviceChild
{
public:
    FD3D12Shader(FD3D12Device* InDevice, EShaderVisibility InShaderVisibility);
    ~FD3D12Shader();

	virtual bool Initialize(const TArray<uint8>& InCode);

    const FShaderResourceCount& GetResourceCount() const { return ResourceCount; }
    const FShaderResourceCount& GetLocalRayTracingResourceCount() const { return LocalRayTracingResourceCount; }
    const D3D12_SHADER_BYTECODE& GetByteCode() const { return ByteCode; }
    EShaderVisibility GetShaderVisibility() const { return ShaderVisibility; }

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
    bool IsRootSignatureInShaderBlob(const TComPtr<IDxcBlob>& ShaderBlob);
    bool GetReflectionInterface(const TComPtr<IDxcBlob>& ShaderBlob, REFIID iid, void** ppvObject);

    template<typename TD3D12ReflectionInterface>
    bool GetShaderResourceBindings(TD3D12ReflectionInterface* Reflection, uint32 NumBoundResources);

    D3D12_SHADER_BYTECODE ByteCode;
    FD3D12ShaderHash      ByteCodeHash;
    EShaderVisibility     ShaderVisibility;
    FShaderResourceCount  ResourceCount;
    FShaderResourceCount  LocalRayTracingResourceCount;
    bool                  bContainsRootSignature;
};

class FD3D12GraphicsShader : public FD3D12Shader
{
public:
    FD3D12GraphicsShader(FD3D12Device* InDevice, EShaderVisibility InShaderVisibility)
		: FD3D12Shader(InDevice, InShaderVisibility)
	{
	}

    virtual bool Initialize(const TArray<uint8>& InCode) override final;
};

class FD3D12VertexShader : public FRHIVertexShader, public FD3D12GraphicsShader
{
public:
    FD3D12VertexShader(FD3D12Device* InDevice)
        : FRHIVertexShader()
        , FD3D12GraphicsShader(InDevice, ShaderVisibility_Vertex)
    {
    }

    // FRHIShader Interface
    virtual void* GetRHINativeHandle() override final { return reinterpret_cast<void*>(&ByteCode); }
    virtual void* GetRHIBaseInterface() override final { return static_cast<FD3D12GraphicsShader*>(this); }
};

class FD3D12HullShader : public FRHIHullShader, public FD3D12GraphicsShader
{
public:
    FD3D12HullShader(FD3D12Device* InDevice)
        : FRHIHullShader()
        , FD3D12GraphicsShader(InDevice, ShaderVisibility_Hull)
    {
    }

	// FRHIShader Interface
    virtual void* GetRHINativeHandle() override final { return reinterpret_cast<void*>(&ByteCode); }
    virtual void* GetRHIBaseInterface() override final { return static_cast<FD3D12GraphicsShader*>(this); }
};

class FD3D12DomainShader : public FRHIDomainShader, public FD3D12GraphicsShader
{
public:
    FD3D12DomainShader(FD3D12Device* InDevice)
        : FRHIDomainShader()
        , FD3D12GraphicsShader(InDevice, ShaderVisibility_Domain)
    {
    }

	// FRHIShader Interface
    virtual void* GetRHINativeHandle() override final { return reinterpret_cast<void*>(&ByteCode); }
    virtual void* GetRHIBaseInterface() override final { return static_cast<FD3D12GraphicsShader*>(this); }
};

class FD3D12GeometryShader : public FRHIGeometryShader, public FD3D12GraphicsShader
{
public:
    FD3D12GeometryShader(FD3D12Device* InDevice)
        : FRHIGeometryShader()
        , FD3D12GraphicsShader(InDevice, ShaderVisibility_Geometry)
    {
    }

	// FRHIShader Interface
    virtual void* GetRHINativeHandle() override final { return reinterpret_cast<void*>(&ByteCode); }
    virtual void* GetRHIBaseInterface() override final { return static_cast<FD3D12GraphicsShader*>(this); }
};

class FD3D12PixelShader : public FRHIPixelShader, public FD3D12GraphicsShader
{
public:
    FD3D12PixelShader(FD3D12Device* InDevice)
        : FRHIPixelShader()
        , FD3D12GraphicsShader(InDevice, ShaderVisibility_Pixel)
    {
    }

	// FRHIShader Interface
    virtual void* GetRHINativeHandle() override final { return reinterpret_cast<void*>(&ByteCode); }
    virtual void* GetRHIBaseInterface() override final { return static_cast<FD3D12GraphicsShader*>(this); }
};

class FD3D12RayTracingShader : public FD3D12Shader
{
public:
    FD3D12RayTracingShader(FD3D12Device* InDevice)
        : FD3D12Shader(InDevice, ShaderVisibility_All)
    {
    }

    virtual bool Initialize(const TArray<uint8>& InCode) override final;
    
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
    FD3D12RayGenShader(FD3D12Device* InDevice)
        : FRHIRayGenShader()
        , FD3D12RayTracingShader(InDevice)
    {
    }

	// FRHIShader Interface
    virtual void* GetRHINativeHandle() override final { return reinterpret_cast<void*>(&ByteCode); }
    virtual void* GetRHIBaseInterface() override final { return static_cast<FD3D12RayTracingShader*>(this); }
};

class FD3D12RayAnyHitShader : public FRHIRayAnyHitShader, public FD3D12RayTracingShader
{
public:
    FD3D12RayAnyHitShader(FD3D12Device* InDevice)
        : FRHIRayAnyHitShader()
        , FD3D12RayTracingShader(InDevice)
    {
    }

	// FRHIShader Interface
    virtual void* GetRHINativeHandle() override final { return reinterpret_cast<void*>(&ByteCode); }
    virtual void* GetRHIBaseInterface() override final { return static_cast<FD3D12RayTracingShader*>(this); }
};

class FD3D12RayClosestHitShader : public FRHIRayClosestHitShader, public FD3D12RayTracingShader
{
public:
    FD3D12RayClosestHitShader(FD3D12Device* InDevice)
        : FRHIRayClosestHitShader()
        , FD3D12RayTracingShader(InDevice)
    {
    }

	// FRHIShader Interface
    virtual void* GetRHINativeHandle() override final { return reinterpret_cast<void*>(&ByteCode); }
    virtual void* GetRHIBaseInterface() override final { return static_cast<FD3D12RayTracingShader*>(this); }
};

class FD3D12RayMissShader : public FRHIRayMissShader, public FD3D12RayTracingShader
{
public:
    FD3D12RayMissShader(FD3D12Device* InDevice)
        : FRHIRayMissShader()
        , FD3D12RayTracingShader(InDevice)
    {
    }

	// FRHIShader Interface
    virtual void* GetRHINativeHandle() override final { return reinterpret_cast<void*>(&ByteCode); }
    virtual void* GetRHIBaseInterface() override final { return static_cast<FD3D12RayTracingShader*>(this); }
};

class FD3D12ComputeShader : public FRHIComputeShader, public FD3D12Shader
{
public:
    FD3D12ComputeShader(FD3D12Device* InDevice)
        : FRHIComputeShader()
        , FD3D12Shader(InDevice, ShaderVisibility_All)
        , ThreadGroupXYZ(0, 0, 0)
    {
    }

    virtual bool Initialize(const TArray<uint8>& InCode) override final;

	// FRHIShader Interface
    virtual void* GetRHINativeHandle() override final { return reinterpret_cast<void*>(&ByteCode); }
    virtual void* GetRHIBaseInterface() override final { return static_cast<FD3D12Shader*>(this); }

protected:
    FIntVector3 ThreadGroupXYZ;
};

NODISCARD inline FD3D12Shader* GetD3D12Shader(FRHIShader* Shader)
{
    return Shader ? reinterpret_cast<FD3D12Shader*>(Shader->GetRHIBaseInterface()) : nullptr;
}

NODISCARD inline FD3D12RayTracingShader* GetD3D12RayTracingShader(FRHIRayTracingShader* Shader)
{
    return Shader ? reinterpret_cast<FD3D12RayTracingShader*>(Shader->GetRHIBaseInterface()) : nullptr;
}
