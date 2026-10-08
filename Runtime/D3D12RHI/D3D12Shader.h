#pragma once
#include "RHI/RHIShader.h"
#include "RHI/RHIResources.h"
#include "Core/Containers/ArrayView.h"
#include "Core/Templates/Utility/EnumOperators.h"
#include "D3D12RHI/D3D12DeviceChild.h"
#include "D3D12RHI/D3D12Constants.h"
#include "ShaderCore/ShaderCode.h"

typedef TSharedRef<class FD3D12VertexShaderRHI>          FD3D12VertexShaderRHIRef;
typedef TSharedRef<class FD3D12HullShaderRHI>            FD3D12HullShaderRHIRef;
typedef TSharedRef<class FD3D12DomainShaderRHI>          FD3D12DomainShaderRHIRef;
typedef TSharedRef<class FD3D12GeometryShaderRHI>        FD3D12GeometryShaderRHIRef;
typedef TSharedRef<class FD3D12PixelShaderRHI>           FD3D12PixelShaderRHIRef;
typedef TSharedRef<class FD3D12MeshShaderRHI>            FD3D12MeshShaderRHIRef;
typedef TSharedRef<class FD3D12AmplificationShaderRHI>   FD3D12AmplificationShaderRHIRef;
typedef TSharedRef<class FD3D12ComputeShaderRHI>         FD3D12ComputeShaderRHIRef;
typedef TSharedRef<class FD3D12RayGenShaderRHI>          FD3D12RayGenShaderRHIRef;
typedef TSharedRef<class FD3D12RayAnyHitShaderRHI>       FD3D12RayAnyHitShaderRHIRef;
typedef TSharedRef<class FD3D12RayClosestHitShaderRHI>   FD3D12RayClosestHitShaderRHIRef;
typedef TSharedRef<class FD3D12RayMissShaderRHI>         FD3D12RayMissShaderRHIRef;
typedef TSharedRef<class FD3D12RayIntersectionShaderRHI> FD3D12RayIntersectionShaderRHIRef;
typedef TSharedRef<class FD3D12RayCallableShaderRHI>     FD3D12RayCallableShaderRHIRef;

using ED3D12ShaderFlags = EShaderFeatureFlags;

struct EShaderVisibility
{
    enum Type : int32
    {
        All = 0,
        Vertex,
        Hull,
        Domain,
        Geometry,
        Pixel,
        Amplification,
        Mesh,
        Count = Mesh + 1,
    };
};

struct EResourceType
{
    enum Type : int32
    {
        CBV     = 0,
        SRV     = 1,
        UAV     = 2,
        Sampler = 3,
        Count   = Sampler + 1,
        Unknown = 5,
    };
};

enum class ED3D12BindingType : uint8
{
    ConstantBuffer = 0,
    SRV,
    UAV,
    Sampler,
    Count,
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
};

template<>
struct THash<FD3D12ShaderHash>
{
    static uint64 GetHash(const FD3D12ShaderHash& Value)
    {
        uint64 Result = Value.Hash[0];
        HashCombine(Result, Value.Hash[1]);
        return Result;
    }
};

struct FD3D12ShaderBytecode
{
    FD3D12ShaderBytecode();
    FD3D12ShaderBytecode(TArrayView<const uint8> InCode);
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
    
    FORCEINLINE const D3D12_SHADER_BYTECODE& GetD3D12Bytecode() const
    {
        return ByteCode;
    }

    FD3D12ShaderBytecode& operator=(const FD3D12ShaderBytecode& Other);
    FD3D12ShaderBytecode& operator=(FD3D12ShaderBytecode&& Other);

private:
    D3D12_SHADER_BYTECODE ByteCode;
};

struct FD3D12ShaderBindingInfo
{   
    struct FResourceBinding
    {
        ED3D12BindingType        BindingType          = ED3D12BindingType::ConstantBuffer;
        uint8                    BindingIndex         = 0;
        uint16                   OriginalBindingIndex = 0;
        bool                     bIsTexture           = false;
        ED3D12NullDescriptorType NullDescriptorType   = ED3D12NullDescriptorType::Texture2D;
#if D3D12_ENABLE_BINDING_DEBUG_NAMES
        String                   DebugName;
#endif
    };
    
    void AddBinding(ED3D12BindingType InType, uint16 InOriginalBindingIndex, const CHAR* InDebugName, bool bInIsTexture = false, ED3D12NullDescriptorType InNullDescriptorType = ED3D12NullDescriptorType::Texture2D)
    {
        FResourceBinding& Binding    = ResourceBindings.Emplace();
        Binding.BindingType          = InType;
        Binding.BindingIndex         = 0;
        Binding.OriginalBindingIndex = InOriginalBindingIndex;
        Binding.bIsTexture           = bInIsTexture;
        Binding.NullDescriptorType   = InNullDescriptorType;
#if D3D12_ENABLE_BINDING_DEBUG_NAMES
        Binding.DebugName            = InDebugName;
#else
        UNREFERENCED_VARIABLE(InDebugName);
#endif
    }

    TArray<FResourceBinding> ResourceBindings;
    uint32                   NumPushConstants = 0;
};

class FD3D12Shader : public FD3D12DeviceChild
{
public:
    FD3D12Shader(FD3D12Device* InDevice, EShaderVisibility::Type InShaderVisibility);
    ~FD3D12Shader();

    virtual bool Initialize(const FShaderCodeView& InCode);

    bool HasRootSignature() const { return bContainsRootSignature; }
    
    const FD3D12ShaderBytecode&    GetByteCode()         const { return ByteCode; }
    const FD3D12ShaderBindingInfo& GetBindingInfo()      const { return BindingInfo; }
    EShaderVisibility::Type        GetShaderVisibility() const { return ShaderVisibility; }

    FORCEINLINE FD3D12ShaderHash GetHash() const
    {
        return ByteCodeHash;
    }

    FORCEINLINE ED3D12ShaderFlags GetFlags() const
    {
        return Flags;
    }

    FORCEINLINE bool HasFlag(ED3D12ShaderFlags InFlag) const
    {
        return IsEnumFlagSet(Flags, InFlag);
    }

protected:
    static void BuildBindingInfo(const FShaderCodeView& InCode, EShaderBindingSpace Space, FD3D12ShaderBindingInfo& OutBindingInfo);
    static bool ValidateRequiresFlags(ED3D12ShaderFlags InFlags, const CHAR* InShaderName);

    FD3D12ShaderBytecode    ByteCode;
    FD3D12ShaderHash        ByteCodeHash;
    EShaderVisibility::Type ShaderVisibility;
    FD3D12ShaderBindingInfo BindingInfo;
    ED3D12ShaderFlags       Flags;
    bool                    bContainsRootSignature;
};

class FD3D12GraphicsShader : public FD3D12Shader
{
public:
    FD3D12GraphicsShader(FD3D12Device* InDevice, EShaderVisibility::Type InShaderVisibility);
    virtual ~FD3D12GraphicsShader();
};

class FD3D12VertexShaderRHI : public FRHIVertexShader, public FD3D12GraphicsShader
{
public:
    FD3D12VertexShaderRHI(FD3D12Device* InDevice);
    virtual ~FD3D12VertexShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

class FD3D12HullShaderRHI : public FRHIHullShader, public FD3D12GraphicsShader
{
public:
    FD3D12HullShaderRHI(FD3D12Device* InDevice);
    virtual ~FD3D12HullShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

class FD3D12DomainShaderRHI : public FRHIDomainShader, public FD3D12GraphicsShader
{
public:
    FD3D12DomainShaderRHI(FD3D12Device* InDevice);
    virtual ~FD3D12DomainShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

class FD3D12GeometryShaderRHI : public FRHIGeometryShader, public FD3D12GraphicsShader
{
public:
    FD3D12GeometryShaderRHI(FD3D12Device* InDevice);
    virtual ~FD3D12GeometryShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

class FD3D12PixelShaderRHI : public FRHIPixelShader, public FD3D12GraphicsShader
{
public:
    FD3D12PixelShaderRHI(FD3D12Device* InDevice);
    virtual ~FD3D12PixelShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

class FD3D12MeshShaderRHI : public FRHIMeshShader, public FD3D12GraphicsShader
{
public:
    FD3D12MeshShaderRHI(FD3D12Device* InDevice);
    virtual ~FD3D12MeshShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

class FD3D12AmplificationShaderRHI : public FRHIAmplificationShader, public FD3D12GraphicsShader
{
public:
    FD3D12AmplificationShaderRHI(FD3D12Device* InDevice);
    virtual ~FD3D12AmplificationShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

class FD3D12RayTracingShader : public FD3D12Shader
{
public:
    FD3D12RayTracingShader(FD3D12Device* InDevice);
    virtual ~FD3D12RayTracingShader();

    // FD3D12Shader Interface
    virtual bool Initialize(const FShaderCodeView& InCode) override final;
    
    FORCEINLINE const String& GetIdentifier() const
    {
        return Identifier;
    }

    FORCEINLINE const FD3D12ShaderBindingInfo& GetLocalBindingInfo() const
    {
        return LocalBindingInfo;
    }

protected:
    String Identifier;

private:
    FD3D12ShaderBindingInfo LocalBindingInfo;
};

class FD3D12RayGenShaderRHI : public FRHIRayGenShader, public FD3D12RayTracingShader
{
public:
    FD3D12RayGenShaderRHI(FD3D12Device* InDevice);
    virtual ~FD3D12RayGenShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

class FD3D12RayAnyHitShaderRHI : public FRHIRayAnyHitShader, public FD3D12RayTracingShader
{
public:
    FD3D12RayAnyHitShaderRHI(FD3D12Device* InDevice);
    virtual ~FD3D12RayAnyHitShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

class FD3D12RayClosestHitShaderRHI : public FRHIRayClosestHitShader, public FD3D12RayTracingShader
{
public:
    FD3D12RayClosestHitShaderRHI(FD3D12Device* InDevice);
    virtual ~FD3D12RayClosestHitShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

class FD3D12RayMissShaderRHI : public FRHIRayMissShader, public FD3D12RayTracingShader
{
public:
    FD3D12RayMissShaderRHI(FD3D12Device* InDevice);
    virtual ~FD3D12RayMissShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

class FD3D12RayIntersectionShaderRHI : public FRHIRayIntersectionShader, public FD3D12RayTracingShader
{
public:
    FD3D12RayIntersectionShaderRHI(FD3D12Device* InDevice);
    virtual ~FD3D12RayIntersectionShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

class FD3D12RayCallableShaderRHI : public FRHIRayCallableShader, public FD3D12RayTracingShader
{
public:
    FD3D12RayCallableShaderRHI(FD3D12Device* InDevice);
    virtual ~FD3D12RayCallableShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

class FD3D12ComputeShaderRHI : public FRHIComputeShader, public FD3D12Shader
{
public:
    FD3D12ComputeShaderRHI(FD3D12Device* InDevice);
    virtual ~FD3D12ComputeShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

NODISCARD inline FD3D12Shader* GetD3D12Shader(FRHIShader* Shader)
{
    return Shader ? reinterpret_cast<FD3D12Shader*>(Shader->GetRHIBaseInterface()) : nullptr;
}

NODISCARD inline FD3D12RayTracingShader* GetD3D12RayTracingShader(FRHIRayTracingShader* Shader)
{
    return Shader ? reinterpret_cast<FD3D12RayTracingShader*>(Shader->GetRHIBaseInterface()) : nullptr;
}
