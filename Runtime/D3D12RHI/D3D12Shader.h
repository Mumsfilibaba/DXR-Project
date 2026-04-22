#pragma once
#include "RHI/RHIShader.h"
#include "RHI/RHIResources.h"
#include "RHI/ShaderCompilerInclude.h"
#include "D3D12RHI/D3D12DeviceChild.h"
#include "D3D12RHI/D3D12Constants.h"
#include <d3d12shader.h>

typedef TSharedRef<class FD3D12Shader>                 FD3D12ShaderRef;
typedef TSharedRef<class FD3D12VertexShaderRHI>        FD3D12VertexShaderRHIRef;
typedef TSharedRef<class FD3D12HullShaderRHI>          FD3D12HullShaderRHIRef;
typedef TSharedRef<class FD3D12DomainShaderRHI>        FD3D12DomainShaderRHIRef;
typedef TSharedRef<class FD3D12GeometryShaderRHI>      FD3D12GeometryShaderRHIRef;
typedef TSharedRef<class FD3D12PixelShaderRHI>         FD3D12PixelShaderRHIRef;
typedef TSharedRef<class FD3D12ComputeShaderRHI>       FD3D12ComputeShaderRHIRef;
typedef TSharedRef<class FD3D12RayTracingShader>       FD3D12RayTracingShaderRef;
typedef TSharedRef<class FD3D12RayGenShaderRHI>        FD3D12RayGenShaderRHIRef;
typedef TSharedRef<class FD3D12RayAnyHitShaderRHI>     FD3D12RayAnyHitShaderRHIRef;
typedef TSharedRef<class FD3D12RayClosestHitShaderRHI> FD3D12RayClosestHitShaderRHIRef;
typedef TSharedRef<class FD3D12RayMissShaderRHI>       FD3D12RayMissShaderRHIRef;

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

struct FD3D12ShaderBytecode
{
    FD3D12ShaderBytecode();
    FD3D12ShaderBytecode(const TArray<uint8>& InCode);
    FD3D12ShaderBytecode(const FD3D12ShaderBytecode& Other);
    FD3D12ShaderBytecode(FD3D12ShaderBytecode&& Other);
    ~FD3D12ShaderBytecode();
    
    FORCEINLINE const void* GetCode() const
    {
        return ByteCode.pShaderBytecode;
    }

    FORCEINLINE uint64 GetCodeSize() const
    {
        return static_cast<uint64>(ByteCode.BytecodeLength);
    }
    
    const D3D12_SHADER_BYTECODE& GetD3D12Bytecode() const
    {
        return ByteCode;
    }

    FD3D12ShaderBytecode& operator=(const FD3D12ShaderBytecode& Other);
    FD3D12ShaderBytecode& operator=(FD3D12ShaderBytecode&& Other);

private:
    D3D12_SHADER_BYTECODE ByteCode;
};

enum ED3D12BindingType : uint8
{
    D3D12BindingType_ConstantBuffer = 0,
    D3D12BindingType_SRV,
    D3D12BindingType_UAV,
    D3D12BindingType_Sampler,
    D3D12BindingType_Count = D3D12BindingType_Sampler + 1,
};

struct FD3D12ShaderBindingInfo
{   
    struct FResourceBinding
    {
        ED3D12BindingType BindingType;
        uint8             BindingIndex;
        uint16            OriginalBindingIndex;
        FString           DebugName;
    };
    
    void AddBinding(ED3D12BindingType InType, uint16 InOriginalBindingIndex, const FString& InDebugName)
    {
        FResourceBinding& Binding    = ResourceBindings.Emplace();
        Binding.BindingType          = InType;
        Binding.BindingIndex         = 0;
        Binding.OriginalBindingIndex = InOriginalBindingIndex;
        Binding.DebugName            = InDebugName;
    }

    TArray<FResourceBinding> ResourceBindings;
    uint32                   NumPushConstants = 0;
};

class FD3D12Shader : public FD3D12DeviceChild
{
public:
    FD3D12Shader(FD3D12Device* InDevice, EShaderVisibility InShaderVisibility);
    ~FD3D12Shader();

	virtual bool Initialize(const TArray<uint8>& InCode);

    bool HasRootSignature() const { return bContainsRootSignature; }
    
    const FD3D12ShaderBytecode&    GetByteCode()    const { return ByteCode; }
    const FD3D12ShaderBindingInfo& GetBindingInfo() const { return BindingInfo; }
    
    EShaderVisibility GetShaderVisibility() const { return ShaderVisibility; }

    FORCEINLINE FD3D12ShaderHash GetHash() const
    {
        return ByteCodeHash;
    }

protected:
    bool IsRootSignatureInShaderBlob(const TComPtr<IDxcBlob>& ShaderBlob);
    bool GetReflectionInterface(const TComPtr<IDxcBlob>& ShaderBlob, REFIID iid, void** ppvObject);

    bool GetShaderResourceBindings(ID3D12ShaderReflection* Reflection, uint32 NumBoundResources);

    FD3D12ShaderBytecode     ByteCode;
    FD3D12ShaderHash         ByteCodeHash;
    EShaderVisibility        ShaderVisibility;
    FD3D12ShaderBindingInfo  BindingInfo;
    bool                     bContainsRootSignature;
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

class FD3D12VertexShaderRHI : public FRHIVertexShader, public FD3D12GraphicsShader
{
public:
    FD3D12VertexShaderRHI(FD3D12Device* InDevice)
        : FRHIVertexShader()
        , FD3D12GraphicsShader(InDevice, ShaderVisibility_Vertex)
    {
    }

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final { return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode()); }
    virtual void* GetRHIBaseInterface() override final { return static_cast<FD3D12GraphicsShader*>(this); }
};

class FD3D12HullShaderRHI : public FRHIHullShader, public FD3D12GraphicsShader
{
public:
    FD3D12HullShaderRHI(FD3D12Device* InDevice)
        : FRHIHullShader()
        , FD3D12GraphicsShader(InDevice, ShaderVisibility_Hull)
    {
    }

	// FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final { return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode()); }
    virtual void* GetRHIBaseInterface() override final { return static_cast<FD3D12GraphicsShader*>(this); }
};

class FD3D12DomainShaderRHI : public FRHIDomainShader, public FD3D12GraphicsShader
{
public:
    FD3D12DomainShaderRHI(FD3D12Device* InDevice)
        : FRHIDomainShader()
        , FD3D12GraphicsShader(InDevice, ShaderVisibility_Domain)
    {
    }

	// FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final { return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode()); }
    virtual void* GetRHIBaseInterface() override final { return static_cast<FD3D12GraphicsShader*>(this); }
};

class FD3D12GeometryShaderRHI : public FRHIGeometryShader, public FD3D12GraphicsShader
{
public:
    FD3D12GeometryShaderRHI(FD3D12Device* InDevice)
        : FRHIGeometryShader()
        , FD3D12GraphicsShader(InDevice, ShaderVisibility_Geometry)
    {
    }

	// FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final { return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode()); }
    virtual void* GetRHIBaseInterface() override final { return static_cast<FD3D12GraphicsShader*>(this); }
};

class FD3D12PixelShaderRHI : public FRHIPixelShader, public FD3D12GraphicsShader
{
public:
    FD3D12PixelShaderRHI(FD3D12Device* InDevice)
        : FRHIPixelShader()
        , FD3D12GraphicsShader(InDevice, ShaderVisibility_Pixel)
    {
    }

	// FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final { return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode()); }
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

    FORCEINLINE const FD3D12ShaderBindingInfo& GetLocalBindingInfo() const
    {
        return LocalBindingInfo;
    }

protected:
    bool GetShaderResourceBindings(ID3D12FunctionReflection* Reflection, uint32 NumBoundResources);

    FString Identifier;

private:
    FD3D12ShaderBindingInfo LocalBindingInfo;
};

class FD3D12RayGenShaderRHI : public FRHIRayGenShader, public FD3D12RayTracingShader
{
public:
    FD3D12RayGenShaderRHI(FD3D12Device* InDevice)
        : FRHIRayGenShader()
        , FD3D12RayTracingShader(InDevice)
    {
    }

	// FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final { return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode()); }
    virtual void* GetRHIBaseInterface() override final { return static_cast<FD3D12RayTracingShader*>(this); }
};

class FD3D12RayAnyHitShaderRHI : public FRHIRayAnyHitShader, public FD3D12RayTracingShader
{
public:
    FD3D12RayAnyHitShaderRHI(FD3D12Device* InDevice)
        : FRHIRayAnyHitShader()
        , FD3D12RayTracingShader(InDevice)
    {
    }

	// FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final { return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode()); }
    virtual void* GetRHIBaseInterface() override final { return static_cast<FD3D12RayTracingShader*>(this); }
};

class FD3D12RayClosestHitShaderRHI : public FRHIRayClosestHitShader, public FD3D12RayTracingShader
{
public:
    FD3D12RayClosestHitShaderRHI(FD3D12Device* InDevice)
        : FRHIRayClosestHitShader()
        , FD3D12RayTracingShader(InDevice)
    {
    }

	// FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final { return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode()); }
    virtual void* GetRHIBaseInterface() override final { return static_cast<FD3D12RayTracingShader*>(this); }
};

class FD3D12RayMissShaderRHI : public FRHIRayMissShader, public FD3D12RayTracingShader
{
public:
    FD3D12RayMissShaderRHI(FD3D12Device* InDevice)
        : FRHIRayMissShader()
        , FD3D12RayTracingShader(InDevice)
    {
    }

	// FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final { return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode()); }
    virtual void* GetRHIBaseInterface() override final { return static_cast<FD3D12RayTracingShader*>(this); }
};

class FD3D12ComputeShaderRHI : public FRHIComputeShader, public FD3D12Shader
{
public:
    FD3D12ComputeShaderRHI(FD3D12Device* InDevice)
        : FRHIComputeShader()
        , FD3D12Shader(InDevice, ShaderVisibility_All)
        , ThreadGroupXYZ(0, 0, 0)
    {
    }

    virtual bool Initialize(const TArray<uint8>& InCode) override final;

	// FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final { return const_cast<D3D12_SHADER_BYTECODE*>(&ByteCode.GetD3D12Bytecode()); }
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
