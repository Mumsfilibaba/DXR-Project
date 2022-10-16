#pragma once
#include "MetalObject.h"
#include "MetalDeviceContext.h"

#include "RHI/RHIResources.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

enum EShaderVisibility : uint8
{
    ShaderVisibility_Compute  = 0,
    ShaderVisibility_Vertex   = 1,
    ShaderVisibility_Pixel    = 2,
    ShaderVisibility_Count    = ShaderVisibility_Pixel + 1
};


class FMetalShader
    : public FMetalObject
{
public:
    FMetalShader(FMetalDeviceContext* InDevice, EShaderVisibility InVisibility, const TArray<uint8>& InCode);
    ~FMetalShader();

    id<MTLLibrary>  GetMTLLibrary()  const { return Library; }
    id<MTLFunction> GetMTLFunction() const { return Function; }

    EShaderVisibility GetVisbility() const { return Visbility; }

protected:
    id<MTLLibrary>    Library;
    NSString*         FunctionName;

    EShaderVisibility Visbility;
    
    // TODO: Release after use, high memory usage to keep this
    id<MTLFunction> Function;
};


class FMetalVertexShader 
    : public FRHIVertexShader
    , public FMetalShader
{
public:
    FMetalVertexShader(FMetalDeviceContext* InDevice, const TArray<uint8>& InCode)
        : FRHIVertexShader()
        , FMetalShader(InDevice, ShaderVisibility_Vertex, InCode)
    { }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(GetMTLFunction()); }
    virtual void* GetRHIBaseShader()   override final { return reinterpret_cast<void*>(static_cast<FMetalShader*>(this)); }
};


class FMetalPixelShader 
    : public FRHIPixelShader
    , public FMetalShader
{
public:
    FMetalPixelShader(FMetalDeviceContext* InDevice, const TArray<uint8>& InCode)
        : FRHIPixelShader()
        , FMetalShader(InDevice, ShaderVisibility_Pixel, InCode)
    { }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(GetMTLFunction()); }
    virtual void* GetRHIBaseShader()   override final { return reinterpret_cast<void*>(static_cast<FMetalShader*>(this)); }
};


class FMetalRayTracingShader 
    : public FMetalShader
{
public:
    FMetalRayTracingShader(FMetalDeviceContext* InDevice, const TArray<uint8>& InCode)
        : FMetalShader(InDevice, ShaderVisibility_Compute, InCode)
    { }

    static bool GetRayTracingShaderReflection(class FMetalRayTracingShader* Shader);
    
    const FString& GetIdentifier() const { return Identifier; }

protected:
    FString Identifier;
};


class FMetalRayGenShader 
    : public FRHIRayGenShader
    , public FMetalRayTracingShader
{
public:
    FMetalRayGenShader(FMetalDeviceContext* InDevice, const TArray<uint8>& InCode)
        : FRHIRayGenShader()
        , FMetalRayTracingShader(InDevice, InCode)
    { }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(GetMTLFunction()); }
    virtual void* GetRHIBaseShader()   override final { return reinterpret_cast<void*>(static_cast<FMetalRayTracingShader*>(this)); }
};


class FMetalRayAnyHitShader 
    : public FRHIRayAnyHitShader
    , public FMetalRayTracingShader
{
public:
    FMetalRayAnyHitShader(FMetalDeviceContext* InDevice, const TArray<uint8>& InCode)
        : FRHIRayAnyHitShader()
        , FMetalRayTracingShader(InDevice, InCode)
    { }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(GetMTLFunction()); }
    virtual void* GetRHIBaseShader()   override final { return reinterpret_cast<void*>(static_cast<FMetalRayTracingShader*>(this)); }
};


class FMetalRayClosestHitShader 
    : public FRHIRayClosestHitShader
    , public FMetalRayTracingShader
{
public:
    
    FMetalRayClosestHitShader(FMetalDeviceContext* InDevice, const TArray<uint8>& InCode)
        : FRHIRayClosestHitShader()
        , FMetalRayTracingShader(InDevice, InCode)
    { }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(GetMTLFunction()); }
    virtual void* GetRHIBaseShader()   override final { return reinterpret_cast<void*>(static_cast<FMetalRayTracingShader*>(this)); }
};


class FMetalRayMissShader 
    : public FRHIRayMissShader
    , public FMetalRayTracingShader
{
public:
    FMetalRayMissShader(FMetalDeviceContext* InDevice, const TArray<uint8>& InCode)
        : FRHIRayMissShader()
        , FMetalRayTracingShader(InDevice, InCode)
    { }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(GetMTLFunction()); }
    virtual void* GetRHIBaseShader()   override final { return reinterpret_cast<void*>(static_cast<FMetalRayTracingShader*>(this)); }
};


class FMetalComputeShader 
    : public FRHIComputeShader
    , public FMetalShader
{
public:
    FMetalComputeShader(FMetalDeviceContext* InDevice, const TArray<uint8>& InCode)
        : FRHIComputeShader()
        , FMetalShader(InDevice, ShaderVisibility_Compute, InCode)
        , ThreadGroupXYZ(1, 1, 1)
    { }

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(GetMTLFunction()); }
    virtual void* GetRHIBaseShader()   override final { return reinterpret_cast<void*>(static_cast<FMetalShader*>(this)); }

    virtual FIntVector3 GetThreadGroupXYZ() const override final { return ThreadGroupXYZ; }

protected:
    FIntVector3 ThreadGroupXYZ;
};


inline FMetalShader* GetMetalShader(FRHIShader* Shader)
{
    return Shader ? reinterpret_cast<FMetalShader*>(Shader->GetRHIBaseShader()) : nullptr;
}

inline FMetalRayTracingShader* MetalRayTracingShaderCast(FRHIRayTracingShader* Shader)
{
    return Shader ? reinterpret_cast<FMetalRayTracingShader*>(Shader->GetRHIBaseShader()) : nullptr;
}

#pragma clang diagnostic pop
