#pragma once
#include "VulkanDeviceObject.h"
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


enum EShaderVisibility : int32
{
    ShaderVisibility_All = 0,
    ShaderVisibility_Vertex,
    ShaderVisibility_Hull,
    ShaderVisibility_Domain,
    ShaderVisibility_Geometry,
    ShaderVisibility_Pixel,
    ShaderVisibility_Compute,
    ShaderVisibility_Count = ShaderVisibility_Compute + 1
};


class FVulkanShader : public FVulkanDeviceObject
{
public:
    FVulkanShader(FVulkanDevice* InDevice, EShaderVisibility InShaderVisibility);
    ~FVulkanShader();

    bool Initialize(const TArray<uint8>& InCode);

    VkShaderModule GetVkShaderModule() const 
    {
        return ShaderModule;
    }

    EShaderVisibility GetShaderVisibility() const
    {
        return ShaderVisibility;
    }

protected:
    VkShaderModule    ShaderModule;
    EShaderVisibility ShaderVisibility;
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
    
    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<FVulkanShader*>(this)); }
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
    
    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<FVulkanShader*>(this)); }
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
    
    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<FVulkanShader*>(this)); }
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
    
    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<FVulkanShader*>(this)); }
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
    
    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<FVulkanShader*>(this)); }
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
    
    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<FVulkanRayTracingShader*>(this)); }
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
    
    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<FVulkanRayTracingShader*>(this)); }
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
    
    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<FVulkanRayTracingShader*>(this)); }
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
    
    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<FVulkanRayTracingShader*>(this)); }
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
    
    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<FVulkanShader*>(this)); }
};


inline FVulkanShader* GetVulkanShader(FRHIShader* Shader)
{
    return Shader ? reinterpret_cast<FVulkanShader*>(Shader->GetRHIBaseShader()) : nullptr;
}

inline FVulkanRayTracingShader* GetVulkanRayTracingShader(FRHIRayTracingShader* Shader)
{
    return Shader ? reinterpret_cast<FVulkanRayTracingShader*>(Shader->GetRHIBaseShader()) : nullptr;
}
