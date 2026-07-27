#pragma once
#include "RHI/RHIResources.h"
#include "RHI/RayTracing/RHIRayTracingTypes.h"

DISABLE_UNREFERENCED_VARIABLE_WARNING

class FRHIBuffer;
class FRHIShaderResourceView;
class FRHIUnorderedAccessView;
class FRHISamplerState;
class FRHIRayTracingPipelineState;

struct FRHIRayTracingShaderIdentifier
{
    static constexpr uint32 MAX_SIZE_IN_BYTES = 32;
    
    NODISCARD bool IsValid() const
    {
        return SizeInBytes != 0;
    }

    uint8  Data[MAX_SIZE_IN_BYTES] = { };
    uint32 SizeInBytes             = 0;
};

enum class ERayTracingLocalBindingType : uint8
{
    ConstantBuffer,
    ShaderResourceView,
    UnorderedAccessView,
    SamplerState,
};

struct FRHIHitGroupLocalShaderBinding
{
    static FRHIHitGroupLocalShaderBinding MakeConstantBuffer(FRHIBuffer* InBuffer, uint32 RegisterIndex)
    {
        FRHIHitGroupLocalShaderBinding Result;
        Result.Type          = ERayTracingLocalBindingType::ConstantBuffer;
        Result.RegisterIndex = RegisterIndex;
        Result.Buffer        = InBuffer;
        return Result;
    }

    static FRHIHitGroupLocalShaderBinding MakeShaderResourceView(FRHIShaderResourceView* InView, uint32 RegisterIndex)
    {
        FRHIHitGroupLocalShaderBinding Result;
        Result.Type               = ERayTracingLocalBindingType::ShaderResourceView;
        Result.RegisterIndex      = RegisterIndex;
        Result.ShaderResourceView = InView;
        return Result;
    }

    static FRHIHitGroupLocalShaderBinding MakeUnorderedAccessView(FRHIUnorderedAccessView* InView, uint32 RegisterIndex)
    {
        FRHIHitGroupLocalShaderBinding Result;
        Result.Type                = ERayTracingLocalBindingType::UnorderedAccessView;
        Result.RegisterIndex       = RegisterIndex;
        Result.UnorderedAccessView = InView;
        return Result;
    }

    static FRHIHitGroupLocalShaderBinding MakeSamplerState(FRHISamplerState* InState, uint32 RegisterIndex)
    {
        FRHIHitGroupLocalShaderBinding Result;
        Result.Type          = ERayTracingLocalBindingType::SamplerState;
        Result.RegisterIndex = RegisterIndex;
        Result.Sampler       = InState;
        return Result;
    }

    ERayTracingLocalBindingType Type          = ERayTracingLocalBindingType::ConstantBuffer;
    uint32                      RegisterIndex = 0;

    union
    {
        FRHIBuffer*              Buffer = nullptr;
        FRHIShaderResourceView*  ShaderResourceView;
        FRHIUnorderedAccessView* UnorderedAccessView;
        FRHISamplerState*        Sampler;
    };
};

struct FRHIShaderBindingTableDesc
{
    FRHIShaderBindingTableDesc() noexcept = default;

    FRHIShaderBindingTableDesc(FRHIRayTracingPipelineState* InPipeline, uint32 InNumRayGenerationShaders, uint32 InNumMissShaders, uint32 InNumCallableShaders, uint32 InNumHitGroupRecords) noexcept
        : Pipeline(InPipeline)
        , NumRayGenerationShaders(InNumRayGenerationShaders)
        , NumMissShaders(InNumMissShaders)
        , NumCallableShaders(InNumCallableShaders)
        , NumHitGroupRecords(InNumHitGroupRecords)
    {
    }

    FRHIRayTracingPipelineState* Pipeline                = nullptr;
    uint32                       NumRayGenerationShaders = 1;
    uint32                       NumMissShaders          = 0;
    uint32                       NumCallableShaders      = 0;
    uint32                       NumHitGroupRecords      = 0;
};

struct FRHIShaderBindingTableRegion
{
    uint64 StartAddress  = 0;
    uint64 SizeInBytes   = 0;
    uint64 StrideInBytes = 0;
};

struct FRHIShaderBindingTableAddressInfo
{
    FRHIShaderBindingTableRegion RayGeneration;
    FRHIShaderBindingTableRegion Miss;
    FRHIShaderBindingTableRegion HitGroup;
    FRHIShaderBindingTableRegion Callable;
};

typedef TSharedRef<class FRHIShaderBindingTable> FRHIShaderBindingTableRef;

class FRHIShaderBindingTable : public FRHIResource
{
protected:
    explicit FRHIShaderBindingTable(const FRHIShaderBindingTableDesc& InDesc)
        : FRHIResource(ERHIResourceType::ShaderBindingTable)
        , Descriptor(InDesc)
    {
    }

    virtual ~FRHIShaderBindingTable() = default;

public:
    virtual void* GetRHINativeResource() const = 0;
    virtual FRHIShaderBindingTableAddressInfo GetAddressInfo() const = 0;

    NODISCARD const FRHIShaderBindingTableDesc& GetDesc() const
    {
        return Descriptor;
    }

private:
    FRHIShaderBindingTableDesc Descriptor;
};

ENABLE_UNREFERENCED_VARIABLE_WARNING
