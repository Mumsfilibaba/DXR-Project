#pragma once
#include "RHI/RHIShader.h"
#include "RHI/RHIResources.h"
#include "RHI/ShaderCompilerInclude.h"
#include "Core/Templates/Utility/EnumOperators.h"
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

enum class ED3D12ShaderFlags : uint32
{
    None                                        = 0,
    RequiresResourceDescriptorHeapIndexing      = FLAG(1),  // D3D_SHADER_REQUIRES_RESOURCE_DESCRIPTOR_HEAP_INDEXING                              (0x02000000)
    RequiresSamplerDescriptorHeapIndexing       = FLAG(2),  // D3D_SHADER_REQUIRES_SAMPLER_DESCRIPTOR_HEAP_INDEXING                               (0x04000000)
    RequiresEarlyDepthStencil                   = FLAG(3),  // D3D_SHADER_REQUIRES_EARLY_DEPTH_STENCIL                                            (0x00000002)
    RequiresStencilRef                          = FLAG(4),  // D3D_SHADER_REQUIRES_STENCIL_REF                                                    (0x00000200)
    RequiresInnerCoverage                       = FLAG(5),  // D3D_SHADER_REQUIRES_INNER_COVERAGE                                                 (0x00000400)
    RequiresROVs                                = FLAG(6),  // D3D_SHADER_REQUIRES_ROVS                                                           (0x00001000)
    RequiresWaveOps                             = FLAG(7),  // D3D_SHADER_REQUIRES_WAVE_OPS                                                       (0x00004000)
    RequiresInt64Ops                            = FLAG(8),  // D3D_SHADER_REQUIRES_INT64_OPS                                                      (0x00008000)
    RequiresNative16BitOps                      = FLAG(9),  // D3D_SHADER_REQUIRES_NATIVE_16BIT_OPS                                               (0x00040000)
    RequiresBarycentrics                        = FLAG(10), // D3D_SHADER_REQUIRES_BARYCENTRICS                                                   (0x00020000)
    RequiresViewID                              = FLAG(11), // D3D_SHADER_REQUIRES_VIEW_ID                                                        (0x00010000)
    RequiresShadingRate                         = FLAG(12), // D3D_SHADER_REQUIRES_SHADING_RATE                                                   (0x00080000)
    RequiresRaytracingTier1_1                   = FLAG(13), // D3D_SHADER_REQUIRES_RAYTRACING_TIER_1_1                                            (0x00100000)
    RequiresSamplerFeedback                     = FLAG(14), // D3D_SHADER_REQUIRES_SAMPLER_FEEDBACK                                               (0x00200000)
    RequiresTiledResources                      = FLAG(15), // D3D_SHADER_REQUIRES_TILED_RESOURCES                                                (0x00000100)
    RequiresTypedUAVLoadAdditionalFormats       = FLAG(16), // D3D_SHADER_REQUIRES_TYPED_UAV_LOAD_ADDITIONAL_FORMATS                              (0x00000800)
    RequiresVPAndRTArrayIndexFromAnyShader      = FLAG(17), // D3D_SHADER_REQUIRES_VIEWPORT_AND_RT_ARRAY_INDEX_FROM_ANY_SHADER_FEEDING_RASTERIZER (0x00002000)
    RequiresAtomicInt64OnTypedResource          = FLAG(18), // D3D_SHADER_REQUIRES_ATOMIC_INT64_ON_TYPED_RESOURCE                                 (0x00400000)
    RequiresAtomicInt64OnGroupShared            = FLAG(19), // D3D_SHADER_REQUIRES_ATOMIC_INT64_ON_GROUP_SHARED                                   (0x00800000)
    RequiresAtomicInt64OnDescriptorHeapResource = FLAG(20), // D3D_SHADER_REQUIRES_ATOMIC_INT64_ON_DESCRIPTOR_HEAP_RESOURCE                       (0x10000000)
    RequiresWaveMMA                             = FLAG(21), // D3D_SHADER_REQUIRES_WAVE_MMA                                                       (0x08000000)
    RequiresDerivativesInMeshAndAmpShaders      = FLAG(22), // D3D_SHADER_REQUIRES_DERIVATIVES_IN_MESH_AND_AMPLIFICATION_SHADERS                  (0x01000000)
};

ENUM_CLASS_OPERATORS(ED3D12ShaderFlags);

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
        Count = Pixel + 1
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
        ED3D12BindingType BindingType;
        uint8                   BindingIndex;
        uint16                  OriginalBindingIndex;
        FString                 DebugName;
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
    FD3D12Shader(FD3D12Device* InDevice, EShaderVisibility::Type InShaderVisibility);
    ~FD3D12Shader();

	virtual bool Initialize(const TArray<uint8>& InCode);

    bool HasRootSignature() const { return bContainsRootSignature; }
    
    const FD3D12ShaderBytecode&    GetByteCode()    const { return ByteCode; }
    const FD3D12ShaderBindingInfo& GetBindingInfo() const { return BindingInfo; }
    
    EShaderVisibility::Type GetShaderVisibility() const { return ShaderVisibility; }

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
    bool IsRootSignatureInShaderBlob(const TComPtr<IDxcBlob>& ShaderBlob);
    bool GetReflectionInterface(const TComPtr<IDxcBlob>& ShaderBlob, REFIID iid, void** ppvObject);
    bool GetShaderResourceBindings(ID3D12ShaderReflection* Reflection, uint32 NumBoundResources);

    static bool ReadShaderFeatureFlags(const TComPtr<IDxcBlob>& ShaderBlob, uint64& OutFlags);
    static ED3D12ShaderFlags TranslateD3D12ShaderRequires(uint64 Mask);
    static bool ValidateRequiresFlags(ED3D12ShaderFlags InFlags, const CHAR* InShaderName);

    FD3D12ShaderBytecode     ByteCode;
    FD3D12ShaderHash         ByteCodeHash;
    EShaderVisibility::Type  ShaderVisibility;
    FD3D12ShaderBindingInfo  BindingInfo;
    ED3D12ShaderFlags        Flags;
    bool                     bContainsRootSignature;
};

class FD3D12GraphicsShader : public FD3D12Shader
{
public:
    FD3D12GraphicsShader(FD3D12Device* InDevice, EShaderVisibility::Type InShaderVisibility);
    virtual ~FD3D12GraphicsShader();

    // FD3D12Shader Interface
    virtual bool Initialize(const TArray<uint8>& InCode) override final;
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

class FD3D12RayTracingShader : public FD3D12Shader
{
public:
    FD3D12RayTracingShader(FD3D12Device* InDevice);
    virtual ~FD3D12RayTracingShader();

    // FD3D12Shader Interface
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

class FD3D12ComputeShaderRHI : public FRHIComputeShader, public FD3D12Shader
{
public:
    FD3D12ComputeShaderRHI(FD3D12Device* InDevice);
    virtual ~FD3D12ComputeShaderRHI();

    // FD3D12Shader Interface
    virtual bool Initialize(const TArray<uint8>& InCode) override final;

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;

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
