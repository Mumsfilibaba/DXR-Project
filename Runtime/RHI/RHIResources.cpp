#include "RHIResources.h"
#include "RHIInstance.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIShaderResourceViewCache

CRHIShaderResourceViewCache& CRHIShaderResourceViewCache::Get()
{
    TOptional<CRHIShaderResourceViewCache>& Cache = GetCacheInstance();
    return Cache.GetValue();
}

void CRHIShaderResourceViewCache::Destroy()
{
    TOptional<CRHIShaderResourceViewCache>& Cache = GetCacheInstance();
    Cache.Reset();
}

CRHIShaderResourceViewRef CRHIShaderResourceViewCache::GetOrCreateView(const CRHITextureSRVInitializer& Initializer)
{
    auto TexturesIt = Textures.find(Initializer);
    if (TexturesIt != Textures.end())
    {
        return TexturesIt->second;
    }

    CRHIShaderResourceViewRef NewSRV = RHICreateShaderResourceView(Initializer);

    auto Result = Textures.insert(std::make_pair(Initializer, NewSRV));
    Check(Result.second);

    return NewSRV;
}

CRHIShaderResourceViewRef CRHIShaderResourceViewCache::GetOrCreateView(const CRHIBufferSRVInitializer& Initializer)
{
    auto BufferIt = Buffers.find(Initializer);
    if (BufferIt != Buffers.end())
    {
        return BufferIt->second;
    }

    CRHIShaderResourceViewRef NewSRV = RHICreateShaderResourceView(Initializer);

    auto Result = Buffers.insert(std::make_pair(Initializer, NewSRV));
    Check(Result.second);

    return NewSRV;
}

TOptional<CRHIShaderResourceViewCache>& CRHIShaderResourceViewCache::GetCacheInstance()
{
    static TOptional<CRHIShaderResourceViewCache> Instance(InPlace);
    return Instance;
}


/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIUnorderedAccessViewCache

CRHIUnorderedAccessViewCache& CRHIUnorderedAccessViewCache::Get()
{
    TOptional<CRHIUnorderedAccessViewCache>& Cache = GetCacheInstance();
    return Cache.GetValue();
}

void CRHIUnorderedAccessViewCache::Destroy()
{
    TOptional<CRHIUnorderedAccessViewCache>& Cache = GetCacheInstance();
    Cache.Reset();
}

CRHIUnorderedAccessViewRef CRHIUnorderedAccessViewCache::GetOrCreateView(const CRHITextureUAVInitializer& Initializer)
{
    auto TexturesIt = Textures.find(Initializer);
    if (TexturesIt != Textures.end())
    {
        return TexturesIt->second;
    }

    CRHIUnorderedAccessViewRef NewUAV = RHICreateUnorderedAccessView(Initializer);

    auto Result = Textures.insert(std::make_pair(Initializer, NewUAV));
    Check(Result.second);

    return NewUAV;
}

CRHIUnorderedAccessViewRef CRHIUnorderedAccessViewCache::GetOrCreateView(const CRHIBufferUAVInitializer& Initializer)
{
    auto BufferIt = Buffers.find(Initializer);
    if (BufferIt != Buffers.end())
    {
        return BufferIt->second;
    }

    CRHIUnorderedAccessViewRef NewUAV = RHICreateUnorderedAccessView(Initializer);

    auto Result = Buffers.insert(std::make_pair(Initializer, NewUAV));
    Check(Result.second);

    return NewUAV;
}

TOptional<CRHIUnorderedAccessViewCache>& CRHIUnorderedAccessViewCache::GetCacheInstance()
{
    static TOptional<CRHIUnorderedAccessViewCache> Instance(InPlace);
    return Instance;
}

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHISamplerStateCache

CRHISamplerStateCache& CRHISamplerStateCache::Get()
{
    TOptional<CRHISamplerStateCache>& Cache = GetCacheInstance();
    return Cache.GetValue();
}

void CRHISamplerStateCache::Destroy()
{
    TOptional<CRHISamplerStateCache>& Cache = GetCacheInstance();
    Cache.Reset();
}

CRHISamplerStateRef CRHISamplerStateCache::GetOrCreateSampler(const CRHISamplerStateInitializer& Initializer)
{
    auto SamplerIt = Samplers.find(Initializer);
    if (SamplerIt != Samplers.end())
    {
        return SamplerIt->second;
    }

    CRHISamplerStateRef NewSampler = RHICreateSamplerState(Initializer);
    
    auto Result = Samplers.insert(std::make_pair(Initializer, NewSampler));
    Check(Result.second);

    return NewSampler;
}

TOptional<CRHISamplerStateCache>& CRHISamplerStateCache::GetCacheInstance()
{
    static TOptional<CRHISamplerStateCache> Instance(InPlace);
    return Instance;
}