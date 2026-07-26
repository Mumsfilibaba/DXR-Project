#ifndef BINDLESS_HELPERS_HLSLI
#define BINDLESS_HELPERS_HLSLI

#include "DescriptorTypes.hlsli"

#define GetResourceFromDescriptorHandle(Handle) ResourceDescriptorHeap[(Handle).GetIndex()]
#define GetResourceFromDescriptorHandleNonUniform(Handle) ResourceDescriptorHeap[NonUniformResourceIndex((Handle).GetIndex())]
#define GetResourceFromPackedDescriptorIndex(Packed) GetResourceFromDescriptorHandle(FDescriptorHandle::FromPacked(Packed))
#define GetResourceFromPackedDescriptorIndexNonUniform(Packed) GetResourceFromDescriptorHandleNonUniform(FDescriptorHandle::FromPacked(Packed))

#define GetSamplerFromDescriptorHandle(Handle) SamplerDescriptorHeap[(Handle).GetIndex()]
#define GetSamplerFromDescriptorHandleNonUniform(Handle) SamplerDescriptorHeap[NonUniformResourceIndex((Handle).GetIndex())]
#define GetSamplerFromPackedDescriptorIndex(Packed) GetSamplerFromDescriptorHandle(FDescriptorHandle::FromPacked(Packed))
#define GetSamplerFromPackedDescriptorIndexNonUniform(Packed) GetSamplerFromDescriptorHandleNonUniform(FDescriptorHandle::FromPacked(Packed))

#endif
