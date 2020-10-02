#pragma once
#include "ResourceViews.h"

#include "Core/RefCountedObject.h"

#include "Containers/TArray.h"

/*
* SubresourceIndex
*/

struct SubresourceIndex
{
	inline explicit SubresourceIndex(Int32 InMipSlice, Int32 InArraySlice, Int32 InPlaneSlice, Int32 InMipLevels, Int32 InArraySize)
		: MipSlice(InMipSlice)
		, MipLevels(InMipLevels)
		, ArraySlice(InArraySlice)
		, ArraySize(InArraySize)
		, PlaneSlice(InPlaneSlice)
	{
	}

	inline SubresourceIndex(const SubresourceIndex& Other)
		: MipSlice(Other.MipSlice)
		, MipLevels(Other.MipLevels)
		, ArraySlice(Other.ArraySlice)
		, ArraySize(Other.ArraySize)
		, PlaneSlice(Other.PlaneSlice)
	{
	}

	inline Int32 GetSubresourceIndex() const
	{
		return MipSlice + (ArraySlice * MipLevels) + (PlaneSlice * MipLevels * ArraySize); 
	}

	const Int32 MipSlice;
	const Int32 MipLevels;
	const Int32 ArraySlice;
	const Int32 ArraySize;
	const Int32 PlaneSlice;
};

class Texture;
class Buffer;

/*
* Resource
*/

class Resource : public RefCountedObject
{
public: 
	virtual ~Resource() = default;

	// Casting Functions
	virtual Texture* AsTexture()
	{
		return nullptr;
	}

	virtual const Texture* AsTexture() const
	{
		return nullptr;
	}

	virtual Buffer* AsBuffer()
	{
		return nullptr;
	}

	virtual const Buffer* AsBuffer() const
	{
		return nullptr;
	}

	// Resource views
	void SetShaderResourceView(ShaderResourceView* InShaderResourceView, const SubresourceIndex& InSubresourceIndex)
	{
		const Int32 SubresourceIndex = InSubresourceIndex.GetSubresourceIndex();
		if (SubresourceIndex < ShaderResourceViews.Size())
		{
			ShaderResourceViews.Resize(SubresourceIndex + 1);
		}

		ShaderResourceViews[SubresourceIndex] = InShaderResourceView;
	}

	void SetUnorderedAccessView(UnorderedAccessView* InUnorderedAccessView, const SubresourceIndex& InSubresourceIndex)
	{
		const Int32 SubresourceIndex = InSubresourceIndex.GetSubresourceIndex();
		if (SubresourceIndex < UnorderedAccessViews.Size())
		{
			UnorderedAccessViews.Resize(SubresourceIndex + 1);
		}

		UnorderedAccessViews[SubresourceIndex] = InUnorderedAccessView;
	}

	ShaderResourceView* GetShaderResourceView(const SubresourceIndex& InSubresourceIndex) const
	{
		const Int32 SubresourceIndex = InSubresourceIndex.GetSubresourceIndex();
		return ShaderResourceViews[SubresourceIndex];
	}

	UnorderedAccessView* GetUnorderedAccessView(const SubresourceIndex& InSubresourceIndex) const
	{
		const Int32 SubresourceIndex = InSubresourceIndex.GetSubresourceIndex();
		return UnorderedAccessViews[SubresourceIndex];
	}

protected:
	TArray<ShaderResourceView*>		ShaderResourceViews;
	TArray<UnorderedAccessView*>	UnorderedAccessViews;
};