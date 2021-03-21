#pragma once
#include "ResourceBase.h"
#include "ResourceViews.h"

#include "Core/Containers/SharedPtr.h"

enum ERayTracingStructureBuildFlag
{
    RayTracingStructureBuildFlag_None            = 0x0,
    RayTracingStructureBuildFlag_AllowUpdate     = FLAG(1),
    RayTracingStructureBuildFlag_PreferFastTrace = FLAG(2),
    RayTracingStructureBuildFlag_PreferFastBuild = FLAG(3),
};

enum ERayTracingInstanceFlags
{
    RayTracingInstanceFlags_None                  = 0,
    RayTracingInstanceFlags_CullDisable           = FLAG(1),
    RayTracingInstanceFlags_FrontCounterClockwise = FLAG(2),
    RayTracingInstanceFlags_ForceOpaque           = FLAG(3),
    RayTracingInstanceFlags_ForceNonOpaque        = FLAG(4),
};

// RayTracing Geometry (Bottom Level Acceleration Structure)
class RayTracingGeometry : public Resource
{
public:
    RayTracingGeometry(uint32 InFlags)
        : Flags(InFlags)
    {
    }

    uint32 GetFlags() const { return Flags; }

private:
    uint32 Flags;
};

// RayTracing Scene (Top Level Acceleration Structure)
class RayTracingScene : public Resource
{
public:
    RayTracingScene(uint32 InFlags)
        : Flags(InFlags)
    {
    }

    virtual ShaderResourceView* GetShaderResourceView() const = 0;

    uint32 GetFlags() const { return Flags; }

private:
    uint32 Flags;
};

struct RayTracingGeometryInstance
{
    TRef<RayTracingGeometry> Instance;
    uint32     InstanceIndex = 0;
    uint32     HitGroupIndex = 0;
    uint32     Flags         = RayTracingInstanceFlags_None;
    uint32     Mask          = 0xff;
    XMFLOAT3X4 Transform;
};

struct RayPayload
{
    XMFLOAT3 Color;
    uint32   CurrentDepth;
};

struct RayIntersectionAttributes
{
    float Attrib0;
    float Attrib1;
};

struct RayTracingShaderResources
{
    void AddConstantBuffer(ConstantBuffer* Buffer)
    {
        ConstantBuffers.EmplaceBack(Buffer);
    }

    void AddShaderResourceView(ShaderResourceView* View)
    {
        ShaderResourceViews.EmplaceBack(View);
    }

    void AddUnorderedAccessView(UnorderedAccessView* View)
    {
        UnorderedAccessViews.EmplaceBack(View);
    }

    void AddSamplerState(SamplerState* State)
    {
        SamplerStates.EmplaceBack(State);
    }

    uint32 NumResources() const
    {
        return ConstantBuffers.Size() + ShaderResourceViews.Size() + UnorderedAccessViews.Size();
    }

    uint32 NumSamplers() const
    {
        return SamplerStates.Size();
    }

    void Reset()
    {
        ConstantBuffers.Clear();
        ShaderResourceViews.Clear();
        UnorderedAccessViews.Clear();
        SamplerStates.Clear();
    }

    std::string Identifier;
    TArray<ConstantBuffer*>      ConstantBuffers;
    TArray<ShaderResourceView*>  ShaderResourceViews;
    TArray<UnorderedAccessView*> UnorderedAccessViews;
    TArray<SamplerState*>        SamplerStates;
};