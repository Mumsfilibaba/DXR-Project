#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Map.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Platform/CriticalSection.h"
#include "RendererCore/VertexDeclaration.h"
#include "RHI/RHITypes.h"

struct FVertexStreamBinding
{
    FRHIInputLayoutRef InputLayout;
    uint8              StreamIndices[VERTEX_MAX_STREAMS] = {};
    uint8              NumStreams                        = 0;
};

struct RENDERERCORE_API FVertexStreamCache
{
public:
    static bool Initialize();
    static void Release();

    static FORCEINLINE FVertexStreamCache& Get()
    {
        return *VertexStreamCache;
    }

public:
    const FVertexStreamBinding* GetBinding(const FVertexDeclaration& Declaration, EVertexAttributeFlags Required);

private:
    FVertexStreamCache();
    ~FVertexStreamCache();

    const FVertexStreamBinding* CreateBinding(const FVertexDeclaration& Declaration, EVertexAttributeFlags Required);

    TArray<TUniquePtr<FVertexStreamBinding>> Bindings;
    TMap<uint32, int32>                      BindingLookup;
    FCriticalSection                         BindingsCS;

    static FVertexStreamCache* VertexStreamCache;
};
