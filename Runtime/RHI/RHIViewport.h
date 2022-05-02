#pragma once
#include "RHIResources.h"
#include "RHIResourceViews.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedefs

typedef TSharedRef<class CRHIViewport> RHIViewportRef;

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
    
    explicit CRHIViewport(const CRHIViewportInitializer& Initializer)
        : CRHIResource()
        , Width(Initializer.Width)
        , Height(Initializer.Height)
        , Format(Initializer.ColorFormat)
    { }

    ~CRHIViewport() = default;

public:

    /**
     * @brief: Resize the viewport
     * 
     * @param InWidth: New width of the viewport
     * @param InHeight: New height of the viewport
     * @return: Returns true if the resize is successful
     */
    virtual bool Resize(uint32 InWidth, uint32 InHeight) { return true; }

    /** @param bVerticalSync: True if the swap should have VerticalSync enabled */
    virtual bool Present(bool bVerticalSync) { return true; }

    /** @return: Returns the current RenderTargetView */
    virtual CRHIRenderTargetView* GetRenderTargetView() const { return nullptr; };
    
    /** @return: Returns the current Texture2D */
    virtual CRHITexture2D* GetBackBuffer() const { return nullptr; };

    /** @return: Returns the Width of the Viewport */
    uint32 GetWidth() const { return Width; }

    /** @return: Returns the Height of the Viewport */
    uint32 GetHeight() const { return Height; }

    /** @return: Returns the ColorFormat of the Viewport */
    EFormat GetColorFormat() const { return Format; }

protected:
    uint16  Width;
    uint16  Height;
    EFormat Format;
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif