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
    {
        Check(Width  != 0);
        Check(Height != 0);
    }

    ~CRHIViewport() = default;

public:

    virtual bool Resize(uint32 InWidth, uint32 InHeight) { return true; }

    virtual bool Present(bool bVerticalSync) { return true; }
    
    virtual CRHITexture2D* GetBackBuffer() const { return nullptr; };

    uint32 GetWidth() const { return Width; }

    uint32 GetHeight() const { return Height; }

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
