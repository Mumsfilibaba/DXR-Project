#pragma once
#include "RHI/RHIResources.h"
#include "RHI/RHIShader.h"
#include "RHI/MSLShaderBindings.h"
#include "MetalRHI/MetalDeviceChild.h"
#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalShaderLibraryCache.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FMetalShader : public FMetalDeviceChild
{
public:
    explicit FMetalShader(FMetalDevice* InDevice);
    ~FMetalShader();
    
    bool Initialize(const TArray<uint8>& InCode);

    id<MTLFunction> GetMTLFunction() const
    {
        return CompiledShader ? CompiledShader->Function : nil;
    }

    const TArray<FMSLShaderBinding>& GetBindings() const
    {
        return Bindings;
    }

    uint16 GetThreadGroupSizeX() const { return ThreadGroupSizeX; }
    uint16 GetThreadGroupSizeY() const { return ThreadGroupSizeY; }
    uint16 GetThreadGroupSizeZ() const { return ThreadGroupSizeZ; }

    uint16 GetShaderConstantsSize() const { return ShaderConstantsSize; }

protected:
    TSharedRef<FMetalCompiledShader> CompiledShader;
    TArray<FMSLShaderBinding>        Bindings;
    uint16                           ThreadGroupSizeX;
    uint16                           ThreadGroupSizeY;
    uint16                           ThreadGroupSizeZ;
    uint16                           ShaderConstantsSize;
};

class FMetalRayTracingShader : public FMetalShader
{
public:
    explicit FMetalRayTracingShader(FMetalDevice* InDevice);
    virtual ~FMetalRayTracingShader();

    bool Initialize(const TArray<uint8>& InCode);

    const String& GetIdentifier() const
    {
        return Identifier;
    }

protected:
    String Identifier;
};

// GetMetalShader and GetMetalRayTracingShader reinterpret the base interface, so MetalBaseType has to sit at the same address as FMetalShader
template<typename RHIShaderType, typename MetalBaseType = FMetalShader>
class TMetalShader final : public RHIShaderType, public MetalBaseType
{
public:
    explicit TMetalShader(FMetalDevice* InDevice)
        : RHIShaderType()
        , MetalBaseType(InDevice)
    {
    }

    // FRHIShader Interface
    virtual void* GetRHINativeHandle()  override final { return reinterpret_cast<void*>(this->GetMTLFunction()); }
    virtual void* GetRHIBaseInterface() override final { return static_cast<MetalBaseType*>(this); }
};

using FMetalVertexShaderRHI          = TMetalShader<FRHIVertexShader>;
using FMetalPixelShaderRHI           = TMetalShader<FRHIPixelShader>;
using FMetalMeshShaderRHI            = TMetalShader<FRHIMeshShader>;
using FMetalAmplificationShaderRHI   = TMetalShader<FRHIAmplificationShader>;
using FMetalComputeShaderRHI         = TMetalShader<FRHIComputeShader>;
using FMetalRayGenShaderRHI          = TMetalShader<FRHIRayGenShader, FMetalRayTracingShader>;
using FMetalRayAnyHitShaderRHI       = TMetalShader<FRHIRayAnyHitShader, FMetalRayTracingShader>;
using FMetalRayClosestHitShaderRHI   = TMetalShader<FRHIRayClosestHitShader, FMetalRayTracingShader>;
using FMetalRayMissShaderRHI         = TMetalShader<FRHIRayMissShader, FMetalRayTracingShader>;
using FMetalRayIntersectionShaderRHI = TMetalShader<FRHIRayIntersectionShader, FMetalRayTracingShader>;
using FMetalRayCallableShaderRHI     = TMetalShader<FRHIRayCallableShader, FMetalRayTracingShader>;

inline FMetalShader* GetMetalShader(FRHIShader* Shader)
{
    return Shader ? reinterpret_cast<FMetalShader*>(Shader->GetRHIBaseInterface()) : nullptr;
}

inline FMetalRayTracingShader* GetMetalRayTracingShader(FRHIRayTracingShader* Shader)
{
    return Shader ? reinterpret_cast<FMetalRayTracingShader*>(Shader->GetRHIBaseInterface()) : nullptr;
}

ENABLE_UNREFERENCED_VARIABLE_WARNING
