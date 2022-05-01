#pragma once
#include "RHIResources.h"
#include "RHIResourceViews.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIViewportInitializer

class CRHIViewportInitializer
{
public:

    CRHIViewportInitializer()
        : WindowHandle(nullptr)
        , ColorFormat(EFormat::Unknown)
        , DepthFormat(EFormat::Unknown)
        , Width(0)
        , Height(0)
    { }

    CRHIViewportInitializer( void* InWindowHandle
                           , EFormat InColorFormat
                           , EFormat InDepthFormat
                           , uint16 InWidth
                           , uint16 InHeight)
        : WindowHandle(InWindowHandle)
        , ColorFormat(InColorFormat)
        , DepthFormat(InDepthFormat)
        , Width(InWidth)
        , Height(InHeight)
    { }

    bool operator==(const CRHIViewportInitializer& RHS) const
    {
        return (WindowHandle == RHS.WindowHandle)
            && (ColorFormat  == RHS.ColorFormat)
            && (DepthFormat  == RHS.DepthFormat)
            && (Width        == RHS.Width)
            && (Height       == RHS.Height);
    }

    bool operator!=(const CRHIViewportInitializer& RHS) const
    {
        return !(*this == RHS);
    }

    void*   WindowHandle;

    EFormat ColorFormat;
    EFormat DepthFormat;

    uint16  Width;
    uint16  Height;
};

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CRHIViewport

class CRHIViewport : public CRHIResource
{
protected:
    
    explicit CRHIViewport(EFormat InFormat, uint32 InWidth, uint32 InHeight)
        : CRHIResource()
        , Width(InWidth)
        , Height(InHeight)
        , Format(InFormat)
    { }

    ~CRHIViewport() = default;

public:

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