#pragma once
#include "Core/Math/Vector3.h"
#include "Core/Math/Matrix3x4.h"
#include "Core/Containers/SharedRef.h"
#include "RHI/RHIResources.h"

#define RHI_DEFAULT_GEOMETRY_INSTANCE_MASK (0xff)

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FRHISceneAccelerationStructure;
class FRHIGeometryAccelerationStructure;
class FRHIShaderResourceView;
class FRHIUnorderedAccessView;
class FRHIBuffer;
class FRHISamplerState;
struct FRHIGeometryAccelerationStructureInstance;

typedef TSharedRef<FRHISceneAccelerationStructure>    FRHISceneAccelerationStructureRef;
typedef TSharedRef<FRHIGeometryAccelerationStructure> FRHIGeometryAccelerationStructureRef;

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

struct FRHIGeometryAccelerationStructureInstance
{
    FRHIGeometryAccelerationStructureInstance() noexcept = default;

    FRHIGeometryAccelerationStructureInstance(FRHIGeometryAccelerationStructure* InGeometry, uint32 InInstanceIndex, uint32 InHitGroupIndex,
        ERayTracingInstanceFlags InFlags, uint32 InMask, const FMatrix3x4& InTransform) noexcept
        : Geometry(InGeometry)
        , InstanceIndex(InInstanceIndex)
        , HitGroupIndex(InHitGroupIndex)
        , Flags(InFlags)
        , Mask(InMask)
        , Transform(InTransform)
    {
    }

    bool operator==(const FRHIGeometryAccelerationStructureInstance& Other) const noexcept = default;

    FRHIGeometryAccelerationStructure*  Geometry      = nullptr;
    uint32                   InstanceIndex = 0;
    uint32                   HitGroupIndex = 0;
    ERayTracingInstanceFlags Flags         = ERayTracingInstanceFlags::None;
    uint32                   Mask          = RHI_DEFAULT_GEOMETRY_INSTANCE_MASK;
    FMatrix3x4               Transform     = { };
};

struct FRHIGeometryAccelerationStructureDesc
{
    constexpr FRHIGeometryAccelerationStructureDesc() noexcept = default;

    constexpr FRHIGeometryAccelerationStructureDesc(FRHIBuffer* InVertexBuffer, uint32 InNumVerticies, FRHIBuffer* InIndexBuffer, uint32 InNumIndices,
        EIndexFormat InIndexFormat, EAccelerationStructureBuildFlags InFlags) noexcept
        : VertexBuffer(InVertexBuffer)
        , NumVertices(InNumVerticies)
        , IndexBuffer(InIndexBuffer)
        , NumIndices(InNumIndices)
        , IndexFormat(InIndexFormat)
        , Flags(InFlags)
    {
    }

    NODISCARD constexpr bool AllowUpdate() const noexcept { return IsEnumFlagSet(Flags, EAccelerationStructureBuildFlags::AllowUpdate); }

    NODISCARD constexpr bool PreferFastTrace() const noexcept { return IsEnumFlagSet(Flags, EAccelerationStructureBuildFlags::PreferFastTrace); }
    NODISCARD constexpr bool PreferFastBuild() const noexcept { return IsEnumFlagSet(Flags, EAccelerationStructureBuildFlags::PreferFastBuild); }

    constexpr bool operator==(const FRHIGeometryAccelerationStructureDesc& Other) const noexcept = default;
    
    FRHIBuffer*                      VertexBuffer = nullptr;
    uint32                           NumVertices  = 0;
    FRHIBuffer*                      IndexBuffer  = nullptr;
    uint32                           NumIndices   = 0;
    EIndexFormat                     IndexFormat  = EIndexFormat::Unknown;
    EAccelerationStructureBuildFlags Flags        = EAccelerationStructureBuildFlags::None;
};

struct FRHISceneAccelerationStructureDesc
{
    FRHISceneAccelerationStructureDesc() noexcept = default;

    FRHISceneAccelerationStructureDesc(const TArrayView<const FRHIGeometryAccelerationStructureInstance>& InInstances, EAccelerationStructureBuildFlags InFlags) noexcept
        : Instances(InInstances)
        , Flags(InFlags)
    {
    }

    NODISCARD constexpr bool AllowUpdate() const noexcept { return IsEnumFlagSet(Flags, EAccelerationStructureBuildFlags::AllowUpdate); }

    NODISCARD constexpr bool PreferFastTrace() const noexcept { return IsEnumFlagSet(Flags, EAccelerationStructureBuildFlags::PreferFastTrace); }
    NODISCARD constexpr bool PreferFastBuild() const noexcept { return IsEnumFlagSet(Flags, EAccelerationStructureBuildFlags::PreferFastBuild); }

    bool operator==(const FRHISceneAccelerationStructureDesc& Other) const noexcept = default;
    
    TArray<FRHIGeometryAccelerationStructureInstance> Instances;
    EAccelerationStructureBuildFlags       Flags = EAccelerationStructureBuildFlags::None;
};

class FRHIGeometryAccelerationStructure : public FRHIResource
{
protected: 
    explicit FRHIGeometryAccelerationStructure(const FRHIGeometryAccelerationStructureDesc& InGeometryDesc)
        : FRHIResource()
        , Flags(InGeometryDesc.Flags)
    {
    }

    virtual ~FRHIGeometryAccelerationStructure() = default;

public:
    virtual void* GetRHINativeHandle() const { return nullptr; }

    virtual void SetDebugName(const FString& InName) { }
    virtual FString GetDebugName() const { return FString(); }

    EAccelerationStructureBuildFlags GetFlags() const 
    {
        return Flags;
    }

protected:
    EAccelerationStructureBuildFlags Flags;
};

class FRHISceneAccelerationStructure : public FRHIResource
{
protected:
    explicit FRHISceneAccelerationStructure(const FRHISceneAccelerationStructureDesc& InSceneDesc)
        : FRHIResource()
        , Flags(InSceneDesc.Flags)
    {
    }

    virtual ~FRHISceneAccelerationStructure() = default;

public:
    virtual void* GetRHINativeHandle() const { return nullptr; }

    virtual FRHIShaderResourceView* GetShaderResourceView() const { return nullptr; }
    virtual FRHIDescriptorHandle GetBindlessHandle() const { return FRHIDescriptorHandle(); }

    virtual void SetDebugName(const FString& InName) { }
    virtual FString GetDebugName() const { return FString(); }

    EAccelerationStructureBuildFlags GetFlags() const 
    {
        return Flags;
    }

protected:
    EAccelerationStructureBuildFlags Flags;
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
