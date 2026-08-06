#include "Core/Misc/OutputDeviceLogger.h"
#include "RHI/RHI.h"
#include "RendererCore/VertexStreamCache.h"

FVertexStreamCache* FVertexStreamCache::GVertexStreamCache = nullptr;

FVertexStreamCache::FVertexStreamCache()
    : Bindings()
    , BindingLookup()
{
}

FVertexStreamCache::~FVertexStreamCache()
{
    Bindings.Clear();
    BindingLookup.Clear();
}

bool FVertexStreamCache::Initialize()
{
    GVertexStreamCache = new FVertexStreamCache();
    return true;
}

void FVertexStreamCache::Release()
{
    if (GVertexStreamCache)
    {
        delete GVertexStreamCache;
        GVertexStreamCache = nullptr;
    }
}

const FVertexStreamBinding* FVertexStreamCache::GetBinding(const FVertexDeclaration& Declaration, EVertexAttributeFlags Required)
{
    if (!Declaration.HasAttributes(Required))
    {
        return nullptr;
    }

    TScopedLock Lock(BindingsCS);

    const uint32 Key = (uint32(Declaration.GetID()) << 16) | uint32(UnderlyingTypeValue(Required));
    if (const int32* ExistingIndex = BindingLookup.Find(Key))
    {
        return Bindings[*ExistingIndex].Get();
    }

    const FVertexStreamBinding* NewBinding = CreateBinding(Declaration, Required);
    if (!NewBinding)
    {
        return nullptr;
    }

    BindingLookup.Add(Key, Bindings.LastIndex());
    return NewBinding;
}

const FVertexStreamBinding* FVertexStreamCache::CreateBinding(const FVertexDeclaration& Declaration, EVertexAttributeFlags Required)
{
    bool bStreamIsUsed[VERTEX_MAX_STREAMS] = {};
    for (const FVertexAttributeInfo& Attribute : Declaration.GetAttributes())
    {
        if (IsEnumFlagSet(Required, Attribute.Attribute))
        {
            bStreamIsUsed[Attribute.StreamIndex] = true;
        }
    }

    TUniquePtr<FVertexStreamBinding> NewBinding = MakeUniquePtr<FVertexStreamBinding>();

    uint8 SlotForStream[VERTEX_MAX_STREAMS] = {};
    for (uint8 StreamIndex = 0; StreamIndex < VERTEX_MAX_STREAMS; ++StreamIndex)
    {
        if (bStreamIsUsed[StreamIndex])
        {
            SlotForStream[StreamIndex] = NewBinding->NumStreams;
            NewBinding->StreamIndices[NewBinding->NumStreams] = StreamIndex;
            NewBinding->NumStreams++;
        }
    }

    TArray<FRHIInputElementDesc> InputElements;
    InputElements.Reserve(Declaration.GetAttributes().Size());

    for (const FVertexAttributeInfo& Attribute : Declaration.GetAttributes())
    {
        if (!IsEnumFlagSet(Required, Attribute.Attribute))
        {
            continue;
        }

        FRHIInputElementDesc Element;
        Element.Semantic           = Attribute.Semantic;
        Element.SemanticIndex      = Attribute.SemanticIndex;
        Element.Format             = Attribute.Format;
        Element.VertexStride       = Declaration.GetStreamStride(Attribute.StreamIndex);
        Element.InputSlot          = SlotForStream[Attribute.StreamIndex];
        Element.ByteOffset         = Attribute.ByteOffset;
        Element.ShaderElementIndex = InputElements.Size();
        Element.InputClass         = EVertexInputClass::Vertex;
        Element.InstanceStepRate   = 0;

        InputElements.Add(Element);
    }

    NewBinding->InputLayout = RHI::CreateInputLayout(InputElements);
    if (!NewBinding->InputLayout)
    {
        LOG_ERROR("Failed to create InputLayout for VertexDeclaration %u with attributes %u", Declaration.GetID(), UnderlyingTypeValue(Required));
        return nullptr;
    }

    Bindings.Emplace(Move(NewBinding));
    return Bindings.Last().Get();
}
