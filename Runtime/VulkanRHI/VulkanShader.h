#pragma once
#include "Core/Containers/Map.h"
#include "Core/RefCountedBase.h"
#include "RHI/RHIShader.h"
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanDeviceChild.h"

typedef TSharedRef<class FVulkanShader>                 FVulkanShaderRef;
typedef TSharedRef<class FVulkanVertexShaderRHI>        FVulkanVertexShaderRHIRef;
typedef TSharedRef<class FVulkanHullShaderRHI>          FVulkanHullShaderRHIRef;
typedef TSharedRef<class FVulkanDomainShaderRHI>        FVulkanDomainShaderRHIRef;
typedef TSharedRef<class FVulkanGeometryShaderRHI>      FVulkanGeometryShaderRHIRef;
typedef TSharedRef<class FVulkanPixelShaderRHI>         FVulkanPixelShaderRHIRef;
typedef TSharedRef<class FVulkanMeshShaderRHI>          FVulkanMeshShaderRHIRef;
typedef TSharedRef<class FVulkanAmplificationShaderRHI> FVulkanAmplificationShaderRHIRef;
typedef TSharedRef<class FVulkanComputeShaderRHI>       FVulkanComputeShaderRHIRef;
typedef TSharedRef<class FVulkanRayTracingShader>         FVulkanRayTracingShaderRef;
typedef TSharedRef<class FVulkanRayGenShaderRHI>          FVulkanRayGenShaderRHIRef;
typedef TSharedRef<class FVulkanRayAnyHitShaderRHI>       FVulkanRayAnyHitShaderRHIRef;
typedef TSharedRef<class FVulkanRayClosestHitShaderRHI>   FVulkanRayClosestHitShaderRHIRef;
typedef TSharedRef<class FVulkanRayMissShaderRHI>         FVulkanRayMissShaderRHIRef;
typedef TSharedRef<class FVulkanRayIntersectionShaderRHI> FVulkanRayIntersectionShaderRHIRef;
typedef TSharedRef<class FVulkanRayCallableShaderRHI>     FVulkanRayCallableShaderRHIRef;

struct EShaderVisibility
{
    enum Type : uint32
    {
        Vertex = 0,
        Hull,
        Domain,
        Geometry,
        Pixel,
        Compute,
        Task,
        Mesh,
        RayTracing,
        Count = RayTracing + 1,
    };
};

inline const CHAR* ToString(EShaderVisibility::Type ShaderVisibility)
{
    CHECK(ShaderVisibility < EShaderVisibility::Count);
    
    static constexpr const char* ShaderVisibilityStrings[]
    {
        "Vertex",
        "Hull",
        "Domain",
        "Geometry",
        "Pixel",
        "Compute",
        "Task",
        "Mesh",
        "RayTracing",
    };
    
    static_assert(ARRAY_COUNT(ShaderVisibilityStrings) == EShaderVisibility::Count, "ShaderVisibilityStrings is out of date");
    return ShaderVisibilityStrings[ShaderVisibility];
}

struct EVulkanBindingType
{
    enum Type : uint8
    {
        UniformBuffer = 0,
        UniformBufferDynamic,
        SampledImage,
        StorageImage,
        StorageBufferRead,
        StorageBufferReadWrite,
        Sampler,
        TexelBufferRead,
        TexelBufferReadWrite,
        ImmutableSampler,
        AccelerationStructure,
        Count = AccelerationStructure + 1,
    };
};

inline const CHAR* ToString(EVulkanBindingType::Type Binding)
{
    static constexpr const char* const BindingTypeStrings[]
    {
        "UniformBuffer",
        "UniformBufferDynamic",
        "SampledImage",
        "StorageImage",
        "StorageBufferRead",
        "StorageBufferReadWrite",
        "Sampler",
        "TexelBufferRead",
        "TexelBufferReadWrite",
        "ImmutableSampler",
        "AccelerationStructure",
    };
    
    static_assert(ARRAY_COUNT(BindingTypeStrings) == EVulkanBindingType::Count, "BindingTypeStrings is out of date");
    return Binding < EVulkanBindingType::Count ? BindingTypeStrings[Binding] : "Unknown BindingType";
}

inline VkDescriptorType GetDescriptorTypeFromBindingType(EVulkanBindingType::Type BindingType)
{
    // DescriptorType Lookup-table
    static constexpr VkDescriptorType DescriptorTypes[] =
    {
        // ConstantBuffers
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        // Dynamic ConstantBuffers
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        // SRV Images
        VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        // UAV Images
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        // SRV Buffers (StructuredBuffer, ByteAddressBuffer)
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        // UAV Buffers (RWStructuredBuffer, RWByteAddressBuffer)
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        // Samplers
        VK_DESCRIPTOR_TYPE_SAMPLER,
        // SRV (Buffer)
        VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
        // UAV (Buffer)
        VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
        // Immutable Sampler (baked into layout, no descriptor write needed)
        VK_DESCRIPTOR_TYPE_SAMPLER,
        // RaytracingAccelerationStructure (TLAS bound via VkWriteDescriptorSetAccelerationStructureKHR)
        VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
    };

    static_assert(ARRAY_COUNT(DescriptorTypes) == EVulkanBindingType::Count, "The DescriptorTypes array is out of date");
    return DescriptorTypes[BindingType];
}

inline uint32 GetBindlessBindingForType(VkDescriptorType DescriptorType)
{
    switch (DescriptorType)
    {
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:              return VULKAN_BINDLESS_SPLIT_BINDING_SAMPLED_IMAGE;
        case VK_DESCRIPTOR_TYPE_SAMPLER:                    return VULKAN_BINDLESS_SPLIT_BINDING_SAMPLER;
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:              return VULKAN_BINDLESS_SPLIT_BINDING_STORAGE_IMAGE;
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:             return VULKAN_BINDLESS_SPLIT_BINDING_UNIFORM_BUFFER;
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:             return VULKAN_BINDLESS_SPLIT_BINDING_STORAGE_BUFFER;
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:       return VULKAN_BINDLESS_SPLIT_BINDING_UNIFORM_TEXEL;
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:       return VULKAN_BINDLESS_SPLIT_BINDING_STORAGE_TEXEL;
        case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: return VULKAN_BINDLESS_SPLIT_BINDING_ACCEL_STRUCT;
        default:                                            return VULKAN_BINDLESS_RESOURCE_BINDING;
    }
}

struct FVulkanShaderInfo
{
    struct FBindingOffsets
    {
        uint32                   DescriptorSetOffset = UINT32_MAX;
        uint32                   BindingOffset       = UINT32_MAX;
        EVulkanBindingType::Type HeapBindingType     = EVulkanBindingType::Count;
    };
    
    struct FResourceBinding
    {
        EVulkanBindingType::Type BindingType;
        uint8                    BindingIndex;
        EVulkanNullImageViewType NullViewType = EVulkanNullImageViewType::Texture2D;
        uint16                   OriginalBindingIndex;
    #if VULKAN_ENABLE_BINDING_DEBUG_NAMES
        String                   DebugName;
    #endif
    };
    
    TArray<FBindingOffsets>  HeapBindingOffsets;
    TArray<FBindingOffsets>  BindingOffsets;
    TArray<FResourceBinding> ResourceBindings;
    uint32                   NumPushConstants;

    bool UsesBindlessHeap() const
    {
        return HeapBindingOffsets.Size() > 0;
    }
};

class FVulkanShaderModule : public FRefCountedBase
{
public:
    FVulkanShaderModule(FVulkanDevice* InDevice, VkShaderModule InShaderModule);
    ~FVulkanShaderModule();

    VkShaderModule GetVkShaderModule() const
    {
        return ShaderModule;
    }

private:
    static FVulkanDevice* GetDevice()
    {
        CHECK(StaticDevice != nullptr);
        return StaticDevice;
    }

    VkShaderModule ShaderModule;
    static FVulkanDevice* StaticDevice;
};

typedef TArray<uint32> FSpirvArray;

class FVulkanShader : public FVulkanDeviceChild
{
public:
    FVulkanShader(FVulkanDevice* InDevice, EShaderVisibility::Type InShaderVisibility);
    ~FVulkanShader();

    bool Initialize(const TArray<uint8>& InCode);

    TSharedRef<FVulkanShaderModule> GetOrCreateShaderModule(class FVulkanPipelineLayout* Layout);
    bool PatchShaderBindings(FSpirvArray& OutSpirv, class FVulkanPipelineLayout* Layout, uint32 DescriptorSetIndex);
    bool StripGoogleSpirvRequirements(const FSpirvArray& InWords, FSpirvArray& OutWords);
    bool ValidateNoGoogleSpirvRequirements(const FSpirvArray& Words, String* OutErrorMessage = nullptr);
    bool ForceUnknownStorageImageFormats(const FSpirvArray& InWords, FSpirvArray& OutWords, bool& bOutRewroteFormats);
    bool MergeDuplicateTypeDeclarations(const FSpirvArray& InWords, FSpirvArray& OutWords);

    EShaderVisibility::Type GetShaderVisibility() const
    {
        return ShaderVisibility;
    }

    const FVulkanShaderInfo& GetShaderInfo() const
    {
        return ShaderInfo;
    }

    const String& GetEntryPointName() const
    {
        return EntryPointName;
    }

protected:
    bool InitializeShaderLayout();
    
    FSpirvArray                                   SpirvCode;
    FVulkanShaderInfo                             ShaderInfo;
    EShaderVisibility::Type                       ShaderVisibility;
    String                                        EntryPointName;
    TMap<uint64, TSharedRef<FVulkanShaderModule>> ShaderModules;
    FCriticalSection                              ShaderModulesCS;
};

class FVulkanVertexShaderRHI : public FRHIVertexShader, public FVulkanShader
{
public:
    FVulkanVertexShaderRHI(FVulkanDevice* InDevice);
    virtual ~FVulkanVertexShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

class FVulkanHullShaderRHI : public FRHIHullShader, public FVulkanShader
{
public:
    FVulkanHullShaderRHI(FVulkanDevice* InDevice);
    virtual ~FVulkanHullShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

class FVulkanDomainShaderRHI : public FRHIDomainShader, public FVulkanShader
{
public:
    FVulkanDomainShaderRHI(FVulkanDevice* InDevice);
    virtual ~FVulkanDomainShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

class FVulkanGeometryShaderRHI : public FRHIGeometryShader, public FVulkanShader
{
public:
    FVulkanGeometryShaderRHI(FVulkanDevice* InDevice);
    virtual ~FVulkanGeometryShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

class FVulkanPixelShaderRHI : public FRHIPixelShader, public FVulkanShader
{
public:
    FVulkanPixelShaderRHI(FVulkanDevice* InDevice);
    virtual ~FVulkanPixelShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};


class FVulkanMeshShaderRHI : public FRHIMeshShader, public FVulkanShader
{
public:
    FVulkanMeshShaderRHI(FVulkanDevice* InDevice);
    virtual ~FVulkanMeshShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

class FVulkanAmplificationShaderRHI : public FRHIAmplificationShader, public FVulkanShader
{
public:
    FVulkanAmplificationShaderRHI(FVulkanDevice* InDevice);
    virtual ~FVulkanAmplificationShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};


class FVulkanRayTracingShader : public FVulkanShader
{
public:
    static bool GetRayTracingShaderReflection(class FVulkanRayTracingShader* Shader);
    
public:
    FVulkanRayTracingShader(FVulkanDevice* InDevice);
    virtual ~FVulkanRayTracingShader();

    const String& GetIdentifier() const
    {
        return GetEntryPointName();
    }
};

class FVulkanRayGenShaderRHI : public FRHIRayGenShader, public FVulkanRayTracingShader
{
public:
    FVulkanRayGenShaderRHI(FVulkanDevice* InDevice);
    virtual ~FVulkanRayGenShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

class FVulkanRayAnyHitShaderRHI : public FRHIRayAnyHitShader, public FVulkanRayTracingShader
{
public:
    FVulkanRayAnyHitShaderRHI(FVulkanDevice* InDevice);
    virtual ~FVulkanRayAnyHitShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

class FVulkanRayClosestHitShaderRHI : public FRHIRayClosestHitShader, public FVulkanRayTracingShader
{
public:
    FVulkanRayClosestHitShaderRHI(FVulkanDevice* InDevice);
    virtual ~FVulkanRayClosestHitShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

class FVulkanRayMissShaderRHI : public FRHIRayMissShader, public FVulkanRayTracingShader
{
public:
    FVulkanRayMissShaderRHI(FVulkanDevice* InDevice);
    virtual ~FVulkanRayMissShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

class FVulkanRayIntersectionShaderRHI : public FRHIRayIntersectionShader, public FVulkanRayTracingShader
{
public:
    FVulkanRayIntersectionShaderRHI(FVulkanDevice* InDevice);
    virtual ~FVulkanRayIntersectionShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

class FVulkanRayCallableShaderRHI : public FRHIRayCallableShader, public FVulkanRayTracingShader
{
public:
    FVulkanRayCallableShaderRHI(FVulkanDevice* InDevice);
    virtual ~FVulkanRayCallableShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

class FVulkanComputeShaderRHI : public FRHIComputeShader, public FVulkanShader
{
public:
    FVulkanComputeShaderRHI(FVulkanDevice* InDevice);
    virtual ~FVulkanComputeShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

FORCEINLINE FVulkanShader* GetVulkanShader(FRHIShader* Shader)
{
    return Shader ? reinterpret_cast<FVulkanShader*>(Shader->GetRHIBaseInterface()) : nullptr;
}

FORCEINLINE FVulkanRayTracingShader* GetVulkanRayTracingShader(FRHIRayTracingShader* Shader)
{
    return Shader ? reinterpret_cast<FVulkanRayTracingShader*>(Shader->GetRHIBaseInterface()) : nullptr;
}
