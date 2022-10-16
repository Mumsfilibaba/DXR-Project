#pragma once
#include "RHIResourceBase.h"
#include "RHIResourceViews.h"

#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix3x4.h"
#include "Core/Containers/SharedRef.h"

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

class FRHIRayTracingGeometry;

typedef TSharedRef<class FRHIAccelerationStructure>  FRHIAccelerationStructureRef;
typedef TSharedRef<class FRHIRayTracingGeometry>     FRHIRayTracingGeometryRef;
typedef TSharedRef<class FRHIRayTracingScene>        FRHIRayTracingSceneRef;

struct FRayPayload
{
    FVector3 Color;
    uint32   CurrentDepth;
};

struct FRayIntersectionAttributes
{
    float Attrib0;
    float Attrib1;
};


enum class EAccelerationStructureBuildFlags : uint8
{
    None            = 0,
    AllowUpdate     = FLAG(1),
    PreferFastTrace = FLAG(2),
    PreferFastBuild = FLAG(3),
};

ENUM_CLASS_OPERATORS(EAccelerationStructureBuildFlags);


enum class ERayTracingInstanceFlags : uint8
{
    None                  = 0,
    CullDisable           = FLAG(1),
    FrontCounterClockwise = FLAG(2),
    ForceOpaque           = FLAG(3),
    ForceNonOpaque        = FLAG(4),
};

ENUM_CLASS_OPERATORS(ERayTracingInstanceFlags);


struct FRHIRayTracingGeometryInstance
{
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


struct FRHIAccelerationStructureInitializer
{
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


struct FRHIRayTracingGeometryInitializer 
    : public FRHIAccelerationStructureInitializer
{
    FRHIRayTracingGeometryInitializer()
        : FRHIAccelerationStructureInitializer()
        , VertexBuffer(nullptr)
        , NumVertices(0)
        , IndexBuffer(nullptr)
        , NumIndices(0)
        , IndexFormat(EIndexFormat::Unknown)
    { }

    FRHIRayTracingGeometryInitializer(
        FRHIBuffer* InVertexBuffer,
        uint32 InNumVerticies,
        FRHIBuffer* InIndexBuffer,
        uint32 InNumIndices,
        EIndexFormat InIndexFormat,
        EAccelerationStructureBuildFlags InFlags)
        : FRHIAccelerationStructureInitializer(InFlags)
        , VertexBuffer(InVertexBuffer)
        , NumVertices(InNumVerticies)
        , IndexBuffer(InIndexBuffer)
        , NumIndices(InNumIndices)
        , IndexFormat(InIndexFormat)
    { }

    bool operator==(const FRHIRayTracingGeometryInitializer& RHS) const
    {
        return FRHIAccelerationStructureInitializer::operator==(RHS)
            && (VertexBuffer == RHS.VertexBuffer) 
            && (NumVertices == RHS.NumVertices)
            && (IndexBuffer  == RHS.IndexBuffer)
            && (NumIndices == RHS.NumIndices)
            && (IndexFormat == RHS.IndexFormat);
    }

    bool operator!=(const FRHIRayTracingGeometryInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    FRHIBuffer*  VertexBuffer;
    uint32       NumVertices;
    FRHIBuffer*  IndexBuffer;
    uint32       NumIndices;
    EIndexFormat IndexFormat;
};


struct FRHIRayTracingSceneInitializer 
    : public FRHIAccelerationStructureInitializer
{
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


class FRHIAccelerationStructure 
    : public FRHIResource
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

    virtual FString GetName() const { return ""; }
    virtual void    SetName(const FString& InName) { }

    EAccelerationStructureBuildFlags GetFlags() const { return Flags; }

protected:
    EAccelerationStructureBuildFlags Flags;
};


class FRHIRayTracingGeometry 
    : public FRHIAccelerationStructure
{
protected: 
    explicit FRHIRayTracingGeometry(const FRHIRayTracingGeometryInitializer& Initializer)
        : FRHIAccelerationStructure(Initializer)
    { }

public:
    virtual class FRHIRayTracingGeometry* GetRayTracingGeometry() override final { return this; }
};


class FRHIRayTracingScene 
    : public FRHIAccelerationStructure
{
protected:
    explicit FRHIRayTracingScene(const FRHIRayTracingSceneInitializer& Initializer)
        : FRHIAccelerationStructure(Initializer)
    { }

public:
    virtual FRHIShaderResourceView* GetShaderResourceView() const       { return nullptr; }
    virtual FRHIRayTracingScene*    GetRayTracingScene() override final { return this; }

    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRayTracingShaderResources

struct FRayTracingShaderResources
{
    void AddConstantBuffer(FRHIBuffer* Buffer)
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
        return ConstantBuffers.GetSize() + ShaderResourceViews.GetSize() + UnorderedAccessViews.GetSize();
    }

    uint32 NumSamplers() const
    {
        return SamplerStates.GetSize();
    }

    void Reset()
    {
        ConstantBuffers.Clear();
        ShaderResourceViews.Clear();
        UnorderedAccessViews.Clear();
        SamplerStates.Clear();
    }

    FString Identifier;

    TArray<FRHIBuffer*>              ConstantBuffers;
    TArray<FRHIShaderResourceView*>  ShaderResourceViews;
    TArray<FRHIUnorderedAccessView*> UnorderedAccessViews;
    TArray<FRHISamplerState*>        SamplerStates;
};

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
