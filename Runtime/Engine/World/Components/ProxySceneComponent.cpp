#include "ProxySceneComponent.h"
#include "RHI/RHI.h"
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
    , bIsOccluded(false)
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

void FProxySceneComponent::UpdateOcclusion()
{
    if (!FrustumVisibility.bWasVisible)
    {
        bIsOccluded = false;
        return;
    }

    if (!CurrentOcclusionQuery)
    {
        bIsOccluded = false;
        return;
    }

    uint64 NumSamples;
    if (!GetRHI()->RHIGetQueryResult(CurrentOcclusionQuery, NumSamples))
    {
        bIsOccluded = false;
        return;
    }

    if (!NumSamples)
    {
        bIsOccluded = true;
        return;
    }

    bIsOccluded = false;
}
