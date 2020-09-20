#include "ResourceViews.h"

#include <vector>

/*
* SubresourceIndex
*/
struct SubresourceIndex
{
    inline explicit SubresourceIndex(int InMipSlice, int InArraySlice, int InPlaneSlice, int InMipLevels, int InArraySize)
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

    inline int GetSubresourceIndex() const
    {
        return MipSlice + (ArraySlice * MipLevels) + (PlaneSlice * MipLevels * ArraySize); 
    }

    const int MipSlice;
    const int MipLevels;
    const int ArraySlice;
    const int ArraySize;
    const int PlaneSlice;
};

class Texture;
class Buffer;

/*
* Resource
*/
class Resource
{
public: 
    virtual ~Resource() = default;

    // Casting Functions
    virtual Texture* AsTexture() = 0;
    virtual const Texture* AsTexture() const = 0;
    virtual Buffer* AsBuffer() = 0;
    virtual const Buffer* AsBuffer() const = 0;

    // Resource views
    void SetShaderResourceView(ShaderResourceView* InShaderResourceView, const SubresourceIndex& InSubresourceIndex)
    {
        const int SubresourceIndex = InSubresourceIndex.GetSubresourceIndex();
        if (SubresourceIndex < ShaderResourceViews.size())
        {
            ShaderResourceViews.resize(SubresourceIndex + 1);
        }

        ShaderResourceViews[SubresourceIndex] = InShaderResourceView;
    }

    void SetUnorderedAccessView(UnorderedAccessView* InUnorderedAccessView, const SubresourceIndex& InSubresourceIndex)
    {
        const int SubresourceIndex = InSubresourceIndex.GetSubresourceIndex();
        if (SubresourceIndex < UnorderedAccessViews.size())
        {
            UnorderedAccessViews.resize(SubresourceIndex + 1);
        }

        UnorderedAccessViews[SubresourceIndex] = InUnorderedAccessView;
    }

    ShaderResourceView* GetShaderResourceView(const SubresourceIndex& InSubresourceIndex) const
    {
        const int SubresourceIndex = InSubresourceIndex.GetSubresourceIndex();
        return ShaderResourceViews[SubresourceIndex];
    }

    UnorderedAccessView* GetUnorderedAccessView(const SubresourceIndex& InSubresourceIndex) const
    {
        const int SubresourceIndex = InSubresourceIndex.GetSubresourceIndex();
        return UnorderedAccessViews[SubresourceIndex];
    }

protected:
    std::vector<ShaderResourceView*> ShaderResourceViews;
    std::vector<UnorderedAccessView*> UnorderedAccessViews;
};