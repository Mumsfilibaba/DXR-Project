#pragma once
#include "Core/Containers/Map.h"
#include "RHI/RHIShader.h"
#include "RHI/RHIResources.h"
#include "VulkanRHI/VulkanDeviceChild.h"
#include "VulkanRHI/VulkanRefCounted.h"

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

enum EBindingType : uint8
{
    BindingType_UniformBuffer = 0,
    BindingType_SampledImage,
    BindingType_StorageImage,
    BindingType_StorageBufferRead,
    BindingType_StorageBufferReadWrite,
    BindingType_Sampler,
    BindingType_TexelBufferRead,
    BindingType_TexelBufferReadWrite,
    BindingType_Count = BindingType_TexelBufferReadWrite + 1,
};

inline const CHAR* ToString(EBindingType Binding)
{
    static constexpr const char* BindingTypeStrings[]
    {
        "SampledImage",
        "UniformBuffer",
        "StorageImage",
        "StorageBufferRead",
        "StorageBufferReadWrite",
        "Sampler",
        "TexelBufferRead",
        "TexelBufferReadWrite",
    };
    
    static_assert(ARRAY_COUNT(BindingTypeStrings) == BindingType_Count, "BindingTypeStrings is out of date");
    return Binding < BindingType_Count ? BindingTypeStrings[Binding] : "Unknown BindingType";
}

inline VkDescriptorType GetDescriptorTypeFromBindingType(EBindingType BindingType)
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
        VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
        // UAV (Buffer)
        VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
    };

    static_assert(ARRAY_COUNT(DescriptorTypes) == BindingType_Count, "The DescriptorTypes array is out of date");
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
        EBindingType BindingType;
        uint8        BindingIndex;
        uint8        OriginalBindingIndex;
        FString      DebugName;
    };
    
    TArray<FBindingOffsets>  BindingOffsets;
    TArray<FResourceBinding> ResourceBindings;
    uint32                   NumPushConstants;
};

class FVulkanShaderModule : public FVulkanRefCounted
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

    // Creates a ShaderModule based on what DescriptorSetIndex we need to use
    TSharedRef<FVulkanShaderModule> GetOrCreateShaderModule(class FVulkanPipelineLayout* Layout);

    // Patch the shader bindings based on the DescriptorSet-Index and receive the new code with the patched bindings
    bool PatchShaderBindings(FSpirvArray& OutSpirv, uint32 DescriptorSetIndex);

    EShaderVisibility GetShaderVisibility() const
    {
        return ShaderVisibility;
    }

    const FVulkanShaderInfo& GetShaderInfo() const
    {
        return ShaderInfo;
    }

protected:
    bool InitializeShaderLayout();
    
    FSpirvArray       SpirvCode;
    FVulkanShaderInfo ShaderInfo;
    EShaderVisibility ShaderVisibility;
    
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

public: 

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

public: 

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

public: 

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

public: 

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

public: 

    // FRHIShader Interface
    virtual void* GetRHINativeHandle() override final { return reinterpret_cast<void*>(&SpirvCode); }

    virtual void* GetRHIBaseInterface() { return static_cast<FVulkanShader*>(this); }
};


class FVulkanRayTracingShader : public FVulkanShader
{
public:
    FVulkanRayTracingShader(FVulkanDevice* InDevice)
        : FVulkanShader(InDevice, ShaderVisibility_Compute)
    {
    }

    static bool GetRayTracingShaderReflection(class FVulkanRayTracingShader* Shader);
    
    const FString& GetIdentifier() const { return Identifier; }

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

public: 

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

public: 

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

public: 

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

public: 

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

public: 

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
