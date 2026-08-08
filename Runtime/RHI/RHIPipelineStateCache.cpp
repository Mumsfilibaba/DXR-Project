#include "Core/Memory/Memory.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "RHI/RHI.h"
#include "RHI/RHIPipelineStateCache.h"

FRHIPipelineStateCache* FRHIPipelineStateCache::PipelineStateCache = nullptr;

static FAutoConsoleCommand CCmdDumpPipelineCacheStats(
    "RHI.DumpPipelineCacheStats",
    "Logs how many objects the pipeline state cache was asked for against how many it actually created",
    FConsoleCommandDelegate::CreateLambda([](StringView)
    {
        if (FRHIPipelineStateCache* Cache = FRHIPipelineStateCache::TryGet())
        {
            Cache->LogStats();
        }
    }));

static const CHAR* GetPipelineCacheKindName(EPipelineCacheKind::Type Kind)
{
    switch (Kind)
    {
        case EPipelineCacheKind::DepthStencilState:  return "DepthStencilState";
        case EPipelineCacheKind::RasterizerState:    return "RasterizerState";
        case EPipelineCacheKind::BlendState:         return "BlendState";
        case EPipelineCacheKind::InputLayout:        return "InputLayout";
        case EPipelineCacheKind::Shader:             return "Shader";
        case EPipelineCacheKind::GraphicsPipeline:   return "GraphicsPipeline";
        case EPipelineCacheKind::ComputePipeline:    return "ComputePipeline";
        case EPipelineCacheKind::MeshletPipeline:    return "MeshletPipeline";
        case EPipelineCacheKind::RayTracingPipeline: return "RayTracingPipeline";

        default: return "Unknown";
    }
}

// -------------------------------------------------------------------------------------------
// Hashing
// -------------------------------------------------------------------------------------------

static uint64 HashInputElement(const FRHIInputElementDesc& Element)
{
    uint64 Result = THash<String>::GetHash(Element.Semantic);
    HashCombine(Result, Element.SemanticIndex);
    HashCombine(Result, UnderlyingTypeValue(Element.Format));
    HashCombine(Result, Element.VertexStride);
    HashCombine(Result, Element.InputSlot);
    HashCombine(Result, Element.ByteOffset);
    HashCombine(Result, Element.ShaderElementIndex);
    HashCombine(Result, UnderlyingTypeValue(Element.InputClass));
    HashCombine(Result, Element.InstanceStepRate);
    return Result;
}

static bool AreInputElementsEqual(const FRHIInputElementDesc& Lhs, const FRHIInputElementDesc& Rhs)
{
    return Lhs.Semantic           == Rhs.Semantic
        && Lhs.SemanticIndex      == Rhs.SemanticIndex
        && Lhs.Format             == Rhs.Format
        && Lhs.VertexStride       == Rhs.VertexStride
        && Lhs.InputSlot          == Rhs.InputSlot
        && Lhs.ByteOffset         == Rhs.ByteOffset
        && Lhs.ShaderElementIndex == Rhs.ShaderElementIndex
        && Lhs.InputClass         == Rhs.InputClass
        && Lhs.InstanceStepRate   == Rhs.InstanceStepRate;
}

static uint64 HashInputElements(const TArray<FRHIInputElementDesc>& Elements)
{
    uint64 Result = static_cast<uint64>(Elements.Size());
    for (const FRHIInputElementDesc& Element : Elements)
    {
        HashCombine(Result, HashInputElement(Element));
    }

    return Result;
}

static bool AreInputElementArraysEqual(const TArray<FRHIInputElementDesc>& Lhs, const TArray<FRHIInputElementDesc>& Rhs)
{
    if (Lhs.Size() != Rhs.Size())
    {
        return false;
    }

    for (int32 Index = 0; Index < Lhs.Size(); ++Index)
    {
        if (!AreInputElementsEqual(Lhs[Index], Rhs[Index]))
        {
            return false;
        }
    }

    return true;
}

static uint64 HashStaticSamplers(const TArrayView<const FRHIStaticSamplerInfo>& StaticSamplers)
{
    uint64 Result = static_cast<uint64>(StaticSamplers.Size());
    for (const FRHIStaticSamplerInfo& Sampler : StaticSamplers)
    {
        HashCombine(Result, THash<FRHIStaticSamplerInfo>::GetHash(Sampler));
    }

    return Result;
}

static bool AreStaticSamplersEqual(const TArray<FRHIStaticSamplerInfo>& Lhs, const TArray<FRHIStaticSamplerInfo>& Rhs)
{
    if (Lhs.Size() != Rhs.Size())
    {
        return false;
    }

    for (int32 Index = 0; Index < Lhs.Size(); ++Index)
    {
        if (!(Lhs[Index] == Rhs[Index]))
        {
            return false;
        }
    }

    return true;
}

static void CopyStaticSamplers(const TArrayView<const FRHIStaticSamplerInfo>& Source, TArray<FRHIStaticSamplerInfo>& OutSamplers)
{
    OutSamplers.Reserve(Source.Size());
    for (const FRHIStaticSamplerInfo& Sampler : Source)
    {
        OutSamplers.Add(Sampler);
    }
}

static uint64 HashMultiSampleState(const FRHIMultiSampleState& State)
{
    uint64 Result = State.SampleCount;
    HashCombine(Result, State.SampleMask);
    HashCombine(Result, State.bProgrammableSamplePositions);
    return Result;
}

static bool AreMultiSampleStatesEqual(const FRHIMultiSampleState& Lhs, const FRHIMultiSampleState& Rhs)
{
    return Lhs.SampleCount == Rhs.SampleCount && Lhs.SampleMask == Rhs.SampleMask && Lhs.bProgrammableSamplePositions == Rhs.bProgrammableSamplePositions;
}

static uint64 HashPipelineFormats(const FRHIGraphicsPipelineFormats& Formats)
{
    uint64 Result = Formats.NumRenderTargets;
    for (uint8 Index = 0; Index < Formats.NumRenderTargets; ++Index)
    {
        HashCombine(Result, UnderlyingTypeValue(Formats.RenderTargetFormats[Index]));
    }

    HashCombine(Result, UnderlyingTypeValue(Formats.DepthStencilFormat));
    return Result;
}

static bool ArePipelineFormatsEqual(const FRHIGraphicsPipelineFormats& Lhs, const FRHIGraphicsPipelineFormats& Rhs)
{
    if (Lhs.NumRenderTargets != Rhs.NumRenderTargets || Lhs.DepthStencilFormat != Rhs.DepthStencilFormat)
    {
        return false;
    }

    for (uint8 Index = 0; Index < Lhs.NumRenderTargets; ++Index)
    {
        if (Lhs.RenderTargetFormats[Index] != Rhs.RenderTargetFormats[Index])
        {
            return false;
        }
    }

    return true;
}

static uint64 HashGraphicsDesc(const FRHIGraphicsPipelineStateDesc& Desc)
{
    uint64 Result = THash<FRHIVertexShader*>::GetHash(Desc.VertexShader);
    HashCombine(Result, THash<FRHIHullShader*>::GetHash(Desc.HullShader));
    HashCombine(Result, THash<FRHIDomainShader*>::GetHash(Desc.DomainShader));
    HashCombine(Result, THash<FRHIGeometryShader*>::GetHash(Desc.GeometryShader));
    HashCombine(Result, THash<FRHIPixelShader*>::GetHash(Desc.PixelShader));
    HashCombine(Result, THash<FRHIInputLayout*>::GetHash(Desc.InputLayout));
    HashCombine(Result, THash<FRHIDepthStencilState*>::GetHash(Desc.DepthStencilState));
    HashCombine(Result, THash<FRHIRasterizerState*>::GetHash(Desc.RasterizerState));
    HashCombine(Result, THash<FRHIBlendState*>::GetHash(Desc.BlendState));
    HashCombine(Result, HashStaticSamplers(Desc.StaticSamplers));
    HashCombine(Result, HashMultiSampleState(Desc.MultiSampleState));
    HashCombine(Result, HashPipelineFormats(Desc.RasterizerOutputFormats));
    HashCombine(Result, Desc.ViewInstancingState.NumArraySlices);
    HashCombine(Result, Desc.ViewInstancingState.StartRenderTargetArrayIndex);
    HashCombine(Result, Desc.ViewInstancingState.bEnableViewInstancing);
    HashCombine(Result, UnderlyingTypeValue(Desc.PrimitiveTopology));
    HashCombine(Result, Desc.bPrimitiveRestartEnable);
    return Result;
}

static uint64 HashMeshletDesc(const FRHIMeshletPipelineStateDesc& Desc)
{
    uint64 Result = THash<FRHIAmplificationShader*>::GetHash(Desc.AmplificationShader);
    HashCombine(Result, THash<FRHIMeshShader*>::GetHash(Desc.MeshShader));
    HashCombine(Result, THash<FRHIPixelShader*>::GetHash(Desc.PixelShader));
    HashCombine(Result, THash<FRHIDepthStencilState*>::GetHash(Desc.DepthStencilState));
    HashCombine(Result, THash<FRHIRasterizerState*>::GetHash(Desc.RasterizerState));
    HashCombine(Result, THash<FRHIBlendState*>::GetHash(Desc.BlendState));
    HashCombine(Result, HashStaticSamplers(Desc.StaticSamplers));
    HashCombine(Result, HashMultiSampleState(Desc.MultiSampleState));
    HashCombine(Result, HashPipelineFormats(Desc.RasterizerOutputFormats));
    HashCombine(Result, Desc.ViewInstancingState.NumArraySlices);
    HashCombine(Result, Desc.ViewInstancingState.StartRenderTargetArrayIndex);
    HashCombine(Result, Desc.ViewInstancingState.bEnableViewInstancing);
    return Result;
}

static uint64 HashRayTracingDesc(const FRHIRayTracingPipelineStateDesc& Desc)
{
    uint64 Result = Desc.MaxAttributeSizeInBytes;
    HashCombine(Result, Desc.MaxPayloadSizeInBytes);
    HashCombine(Result, Desc.MaxRecursionDepth);
    HashCombine(Result, UnderlyingTypeValue(Desc.Flags));
    HashCombine(Result, Desc.MaxClusterTrianglesPerCluster);
    HashCombine(Result, Desc.MaxClusterVerticesPerCluster);
    HashCombine(Result, THash<FRHIRayTracingPipelineState*>::GetHash(Desc.BasePipeline));

    for (FRHIRayGenShader* Shader : Desc.RayGenShaders)
    {
        HashCombine(Result, THash<FRHIRayGenShader*>::GetHash(Shader));
    }

    for (FRHIRayCallableShader* Shader : Desc.CallableShaders)
    {
        HashCombine(Result, THash<FRHIRayCallableShader*>::GetHash(Shader));
    }

    for (FRHIRayMissShader* Shader : Desc.MissShaders)
    {
        HashCombine(Result, THash<FRHIRayMissShader*>::GetHash(Shader));
    }

    for (const FRHIRayTracingHitGroupInfo& HitGroup : Desc.HitGroups)
    {
        HashCombine(Result, THash<String>::GetHash(HitGroup.Name));
        HashCombine(Result, UnderlyingTypeValue(HitGroup.Type));

        for (FRHIRayTracingShader* Shader : HitGroup.Shaders)
        {
            HashCombine(Result, THash<FRHIRayTracingShader*>::GetHash(Shader));
        }
    }

    return Result;
}

// -------------------------------------------------------------------------------------------
// Keys
// -------------------------------------------------------------------------------------------

FRHIPipelineStateCache::FGraphicsKey::FGraphicsKey(const FRHIGraphicsPipelineStateDesc& InDesc)
    : Desc(InDesc)
    , StaticSamplers()
    , VertexShader(InDesc.VertexShader)
    , HullShader(InDesc.HullShader)
    , DomainShader(InDesc.DomainShader)
    , GeometryShader(InDesc.GeometryShader)
    , PixelShader(InDesc.PixelShader)
    , InputLayout(InDesc.InputLayout)
    , DepthStencilState(InDesc.DepthStencilState)
    , RasterizerState(InDesc.RasterizerState)
    , BlendState(InDesc.BlendState)
{
    // The refs above adopt raw pointers the caller still owns, so each needs a reference of its own.
    VertexShader.AddRef();
    HullShader.AddRef();
    DomainShader.AddRef();
    GeometryShader.AddRef();
    PixelShader.AddRef();
    InputLayout.AddRef();
    DepthStencilState.AddRef();
    RasterizerState.AddRef();
    BlendState.AddRef();

    CopyStaticSamplers(InDesc.StaticSamplers, StaticSamplers);

    Desc.StaticSamplers          = TArrayView<const FRHIStaticSamplerInfo>();
    Desc.StreamOutputDeclaration = nullptr;
}

bool FRHIPipelineStateCache::FGraphicsKey::operator==(const FGraphicsKey& Other) const
{
    return Desc.VertexShader            == Other.Desc.VertexShader
        && Desc.HullShader              == Other.Desc.HullShader
        && Desc.DomainShader            == Other.Desc.DomainShader
        && Desc.GeometryShader          == Other.Desc.GeometryShader
        && Desc.PixelShader             == Other.Desc.PixelShader
        && Desc.InputLayout             == Other.Desc.InputLayout
        && Desc.DepthStencilState       == Other.Desc.DepthStencilState
        && Desc.RasterizerState         == Other.Desc.RasterizerState
        && Desc.BlendState              == Other.Desc.BlendState
        && Desc.PrimitiveTopology       == Other.Desc.PrimitiveTopology
        && Desc.bPrimitiveRestartEnable == Other.Desc.bPrimitiveRestartEnable
        && Desc.ViewInstancingState     == Other.Desc.ViewInstancingState
        && AreMultiSampleStatesEqual(Desc.MultiSampleState, Other.Desc.MultiSampleState)
        && ArePipelineFormatsEqual(Desc.RasterizerOutputFormats, Other.Desc.RasterizerOutputFormats)
        && AreStaticSamplersEqual(StaticSamplers, Other.StaticSamplers);
}

FRHIPipelineStateCache::FComputeKey::FComputeKey(const FRHIComputePipelineStateDesc& InDesc)
    : Desc(InDesc)
    , StaticSamplers()
    , Shader(InDesc.Shader)
{
    Shader.AddRef();

    CopyStaticSamplers(InDesc.StaticSamplers, StaticSamplers);
    Desc.StaticSamplers = TArrayView<const FRHIStaticSamplerInfo>();
}

bool FRHIPipelineStateCache::FComputeKey::operator==(const FComputeKey& Other) const
{
    return Desc.Shader == Other.Desc.Shader && AreStaticSamplersEqual(StaticSamplers, Other.StaticSamplers);
}

FRHIPipelineStateCache::FMeshletKey::FMeshletKey(const FRHIMeshletPipelineStateDesc& InDesc)
    : Desc(InDesc)
    , StaticSamplers()
    , AmplificationShader(InDesc.AmplificationShader)
    , MeshShader(InDesc.MeshShader)
    , PixelShader(InDesc.PixelShader)
    , DepthStencilState(InDesc.DepthStencilState)
    , RasterizerState(InDesc.RasterizerState)
    , BlendState(InDesc.BlendState)
{
    AmplificationShader.AddRef();
    MeshShader.AddRef();
    PixelShader.AddRef();
    DepthStencilState.AddRef();
    RasterizerState.AddRef();
    BlendState.AddRef();

    CopyStaticSamplers(InDesc.StaticSamplers, StaticSamplers);
    Desc.StaticSamplers = TArrayView<const FRHIStaticSamplerInfo>();
}

bool FRHIPipelineStateCache::FMeshletKey::operator==(const FMeshletKey& Other) const
{
    return Desc.AmplificationShader == Other.Desc.AmplificationShader
        && Desc.MeshShader          == Other.Desc.MeshShader
        && Desc.PixelShader         == Other.Desc.PixelShader
        && Desc.DepthStencilState   == Other.Desc.DepthStencilState
        && Desc.RasterizerState     == Other.Desc.RasterizerState
        && Desc.BlendState          == Other.Desc.BlendState
        && Desc.ViewInstancingState == Other.Desc.ViewInstancingState
        && AreMultiSampleStatesEqual(Desc.MultiSampleState, Other.Desc.MultiSampleState)
        && ArePipelineFormatsEqual(Desc.RasterizerOutputFormats, Other.Desc.RasterizerOutputFormats)
        && AreStaticSamplersEqual(StaticSamplers, Other.StaticSamplers);
}

FRHIPipelineStateCache::FRayTracingKey::FRayTracingKey(const FRHIRayTracingPipelineStateDesc& InDesc)
    : Desc(InDesc)
    , ReferencedShaders()
    , BasePipeline(InDesc.BasePipeline)
{
    BasePipeline.AddRef();

    const auto AddShader = [this](FRHIRayTracingShader* Shader)
    {
        if (Shader)
        {
            FRHIRayTracingShaderRef& Ref = ReferencedShaders.Emplace(Shader);
            Ref.AddRef();
        }
    };

    for (FRHIRayGenShader* Shader : Desc.RayGenShaders)
    {
        AddShader(Shader);
    }

    for (FRHIRayCallableShader* Shader : Desc.CallableShaders)
    {
        AddShader(Shader);
    }

    for (FRHIRayMissShader* Shader : Desc.MissShaders)
    {
        AddShader(Shader);
    }

    for (const FRHIRayTracingHitGroupInfo& HitGroup : Desc.HitGroups)
    {
        for (FRHIRayTracingShader* Shader : HitGroup.Shaders)
        {
            AddShader(Shader);
        }
    }
}

bool FRHIPipelineStateCache::FShaderKey::operator==(const FShaderKey& Other) const
{
    if (Stage != Other.Stage || ShaderCode.Size() != Other.ShaderCode.Size())
    {
        return false;
    }

    return Memory::Memcmp(ShaderCode.Data(), Other.ShaderCode.Data(), ShaderCode.Size()) == 0;
}

// -------------------------------------------------------------------------------------------
// Lifetime
// -------------------------------------------------------------------------------------------

FRHIPipelineStateCache::FRHIPipelineStateCache()
    : DepthStencilStates()
    , RasterizerStates()
    , BlendStates()
    , InputLayouts()
    , Shaders()
    , GraphicsPipelines()
    , ComputePipelines()
    , MeshletPipelines()
    , RayTracingPipelines()
    , Stats()
{
}

FRHIPipelineStateCache::~FRHIPipelineStateCache()
{
    // Pipelines first, since their keys hold the shaders and states the other maps own.
    GraphicsPipelines.Clear();
    ComputePipelines.Clear();
    MeshletPipelines.Clear();
    RayTracingPipelines.Clear();

    Shaders.Clear();
    InputLayouts.Clear();
    BlendStates.Clear();
    RasterizerStates.Clear();
    DepthStencilStates.Clear();
}

bool FRHIPipelineStateCache::Initialize()
{
    PipelineStateCache = new FRHIPipelineStateCache();
    return true;
}

void FRHIPipelineStateCache::Release()
{
    if (PipelineStateCache)
    {
        delete PipelineStateCache;
        PipelineStateCache = nullptr;
    }
}

void FRHIPipelineStateCache::FlushPipelineStates()
{
    TScopedLock Lock(CacheCS);

    GraphicsPipelines.Clear();
    ComputePipelines.Clear();
    MeshletPipelines.Clear();
    RayTracingPipelines.Clear();
}

void FRHIPipelineStateCache::LogStats() const
{
    LOG_INFO("[RHIPipelineStateCache] Kind                 Requests   Created");

    for (uint8 Index = 0; Index < EPipelineCacheKind::Count; ++Index)
    {
        const EPipelineCacheKind::Type Kind = static_cast<EPipelineCacheKind::Type>(Index);
        LOG_INFO("[RHIPipelineStateCache] %-20s %8u  %8u", GetPipelineCacheKindName(Kind), Stats.NumRequests[Index], Stats.NumCreated[Index]);
    }
}

template<typename ObjectType, typename KeyType, typename CreateFunctorType>
ObjectType* FRHIPipelineStateCache::FindOrCreate(TCacheMap<KeyType, TSharedRef<ObjectType>>& Map, uint64 Hash, const KeyType& Key, EPipelineCacheKind::Type Kind, CreateFunctorType&& Create)
{
    using FEntry = TPair<KeyType, TSharedRef<ObjectType>>;

    TScopedLock Lock(CacheCS);

    Stats.NumRequests[Kind]++;

    TArray<FEntry>& Bucket = Map.FindOrAdd(Hash);
    for (FEntry& Entry : Bucket)
    {
        // The stored key is compared rather than trusting the hash, so a collision cannot hand back an object built from a different description.
        if (Entry.First == Key)
        {
            return Entry.Second.GetAndAddRef();
        }
    }

    TSharedRef<ObjectType> NewObject = Create();
    if (!NewObject)
    {
        // A failed create is never stored, or one Metal stub would poison the entry for the session.
        return nullptr;
    }

    Stats.NumCreated[Kind]++;

    FEntry& NewEntry = Bucket.Emplace(KeyType(Key), Move(NewObject));
    return NewEntry.Second.GetAndAddRef();
}

FRHIDepthStencilState* FRHIPipelineStateCache::GetOrCreateDepthStencilState(const FRHIDepthStencilStateDesc& Desc)
{
    const uint64 Hash = THash<FRHIDepthStencilStateDesc>::GetHash(Desc);
    return FindOrCreate(DepthStencilStates, Hash, Desc, EPipelineCacheKind::DepthStencilState, [&Desc]()
    {
        return FRHIDepthStencilStateRef(RHI::Device->CreateDepthStencilState(Desc));
    });
}

FRHIRasterizerState* FRHIPipelineStateCache::GetOrCreateRasterizerState(const FRHIRasterizerStateDesc& Desc)
{
    const uint64 Hash = THash<FRHIRasterizerStateDesc>::GetHash(Desc);
    return FindOrCreate(RasterizerStates, Hash, Desc, EPipelineCacheKind::RasterizerState, [&Desc]()
    {
        return FRHIRasterizerStateRef(RHI::Device->CreateRasterizerState(Desc));
    });
}

FRHIBlendState* FRHIPipelineStateCache::GetOrCreateBlendState(const FRHIBlendStateDesc& Desc)
{
    const uint64 Hash = THash<FRHIBlendStateDesc>::GetHash(Desc);
    return FindOrCreate(BlendStates, Hash, Desc, EPipelineCacheKind::BlendState, [&Desc]()
    {
        return FRHIBlendStateRef(RHI::Device->CreateBlendState(Desc));
    });
}

FRHIInputLayout* FRHIPipelineStateCache::GetOrCreateInputLayout(const TArray<FRHIInputElementDesc>& InputElements)
{
    const uint64 Hash = HashInputElements(InputElements);

    using FEntry = TPair<TArray<FRHIInputElementDesc>, FRHIInputLayoutRef>;

    TScopedLock Lock(CacheCS);

    Stats.NumRequests[EPipelineCacheKind::InputLayout]++;

    TArray<FEntry>& Bucket = InputLayouts.FindOrAdd(Hash);
    for (FEntry& Entry : Bucket)
    {
        if (AreInputElementArraysEqual(Entry.First, InputElements))
        {
            return Entry.Second.GetAndAddRef();
        }
    }

    FRHIInputLayoutRef NewInputLayout = RHI::Device->CreateInputLayout(InputElements);
    if (!NewInputLayout)
    {
        return nullptr;
    }

    Stats.NumCreated[EPipelineCacheKind::InputLayout]++;

    FEntry& NewEntry = Bucket.Emplace(TArray<FRHIInputElementDesc>(InputElements), Move(NewInputLayout));
    return NewEntry.Second.GetAndAddRef();
}

FRHIShader* FRHIPipelineStateCache::GetOrCreateShader(EShaderStage Stage, const TArray<uint8>& ShaderCode)
{
    uint64 Hash = UnderlyingTypeValue(Stage);
    HashCombine(Hash, static_cast<uint64>(ShaderCode.Size()));

    for (uint8 Byte : ShaderCode)
    {
        HashCombine(Hash, Byte);
    }

    FShaderKey Key;
    Key.Stage      = Stage;
    Key.ShaderCode = ShaderCode;

    return FindOrCreate(Shaders, Hash, Key, EPipelineCacheKind::Shader, [Stage, &ShaderCode]() -> FRHIShaderRef
    {
        switch (Stage)
        {
            case EShaderStage::Vertex:          return FRHIShaderRef(RHI::Device->CreateVertexShader(ShaderCode));
            case EShaderStage::Hull:            return FRHIShaderRef(RHI::Device->CreateHullShader(ShaderCode));
            case EShaderStage::Domain:          return FRHIShaderRef(RHI::Device->CreateDomainShader(ShaderCode));
            case EShaderStage::Geometry:        return FRHIShaderRef(RHI::Device->CreateGeometryShader(ShaderCode));
            case EShaderStage::Mesh:            return FRHIShaderRef(RHI::Device->CreateMeshShader(ShaderCode));
            case EShaderStage::Amplification:   return FRHIShaderRef(RHI::Device->CreateAmplificationShader(ShaderCode));
            case EShaderStage::Pixel:           return FRHIShaderRef(RHI::Device->CreatePixelShader(ShaderCode));
            case EShaderStage::Compute:         return FRHIShaderRef(RHI::Device->CreateComputeShader(ShaderCode));
            case EShaderStage::RayGen:          return FRHIShaderRef(RHI::Device->CreateRayGenShader(ShaderCode));
            case EShaderStage::RayAnyHit:       return FRHIShaderRef(RHI::Device->CreateRayAnyHitShader(ShaderCode));
            case EShaderStage::RayClosestHit:   return FRHIShaderRef(RHI::Device->CreateRayClosestHitShader(ShaderCode));
            case EShaderStage::RayMiss:         return FRHIShaderRef(RHI::Device->CreateRayMissShader(ShaderCode));
            case EShaderStage::RayIntersection: return FRHIShaderRef(RHI::Device->CreateRayIntersectionShader(ShaderCode));
            case EShaderStage::RayCallable:     return FRHIShaderRef(RHI::Device->CreateRayCallableShader(ShaderCode));

            default: return FRHIShaderRef();
        }
    });
}

FRHIGraphicsPipelineState* FRHIPipelineStateCache::GetOrCreateGraphicsPipelineState(const FRHIGraphicsPipelineStateDesc& Desc)
{
    // Stream output entries name their semantics with a borrowed const CHAR*, which a stored key cannot
    // own, so those descs bypass the cache entirely. No renderer pass sets one today.
    if (Desc.StreamOutputDeclaration)
    {
        return RHI::Device->CreateGraphicsPipelineState(Desc);
    }

    const uint64 Hash = HashGraphicsDesc(Desc);
    return FindOrCreate(GraphicsPipelines, Hash, FGraphicsKey(Desc), EPipelineCacheKind::GraphicsPipeline, [&Desc]()
    {
        return FRHIGraphicsPipelineStateRef(RHI::Device->CreateGraphicsPipelineState(Desc));
    });
}

FRHIComputePipelineState* FRHIPipelineStateCache::GetOrCreateComputePipelineState(const FRHIComputePipelineStateDesc& Desc)
{
    uint64 Hash = THash<FRHIComputeShader*>::GetHash(Desc.Shader);
    HashCombine(Hash, HashStaticSamplers(Desc.StaticSamplers));

    return FindOrCreate(ComputePipelines, Hash, FComputeKey(Desc), EPipelineCacheKind::ComputePipeline, [&Desc]()
    {
        return FRHIComputePipelineStateRef(RHI::Device->CreateComputePipelineState(Desc));
    });
}

FRHIMeshletPipelineState* FRHIPipelineStateCache::GetOrCreateMeshletPipelineState(const FRHIMeshletPipelineStateDesc& Desc)
{
    const uint64 Hash = HashMeshletDesc(Desc);
    return FindOrCreate(MeshletPipelines, Hash, FMeshletKey(Desc), EPipelineCacheKind::MeshletPipeline, [&Desc]()
    {
        return FRHIMeshletPipelineStateRef(RHI::Device->CreateMeshletPipelineState(Desc));
    });
}

FRHIRayTracingPipelineState* FRHIPipelineStateCache::GetOrCreateRayTracingPipelineState(const FRHIRayTracingPipelineStateDesc& Desc)
{
    const uint64 Hash = HashRayTracingDesc(Desc);
    return FindOrCreate(RayTracingPipelines, Hash, FRayTracingKey(Desc), EPipelineCacheKind::RayTracingPipeline, [&Desc]()
    {
        return FRHIRayTracingPipelineStateRef(RHI::Device->CreateRayTracingPipelineState(Desc));
    });
}
