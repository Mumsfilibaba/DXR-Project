#pragma once
#include "ResourceBase.h"

enum class EShaderStage
{
    Vertex        = 1,
    Hull          = 2,
    Domain        = 3,
    Geometry      = 4,
    Mesh          = 5,
    Amplification = 6,
    Pixel         = 7,
    Compute       = 8,
    RayGen        = 9,
    RayAnyHit     = 10,
    RayClosestHit = 11,
    RayMiss       = 12,
};

struct ShaderParameterInfo
{
    UInt32 NumConstantBuffers      = 0;
    UInt32 NumShaderResourceViews  = 0;
    UInt32 NumUnorderedAccessViews = 0;
    UInt32 NumSamplerStates        = 0;
};

class Shader : public Resource
{
public:
    virtual class VertexShader*   AsVertexShader()   { return nullptr; }
    virtual class HullShader*     AsHullShader()     { return nullptr; }
    virtual class DomainShader*   AsDomainShader()   { return nullptr; }
    virtual class GeometryShader* AsGeometryShader() { return nullptr; }
    virtual class PixelShader*    AsPixelShader()    { return nullptr; }

    virtual class ComputeShader* AsComputeShader() { return nullptr; }

    virtual class RayGenShader*        AsRayGenShader()        { return nullptr; }
    virtual class RayAnyHitShader*     AsRayAnyHitShader()     { return nullptr; }
    virtual class RayClosestHitShader* AsRayClosestHitShader() { return nullptr; }
    virtual class RayMissShader*       AsRayMissShader()       { return nullptr; }

    virtual void GetShaderParameterInfo(ShaderParameterInfo& OutShaderParameterInfo) const = 0;

    // Returns false if no parameter with the specified name exists
    virtual Bool GetConstantBufferIndexByName(const std::string& InName, UInt32& OutIndex) const      = 0;
    virtual Bool GetUnorderedAccessViewIndexByName(const std::string& InName, UInt32& OutIndex) const = 0;
    virtual Bool GetShaderResourceViewIndexByName(const std::string& InName, UInt32& OutIndex) const  = 0;
    virtual Bool GetSamplerIndexByName(const std::string& InName, UInt32& OutIndex) const             = 0;
};

class ComputeShader : public Shader
{
public:
    virtual ComputeShader* AsComputeShader() { return this; }

    virtual XMUINT3 GetThreadGroupXYZ() const = 0;
};

class VertexShader : public Shader
{
public:
    virtual VertexShader* AsVertexShader() { return this; }
};

class HullShader : public Shader
{
public:
    virtual HullShader* AsHullShader() { return this; }
};

class DomainShader : public Shader
{
public:
    virtual DomainShader* AsDomainShader() { return this; }
};

class GeometryShader : public Shader
{
public:
    virtual GeometryShader* AsGeometryShader() { return this; }
};

class MeshShader : public Shader
{
};

class AmplificationShader : public Shader
{
};

class PixelShader : public Shader
{
public:
    virtual PixelShader* AsPixelShader() { return this; }
};

class RayGenShader : public Shader
{
public:
    virtual RayGenShader* AsRayGenShader() { return this; }
};

class RayAnyHitShader : public Shader
{
public:
    virtual RayAnyHitShader* AsRayAnyHitShader() { return this; }
};

class RayClosestHitShader : public Shader
{
public:
    virtual RayClosestHitShader* AsRayClosestHitShader() { return this; }
};

class RayMissShader : public Shader
{
public:
    virtual RayMissShader* AsRayMissShader() { return this; }
};

// Helpers
inline Bool ShaderStageIsGraphics(EShaderStage ShaderStage)
{
    switch (ShaderStage)
    {
        case EShaderStage::Vertex:
        case EShaderStage::Hull:
        case EShaderStage::Domain:
        case EShaderStage::Geometry:
        case EShaderStage::Pixel:
        case EShaderStage::Mesh:
        case EShaderStage::Amplification:
        {
            return true;
        }

        default:
        {
            return false;
        }
    }
}

inline Bool ShaderStageIsCompute(EShaderStage ShaderStage)
{
    switch (ShaderStage)
    {
        case EShaderStage::Compute:
        case EShaderStage::RayGen:
        case EShaderStage::RayClosestHit:
        case EShaderStage::RayAnyHit:
        case EShaderStage::RayMiss:
        {
            return true;
        }

        default:
        {
            return false;
        }
    }
}

inline Bool ShaderStageIsRayTracing(EShaderStage ShaderStage)
{
    switch (ShaderStage)
    {
        case EShaderStage::RayGen:
        case EShaderStage::RayClosestHit:
        case EShaderStage::RayAnyHit:
        case EShaderStage::RayMiss:
        {
            return true;
        }

        default:
        {
            return false;
        }
    }
}