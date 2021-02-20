#pragma once
#include "ResourceBase.h"
#include "ResourceViews.h"

#include <Containers/SharedPtr.h>

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
    RayTracingGeometry(UInt32 InFlags)
        : Flags(InFlags)
    {
    }

    UInt32 GetFlags() const { return Flags; }

private:
    UInt32 Flags;
};

// RayTracing Scene (Top Level Acceleration Structure)
class RayTracingScene : public Resource
{
public:
    RayTracingScene(UInt32 InFlags)
        : Flags(InFlags)
    {
    }

    UInt32 GetFlags() const { return Flags; }

private:
    UInt32 Flags;
};

struct RayTracingGeometryInstance
{
    TRef<RayTracingGeometry> Instance;
    UInt32     InstanceIndex = 0;
    UInt32     HitGroupIndex = 0;
    UInt32     Flags         = RayTracingInstanceFlags_None;
    UInt32     Mask          = 0xff;
    XMFLOAT3X4 Transform;
};

struct RayTracingShaderResources
{
    void AddConstantBuffer(const TRef<ConstantBuffer>& Buffer)
    {
        ConstantBuffers.EmplaceBack(Buffer);
    }

    void AddShaderResourceView(const TRef<ShaderResourceView>& View)
    {
        ShaderResourceViews.EmplaceBack(View);
    }

    void AddUnorderedAccessView(const TRef<UnorderedAccessView>& View)
    {
        UnorderedAccessViews.EmplaceBack(View);
    }

    void AddSamplerState(const TRef<SamplerState>& State)
    {
        SamplerStates.EmplaceBack(State);
    }

    void Reset()
    {
        ConstantBuffers.Clear();
        ShaderResourceViews.Clear();
        UnorderedAccessViews.Clear();
        SamplerStates.Clear();
    }

    TArray<TRef<ConstantBuffer>>      ConstantBuffers;
    TArray<TRef<ShaderResourceView>>  ShaderResourceViews;
    TArray<TRef<UnorderedAccessView>> UnorderedAccessViews;
    TArray<TRef<SamplerState>>        SamplerStates;
};