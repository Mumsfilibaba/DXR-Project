#pragma once
#include "MetalObject.h"
#include "MetalDeviceContext.h"
#include "RHI/RHIResources.h"
#include "RHI/RHIShader.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

typedef TSharedRef<class FMetalShader>              FMetalShaderRef;

typedef TSharedRef<class FMetalVertexShader>        FMetalVertexShaderRef;
typedef TSharedRef<class FMetalPixelShader>         FMetalPixelShaderRef;

typedef TSharedRef<class FMetalComputeShader>       FMetalComputeShaderRef;

typedef TSharedRef<class FMetalRayTracingShader>    FMetalRayTracingShaderRef;
typedef TSharedRef<class FMetalRayGenShader>        FMetalRayGenShaderRef;
typedef TSharedRef<class FMetalRayAnyHitShader>     FMetalRayAnyHitShaderRef;
typedef TSharedRef<class FMetalRayClosestHitShader> FMetalRayClosestHitShaderRef;
typedef TSharedRef<class FMetalRayMissShader>       FMetalRayMissShaderRef;


enum EShaderVisibility : uint8
{
    ShaderVisibility_Compute  = 0,
    ShaderVisibility_Vertex   = 1,
    ShaderVisibility_Pixel    = 2,
    ShaderVisibility_Count    = ShaderVisibility_Pixel + 1
};


class FMetalShader : public FMetalObject
{
public:
    FMetalShader(FMetalDeviceContext* InDevice, EShaderVisibility InVisibility);
    ~FMetalShader();
    
    bool Initialize(const TArray<uint8>& InCode);

    id<MTLLibrary> GetMTLLibrary()  const
    {
        return Library;
    }
    
    id<MTLFunction> GetMTLFunction() const
    {
        return Function;
    }

    EShaderVisibility GetVisibility() const
    {
        return Visibility;
    }

protected:
    id<MTLLibrary>    Library;
    NSString*         FunctionName;
    EShaderVisibility Visibility;
    // TODO: Release after use, high memory usage to keep this
    id<MTLFunction>   Function;
};


class FMetalVertexShader : public FRHIVertexShader, public FMetalShader
{
public:
    FMetalVertexShader(FMetalDeviceContext* InDevice)
        : FRHIVertexShader()
        , FMetalShader(InDevice, ShaderVisibility_Vertex)
    {
    }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(GetMTLFunction()); }
    
    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<FMetalShader*>(this)); }
};


class FMetalPixelShader : public FRHIPixelShader, public FMetalShader
{
public:
    FMetalPixelShader(FMetalDeviceContext* InDevice)
        : FRHIPixelShader()
        , FMetalShader(InDevice, ShaderVisibility_Pixel)
    {
    }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(GetMTLFunction()); }
    
    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<FMetalShader*>(this)); }
};


class FMetalRayTracingShader : public FMetalShader
{
public:
    FMetalRayTracingShader(FMetalDeviceContext* InDevice)
        : FMetalShader(InDevice, ShaderVisibility_Compute)
    {
    }

    static bool GetRayTracingShaderReflection(class FMetalRayTracingShader* Shader);
    
    const FString& GetIdentifier() const { return Identifier; }

protected:
    FString Identifier;
};


class FMetalRayGenShader : public FRHIRayGenShader, public FMetalRayTracingShader
{
public:
    FMetalRayGenShader(FMetalDeviceContext* InDevice)
        : FRHIRayGenShader()
        , FMetalRayTracingShader(InDevice)
    {
    }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(GetMTLFunction()); }
    
    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<FMetalRayTracingShader*>(this)); }
};


class FMetalRayAnyHitShader : public FRHIRayAnyHitShader, public FMetalRayTracingShader
{
public:
    FMetalRayAnyHitShader(FMetalDeviceContext* InDevice)
        : FRHIRayAnyHitShader()
        , FMetalRayTracingShader(InDevice)
    {
    }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(GetMTLFunction()); }
    
    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<FMetalRayTracingShader*>(this)); }
};


class FMetalRayClosestHitShader : public FRHIRayClosestHitShader, public FMetalRayTracingShader
{
public:
    
    FMetalRayClosestHitShader(FMetalDeviceContext* InDevice)
        : FRHIRayClosestHitShader()
        , FMetalRayTracingShader(InDevice)
    {
    }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(GetMTLFunction()); }
    
    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<FMetalRayTracingShader*>(this)); }
};


class FMetalRayMissShader : public FRHIRayMissShader, public FMetalRayTracingShader
{
public:
    FMetalRayMissShader(FMetalDeviceContext* InDevice)
        : FRHIRayMissShader()
        , FMetalRayTracingShader(InDevice)
    {
    }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(GetMTLFunction()); }
    
    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<FMetalRayTracingShader*>(this)); }
};


class FMetalComputeShader : public FRHIComputeShader, public FMetalShader
{
public:
    FMetalComputeShader(FMetalDeviceContext* InDevice)
        : FRHIComputeShader()
        , FMetalShader(InDevice, ShaderVisibility_Compute)
        , ThreadGroupXYZ(1, 1, 1)
    {
    }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(GetMTLFunction()); }
    
    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<FMetalShader*>(this)); }
};


inline FMetalShader* GetMetalShader(FRHIShader* Shader)
{
    return Shader ? reinterpret_cast<FMetalShader*>(Shader->GetRHIBaseShader()) : nullptr;
}

inline FMetalRayTracingShader* MetalRayTracingShaderCast(FRHIRayTracingShader* Shader)
{
    return Shader ? reinterpret_cast<FMetalRayTracingShader*>(Shader->GetRHIBaseShader()) : nullptr;
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
