#pragma once
#include "Core/Containers/Array.h"
#include "Core/Containers/Map.h"
#include "Core/Containers/UniquePtr.h"
#include "Core/Platform/CriticalSection.h"
#include "RendererCore/VertexDeclaration.h"
#include "RHI/RHITypes.h"

/** @brief Everything a pass needs to bind a mesh's vertex streams and build a matching pipeline. */
struct FVertexStreamBinding
{
    FRHIInputLayoutRef InputLayout;
    uint8              StreamIndices[VERTEX_MAX_STREAMS] = {};
    uint8              NumStreams                        = 0;
};

/** @brief Resolves (declaration, required attributes) pairs into cached, stable stream bindings. */
struct RENDERERCORE_API FVertexStreamCache
{
public:
    static bool Initialize();
    static void Release();

    static FORCEINLINE FVertexStreamCache& Get()
    {
        return *GVertexStreamCache;
    }

public:

    /**
     * @brief Resolves the binding a pass needs to read Required out of a mesh using Declaration.
     * Bindings are interned and live until Release(), so the returned pointer is stable and can be
     * stored on a cached pipeline-state instance.
     * @param Declaration The mesh's declaration.
     * @param Required The attributes the pass reads.
     * @return The binding, or nullptr when the declaration cannot satisfy Required.
     */
    const FVertexStreamBinding* GetBinding(const FVertexDeclaration& Declaration, EVertexAttributeFlags Required);

private:
    FVertexStreamCache();
    ~FVertexStreamCache();

    const FVertexStreamBinding* CreateBinding(const FVertexDeclaration& Declaration, EVertexAttributeFlags Required);

    TArray<TUniquePtr<FVertexStreamBinding>> Bindings;
    TMap<uint32, int32>                      BindingLookup;
    FCriticalSection                         BindingsCS;

    static FVertexStreamCache* GVertexStreamCache;
};
