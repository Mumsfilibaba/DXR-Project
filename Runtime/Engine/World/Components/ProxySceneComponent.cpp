#include "ProxySceneComponent.h"
#include "RHI/RHIQuery.h"
#include "Core/Memory/Memory.h"

FProxySceneComponent::FProxySceneComponent()
    : Material(nullptr)
    , Mesh(nullptr)
    , CurrentActor(nullptr)
    , Geometry(nullptr)
    , CurrentOcclusionQuery(nullptr)
    , VertexBuffer(nullptr)
    , IndexBuffer(nullptr)
    , NumVertices(0)
    , NumIndices(0)
    , IndexFormat(EIndexFormat::Unknown)
    , CurrentOcclusionQueryIndex(0)
{
    FMemory::Memzero(OcclusionQueries, sizeof(OcclusionQueries));
}

FProxySceneComponent::~FProxySceneComponent()
{
    for (FRHIQuery* Query : OcclusionQueries)
    {
        if (Query)
        {
            Query->Release();
        }
    }
}