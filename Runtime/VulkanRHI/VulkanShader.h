#pragma once
#include "Core/Containers/Map.h"
#include "RHI/RHIShader.h"
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanDeviceChild.h"
#include "VulkanRHI/VulkanRefCounted.h"

typedef TSharedRef<class FVulkanShader>              FVulkanShaderRef;
typedef TSharedRef<class FVulkanShaderModule>        FVulkanShaderModuleRef;
typedef TSharedRef<class FVulkanVertexShaderRHI>        FVulkanVertexShaderRHIRef;
typedef TSharedRef<class FVulkanHullShaderRHI>          FVulkanHullShaderRHIRef;
typedef TSharedRef<class FVulkanDomainShaderRHI>        FVulkanDomainShaderRHIRef;
typedef TSharedRef<class FVulkanGeometryShaderRHI>      FVulkanGeometryShaderRHIRef;
typedef TSharedRef<class FVulkanPixelShaderRHI>         FVulkanPixelShaderRHIRef;
typedef TSharedRef<class FVulkanComputeShaderRHI>       FVulkanComputeShaderRHIRef;
typedef TSharedRef<class FVulkanRayTracingShader>    FVulkanRayTracingShaderRef;
typedef TSharedRef<class FVulkanRayGenShaderRHI>        FVulkanRayGenShaderRHIRef;
typedef TSharedRef<class FVulkanRayAnyHitShaderRHI>     FVulkanRayAnyHitShaderRHIRef;
typedef TSharedRef<class FVulkanRayClosestHitShaderRHI> FVulkanRayClosestHitShaderRHIRef;
typedef TSharedRef<class FVulkanRayMissShaderRHI>       FVulkanRayMissShaderRHIRef;

typedef TArray<uint32> FSpirvArray;

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
    VulkanBindingType_SampledImage,
    VulkanBindingType_StorageImage,
    VulkanBindingType_StorageBufferRead,
    VulkanBindingType_StorageBufferReadWrite,
    VulkanBindingType_Sampler,
    VulkanBindingType_TexelBufferRead,
    VulkanBindingType_TexelBufferReadWrite,
    VulkanBindingType_Count = VulkanBindingType_TexelBufferReadWrite + 1,
};

inline const CHAR* ToString(EVulkanBindingType Binding)
{
    static constexpr const char* const BindingTypeStrings[]
    {
        "UniformBuffer",
        "SampledImage",
        "StorageImage",
        "StorageBufferRead",
        "StorageBufferReadWrite",
        "Sampler",
        "TexelBufferRead",
        "TexelBufferReadWrite",
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
        uint8              OriginalBindingIndex;
        FString            DebugName;
    };
    
    TArray<FBindingOffsets>  BindingOffsets;
    TArray<FResourceBinding> ResourceBindings;
    uint32                   NumPushConstants;
};

class FVulkanShaderModule : public FVulkanDeviceChild, public FVulkanRefCounted
{
public:
    FVulkanShaderModule(FVulkanDevice* InDevice, VkShaderModule InShaderModule);
    ~FVulkanShaderModule();

    VkShaderModule GetVkShaderModule() const
    {
        return ShaderModule;
    }

private:
    VkShaderModule ShaderModule;
};

class FVulkanShader : public FVulkanDeviceChild
{
public:
    FVulkanShader(FVulkanDevice* InDevice, EShaderVisibility InShaderVisibility);
    ~FVulkanShader();

    bool Initialize(const TArray<uint8>& InCode);

    FVulkanShaderModuleRef GetOrCreateShaderModule(class FVulkanPipelineLayout* Layout);
    bool PatchShaderBindings(FSpirvArray& OutSpirv, uint32 DescriptorSetIndex);
    bool StripGoogleSpirvRequirements(const FSpirvArray& InWords, FSpirvArray& OutWords);
    bool ValidateNoGoogleSpirvRequirements(const FSpirvArray& Words, FString* OutErrorMessage);

    EShaderVisibility GetShaderVisibility() const
    {
        return ShaderVisibility;
    }

    const FVulkanShaderInfo& GetShaderInfo() const
    {
        return ShaderInfo;
    }

    const CHAR* GetEntryPoint() const
    {
        return *EntryPoint;
    }

protected:
    bool InitializeShaderLayout();

    FSpirvArray                          SpirvCode;
    FString                              EntryPoint;
    FVulkanShaderInfo                    ShaderInfo;
    EShaderVisibility                    ShaderVisibility;
    TMap<uint32, FVulkanShaderModuleRef> ShaderModules;
    FCriticalSection                     ShaderModulesCS;
};

class FVulkanVertexShaderRHI : public FRHIVertexShader, public FVulkanShader
{
public:
    FVulkanVertexShaderRHI(FVulkanDevice* InDevice)
        : FRHIVertexShader()
        , FVulkanShader(InDevice, ShaderVisibility_Vertex)
    {
    }

    // FRHIShader Interface
    virtual void* GetRHINativeHandle() override final { return reinterpret_cast<void*>(&SpirvCode); }
    virtual void* GetRHIBaseInterface() { return static_cast<FVulkanShader*>(this); }
};

class FVulkanHullShaderRHI : public FRHIHullShader, public FVulkanShader
{
public:
    FVulkanHullShaderRHI(FVulkanDevice* InDevice)
        : FRHIHullShader()
        , FVulkanShader(InDevice, ShaderVisibility_Hull)
    {
    }

    // FRHIShader Interface
    virtual void* GetRHINativeHandle() override final { return reinterpret_cast<void*>(&SpirvCode); }
    virtual void* GetRHIBaseInterface() { return static_cast<FVulkanShader*>(this); }
};

class FVulkanDomainShaderRHI : public FRHIDomainShader, public FVulkanShader
{
public:
    FVulkanDomainShaderRHI(FVulkanDevice* InDevice)
        : FRHIDomainShader()
        , FVulkanShader(InDevice, ShaderVisibility_Domain)
    {
    }

    // FRHIShader Interface
    virtual void* GetRHINativeHandle() override final { return reinterpret_cast<void*>(&SpirvCode); }
    virtual void* GetRHIBaseInterface() { return static_cast<FVulkanShader*>(this); }
};

class FVulkanGeometryShaderRHI : public FRHIGeometryShader, public FVulkanShader
{
public:
    FVulkanGeometryShaderRHI(FVulkanDevice* InDevice)
        : FRHIGeometryShader()
        , FVulkanShader(InDevice, ShaderVisibility_Geometry)
    {
    }

    // FRHIShader Interface
    virtual void* GetRHINativeHandle() override final { return reinterpret_cast<void*>(&SpirvCode); }
    virtual void* GetRHIBaseInterface() { return static_cast<FVulkanShader*>(this); }
};

class FVulkanPixelShaderRHI : public FRHIPixelShader, public FVulkanShader
{
public:
    FVulkanPixelShaderRHI(FVulkanDevice* InDevice)
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

class FVulkanRayGenShaderRHI : public FRHIRayGenShader, public FVulkanRayTracingShader
{
public:
    FVulkanRayGenShaderRHI(FVulkanDevice* InDevice)
        : FRHIRayGenShader()
        , FVulkanRayTracingShader(InDevice)
    {
    }

    // FRHIShader Interface
    virtual void* GetRHINativeHandle() override final { return reinterpret_cast<void*>(&SpirvCode); }
    virtual void* GetRHIBaseInterface() { return static_cast<FVulkanRayTracingShader*>(this); }
};

class FVulkanRayAnyHitShaderRHI : public FRHIRayAnyHitShader, public FVulkanRayTracingShader
{
public:
    FVulkanRayAnyHitShaderRHI(FVulkanDevice* InDevice)
        : FRHIRayAnyHitShader()
        , FVulkanRayTracingShader(InDevice)
    {
    }

    // FRHIShader Interface
    virtual void* GetRHINativeHandle() override final { return reinterpret_cast<void*>(&SpirvCode); }
    virtual void* GetRHIBaseInterface() { return static_cast<FVulkanRayTracingShader*>(this); }
};

class FVulkanRayClosestHitShaderRHI : public FRHIRayClosestHitShader, public FVulkanRayTracingShader
{
public:
    
    FVulkanRayClosestHitShaderRHI(FVulkanDevice* InDevice)
        : FRHIRayClosestHitShader()
        , FVulkanRayTracingShader(InDevice)
    {
    }

    // FRHIShader Interface
    virtual void* GetRHINativeHandle() override final { return reinterpret_cast<void*>(&SpirvCode); }
    virtual void* GetRHIBaseInterface() { return static_cast<FVulkanRayTracingShader*>(this); }
};

class FVulkanRayMissShaderRHI : public FRHIRayMissShader, public FVulkanRayTracingShader
{
public:
    FVulkanRayMissShaderRHI(FVulkanDevice* InDevice)
        : FRHIRayMissShader()
        , FVulkanRayTracingShader(InDevice)
    {
    }

    // FRHIShader Interface
    virtual void* GetRHINativeHandle() override final { return reinterpret_cast<void*>(&SpirvCode); }
    virtual void* GetRHIBaseInterface() { return static_cast<FVulkanRayTracingShader*>(this); }
};

class FVulkanComputeShaderRHI : public FRHIComputeShader, public FVulkanShader
{
public:
    FVulkanComputeShaderRHI(FVulkanDevice* InDevice)
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
