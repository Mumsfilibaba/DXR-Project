#pragma once
#include "Core/Containers/Map.h"
#include "Core/RefCountedBase.h"
#include "RHI/RHIShader.h"
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanDeviceChild.h"

typedef TSharedRef<class FVulkanShader>              FVulkanShaderRef;
typedef TSharedRef<class FVulkanVertexShader>        FVulkanVertexShaderRef;
typedef TSharedRef<class FVulkanHullShader>          FVulkanHullShaderRef;
typedef TSharedRef<class FVulkanDomainShader>        FVulkanDomainShaderRef;
typedef TSharedRef<class FVulkanGeometryShader>      FVulkanGeometryShaderRef;
typedef TSharedRef<class FVulkanPixelShader>         FVulkanPixelShaderRef;
typedef TSharedRef<class FVulkanComputeShader>       FVulkanComputeShaderRef;
typedef TSharedRef<class FVulkanRayTracingShader>    FVulkanRayTracingShaderRef;
typedef TSharedRef<class FVulkanRayGenShader>        FVulkanRayGenShaderRef;
typedef TSharedRef<class FVulkanRayAnyHitShader>     FVulkanRayAnyHitShaderRef;
typedef TSharedRef<class FVulkanRayClosestHitShader> FVulkanRayClosestHitShaderRef;
typedef TSharedRef<class FVulkanRayMissShader>       FVulkanRayMissShaderRef;

enum EShaderVisibility : uint32
{
    ShaderVisibility_Vertex = 0,
    ShaderVisibility_Hull,
    ShaderVisibility_Domain,
    ShaderVisibility_Geometry,
    ShaderVisibility_Pixel,
    ShaderVisibility_Compute,
    ShaderVisibility_Count = ShaderVisibility_Compute + 1
};

inline const CHAR* ToString(EShaderVisibility ShaderVisibility)
{
    CHECK(ShaderVisibility < ShaderVisibility_Count);
    
    static constexpr const char* ShaderVisibilityStrings[]
    {
        "Vertex",
        "Hull",
        "Domain",
        "Geometry",
        "Pixel",
        "Compute",
    };
    
    static_assert(ARRAY_COUNT(ShaderVisibilityStrings) == ShaderVisibility_Count, "ShaderVisibilityStrings is out of date");
    return ShaderVisibilityStrings[ShaderVisibility];
}

enum EVulkanBindingType : uint8
{
    VulkanBindingType_UniformBuffer = 0,
    VulkanBindingType_UniformBufferDynamic,
    VulkanBindingType_SampledImage,
    VulkanBindingType_StorageImage,
    VulkanBindingType_StorageBufferRead,
    VulkanBindingType_StorageBufferReadWrite,
    VulkanBindingType_Sampler,
    VulkanBindingType_TexelBufferRead,
    VulkanBindingType_TexelBufferReadWrite,
    VulkanBindingType_ImmutableSampler,
    VulkanBindingType_Count = VulkanBindingType_ImmutableSampler + 1,
};

inline const CHAR* ToString(EVulkanBindingType Binding)
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
    };
    
    static_assert(ARRAY_COUNT(BindingTypeStrings) == VulkanBindingType_Count, "BindingTypeStrings is out of date");
    return Binding < VulkanBindingType_Count ? BindingTypeStrings[Binding] : "Unknown BindingType";
}

inline VkDescriptorType GetDescriptorTypeFromBindingType(EVulkanBindingType BindingType)
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
    };

    static_assert(ARRAY_COUNT(DescriptorTypes) == VulkanBindingType_Count, "The DescriptorTypes array is out of date");
    return DescriptorTypes[BindingType];
}

struct FVulkanShaderInfo
{
    struct FBindingOffsets
    {
        uint32 DescriptorSetOffset = UINT32_MAX;
        uint32 BindingOffset       = UINT32_MAX;
    };
    
    struct FResourceBinding
    {
        EVulkanBindingType BindingType;
        uint8              BindingIndex;
        uint16             OriginalBindingIndex;
    #if VULKAN_ENABLE_BINDING_DEBUG_NAMES
        FString            DebugName;
    #endif
    };
    
    TArray<FBindingOffsets>  BindingOffsets;
    TArray<FResourceBinding> ResourceBindings;
    uint32                   NumPushConstants;
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
    FVulkanShader(FVulkanDevice* InDevice, EShaderVisibility InShaderVisibility);
    ~FVulkanShader();

    bool Initialize(const TArray<uint8>& InCode);

    TSharedRef<FVulkanShaderModule> GetOrCreateShaderModule(class FVulkanPipelineLayout* Layout);
    bool PatchShaderBindings(FSpirvArray& OutSpirv, uint32 DescriptorSetIndex);
    bool StripGoogleSpirvRequirements(const FSpirvArray& InWords, FSpirvArray& OutWords);
    bool ValidateNoGoogleSpirvRequirements(const FSpirvArray& Words, FString* OutErrorMessage = nullptr);

    EShaderVisibility GetShaderVisibility() const
    {
        return ShaderVisibility;
    }

    const FVulkanShaderInfo& GetShaderInfo() const
    {
        return ShaderInfo;
    }

    const FString& GetEntryPointName() const
    {
        return EntryPointName;
    }

protected:
    bool InitializeShaderLayout();
    
    FSpirvArray       SpirvCode;
    FVulkanShaderInfo ShaderInfo;
    EShaderVisibility ShaderVisibility;
    FString           EntryPointName;
    
    TMap<uint32, TSharedRef<FVulkanShaderModule>> ShaderModules;
    FCriticalSection ShaderModulesCS;
};

class FVulkanVertexShader : public FRHIVertexShader, public FVulkanShader
{
public:
    FVulkanVertexShader(FVulkanDevice* InDevice)
        : FRHIVertexShader()
        , FVulkanShader(InDevice, ShaderVisibility_Vertex)
    {
    }

    // FRHIShader Interface
    virtual void* GetRHINativeHandle() override final { return reinterpret_cast<void*>(&SpirvCode); }
    virtual void* GetRHIBaseInterface() { return static_cast<FVulkanShader*>(this); }
};

class FVulkanHullShader : public FRHIHullShader, public FVulkanShader
{
public:
    FVulkanHullShader(FVulkanDevice* InDevice)
        : FRHIHullShader()
        , FVulkanShader(InDevice, ShaderVisibility_Hull)
    {
    }

    // FRHIShader Interface
    virtual void* GetRHINativeHandle() override final { return reinterpret_cast<void*>(&SpirvCode); }
    virtual void* GetRHIBaseInterface() { return static_cast<FVulkanShader*>(this); }
};

class FVulkanDomainShader : public FRHIDomainShader, public FVulkanShader
{
public:
    FVulkanDomainShader(FVulkanDevice* InDevice)
        : FRHIDomainShader()
        , FVulkanShader(InDevice, ShaderVisibility_Domain)
    {
    }

    // FRHIShader Interface
    virtual void* GetRHINativeHandle() override final { return reinterpret_cast<void*>(&SpirvCode); }
    virtual void* GetRHIBaseInterface() { return static_cast<FVulkanShader*>(this); }
};

class FVulkanGeometryShader : public FRHIGeometryShader, public FVulkanShader
{
public:
    FVulkanGeometryShader(FVulkanDevice* InDevice)
        : FRHIGeometryShader()
        , FVulkanShader(InDevice, ShaderVisibility_Geometry)
    {
    }

    // FRHIShader Interface
    virtual void* GetRHINativeHandle() override final { return reinterpret_cast<void*>(&SpirvCode); }
    virtual void* GetRHIBaseInterface() { return static_cast<FVulkanShader*>(this); }
};

class FVulkanPixelShader : public FRHIPixelShader, public FVulkanShader
{
public:
    FVulkanPixelShader(FVulkanDevice* InDevice)
        : FRHIPixelShader()
        , FVulkanShader(InDevice, ShaderVisibility_Pixel)
    {
    }

    // FRHIShader Interface
    virtual void* GetRHINativeHandle() override final { return reinterpret_cast<void*>(&SpirvCode); }
    virtual void* GetRHIBaseInterface() { return static_cast<FVulkanShader*>(this); }
};


class FVulkanRayTracingShader : public FVulkanShader
{
public:
    static bool GetRayTracingShaderReflection(class FVulkanRayTracingShader* Shader);
    
public:
    FVulkanRayTracingShader(FVulkanDevice* InDevice)
        : FVulkanShader(InDevice, ShaderVisibility_Compute)
    {
    }
    
    const FString& GetIdentifier() const
    {
        return Identifier;
    }

protected:
    FString Identifier;
};

class FVulkanRayGenShader : public FRHIRayGenShader, public FVulkanRayTracingShader
{
public:
    FVulkanRayGenShader(FVulkanDevice* InDevice)
        : FRHIRayGenShader()
        , FVulkanRayTracingShader(InDevice)
    {
    }

    // FRHIShader Interface
    virtual void* GetRHINativeHandle() override final { return reinterpret_cast<void*>(&SpirvCode); }
    virtual void* GetRHIBaseInterface() { return static_cast<FVulkanRayTracingShader*>(this); }
};

class FVulkanRayAnyHitShader : public FRHIRayAnyHitShader, public FVulkanRayTracingShader
{
public:
    FVulkanRayAnyHitShader(FVulkanDevice* InDevice)
        : FRHIRayAnyHitShader()
        , FVulkanRayTracingShader(InDevice)
    {
    }

    // FRHIShader Interface
    virtual void* GetRHINativeHandle() override final { return reinterpret_cast<void*>(&SpirvCode); }
    virtual void* GetRHIBaseInterface() { return static_cast<FVulkanRayTracingShader*>(this); }
};

class FVulkanRayClosestHitShader : public FRHIRayClosestHitShader, public FVulkanRayTracingShader
{
public:
    
    FVulkanRayClosestHitShader(FVulkanDevice* InDevice)
        : FRHIRayClosestHitShader()
        , FVulkanRayTracingShader(InDevice)
    {
    }

    // FRHIShader Interface
    virtual void* GetRHINativeHandle() override final { return reinterpret_cast<void*>(&SpirvCode); }
    virtual void* GetRHIBaseInterface() { return static_cast<FVulkanRayTracingShader*>(this); }
};

class FVulkanRayMissShader : public FRHIRayMissShader, public FVulkanRayTracingShader
{
public:
    FVulkanRayMissShader(FVulkanDevice* InDevice)
        : FRHIRayMissShader()
        , FVulkanRayTracingShader(InDevice)
    {
    }

    // FRHIShader Interface
    virtual void* GetRHINativeHandle() override final { return reinterpret_cast<void*>(&SpirvCode); }
    virtual void* GetRHIBaseInterface() { return static_cast<FVulkanRayTracingShader*>(this); }
};

class FVulkanComputeShader : public FRHIComputeShader, public FVulkanShader
{
public:
    FVulkanComputeShader(FVulkanDevice* InDevice)
        : FRHIComputeShader()
        , FVulkanShader(InDevice, ShaderVisibility_Compute)
    {
    }

    // FRHIShader Interface
    virtual void* GetRHINativeHandle() override final { return reinterpret_cast<void*>(&SpirvCode); }
    virtual void* GetRHIBaseInterface() { return static_cast<FVulkanShader*>(this); }
};

FORCEINLINE FVulkanShader* GetVulkanShader(FRHIShader* Shader)
{
    return Shader ? reinterpret_cast<FVulkanShader*>(Shader->GetRHIBaseInterface()) : nullptr;
}

FORCEINLINE FVulkanRayTracingShader* GetVulkanRayTracingShader(FRHIRayTracingShader* Shader)
{
    return Shader ? reinterpret_cast<FVulkanRayTracingShader*>(Shader->GetRHIBaseInterface()) : nullptr;
}
