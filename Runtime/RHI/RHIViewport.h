#pragma once
#include "RHIResources.h"
#include "RHIResourceViews.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIViewport

class CRHIViewport : public CRHIObject
{
public:

    /**
     * @brief: Constructor
     * 
     * @param InFormat: Format for the viewport
     * @param InWidth: Width of the viewport
     * @param InHeight: Height of the viewport
     */
    
    CRHIViewport(EFormat InFormat, uint32 InWidth, uint32 InHeight)
        : CRHIObject()
        , Width(InWidth)
        , Height(InHeight)
        , Format(InFormat)
    { }

    ~CRHIViewport() = default;

    /**
     * @brief: Resize the viewport
     * 
     * @param Width: New width of the viewport
     * @param Height: New height of the viewport
     * @return: Returns true if the resize is successful
     */
    virtual bool Resize(uint32 Width, uint32 Height) = 0;

    /**
     * @brief: Swap the backbuffers of the viewport
     * 
     * @param bVerticalSync: True if the swap should have VerticalSync enabled
     */
    virtual bool Present(bool bVerticalSync) = 0;

    /**
     * @brief: Retrieve the current RenderTargetView of the viewport
     * 
     * @return: Returns the current RenderTargetView
     */
    virtual CRHIRenderTargetView* GetRenderTargetView() const = 0;
    
    /**
     * @brief: Retrieve the current Texture2D of the viewport
     *
     * @return: Returns the current Texture2D
     */
    virtual CRHITexture2D* GetBackBuffer() const = 0;

    /**
     * @brief: Retrieve the width of the viewport
     * 
     * @return: Returns the viewport of the width
     */
    FORCEINLINE uint32 GetWidth()  const
    {
        return Width;
    }

    /**
     * @brief: Retrieve the height of the viewport
     *
     * @return: Returns the height of the width
     */
    FORCEINLINE uint32 GetHeight() const
    {
        return Height;
    }

    /**
     * @brief: Retrieve the color-format of the viewport
     *
     * @return: Returns the color-format of the width
     */
    FORCEINLINE EFormat GetColorFormat() const
    {
        return Format;
    }

protected:
    uint32  Width;
    uint32  Height;
    EFormat Format;
};