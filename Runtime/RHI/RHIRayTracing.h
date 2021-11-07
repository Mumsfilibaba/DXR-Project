#pragma once
#include "RHIResourceBase.h"
#include "RHIResourceViews.h"

#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix3x4.h"
#include "Core/Containers/SharedRef.h"

enum ERayTracingStructureBuildFlag
{
    RayTracingStructureBuildFlag_None = 0x0,
    RayTracingStructureBuildFlag_AllowUpdate = FLAG( 1 ),
    RayTracingStructureBuildFlag_PreferFastTrace = FLAG( 2 ),
    RayTracingStructureBuildFlag_PreferFastBuild = FLAG( 3 ),
};

enum ERayTracingInstanceFlags
{
    RayTracingInstanceFlags_None = 0,
    RayTracingInstanceFlags_CullDisable = FLAG( 1 ),
    RayTracingInstanceFlags_FrontCounterClockwise = FLAG( 2 ),
    RayTracingInstanceFlags_ForceOpaque = FLAG( 3 ),
    RayTracingInstanceFlags_ForceNonOpaque = FLAG( 4 ),
};

// RayTracing Geometry (Bottom Level Acceleration Structure)
class CRHIRayTracingGeometry : public CRHIResource
{
public:
    CRHIRayTracingGeometry( uint32 InFlags )
        : Flags( InFlags )
    {
    }

    FORCEINLINE uint32 GetFlags() const
    {
        return Flags;
    }

private:
    uint32 Flags;
};

// RayTracing Scene (Top Level Acceleration Structure)
class CRHIRayTracingScene : public CRHIResource
{
public:
    CRHIRayTracingScene( uint32 InFlags )
        : Flags( InFlags )
    {
    }

    virtual CRHIShaderResourceView* GetShaderResourceView() const = 0;

    FORCEINLINE uint32 GetFlags() const
    {
        return Flags;
    }

private:
    uint32 Flags;
};

struct SRayTracingGeometryInstance
{
    TSharedRef<CRHIRayTracingGeometry> Instance;
    uint32 InstanceIndex = 0;
    uint32 HitGroupIndex = 0;
    uint32 Flags = RayTracingInstanceFlags_None;
    uint32 Mask = 0xff;
    CMatrix3x4 Transform;
};

struct SRayPayload
{
    CVector3 Color;
    uint32   CurrentDepth;
};

struct SRayIntersectionAttributes
{
    float Attrib0;
    float Attrib1;
};

struct SRayTracingShaderResources
{
    void AddConstantBuffer( CRHIConstantBuffer* Buffer )
    {
        ConstantBuffers.Emplace( Buffer );
    }

    void AddShaderResourceView( CRHIShaderResourceView* View )
    {
        ShaderResourceViews.Emplace( View );
    }

    void AddUnorderedAccessView( CRHIUnorderedAccessView* View )
    {
        UnorderedAccessViews.Emplace( View );
    }

    void AddSamplerState( CRHISamplerState* State )
    {
        SamplerStates.Emplace( State );
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

    CString Identifier;

    TArray<CRHIConstantBuffer*>      ConstantBuffers;
    TArray<CRHIShaderResourceView*>  ShaderResourceViews;
    TArray<CRHIUnorderedAccessView*> UnorderedAccessViews;
    TArray<CRHISamplerState*>        SamplerStates;
};