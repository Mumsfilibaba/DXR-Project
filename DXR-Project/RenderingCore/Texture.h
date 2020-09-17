#include "Resource.h"

class Texture : public Resource
{
public:
    Texture() = default;
    ~Texture() = default;

    // Casting functions
    virtual Texture* AsTexture() override
    {
        return this;
    }

    virtual const Texture* AsTexture() const override
    {
        return this;
    }

    virtual Buffer* AsBuffer() override
    {
        return nullptr;
    }

    virtual const Buffer* AsBuffer() const override
    {
        return nullptr;
    }

    // Resource views
    void SetRenderTargetView(RenderTargetView* InRenderTargetView, const SubresourceIndex& InSubresourceIndex)
    {
        const int SubresourceIndex = InSubresourceIndex.GetSubresourceIndex();
        if (SubresourceIndex < RenderTargetViews.size())
        {
            RenderTargetViews.resize(SubresourceIndex + 1);
        }

        RenderTargetViews[SubresourceIndex] = InRenderTargetView;
    }

    void SetDepthStencilView(DepthStencilView* InDepthStencilView, const SubresourceIndex& InSubresourceIndex)
    {
        const int SubresourceIndex = InSubresourceIndex.GetSubresourceIndex();
        if (SubresourceIndex < DepthStencilViews.size())
        {
            DepthStencilViews.resize(SubresourceIndex + 1);
        }

        DepthStencilViews[SubresourceIndex] = InDepthStencilView;
    }

    RenderTargetView* GetRenderTargetView(const SubresourceIndex& InSubresourceIndex) const
    {
        const int SubresourceIndex = InSubresourceIndex.GetSubresourceIndex();
        return RenderTargetViews[SubresourceIndex];
    }

    DepthStencilView* GetDepthStencilView(const SubresourceIndex& InSubresourceIndex) const
    {
        const int SubresourceIndex = InSubresourceIndex.GetSubresourceIndex();
        return DepthStencilViews[SubresourceIndex];
    }

protected:
    std::vector<RenderTargetView*> RenderTargetViews;
    std::vector<DepthStencilView*> DepthStencilViews;
};