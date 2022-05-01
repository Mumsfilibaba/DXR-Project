#pragma once
#include "RHIResourceBase.h"
#include "RHIResourceViews.h"

#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix3x4.h"
#include "Core/Containers/SharedRef.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERayTracingStructureBuildFlag

enum ERayTracingStructureBuildFlags : uint8
{
    RayTracingStructureBuildFlag_None            = 0,
    RayTracingStructureBuildFlag_AllowUpdate     = FLAG(1),
    RayTracingStructureBuildFlag_PreferFastTrace = FLAG(2),
    RayTracingStructureBuildFlag_PreferFastBuild = FLAG(3),
};

ENUM_CLASS_OPERATORS(ERayTracingStructureBuildFlags);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERayTracingInstanceFlags

enum ERayTracingInstanceFlags : uint8
{
    RayTracingInstanceFlags_None                  = 0,
    RayTracingInstanceFlags_CullDisable           = FLAG(1),
    RayTracingInstanceFlags_FrontCounterClockwise = FLAG(2),
    RayTracingInstanceFlags_ForceOpaque           = FLAG(3),
    RayTracingInstanceFlags_ForceNonOpaque        = FLAG(4),
};

ENUM_CLASS_OPERATORS(ERayTracingInstanceFlags);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRayPayload

struct SRayPayload
{
    CVector3 Color;
    uint32   CurrentDepth;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRayIntersectionAttributes

struct SRayIntersectionAttributes
{
    float Attrib0;
    float Attrib1;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayTracingGeometryInstance

class CRHIRayTracingGeometryInstance
{
public:

    CRHIRayTracingGeometryInstance()
        : Geometry(nullptr)
        , InstanceIndex(0)
        , HitGroupIndex(0)
        , Flags(ERayTracingInstanceFlags::RayTracingInstanceFlags_None)
        , Mask(0xff)
        , Transform()
    { }

    CRHIRayTracingGeometryInstance(CRHIRayTracingGeometry* InGeometry
                                   , uint32 InInstanceIndex
                                   , uint32 InHitGroupIndex
                                   , ERayTracingInstanceFlags InFlags
                                   , uint32 InMask
                                   , const CMatrix3x4& InTransform)
        : Geometry(InGeometry)
        , InstanceIndex(InInstanceIndex)
        , HitGroupIndex(InHitGroupIndex)
        , Flags(InFlags)
        , Mask(InMask)
        , Transform(InTransform)
    { }

    bool operator==(const CRHIRayTracingGeometryInstance& RHS) const
    {
        return (Geometry      == RHS.Geometry)
            && (InstanceIndex == RHS.InstanceIndex)
            && (HitGroupIndex == RHS.HitGroupIndex)
            && (Flags         == RHS.Flags)
            && (Mask          == RHS.Mask)
            && (Transform     == RHS.Transform);
    }

    bool operator!=(const CRHIRayTracingGeometryInstance& RHS) const
    {
        return !(*this == RHS);
    }

    CRHIRayTracingGeometry*  Geometry;

    uint32                   InstanceIndex;
    uint32                   HitGroupIndex;

    ERayTracingInstanceFlags Flags;

    uint32                   Mask;

    CMatrix3x4               Transform;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayTracingGeometryInitializer

class CRHIRayTracingGeometryInitializer
{
public:

    CRHIRayTracingGeometryInitializer()
        : VertexBuffer(nullptr)
        , IndexBuffer(nullptr)
        , Flags(ERayTracingStructureBuildFlags::RayTracingStructureBuildFlag_None)
    { }

    CRHIRayTracingGeometryInitializer( CRHIVertexBuffer* InVertexBuffer
                                     , CRHIIndexBuffer* InIndexBuffer
                                     , ERayTracingStructureBuildFlags InFlags)
        : VertexBuffer(InVertexBuffer)
        , IndexBuffer(InIndexBuffer)
        , Flags(InFlags)
    { }

    bool operator==(const CRHIRayTracingGeometryInitializer& RHS) const
    {
        return (VertexBuffer == RHS.VertexBuffer) && (IndexBuffer == RHS.IndexBuffer) && (Flags == RHS.Flags);
    }

    bool operator!=(const CRHIRayTracingGeometryInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    CRHIVertexBuffer*              VertexBuffer;
    CRHIIndexBuffer*               IndexBuffer;
    ERayTracingStructureBuildFlags Flags;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayTracingSceneInitializer

class CRHIRayTracingSceneInitializer
{
public:

    CRHIRayTracingSceneInitializer()
        : Instances()
        , Flags(ERayTracingStructureBuildFlags::RayTracingStructureBuildFlag_None)
    { }

    CRHIRayTracingSceneInitializer(CRHIRayTracingGeometryInstance* Instances, uint32 NumInstances, ERayTracingStructureBuildFlags InFlags)
        : Instances(Instances, NumInstances)
        , Flags(InFlags)
    { }

    bool operator==(const CRHIRayTracingSceneInitializer& RHS) const
    {
        return (Instances == RHS.Instances) && (Flags == RHS.Flags);
    }

    bool operator!=(const CRHIRayTracingSceneInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    TArray<CRHIRayTracingGeometryInstance> Instances;
    ERayTracingStructureBuildFlags         Flags;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIAccelerationStructure

class CRHIAccelerationStructure : public CRHIResource
{
protected:

    explicit CRHIAccelerationStructure(ERayTracingStructureBuildFlags InFlags)
        : CRHIResource()
        , Flags(InFlags)
    { }

public:

    /** @return: Returns the CRHIRayTracingScene interface if implemented otherwise nullptr */
    virtual class CRHIRayTracingScene* GetRayTracingScene() { return nullptr; }

    /** @return: Returns the CRHIRayTracingGeometry interface if implemented otherwise nullptr */
    virtual class CRHIRayTracingGeometry* GetRayTracingGeometry() { return nullptr; }

    /** @return: Returns the native handle of the resource */
    virtual void* GetRHIHandle() const { return nullptr; }

    /** @brief: Set the name of the AccelerationStructure */
    virtual void SetName(const String& InName) { }

    /** @return: Returns the name of the Texture */
    virtual String GetName() const { return ""; }

    /** @return: Returns the Flags of the RayTracingScene */
    ERayTracingStructureBuildFlags GetFlags() const { return Flags; }

protected:
    ERayTracingStructureBuildFlags Flags;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayTracingGeometry

class CRHIRayTracingGeometry : public CRHIResource
{
protected: 

    explicit CRHIRayTracingGeometry(uint32 InFlags)
        : Flags(InFlags)
    { }

public:

    /** @return: Returns the flag for the RayTracingGeometry */
    uint32 GetFlags() const { return Flags; }

private:
    uint32 Flags;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayTracingScene

class CRHIRayTracingScene : public CRHIResource
{
protected:

    explicit CRHIRayTracingScene(uint32 InFlags)
        : Flags(InFlags)
    { }

public:

    /** @return: Returns the ShaderResourceView for the RayTracingScene */
    virtual CRHIShaderResourceView* GetShaderResourceView() const = 0;

    /** @return: Returns the flag for the RayTracingScene */
    uint32 GetFlags() const { return Flags; }

private:
    uint32 Flags;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRayTracingGeometryInstance

struct SRayTracingGeometryInstance
{
    TSharedRef<CRHIRayTracingGeometry> Instance;
    uint32 InstanceIndex = 0;
    uint32 HitGroupIndex = 0;
    uint32 Flags = RayTracingInstanceFlags_None;
    uint32 Mask = 0xff;
    CMatrix3x4 Transform;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// SRayTracingShaderResources

struct SRayTracingShaderResources
{
    void AddConstantBuffer(CRHIConstantBuffer* Buffer)
    {
        ConstantBuffers.Emplace(Buffer);
    }

    void AddShaderResourceView(CRHIShaderResourceView* View)
    {
        ShaderResourceViews.Emplace(View);
    }

    void AddUnorderedAccessView(CRHIUnorderedAccessView* View)
    {
        UnorderedAccessViews.Emplace(View);
    }

    void AddSamplerState(CRHISamplerState* State)
    {
        SamplerStates.Emplace(State);
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

    String Identifier;

    TArray<CRHIConstantBuffer*>      ConstantBuffers;
    TArray<CRHIShaderResourceView*>  ShaderResourceViews;
    TArray<CRHIUnorderedAccessView*> UnorderedAccessViews;
    TArray<CRHISamplerState*>        SamplerStates;
};