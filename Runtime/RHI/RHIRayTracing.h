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

class FRHIRayTracingGeometry;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedefs

typedef TSharedRef<class CRHIAccelerationStructure>  RHIAccelerationStructureRef;
typedef TSharedRef<class FRHIRayTracingGeometry>     RHIRayTracingGeometryRef;
typedef TSharedRef<class FRHIRayTracingScene>        RHIRayTracingSceneRef;

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
// FRHIRayTracingGeometryInstance

class FRHIRayTracingGeometryInstance
{
public:

    FRHIRayTracingGeometryInstance()
        : Geometry(nullptr)
        , InstanceIndex(0)
        , HitGroupIndex(0)
        , Flags(ERayTracingInstanceFlags::None)
        , Mask(0xff)
        , Transform()
    { }

    FRHIRayTracingGeometryInstance( FRHIRayTracingGeometry* InGeometry
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

    bool operator==(const FRHIRayTracingGeometryInstance& RHS) const
    {
        return (Geometry      == RHS.Geometry)
            && (InstanceIndex == RHS.InstanceIndex)
            && (HitGroupIndex == RHS.HitGroupIndex)
            && (Flags         == RHS.Flags)
            && (Mask          == RHS.Mask)
            && (Transform     == RHS.Transform);
    }

    bool operator!=(const FRHIRayTracingGeometryInstance& RHS) const
    {
        return !(*this == RHS);
    }

    FRHIRayTracingGeometry*  Geometry;

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
// FRHIRayTracingGeometryInitializer

class FRHIRayTracingGeometryInitializer : public CRHIAccelerationStructureInitializer
{
public:

    FRHIRayTracingGeometryInitializer()
        : CRHIAccelerationStructureInitializer()
        , VertexBuffer(nullptr)
        , IndexBuffer(nullptr)
    { }

    FRHIRayTracingGeometryInitializer(CRHIVertexBuffer* InVertexBuffer, FRHIIndexBuffer* InIndexBuffer, EAccelerationStructureBuildFlags InFlags)
        : CRHIAccelerationStructureInitializer(InFlags)
        , VertexBuffer(InVertexBuffer)
        , IndexBuffer(InIndexBuffer)
    { }

    bool operator==(const FRHIRayTracingGeometryInitializer& RHS) const
    {
        return CRHIAccelerationStructureInitializer::operator==(RHS)
            && (VertexBuffer == RHS.VertexBuffer) 
            && (IndexBuffer  == RHS.IndexBuffer);
    }

    bool operator!=(const FRHIRayTracingGeometryInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    CRHIVertexBuffer* VertexBuffer;
    FRHIIndexBuffer*  IndexBuffer;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIRayTracingSceneInitializer

class FRHIRayTracingSceneInitializer : public CRHIAccelerationStructureInitializer
{
public:

    FRHIRayTracingSceneInitializer()
        : CRHIAccelerationStructureInitializer()
        , Instances()
    { }

    FRHIRayTracingSceneInitializer(const TArrayView<const FRHIRayTracingGeometryInstance>& InInstances, EAccelerationStructureBuildFlags InFlags)
        : CRHIAccelerationStructureInitializer(InFlags)
        , Instances(InInstances)
    { }

    bool operator==(const FRHIRayTracingSceneInitializer& RHS) const
    {
        return CRHIAccelerationStructureInitializer::operator==(RHS) && (Instances == RHS.Instances);
    }

    bool operator!=(const FRHIRayTracingSceneInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    TArray<FRHIRayTracingGeometryInstance> Instances;
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

    /** @return: Returns the FRHIRayTracingScene interface if implemented otherwise nullptr */
    virtual class FRHIRayTracingScene* GetRayTracingScene() { return nullptr; }

    /** @return: Returns the FRHIRayTracingGeometry interface if implemented otherwise nullptr */
    virtual class FRHIRayTracingGeometry* GetRayTracingGeometry() { return nullptr; }

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
// FRHIRayTracingGeometry

class FRHIRayTracingGeometry : public CRHIAccelerationStructure
{
protected: 

    explicit FRHIRayTracingGeometry(const FRHIRayTracingGeometryInitializer& Initializer)
        : CRHIAccelerationStructure(Initializer)
    { }

public:
    
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIAccelerationStructure Interface

    virtual class FRHIRayTracingGeometry* GetRayTracingGeometry() override final { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIRayTracingScene

class FRHIRayTracingScene : public CRHIAccelerationStructure
{
protected:

    explicit FRHIRayTracingScene(const FRHIRayTracingSceneInitializer& Initializer)
        : CRHIAccelerationStructure(Initializer)
    { }

public:

    /** @return: Returns the ShaderResourceView for the RayTracingScene */
    virtual FRHIShaderResourceView* GetShaderResourceView() const { return nullptr; }

    /** @return: Returns a Bindless descriptor-handle if the RHI supports it */
    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }

public:

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // CRHIAccelerationStructure Interface

    virtual class FRHIRayTracingScene* GetRayTracingScene() override final { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRayTracingShaderResources

struct FRayTracingShaderResources
{
    void AddConstantBuffer(FRHIConstantBuffer* Buffer)
    {
        ConstantBuffers.Emplace(Buffer);
    }

    void AddShaderResourceView(FRHIShaderResourceView* View)
    {
        ShaderResourceViews.Emplace(View);
    }

    void AddUnorderedAccessView(FRHIUnorderedAccessView* View)
    {
        UnorderedAccessViews.Emplace(View);
    }

    void AddSamplerState(FRHISamplerState* State)
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

    TArray<FRHIConstantBuffer*>      ConstantBuffers;
    TArray<FRHIShaderResourceView*>  ShaderResourceViews;
    TArray<FRHIUnorderedAccessView*> UnorderedAccessViews;
    TArray<FRHISamplerState*>        SamplerStates;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
