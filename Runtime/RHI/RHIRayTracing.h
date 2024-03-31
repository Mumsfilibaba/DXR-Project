#pragma once
#include "RHIResources.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix3x4.h"
#include "Core/Containers/SharedRef.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FRHIRayTracingGeometry;
class FRHIShaderResourceView;
class FRHIUnorderedAccessView;
class FRHIBuffer;
class FRHISamplerState;

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
    FRHIRayTracingGeometryInstance() = default;

    FRHIRayTracingGeometryInstance(
        FRHIRayTracingGeometry*  InGeometry,
        uint32                   InInstanceIndex,
        uint32                   InHitGroupIndex,
        ERayTracingInstanceFlags InFlags,
        uint32                   InMask,
        const FMatrix3x4&        InTransform)
        : Geometry(InGeometry)
        , InstanceIndex(InInstanceIndex)
        , HitGroupIndex(InHitGroupIndex)
        , Flags(InFlags)
        , Mask(InMask)
        , Transform(InTransform)
    {
    }

    bool operator==(const FRHIRayTracingGeometryInstance& RHS) const
    {
        return Geometry      == RHS.Geometry
            && InstanceIndex == RHS.InstanceIndex
            && HitGroupIndex == RHS.HitGroupIndex
            && Flags         == RHS.Flags
            && Mask          == RHS.Mask
            && Transform     == RHS.Transform;
    }

    bool operator!=(const FRHIRayTracingGeometryInstance& RHS) const
    {
        return !(*this == RHS);
    }

    FRHIRayTracingGeometry*  Geometry{nullptr};
    uint32                   InstanceIndex{0};
    uint32                   HitGroupIndex{0};
    ERayTracingInstanceFlags Flags{ERayTracingInstanceFlags::None};
    uint32                   Mask{0xff};
    FMatrix3x4               Transform;
};


struct FRHIAccelerationStructureInitializer
{
    FRHIAccelerationStructureInitializer() = default;

    FRHIAccelerationStructureInitializer(EAccelerationStructureBuildFlags InFlags)
        : Flags(InFlags)
    {
    }

    bool AllowUpdate() const { return IsEnumFlagSet(Flags, EAccelerationStructureBuildFlags::AllowUpdate); }

    bool PreferFastTrace() const { return IsEnumFlagSet(Flags, EAccelerationStructureBuildFlags::PreferFastTrace); }
    bool PreferFastBuild() const { return IsEnumFlagSet(Flags, EAccelerationStructureBuildFlags::PreferFastBuild); }

    bool operator==(const FRHIAccelerationStructureInitializer& RHS) const
    {
        return Flags == RHS.Flags;
    }

    bool operator!=(const FRHIAccelerationStructureInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    EAccelerationStructureBuildFlags Flags{ EAccelerationStructureBuildFlags::None };
};


struct FRHIRayTracingGeometryDesc : public FRHIAccelerationStructureInitializer
{
    FRHIRayTracingGeometryDesc()
        : FRHIAccelerationStructureInitializer()
        , VertexBuffer(nullptr)
        , NumVertices(0)
        , IndexBuffer(nullptr)
        , NumIndices(0)
        , IndexFormat(EIndexFormat::Unknown)
    {
    }

    FRHIRayTracingGeometryDesc(
        FRHIBuffer*                      InVertexBuffer,
        uint32                           InNumVerticies,
        FRHIBuffer*                      InIndexBuffer,
        uint32                           InNumIndices,
        EIndexFormat                     InIndexFormat,
        EAccelerationStructureBuildFlags InFlags)
        : FRHIAccelerationStructureInitializer(InFlags)
        , VertexBuffer(InVertexBuffer)
        , NumVertices(InNumVerticies)
        , IndexBuffer(InIndexBuffer)
        , NumIndices(InNumIndices)
        , IndexFormat(InIndexFormat)
    {
    }

    bool operator==(const FRHIRayTracingGeometryDesc& RHS) const
    {
        return FRHIAccelerationStructureInitializer::operator==(RHS)
            && VertexBuffer == RHS.VertexBuffer 
            && NumVertices  == RHS.NumVertices
            && IndexBuffer  == RHS.IndexBuffer
            && NumIndices   == RHS.NumIndices
            && IndexFormat  == RHS.IndexFormat;
    }

    bool operator!=(const FRHIRayTracingGeometryDesc& RHS) const
    {
        return !(*this == RHS);
    }

    FRHIBuffer*  VertexBuffer{nullptr};
    uint32       NumVertices{0};
    FRHIBuffer*  IndexBuffer{nullptr};
    uint32       NumIndices{0};
    EIndexFormat IndexFormat{EIndexFormat::Unknown};
};


struct FRHIRayTracingSceneDesc : public FRHIAccelerationStructureInitializer
{
    FRHIRayTracingSceneDesc() = default;

    FRHIRayTracingSceneDesc(const TArrayView<const FRHIRayTracingGeometryInstance>& InInstances, EAccelerationStructureBuildFlags InFlags)
        : FRHIAccelerationStructureInitializer(InFlags)
        , Instances(InInstances)
    {
    }

    bool operator==(const FRHIRayTracingSceneDesc& RHS) const
    {
        return FRHIAccelerationStructureInitializer::operator==(RHS) && Instances == RHS.Instances;
    }

    bool operator!=(const FRHIRayTracingSceneDesc& RHS) const
    {
        return !(*this == RHS);
    }

    TArray<FRHIRayTracingGeometryInstance> Instances;
};


class FRHIAccelerationStructure : public FRHIResource
{
protected:
    explicit FRHIAccelerationStructure(const FRHIAccelerationStructureInitializer& Initializer)
        : FRHIResource()
        , Flags(Initializer.Flags)
    {
    }

    virtual ~FRHIAccelerationStructure() = default;

public:
    virtual class FRHIRayTracingScene* GetRayTracingScene() { return nullptr; }

    virtual class FRHIRayTracingGeometry* GetRayTracingGeometry() { return nullptr; }

    virtual void* GetRHIBaseBVHBuffer() { return nullptr; }

    virtual void* GetRHIBaseAccelerationStructure() { return nullptr; }

    virtual FString GetDebugName() const { return ""; }

    virtual void SetDebugName(const FString& InName) { }

    FORCEINLINE EAccelerationStructureBuildFlags GetFlags() const 
    {
        return Flags;
    }

protected:
    EAccelerationStructureBuildFlags Flags;
};


class FRHIRayTracingGeometry : public FRHIAccelerationStructure
{
protected: 
    explicit FRHIRayTracingGeometry(const FRHIRayTracingGeometryDesc& Initializer)
        : FRHIAccelerationStructure(Initializer)
    {
    }

    virtual ~FRHIRayTracingGeometry() = default;

public:
    virtual class FRHIRayTracingGeometry* GetRayTracingGeometry() override final { return this; }
};


class FRHIRayTracingScene : public FRHIAccelerationStructure
{
protected:
    explicit FRHIRayTracingScene(const FRHIRayTracingSceneDesc& Initializer)
        : FRHIAccelerationStructure(Initializer)
    {
    }

    virtual ~FRHIRayTracingScene() = default;

public:
    virtual FRHIShaderResourceView* GetShaderResourceView() const { return nullptr; }

    virtual FRHIRayTracingScene* GetRayTracingScene() override final { return this; }

    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }
};


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

    TArray<FRHIBuffer*>              ConstantBuffers;
    TArray<FRHIShaderResourceView*>  ShaderResourceViews;
    TArray<FRHIUnorderedAccessView*> UnorderedAccessViews;
    TArray<FRHISamplerState*>        SamplerStates;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
