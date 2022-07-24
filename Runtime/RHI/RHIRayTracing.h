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

typedef TSharedRef<class FRHIAccelerationStructure>  FRHIAccelerationStructureRef;
typedef TSharedRef<class FRHIRayTracingGeometry>     FRHIRayTracingGeometryRef;
typedef TSharedRef<class FRHIRayTracingScene>        FRHIRayTracingSceneRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRayPayload

struct FRayPayload
{
    FVector3 Color;
    uint32   CurrentDepth;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRayIntersectionAttributes

struct FRayIntersectionAttributes
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

    FRHIRayTracingGeometryInstance(
        FRHIRayTracingGeometry* InGeometry,
        uint32 InInstanceIndex,
        uint32 InHitGroupIndex,
        ERayTracingInstanceFlags InFlags,
        uint32 InMask,
        const FMatrix3x4& InTransform)
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

    FMatrix3x4               Transform;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIAccelerationStructureInitializer

class FRHIAccelerationStructureInitializer
{
public:

    FRHIAccelerationStructureInitializer()
        : Flags(EAccelerationStructureBuildFlags::None)
    { }

    FRHIAccelerationStructureInitializer(EAccelerationStructureBuildFlags InFlags)
        : Flags(InFlags)
    { }

    bool AllowUpdate() const { return ((Flags & EAccelerationStructureBuildFlags::AllowUpdate) != EAccelerationStructureBuildFlags::None); }

    bool PreferFastTrace() const { return ((Flags & EAccelerationStructureBuildFlags::PreferFastTrace) != EAccelerationStructureBuildFlags::None); }
    bool PreferFastBuild() const { return ((Flags & EAccelerationStructureBuildFlags::PreferFastBuild) != EAccelerationStructureBuildFlags::None); }

    bool operator==(const FRHIAccelerationStructureInitializer& RHS) const
    {
        return (Flags == RHS.Flags);
    }

    bool operator!=(const FRHIAccelerationStructureInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    EAccelerationStructureBuildFlags Flags;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIRayTracingGeometryInitializer

class FRHIRayTracingGeometryInitializer : public FRHIAccelerationStructureInitializer
{
public:

    FRHIRayTracingGeometryInitializer()
        : FRHIAccelerationStructureInitializer()
        , VertexBuffer(nullptr)
        , IndexBuffer(nullptr)
    { }

    FRHIRayTracingGeometryInitializer(
        FRHIVertexBuffer* InVertexBuffer,
        FRHIIndexBuffer* InIndexBuffer,
        EAccelerationStructureBuildFlags InFlags)
        : FRHIAccelerationStructureInitializer(InFlags)
        , VertexBuffer(InVertexBuffer)
        , IndexBuffer(InIndexBuffer)
    { }

    bool operator==(const FRHIRayTracingGeometryInitializer& RHS) const
    {
        return FRHIAccelerationStructureInitializer::operator==(RHS)
            && (VertexBuffer == RHS.VertexBuffer) 
            && (IndexBuffer  == RHS.IndexBuffer);
    }

    bool operator!=(const FRHIRayTracingGeometryInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    FRHIVertexBuffer* VertexBuffer;
    FRHIIndexBuffer*  IndexBuffer;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIRayTracingSceneInitializer

class FRHIRayTracingSceneInitializer : public FRHIAccelerationStructureInitializer
{
public:

    FRHIRayTracingSceneInitializer()
        : FRHIAccelerationStructureInitializer()
        , Instances()
    { }

    FRHIRayTracingSceneInitializer(
        const TArrayView<const FRHIRayTracingGeometryInstance>& InInstances,
        EAccelerationStructureBuildFlags InFlags)
        : FRHIAccelerationStructureInitializer(InFlags)
        , Instances(InInstances)
    { }

    bool operator==(const FRHIRayTracingSceneInitializer& RHS) const
    {
        return FRHIAccelerationStructureInitializer::operator==(RHS) && (Instances == RHS.Instances);
    }

    bool operator!=(const FRHIRayTracingSceneInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    TArray<FRHIRayTracingGeometryInstance> Instances;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIAccelerationStructure

class FRHIAccelerationStructure : public FRHIResource
{
protected:

    explicit FRHIAccelerationStructure(const FRHIAccelerationStructureInitializer& Initializer)
        : FRHIResource()
        , Flags(Initializer.Flags)
    { }

public:

    virtual class FRHIRayTracingScene*    GetRayTracingScene()    { return nullptr; }
    virtual class FRHIRayTracingGeometry* GetRayTracingGeometry() { return nullptr; }

    virtual void* GetRHIBaseBVHBuffer()             { return nullptr; }
    virtual void* GetRHIBaseAccelerationStructure() { return nullptr; }

    virtual void    SetName(const FString& InName) { }
    virtual FString GetName() const { return ""; }

    EAccelerationStructureBuildFlags GetFlags() const { return Flags; }

protected:
    EAccelerationStructureBuildFlags Flags;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIRayTracingGeometry

class FRHIRayTracingGeometry : public FRHIAccelerationStructure
{
protected: 

    explicit FRHIRayTracingGeometry(const FRHIRayTracingGeometryInitializer& Initializer)
        : FRHIAccelerationStructure(Initializer)
    { }

public:
    
    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIAccelerationStructure Interface

    virtual class FRHIRayTracingGeometry* GetRayTracingGeometry() override final { return this; }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIRayTracingScene

class FRHIRayTracingScene : public FRHIAccelerationStructure
{
protected:

    explicit FRHIRayTracingScene(const FRHIRayTracingSceneInitializer& Initializer)
        : FRHIAccelerationStructure(Initializer)
    { }

public:

    virtual FRHIShaderResourceView* GetShaderResourceView() const { return nullptr; }
    virtual FRHIDescriptorHandle    GetBindlessHandle()     const { return FRHIDescriptorHandle(); }

    /*///////////////////////////////////////////////////////////////////////////////////////////////*/
    // FRHIAccelerationStructure Interface

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

    FString Identifier;

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
