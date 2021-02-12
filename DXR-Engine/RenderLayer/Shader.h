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

class Shader : public Resource
{
public:
    Shader() = default;
    ~Shader() = default;

    virtual class VertexShader* AsVertexShader() { return nullptr; }
    virtual class PixelShader* AsPixelShader() { return nullptr; }

    virtual class ComputeShader* AsComputeShader() { return nullptr; }
};

class ComputeShader : public Shader
{
};

class VertexShader : public Shader
{
};

class HullShader : public Shader
{
};

class DomainShader : public Shader
{
};

class GeometryShader : public Shader
{
};

class MeshShader : public Shader
{
};

class AmplificationShader : public Shader
{
};

class PixelShader : public Shader
{
};

class RayGenShader : public Shader
{
};

class RayAnyHitShader : public Shader
{
};

class RayClosestHitShader : public Shader
{
};

class RayMissShader : public Shader
{
};

// helpers
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