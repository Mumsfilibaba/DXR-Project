#include "MetalRHI/MetalPipelineCache.h"
#include "MetalRHI/MetalBinaryArchive.h"
#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalStats.h"
#include "Core/Threading/ScopedLock.h"

static const void* GetFunctionIdentity(id<MTLFunction> Function)
{
    return (__bridge const void*)Function;
}

static NSArray<id<MTLFunction>>* RetainFunctions(id<MTLFunction> First, id<MTLFunction> Second, id<MTLFunction> Third)
{
    NSMutableArray<id<MTLFunction>>* Functions = [[NSMutableArray alloc] initWithCapacity:3];

    for (id<MTLFunction> Function : { First, Second, Third })
    {
        if (Function)
        {
            [Functions addObject:Function];
        }
    }

    return Functions;
}

template<typename DescriptorType>
static void FillAttachmentState(FMetalRenderPipelineKey& Key, DescriptorType* Descriptor)
{
    for (uint32 Index = 0; Index < RHI_MAX_RENDER_TARGETS; ++Index)
    {
        MTLRenderPipelineColorAttachmentDescriptor* Attachment = Descriptor.colorAttachments[Index];
        Key.ColorFormats[Index] = Attachment.pixelFormat;

        FMetalBlendStateRHI::FBlendAttachment& Blend = Key.Blend[Index];
        Blend.WriteMask                   = Attachment.writeMask;
        Blend.bBlendingEnabled            = Attachment.blendingEnabled;
        Blend.AlphaBlendOperation         = Attachment.alphaBlendOperation;
        Blend.ColorBlendOperation         = Attachment.rgbBlendOperation;
        Blend.DestinationAlphaBlendFactor = Attachment.destinationAlphaBlendFactor;
        Blend.DestinationColorBlendFactor = Attachment.destinationRGBBlendFactor;
        Blend.SourceAlphaBlendFactor      = Attachment.sourceAlphaBlendFactor;
        Blend.SourceColorBlendFactor      = Attachment.sourceRGBBlendFactor;
    }

    Key.DepthFormat                 = Descriptor.depthAttachmentPixelFormat;
    Key.StencilFormat               = Descriptor.stencilAttachmentPixelFormat;
    Key.SampleCount                 = static_cast<uint32>(Descriptor.rasterSampleCount);
    Key.MaxVertexAmplificationCount = static_cast<uint32>(Descriptor.maxVertexAmplificationCount);
    Key.bAlphaToCoverage            = Descriptor.alphaToCoverageEnabled == YES;
}

FMetalRenderPipelineKey FMetalRenderPipelineKey::Create(MTLRenderPipelineDescriptor* Descriptor, const FMetalInputLayoutRHI* InputLayout)
{
    FMetalRenderPipelineKey Key{};
    Key.Type                 = EMetalRenderPipelineType::Graphics;
    Key.VertexOrMeshFunction = Descriptor.vertexFunction;
    Key.ObjectFunction       = nil;
    Key.FragmentFunction     = Descriptor.fragmentFunction;
    Key.TopologyClass        = Descriptor.inputPrimitiveTopology;

    FillAttachmentState(Key, Descriptor);

    if (InputLayout)
    {
        MTLVertexDescriptor* VertexDescriptor = InputLayout->GetMTLVertexDescriptor();

        for (uint32 Index = 0; Index < InputLayout->GetNumInputElementDescs(); ++Index)
        {
            MTLVertexAttributeDescriptor* Attribute = VertexDescriptor.attributes[Index];
            if (Attribute.format == MTLVertexFormatInvalid)
            {
                continue;
            }

            MTLVertexBufferLayoutDescriptor* Layout = VertexDescriptor.layouts[Attribute.bufferIndex];

            FMetalVertexAttributeKey& AttributeKey = Key.VertexAttributes.Emplace();
            AttributeKey.AttributeIndex = Index;
            AttributeKey.Format         = Attribute.format;
            AttributeKey.Offset         = static_cast<uint32>(Attribute.offset);
            AttributeKey.BufferIndex    = static_cast<uint32>(Attribute.bufferIndex);
            AttributeKey.Stride         = static_cast<uint32>(Layout.stride);
            AttributeKey.StepFunction   = Layout.stepFunction;
            AttributeKey.StepRate       = static_cast<uint32>(Layout.stepRate);
        }
    }

    return Key;
}

FMetalRenderPipelineKey FMetalRenderPipelineKey::Create(MTLMeshRenderPipelineDescriptor* Descriptor)
{
    FMetalRenderPipelineKey Key{};
    Key.Type                 = EMetalRenderPipelineType::Meshlet;
    Key.VertexOrMeshFunction = Descriptor.meshFunction;
    Key.ObjectFunction       = Descriptor.objectFunction;
    Key.FragmentFunction     = Descriptor.fragmentFunction;
    Key.TopologyClass        = MTLPrimitiveTopologyClassUnspecified;

    FillAttachmentState(Key, Descriptor);
    return Key;
}

bool FMetalRenderPipelineKey::operator==(const FMetalRenderPipelineKey& Other) const
{
    if (Type                        != Other.Type
     || VertexOrMeshFunction        != Other.VertexOrMeshFunction
     || ObjectFunction              != Other.ObjectFunction
     || FragmentFunction            != Other.FragmentFunction
     || DepthFormat                 != Other.DepthFormat
     || StencilFormat               != Other.StencilFormat
     || TopologyClass               != Other.TopologyClass
     || SampleCount                 != Other.SampleCount
     || MaxVertexAmplificationCount != Other.MaxVertexAmplificationCount
     || bAlphaToCoverage            != Other.bAlphaToCoverage
     || VertexAttributes.Size()     != Other.VertexAttributes.Size())
    {
        return false;
    }

    for (uint32 Index = 0; Index < RHI_MAX_RENDER_TARGETS; ++Index)
    {
        if (ColorFormats[Index] != Other.ColorFormats[Index] || !(Blend[Index] == Other.Blend[Index]))
        {
            return false;
        }
    }

    for (int32 Index = 0; Index < VertexAttributes.Size(); ++Index)
    {
        if (!(VertexAttributes[Index] == Other.VertexAttributes[Index]))
        {
            return false;
        }
    }

    return true;
}

uint64 FMetalRenderPipelineKey::GetHash() const
{
    uint64 Hash = static_cast<uint64>(Type);
    HashCombine(Hash, GetFunctionIdentity(VertexOrMeshFunction));
    HashCombine(Hash, GetFunctionIdentity(ObjectFunction));
    HashCombine(Hash, GetFunctionIdentity(FragmentFunction));

    for (const FMetalVertexAttributeKey& Attribute : VertexAttributes)
    {
        HashCombine(Hash, Attribute.AttributeIndex);
        HashCombine(Hash, static_cast<uint64>(Attribute.Format));
        HashCombine(Hash, Attribute.Offset);
        HashCombine(Hash, Attribute.BufferIndex);
        HashCombine(Hash, Attribute.Stride);
        HashCombine(Hash, static_cast<uint64>(Attribute.StepFunction));
        HashCombine(Hash, Attribute.StepRate);
    }

    for (uint32 Index = 0; Index < RHI_MAX_RENDER_TARGETS; ++Index)
    {
        const FMetalBlendStateRHI::FBlendAttachment& Attachment = Blend[Index];
        HashCombine(Hash, static_cast<uint64>(ColorFormats[Index]));
        HashCombine(Hash, static_cast<uint64>(Attachment.WriteMask));
        HashCombine(Hash, static_cast<uint64>(Attachment.bBlendingEnabled));
        HashCombine(Hash, static_cast<uint64>(Attachment.AlphaBlendOperation));
        HashCombine(Hash, static_cast<uint64>(Attachment.ColorBlendOperation));
        HashCombine(Hash, static_cast<uint64>(Attachment.DestinationAlphaBlendFactor));
        HashCombine(Hash, static_cast<uint64>(Attachment.DestinationColorBlendFactor));
        HashCombine(Hash, static_cast<uint64>(Attachment.SourceAlphaBlendFactor));
        HashCombine(Hash, static_cast<uint64>(Attachment.SourceColorBlendFactor));
    }

    HashCombine(Hash, static_cast<uint64>(DepthFormat));
    HashCombine(Hash, static_cast<uint64>(StencilFormat));
    HashCombine(Hash, static_cast<uint64>(TopologyClass));
    HashCombine(Hash, SampleCount);
    HashCombine(Hash, MaxVertexAmplificationCount);
    HashCombine(Hash, bAlphaToCoverage);
    return Hash;
}

FMetalPipelineCache::FMetalPipelineCache(FMetalDevice* InDevice)
    : Device(InDevice)
    , RenderPipelines()
    , ComputePipelines()
    , RenderPipelinesCS()
    , ComputePipelinesCS()
{
}

FMetalPipelineCache::~FMetalPipelineCache() = default;

template<typename CreateFunctionType>
TSharedRef<FMetalCachedRenderPipeline> FMetalPipelineCache::FindOrCreateRenderPipeline(const FMetalRenderPipelineKey& Key, CreateFunctionType&& CreateFunction)
{
    const uint64 Hash = Key.GetHash();
    {
        TScopedLock Lock(RenderPipelinesCS);
        if (TSharedRef<FMetalCachedRenderPipeline> Existing = FindRenderPipeline(Hash, Key))
        {
            STAT_ADD(STAT_Metal_PSOCacheHits, 1);
            return Existing;
        }
    }

    TSharedRef<FMetalCachedRenderPipeline> NewPipeline = CreateFunction();
    if (!NewPipeline)
    {
        return nullptr;
    }

    NewPipeline->Functions = RetainFunctions(Key.VertexOrMeshFunction, Key.ObjectFunction, Key.FragmentFunction);

    TScopedLock Lock(RenderPipelinesCS);
    if (TSharedRef<FMetalCachedRenderPipeline> Existing = FindRenderPipeline(Hash, Key))
    {
        return Existing;
    }

    STAT_ADD(STAT_Metal_PSOCacheMisses, 1);
    RenderPipelines.FindOrAdd(Hash).Emplace(FRenderEntry{ Key, NewPipeline });
    return NewPipeline;
}

TSharedRef<FMetalCachedRenderPipeline> FMetalPipelineCache::GetOrCreateRenderPipeline(const FMetalRenderPipelineKey& Key, MTLRenderPipelineDescriptor* Descriptor)
{
    return FindOrCreateRenderPipeline(Key, [this, Descriptor]()
    {
        return Device->GetBinaryArchive().CreateRenderPipeline(Descriptor);
    });
}

TSharedRef<FMetalCachedRenderPipeline> FMetalPipelineCache::GetOrCreateMeshRenderPipeline(const FMetalRenderPipelineKey& Key, MTLMeshRenderPipelineDescriptor* Descriptor)
{
    return FindOrCreateRenderPipeline(Key, [this, Descriptor]()
    {
        return Device->GetBinaryArchive().CreateMeshRenderPipeline(Descriptor);
    });
}

TSharedRef<FMetalCachedComputePipeline> FMetalPipelineCache::GetOrCreateComputePipeline(id<MTLFunction> Function, MTLComputePipelineDescriptor* Descriptor)
{
    const void* Identity = GetFunctionIdentity(Function);
    {
        TScopedLock Lock(ComputePipelinesCS);
        if (TSharedRef<FMetalCachedComputePipeline>* Existing = ComputePipelines.Find(Identity))
        {
            STAT_ADD(STAT_Metal_PSOCacheHits, 1);
            return *Existing;
        }
    }

    TSharedRef<FMetalCachedComputePipeline> NewPipeline = Device->GetBinaryArchive().CreateComputePipeline(Descriptor);
    if (!NewPipeline)
    {
        return nullptr;
    }

    NewPipeline->Functions = RetainFunctions(Function, nil, nil);

    TScopedLock Lock(ComputePipelinesCS);
    if (TSharedRef<FMetalCachedComputePipeline>* Existing = ComputePipelines.Find(Identity))
    {
        return *Existing;
    }

    STAT_ADD(STAT_Metal_PSOCacheMisses, 1);
    ComputePipelines.Add(Identity, NewPipeline);
    return NewPipeline;
}

void FMetalPipelineCache::Prune()
{
    {
        TScopedLock Lock(RenderPipelinesCS);

        TArray<uint64> EmptyBuckets;
        RenderPipelines.Foreach([&EmptyBuckets](const uint64& Hash, TArray<FRenderEntry>& Bucket)
        {
            for (int32 Index = Bucket.Size() - 1; Index >= 0; --Index)
            {
                if (Bucket[Index].Pipeline->GetRefCount() == 1)
                {
                    Bucket.RemoveAtSwap(Index);
                }
            }

            if (Bucket.IsEmpty())
            {
                EmptyBuckets.Add(Hash);
            }
        });

        for (uint64 Hash : EmptyBuckets)
        {
            RenderPipelines.Remove(Hash);
        }
    }

    {
        TScopedLock Lock(ComputePipelinesCS);

        TArray<const void*> Unreferenced;
        ComputePipelines.Foreach([&Unreferenced](const void* const& Identity, TSharedRef<FMetalCachedComputePipeline>& Pipeline)
        {
            if (Pipeline->GetRefCount() == 1)
            {
                Unreferenced.Add(Identity);
            }
        });

        for (const void* Identity : Unreferenced)
        {
            ComputePipelines.Remove(Identity);
        }
    }
}

TSharedRef<FMetalCachedRenderPipeline> FMetalPipelineCache::FindRenderPipeline(uint64 Hash, const FMetalRenderPipelineKey& Key) const
{
    const TArray<FRenderEntry>* Bucket = RenderPipelines.Find(Hash);
    if (!Bucket)
    {
        return nullptr;
    }

    for (const FRenderEntry& Entry : *Bucket)
    {
        if (Entry.Key == Key)
        {
            return Entry.Pipeline;
        }
    }

    return nullptr;
}
