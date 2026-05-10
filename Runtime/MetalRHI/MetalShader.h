#pragma once
#include "RHI/RHIResources.h"
#include "RHI/RHIShader.h"
#include "MetalRHI/MetalDeviceChild.h"
#include "MetalRHI/MetalDevice.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

typedef TSharedRef<class FMetalShader>                 FMetalShaderRef;
typedef TSharedRef<class FMetalVertexShaderRHI>        FMetalVertexShaderRef;
typedef TSharedRef<class FMetalPixelShaderRHI>         FMetalPixelShaderRef;
typedef TSharedRef<class FMetalComputeShaderRHI>       FMetalComputeShaderRef;
typedef TSharedRef<class FMetalRayTracingShader>       FMetalRayTracingShaderRef;
typedef TSharedRef<class FMetalRayGenShaderRHI>        FMetalRayGenShaderRef;
typedef TSharedRef<class FMetalRayAnyHitShaderRHI>     FMetalRayAnyHitShaderRef;
typedef TSharedRef<class FMetalRayClosestHitShaderRHI> FMetalRayClosestHitShaderRef;
typedef TSharedRef<class FMetalRayMissShaderRHI>       FMetalRayMissShaderRef;

struct EShaderVisibility
{
    enum Type : uint8
    {
        Compute = 0,
        Vertex  = 1,
        Pixel   = 2,
        Count   = Pixel + 1
    };
};

class FMetalShader : public FMetalDeviceChild
{
public:
    FMetalShader(FMetalDevice* InDevice, EShaderVisibility::Type InVisibility);
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

    EShaderVisibility::Type GetVisibility() const
    {
        return Visibility;
    }

protected:
    id<MTLLibrary>          Library;
    NSString*               FunctionName;
    EShaderVisibility::Type Visibility;
    // TODO: Release after use, high memory usage to keep this
    id<MTLFunction>         Function;
};

class FMetalVertexShaderRHI : public FRHIVertexShader, public FMetalShader
{
public:
    FMetalVertexShaderRHI(FMetalDevice* InDevice);
    virtual ~FMetalVertexShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

class FMetalPixelShaderRHI : public FRHIPixelShader, public FMetalShader
{
public:
    FMetalPixelShaderRHI(FMetalDevice* InDevice);
    virtual ~FMetalPixelShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

class FMetalRayTracingShader : public FMetalShader
{
public:
    static bool GetRayTracingShaderReflection(class FMetalRayTracingShader* Shader);

public:
    FMetalRayTracingShader(FMetalDevice* InDevice);
    virtual ~FMetalRayTracingShader();

    const FString& GetIdentifier() const
    {
        return Identifier;
    }

protected:
    FString Identifier;
};

class FMetalRayGenShaderRHI : public FRHIRayGenShader, public FMetalRayTracingShader
{
public:
    FMetalRayGenShaderRHI(FMetalDevice* InDevice);
    virtual ~FMetalRayGenShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

class FMetalRayAnyHitShaderRHI : public FRHIRayAnyHitShader, public FMetalRayTracingShader
{
public:
    FMetalRayAnyHitShaderRHI(FMetalDevice* InDevice);
    virtual ~FMetalRayAnyHitShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

class FMetalRayClosestHitShaderRHI : public FRHIRayClosestHitShader, public FMetalRayTracingShader
{
public:
    FMetalRayClosestHitShaderRHI(FMetalDevice* InDevice);
    virtual ~FMetalRayClosestHitShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

class FMetalRayMissShaderRHI : public FRHIRayMissShader, public FMetalRayTracingShader
{
public:
    FMetalRayMissShaderRHI(FMetalDevice* InDevice);
    virtual ~FMetalRayMissShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

class FMetalComputeShaderRHI : public FRHIComputeShader, public FMetalShader
{
public:
    FMetalComputeShaderRHI(FMetalDevice* InDevice);
    virtual ~FMetalComputeShaderRHI();

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final;
    virtual void* GetRHIBaseInterface() override final;
};

inline FMetalShader* GetMetalShader(FRHIShader* Shader)
{
    return Shader ? reinterpret_cast<FMetalShader*>(Shader->GetRHIBaseInterface()) : nullptr;
}

inline FMetalRayTracingShader* GetMetalRayTracingShader(FRHIRayTracingShader* Shader)
{
    return Shader ? reinterpret_cast<FMetalRayTracingShader*>(Shader->GetRHIBaseInterface()) : nullptr;
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
