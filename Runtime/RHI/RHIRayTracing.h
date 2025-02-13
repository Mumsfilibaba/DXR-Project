#pragma once
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix3x4.h"
#include "Core/Containers/SharedRef.h"
#include "RHI/RHIResources.h"

#define RHI_DEFAULT_GEOMETRY_INSTANCE_MASK (0xff)

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FRHIRayTracingGeometry;
class FRHIShaderResourceView;
class FRHIUnorderedAccessView;
class FRHIBuffer;
class FRHISamplerState;
struct FRHIRayTracingGeometryInstance;

typedef TSharedRef<class FRHIAccelerationStructure> FRHIAccelerationStructureRef;
typedef TSharedRef<class FRHIRayTracingScene>       FRHIRayTracingSceneRef;
typedef TSharedRef<class FRHIRayTracingGeometry>    FRHIRayTracingGeometryRef;

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

    FRHIRayTracingGeometryInstance(FRHIRayTracingGeometry* InGeometry, uint32 InInstanceIndex, uint32 InHitGroupIndex, ERayTracingInstanceFlags InFlags, uint32 InMask, const FMatrix3x4& InTransform)
        : Geometry(InGeometry)
        , InstanceIndex(InInstanceIndex)
        , HitGroupIndex(InHitGroupIndex)
        , Flags(InFlags)
        , Mask(InMask)
        , Transform(InTransform)
    {
    }

    bool operator==(const FRHIRayTracingGeometryInstance& Other) const
    {
        return Geometry      == Other.Geometry 
            && InstanceIndex == Other.InstanceIndex 
            && HitGroupIndex == Other.HitGroupIndex 
            && Flags         == Other.Flags 
            && Mask          == Other.Mask 
            && Transform     == Other.Transform;
    }

    bool operator!=(const FRHIRayTracingGeometryInstance& Other) const
    {
        return !(*this == Other);
    }

    FRHIRayTracingGeometry*  Geometry      = nullptr;
    uint32                   InstanceIndex = 0;
    uint32                   HitGroupIndex = 0;
    ERayTracingInstanceFlags Flags         = ERayTracingInstanceFlags::None;
    uint32                   Mask          = RHI_DEFAULT_GEOMETRY_INSTANCE_MASK;
    FMatrix3x4               Transform;
};

struct FRHIAccelerationStructureInfo
{
    FRHIAccelerationStructureInfo() = default;

    FRHIAccelerationStructureInfo(EAccelerationStructureBuildFlags InFlags)
        : Flags(InFlags)
    {
    }

    bool AllowUpdate() const { return IsEnumFlagSet(Flags, EAccelerationStructureBuildFlags::AllowUpdate); }

    bool PreferFastTrace() const { return IsEnumFlagSet(Flags, EAccelerationStructureBuildFlags::PreferFastTrace); }
    bool PreferFastBuild() const { return IsEnumFlagSet(Flags, EAccelerationStructureBuildFlags::PreferFastBuild); }

    bool operator==(const FRHIAccelerationStructureInfo& Other) const
    {
        return Flags == Other.Flags;
    }

    bool operator!=(const FRHIAccelerationStructureInfo& Other) const
    {
        return !(*this == Other);
    }

    EAccelerationStructureBuildFlags Flags{ EAccelerationStructureBuildFlags::None };
};

struct FRHIRayTracingGeometryInfo : public FRHIAccelerationStructureInfo
{
    FRHIRayTracingGeometryInfo()
        : FRHIAccelerationStructureInfo()
        , VertexBuffer(nullptr)
        , NumVertices(0)
        , IndexBuffer(nullptr)
        , NumIndices(0)
        , IndexFormat(EIndexFormat::Unknown)
    {
    }

    FRHIRayTracingGeometryInfo(
        FRHIBuffer*                      InVertexBuffer,
        uint32                           InNumVerticies,
        FRHIBuffer*                      InIndexBuffer,
        uint32                           InNumIndices,
        EIndexFormat                     InIndexFormat,
        EAccelerationStructureBuildFlags InFlags)
        : FRHIAccelerationStructureInfo(InFlags)
        , VertexBuffer(InVertexBuffer)
        , NumVertices(InNumVerticies)
        , IndexBuffer(InIndexBuffer)
        , NumIndices(InNumIndices)
        , IndexFormat(InIndexFormat)
    {
    }

    bool operator==(const FRHIRayTracingGeometryInfo& Other) const
    {
        return FRHIAccelerationStructureInfo::operator==(Other)
            && VertexBuffer == Other.VertexBuffer
            && NumVertices  == Other.NumVertices
            && IndexBuffer  == Other.IndexBuffer
            && NumIndices   == Other.NumIndices
            && IndexFormat  == Other.IndexFormat;
    }

    bool operator!=(const FRHIRayTracingGeometryInfo& Other) const
    {
        return !(*this == Other);
    }

    FRHIBuffer*  VertexBuffer;
    uint32       NumVertices;
    FRHIBuffer*  IndexBuffer;
    uint32       NumIndices;
    EIndexFormat IndexFormat;
};

struct FRHIRayTracingSceneInfo : public FRHIAccelerationStructureInfo
{
    FRHIRayTracingSceneInfo() = default;

    FRHIRayTracingSceneInfo(const TArrayView<const FRHIRayTracingGeometryInstance>& InInstances, EAccelerationStructureBuildFlags InFlags)
        : FRHIAccelerationStructureInfo(InFlags)
        , Instances(InInstances)
    {
    }

    bool operator==(const FRHIRayTracingSceneInfo& Other) const
    {
        return FRHIAccelerationStructureInfo::operator==(Other) && Instances == Other.Instances;
    }

    bool operator!=(const FRHIRayTracingSceneInfo& Other) const
    {
        return !(*this == Other);
    }

    TArray<FRHIRayTracingGeometryInstance> Instances;
};

class FRHIAccelerationStructure : public FRHIResource
{
protected:
    explicit FRHIAccelerationStructure(const FRHIAccelerationStructureInfo& Initializer)
        : FRHIResource()
        , Flags(Initializer.Flags)
    {
    }

    virtual ~FRHIAccelerationStructure() = default;

public:

    // Returns the native handle for this resource
    virtual void* GetRHINativeHandle() const { return nullptr; }

    // Retrieve the base-interface for the backend
    virtual void* GetRHIBaseInterface() { return this; }

    virtual class FRHIRayTracingScene* GetRayTracingScene() { return nullptr; }
    virtual class FRHIRayTracingGeometry* GetRayTracingGeometry() { return nullptr; }

    virtual void SetDebugName(const FString& InName) { }
    virtual FString GetDebugName() const { return FString(); }

    EAccelerationStructureBuildFlags GetFlags() const 
    {
        return Flags;
    }

protected:
    EAccelerationStructureBuildFlags Flags;
};

class FRHIRayTracingGeometry : public FRHIAccelerationStructure
{
protected: 
    explicit FRHIRayTracingGeometry(const FRHIRayTracingGeometryInfo& InGeometryInfo)
        : FRHIAccelerationStructure(InGeometryInfo)
    {
    }

    virtual ~FRHIRayTracingGeometry() = default;

public:
    virtual class FRHIRayTracingGeometry* GetRayTracingGeometry() override final { return this; }
};

class FRHIRayTracingScene : public FRHIAccelerationStructure
{
protected:
    explicit FRHIRayTracingScene(const FRHIRayTracingSceneInfo& InSceneInfo)
        : FRHIAccelerationStructure(InSceneInfo)
    {
    }

    virtual ~FRHIRayTracingScene() = default;

public:
    virtual FRHIRayTracingScene* GetRayTracingScene() override final { return this; }

    virtual FRHIShaderResourceView* GetShaderResourceView() const { return nullptr; }
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
