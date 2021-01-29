#pragma once
#include "Resources.h"

class VertexShader;
class PixelShader;
class ComputeShader;

enum class EShaderStage
{
    ShaderStage_Vertex        = 1,
    ShaderStage_Hull          = 2,
    ShaderStage_Domain        = 3,
    ShaderStage_Geometry      = 4,
    ShaderStage_Mesh          = 5,
    ShaderStage_Amplification = 6,
    ShaderStage_Pixel         = 7,
    ShaderStage_Compute       = 8,
    ShaderStage_RayGen        = 9,
    ShaderStage_RayAnyHit     = 10,
    ShaderStage_RayClosestHit = 11,
    ShaderStage_RayMiss       = 12,
};

class Shader : public PipelineResource
{
public:
    Shader()  = default;
    ~Shader() = default;

    virtual VertexShader* AsVertexShader()
    {
        return nullptr;
    }

    virtual const VertexShader* AsVertexShader() const
    {
        return nullptr;
    }

    virtual PixelShader* AsPixelShader()
    {
        return nullptr;
    }

    virtual const PixelShader* AsPixelShader() const
    {
        return nullptr;
    }

    virtual ComputeShader* AsComputeShader()
    {
        return nullptr;
    }

    virtual const ComputeShader* AsComputeShader() const
    {
        return nullptr;
    }
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

class RayHitShader : public Shader
{
};

class RayMissShader : public Shader
{
};