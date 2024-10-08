#include "ProxySceneComponent.h"
#include "RHI/RHI.h"
#include "RHI/RHIQuery.h"
#include "Core/Memory/Memory.h"

FProxySceneComponent::FProxySceneComponent()
    : Materials()
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
    , NumFramesOccluded(0)
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
    const auto CheckOcclusion = [this]()
    {
        if (!FrustumVisibility.bWasVisible)
        {
            return false;
        }

        if (!CurrentOcclusionQuery)
        {
            return false;
        }

        uint64 NumSamples;
        if (!GetRHI()->RHIGetQueryResult(CurrentOcclusionQuery, NumSamples))
        {
            return false;
        }

        if (!NumSamples)
        {
            return true;
        }

        return false;
    };

    if (CheckOcclusion())
    {
        NumFramesOccluded++;
    }
    else
    {
        NumFramesOccluded = 0;
    }
}
