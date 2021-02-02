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

class RayHitShader : public Shader
{
};

class RayMissShader : public Shader
{
};