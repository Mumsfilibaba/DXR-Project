#pragma once
#include "MetalObject.h"
#include "MetalDeviceContext.h"

#include "RHI/RHIResources.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// EShaderVisibility

enum EShaderVisibility
{
    ShaderVisibility_Compute  = 0,
    ShaderVisibility_Vertex   = 1,
    ShaderVisibility_Pixel    = 2,
    ShaderVisibility_Count    = ShaderVisibility_Pixel + 1
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalShader

class CMetalShader : public CMetalObject
{
public:
    CMetalShader(CMetalDeviceContext* InDevice, EShaderVisibility InVisibility, const TArray<uint8>& InCode);
    ~CMetalShader();

public:

    id<MTLLibrary> GetMTLLibrary() const { return Library; }

    id<MTLFunction> GetMTLFunction() const { return Function; }

    EShaderVisibility GetVisbility() const { return Visbility; }

protected:
    id<MTLLibrary>    Library;
    NSString*         FunctionName;

    EShaderVisibility Visbility;
    
    // TODO: Release after use, high memory usage to keep this
    id<MTLFunction> Function;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalVertexShader

class CMetalVertexShader : public CRHIVertexShader, public CMetalShader
{
public:

    CMetalVertexShader(CMetalDeviceContext* InDevice, const TArray<uint8>& InCode)
        : CRHIVertexShader()
        , CMetalShader(InDevice, ShaderVisibility_Vertex, InCode)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIShader Interface

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(GetMTLFunction()); }

    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<CMetalShader*>(this)); }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalPixelShader

class CMetalPixelShader : public CRHIPixelShader, public CMetalShader
{
public:

    CMetalPixelShader(CMetalDeviceContext* InDevice, const TArray<uint8>& InCode)
        : CRHIPixelShader()
        , CMetalShader(InDevice, ShaderVisibility_Pixel, InCode)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIShader Interface

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(GetMTLFunction()); }

    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<CMetalShader*>(this)); }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalRayTracingShader

class CMetalRayTracingShader : public CMetalShader
{
public:
    CMetalRayTracingShader(CMetalDeviceContext* InDevice, const TArray<uint8>& InCode)
        : CMetalShader(InDevice, ShaderVisibility_Compute, InCode)
    { }

public:

    static bool GetRayTracingShaderReflection(class CMetalRayTracingShader* Shader);
    
    const String& GetIdentifier() const { return Identifier; }

protected:
    String Identifier;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalRayGenShader

class CMetalRayGenShader : public CRHIRayGenShader, public CMetalRayTracingShader
{
public:

    CMetalRayGenShader(CMetalDeviceContext* InDevice, const TArray<uint8>& InCode)
        : CRHIRayGenShader()
        , CMetalRayTracingShader(InDevice, InCode)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIShader Interface

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(GetMTLFunction()); }

    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<CMetalRayTracingShader*>(this)); }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalRayAnyHitShader

class CMetalRayAnyHitShader : public CRHIRayAnyHitShader, public CMetalRayTracingShader
{
public:
    
    CMetalRayAnyHitShader(CMetalDeviceContext* InDevice, const TArray<uint8>& InCode)
        : CRHIRayAnyHitShader()
        , CMetalRayTracingShader(InDevice, InCode)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIShader Interface

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(GetMTLFunction()); }

    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<CMetalRayTracingShader*>(this)); }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalRayClosestHitShader

class CMetalRayClosestHitShader : public CRHIRayClosestHitShader, public CMetalRayTracingShader
{
public:
    
    CMetalRayClosestHitShader(CMetalDeviceContext* InDevice, const TArray<uint8>& InCode)
        : CRHIRayClosestHitShader()
        , CMetalRayTracingShader(InDevice, InCode)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIShader Interface

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(GetMTLFunction()); }

    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<CMetalRayTracingShader*>(this)); }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalRayMissShader

class CMetalRayMissShader : public CRHIRayMissShader, public CMetalRayTracingShader
{
public:

    CMetalRayMissShader(CMetalDeviceContext* InDevice, const TArray<uint8>& InCode)
        : CRHIRayMissShader()
        , CMetalRayTracingShader(InDevice, InCode)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIShader Interface

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(GetMTLFunction()); }

    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<CMetalRayTracingShader*>(this)); }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalComputeShader

class CMetalComputeShader : public CRHIComputeShader, public CMetalShader
{
public:

    CMetalComputeShader(CMetalDeviceContext* InDevice, const TArray<uint8>& InCode)
        : CRHIComputeShader()
        , CMetalShader(InDevice, ShaderVisibility_Compute, InCode)
        , ThreadGroupXYZ(1, 1, 1)
    { }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIShader Interface

    virtual void* GetRHIBaseResource() override final { return reinterpret_cast<void*>(GetMTLFunction()); }

    virtual void* GetRHIBaseShader() override final { return reinterpret_cast<void*>(static_cast<CMetalShader*>(this)); }

    virtual CIntVector3 GetThreadGroupXYZ() const override final { return ThreadGroupXYZ; }

protected:
    CIntVector3 ThreadGroupXYZ;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// GetMetalShader

inline CMetalShader* GetMetalShader(CRHIShader* Shader)
{
    return Shader ? reinterpret_cast<CMetalShader*>(Shader->GetRHIBaseShader()) : nullptr;
}

inline CMetalRayTracingShader* MetalRayTracingShaderCast(CRHIRayTracingShader* Shader)
{
    return Shader ? reinterpret_cast<CMetalRayTracingShader*>(Shader->GetRHIBaseShader()) : nullptr;
}

#pragma clang diagnostic pop
