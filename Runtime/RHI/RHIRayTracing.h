#pragma once
#include "RHIResourceBase.h"
#include "RHIResourceViews.h"

#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix3x4.h"
#include "Core/Containers/SharedRef.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

class CRHIRayTracingGeometry;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedefs

typedef TSharedRef<class CRHIAccelerationStructure>  RHIAccelerationStructureRef;
typedef TSharedRef<class CRHIRayTracingGeometry>     RHIRayTracingGeometryRef;
typedef TSharedRef<class CRHIRayTracingScene>        RHIRayTracingSceneRef;

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
// ERayTracingStructureBuildFlag

enum class EAccelerationStructureBuildFlags : uint8
{
    None            = 0,
    AllowUpdate     = FLAG(1),
    PreferFastTrace = FLAG(2),
    PreferFastBuild = FLAG(3),
};

ENUM_CLASS_OPERATORS(EAccelerationStructureBuildFlags);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ERayTracingInstanceFlags

enum class ERayTracingInstanceFlags : uint8
{
    None                  = 0,
    CullDisable           = FLAG(1),
    FrontCounterClockwise = FLAG(2),
    ForceOpaque           = FLAG(3),
    ForceNonOpaque        = FLAG(4),
};

ENUM_CLASS_OPERATORS(ERayTracingInstanceFlags);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayTracingGeometryInstance

class CRHIRayTracingGeometryInstance
{
public:

    CRHIRayTracingGeometryInstance()
        : Geometry(nullptr)
        , InstanceIndex(0)
        , HitGroupIndex(0)
        , Flags(ERayTracingInstanceFlags::None)
        , Mask(0xff)
        , Transform()
    { }

    CRHIRayTracingGeometryInstance( CRHIRayTracingGeometry* InGeometry
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
// CRHIAccelerationStructureInitializer

class CRHIAccelerationStructureInitializer
{
public:

    CRHIAccelerationStructureInitializer()
        : Flags(EAccelerationStructureBuildFlags::None)
    { }

    CRHIAccelerationStructureInitializer(EAccelerationStructureBuildFlags InFlags)
        : Flags(InFlags)
    { }

    bool AllowUpdate() const { return ((Flags & EAccelerationStructureBuildFlags::AllowUpdate) != EAccelerationStructureBuildFlags::None); }

    bool PreferFastTrace() const { return ((Flags & EAccelerationStructureBuildFlags::PreferFastTrace) != EAccelerationStructureBuildFlags::None); }

    bool PreferFastBuild() const { return ((Flags & EAccelerationStructureBuildFlags::PreferFastBuild) != EAccelerationStructureBuildFlags::None); }

    bool operator==(const CRHIAccelerationStructureInitializer& RHS) const
    {
        return (Flags == RHS.Flags);
    }

    bool operator!=(const CRHIAccelerationStructureInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    EAccelerationStructureBuildFlags Flags;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayTracingGeometryInitializer

class CRHIRayTracingGeometryInitializer : public CRHIAccelerationStructureInitializer
{
public:

    CRHIRayTracingGeometryInitializer()
        : CRHIAccelerationStructureInitializer()
        , VertexBuffer(nullptr)
        , IndexBuffer(nullptr)
    { }

    CRHIRayTracingGeometryInitializer(CRHIVertexBuffer* InVertexBuffer, CRHIIndexBuffer* InIndexBuffer, EAccelerationStructureBuildFlags InFlags)
        : CRHIAccelerationStructureInitializer(InFlags)
        , VertexBuffer(InVertexBuffer)
        , IndexBuffer(InIndexBuffer)
    { }

    bool operator==(const CRHIRayTracingGeometryInitializer& RHS) const
    {
        return CRHIAccelerationStructureInitializer::operator==(RHS)
            && (VertexBuffer == RHS.VertexBuffer) 
            && (IndexBuffer  == RHS.IndexBuffer);
    }

    bool operator!=(const CRHIRayTracingGeometryInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    CRHIVertexBuffer* VertexBuffer;
    CRHIIndexBuffer*  IndexBuffer;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayTracingSceneInitializer

class CRHIRayTracingSceneInitializer : public CRHIAccelerationStructureInitializer
{
public:

    CRHIRayTracingSceneInitializer()
        : CRHIAccelerationStructureInitializer()
        , Instances()
    { }

    CRHIRayTracingSceneInitializer(const TArrayView<const CRHIRayTracingGeometryInstance>& InInstances, EAccelerationStructureBuildFlags InFlags)
        : CRHIAccelerationStructureInitializer(InFlags)
        , Instances(Instances)
    { }

    bool operator==(const CRHIRayTracingSceneInitializer& RHS) const
    {
        return CRHIAccelerationStructureInitializer::operator==(RHS) && (Instances == RHS.Instances);
    }

    bool operator!=(const CRHIRayTracingSceneInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    TArray<CRHIRayTracingGeometryInstance> Instances;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIAccelerationStructure

class CRHIAccelerationStructure : public CRHIResource
{
protected:

    explicit CRHIAccelerationStructure(const CRHIAccelerationStructureInitializer& Initializer)
        : CRHIResource()
        , Flags(Initializer.Flags)
    { }

public:

    /** @return: Returns the CRHIRayTracingScene interface if implemented otherwise nullptr */
    virtual class CRHIRayTracingScene* GetRayTracingScene() { return nullptr; }

    /** @return: Returns the CRHIRayTracingGeometry interface if implemented otherwise nullptr */
    virtual class CRHIRayTracingGeometry* GetRayTracingGeometry() { return nullptr; }

    /** @return: Returns the native handle of the resource */
    virtual void* GetRHIBaseBVHBuffer() { return nullptr; }

    /** @return: Returns the native handle of the resource */
    virtual void* GetRHIBaseAccelerationStructure() { return nullptr; }

    /** @brief: Set the name of the AccelerationStructure */
    virtual void SetName(const String& InName) { }

    /** @return: Returns the name of the Texture */
    virtual String GetName() const { return ""; }

    /** @return: Returns the Flags of the RayTracingScene */
    EAccelerationStructureBuildFlags GetFlags() const { return Flags; }

protected:
    EAccelerationStructureBuildFlags Flags;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayTracingGeometry

class CRHIRayTracingGeometry : public CRHIAccelerationStructure
{
protected: 

    explicit CRHIRayTracingGeometry(const CRHIRayTracingGeometryInitializer& Initializer)
        : CRHIAccelerationStructure(Initializer)
    { }

public:
    
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIAccelerationStructure Interface

    virtual class CRHIRayTracingGeometry* GetRayTracingGeometry() override final { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIRayTracingScene

class CRHIRayTracingScene : public CRHIAccelerationStructure
{
protected:

    explicit CRHIRayTracingScene(const CRHIRayTracingSceneInitializer& Initializer)
        : CRHIAccelerationStructure(Initializer)
    { }

public:

    /** @return: Returns the ShaderResourceView for the RayTracingScene */
    virtual CRHIShaderResourceView* GetShaderResourceView() const { return nullptr; }

    /** @return: Returns a Bindless descriptor-handle if the RHI supports it */
    virtual CRHIDescriptorHandle GetBindlessHandle() const { return CRHIDescriptorHandle(); }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIAccelerationStructure Interface

    virtual class CRHIRayTracingScene* GetRayTracingScene() override final { return this; }
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

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif