#pragma once
#include "VulkanDeviceChild.h"
#include "RHI/RHIShader.h"
#include "RHI/RHIResources.h"

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
    switch(ShaderVisibility)
    {
    case ShaderVisibility_Vertex:   return "Vertex";
    case ShaderVisibility_Hull:     return "Hull";
    case ShaderVisibility_Domain:   return "Domain";
    case ShaderVisibility_Geometry: return "Geometry";
    case ShaderVisibility_Pixel:    return "Pixel";
    case ShaderVisibility_Compute:  return "Compute";
    default:                        return "Unknown";
    }
}

enum EBindingType : uint32
{
    BindingType_UniformBuffer = 0,
    BindingType_SampledImage,
    BindingType_StorageImage,
    BindingType_StorageBuffer,
    BindingType_Sampler,
    BindingType_Count = BindingType_Sampler + 1,
};

inline const CHAR* ToString(EBindingType Binding)
{
    switch(Binding)
    {
    case BindingType_SampledImage:  return "SampledImage";
    case BindingType_UniformBuffer: return "UniformBuffer";
    case BindingType_StorageImage:  return "StorageImage";
    case BindingType_StorageBuffer: return "StorageBuffer";
    case BindingType_Sampler:       return "Sampler";
    default:                        return "Unknown";
    }
}


struct FVulkanShaderBinding
{
    FVulkanShaderBinding()
        : EncodedBinding(0)
    {
    }
    
    FVulkanShaderBinding(uint32 InBindingType, uint32 InDescriptorSet, uint32 InBinding, uint32 InRegisterIndex)
        : BindingType(InBindingType)
        , DescriptorSet(InDescriptorSet)
        , Binding(InBinding)
        , RegisterIndex(InRegisterIndex)
    {
    }
    
    bool operator==(const FVulkanShaderBinding& Other) const
    {
        return EncodedBinding == Other.EncodedBinding;
    }

    bool operator!=(const FVulkanShaderBinding& Other) const
    {
        return EncodedBinding != Other.EncodedBinding;
    }
    
    union
    {
        struct
        {
            uint32 BindingType   : 8;
            uint32 DescriptorSet : 8;
            uint32 Binding       : 8;
            uint32 RegisterIndex : 8;
        };
        
        uint32 EncodedBinding;
    };
};

class FVulkanShaderLayout
{
public:
    FVulkanShaderLayout()
        : UniformBufferBindings()
        , SampledImageBindings()
        , StorageImageBindings()
        , SRVStorageBufferBindings()
        , UAVStorageBufferBindings()
        , SamplerBindings()
        , NumPushConstants(0)
    {
    }
    
    TArray<FVulkanShaderBinding> UniformBufferBindings;
    TArray<FVulkanShaderBinding> SampledImageBindings;
    TArray<FVulkanShaderBinding> StorageImageBindings;
    TArray<FVulkanShaderBinding> SRVStorageBufferBindings;
    TArray<FVulkanShaderBinding> UAVStorageBufferBindings;
    TArray<FVulkanShaderBinding> SamplerBindings;
    uint32                       NumPushConstants;
    uint32                       NumTotalBindings;
};

class FVulkanShader : public FVulkanDeviceChild
{
public:
    FVulkanShader(FVulkanDevice* InDevice, EShaderVisibility InShaderVisibility);
    ~FVulkanShader();

    bool Initialize(const TArray<uint8>& InCode);
    bool PatchShaderBindings(const TArray<uint8>& InCode, TArray<uint8>& OutCode);

    VkShaderModule GetVkShaderModule() const
    {
        return ShaderModule;
    }

    EShaderVisibility GetShaderVisibility() const
    {
        return ShaderVisibility;
    }

    const FVulkanShaderLayout* GetShaderLayout() const
    {
        return ShaderLayout.NumTotalBindings > 0 ? &ShaderLayout : nullptr;
    }

protected:
    TArray<uint8>       SpirvCode;
    VkShaderModule      ShaderModule;
    EShaderVisibility   ShaderVisibility;
    FVulkanShaderLayout ShaderLayout;
};

class FVulkanVertexShader : public FRHIVertexShader, public FVulkanShader
{
public:
    FVulkanVertexShader(FVulkanDevice* InDevice)
        : FRHIVertexShader()
        , FVulkanShader(InDevice, ShaderVisibility_Vertex)
    {
    }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(GetVkShaderModule()); }
    virtual void* GetRHIBaseShader()   override final { return reinterpret_cast<void*>(static_cast<FVulkanShader*>(this)); }
};

class FVulkanHullShader : public FRHIHullShader, public FVulkanShader
{
public:
    FVulkanHullShader(FVulkanDevice* InDevice)
        : FRHIHullShader()
        , FVulkanShader(InDevice, ShaderVisibility_Hull)
    {
    }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(GetVkShaderModule()); }
    virtual void* GetRHIBaseShader()   override final { return reinterpret_cast<void*>(static_cast<FVulkanShader*>(this)); }
};

class FVulkanDomainShader : public FRHIDomainShader, public FVulkanShader
{
public:
    FVulkanDomainShader(FVulkanDevice* InDevice)
        : FRHIDomainShader()
        , FVulkanShader(InDevice, ShaderVisibility_Domain)
    {
    }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(GetVkShaderModule()); }
    virtual void* GetRHIBaseShader()   override final { return reinterpret_cast<void*>(static_cast<FVulkanShader*>(this)); }
};

class FVulkanGeometryShader : public FRHIGeometryShader, public FVulkanShader
{
public:
    FVulkanGeometryShader(FVulkanDevice* InDevice)
        : FRHIGeometryShader()
        , FVulkanShader(InDevice, ShaderVisibility_Geometry)
    {
    }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(GetVkShaderModule()); }
    virtual void* GetRHIBaseShader()   override final { return reinterpret_cast<void*>(static_cast<FVulkanShader*>(this)); }
};

class FVulkanPixelShader : public FRHIPixelShader, public FVulkanShader
{
public:
    FVulkanPixelShader(FVulkanDevice* InDevice)
        : FRHIPixelShader()
        , FVulkanShader(InDevice, ShaderVisibility_Pixel)
    {
    }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(GetVkShaderModule()); }
    virtual void* GetRHIBaseShader()   override final { return reinterpret_cast<void*>(static_cast<FVulkanShader*>(this)); }
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

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(GetVkShaderModule()); }
    virtual void* GetRHIBaseShader()   override final { return reinterpret_cast<void*>(static_cast<FVulkanRayTracingShader*>(this)); }
};

class FVulkanRayAnyHitShader : public FRHIRayAnyHitShader, public FVulkanRayTracingShader
{
public:
    FVulkanRayAnyHitShader(FVulkanDevice* InDevice)
        : FRHIRayAnyHitShader()
        , FVulkanRayTracingShader(InDevice)
    {
    }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(GetVkShaderModule()); }
    virtual void* GetRHIBaseShader()   override final { return reinterpret_cast<void*>(static_cast<FVulkanRayTracingShader*>(this)); }
};

class FVulkanRayClosestHitShader : public FRHIRayClosestHitShader, public FVulkanRayTracingShader
{
public:
    
    FVulkanRayClosestHitShader(FVulkanDevice* InDevice)
        : FRHIRayClosestHitShader()
        , FVulkanRayTracingShader(InDevice)
    {
    }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(GetVkShaderModule()); }
    virtual void* GetRHIBaseShader()   override final { return reinterpret_cast<void*>(static_cast<FVulkanRayTracingShader*>(this)); }
};

class FVulkanRayMissShader : public FRHIRayMissShader, public FVulkanRayTracingShader
{
public:
    FVulkanRayMissShader(FVulkanDevice* InDevice)
        : FRHIRayMissShader()
        , FVulkanRayTracingShader(InDevice)
    {
    }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(GetVkShaderModule()); }
    virtual void* GetRHIBaseShader()   override final { return reinterpret_cast<void*>(static_cast<FVulkanRayTracingShader*>(this)); }
};

class FVulkanComputeShader : public FRHIComputeShader, public FVulkanShader
{
public:
    FVulkanComputeShader(FVulkanDevice* InDevice)
        : FRHIComputeShader()
        , FVulkanShader(InDevice, ShaderVisibility_Compute)
    {
    }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(GetVkShaderModule()); }
    virtual void* GetRHIBaseShader()   override final { return reinterpret_cast<void*>(static_cast<FVulkanShader*>(this)); }
};

inline FVulkanShader* GetVulkanShader(FRHIShader* Shader)
{
    return Shader ? reinterpret_cast<FVulkanShader*>(Shader->GetRHIBaseShader()) : nullptr;
}

inline FVulkanRayTracingShader* GetVulkanRayTracingShader(FRHIRayTracingShader* Shader)
{
    return Shader ? reinterpret_cast<FVulkanRayTracingShader*>(Shader->GetRHIBaseShader()) : nullptr;
}
