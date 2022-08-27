#pragma once
#include "RHIResources.h"
#include "RHIResourceViews.h"

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Typedefs

typedef TSharedRef<class FRHIViewport> FRHIViewportRef;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FRHIViewportInitializer

struct FRHIViewportInitializer
{
    FRHIViewportInitializer()
        : WindowHandle(nullptr)
        , ColorFormat(EFormat::Unknown)
        , DepthFormat(EFormat::Unknown)
        , Width(0)
        , Height(0)
    { }

    FRHIViewportInitializer(
        void* InWindowHandle,
        EFormat InColorFormat,
        EFormat InDepthFormat,
        uint16 InWidth,
        uint16 InHeight)
        : WindowHandle(InWindowHandle)
        , ColorFormat(InColorFormat)
        , DepthFormat(InDepthFormat)
        , Width(InWidth)
        , Height(InHeight)
    { }

    bool operator==(const FRHIViewportInitializer& RHS) const
    {
        return (WindowHandle == RHS.WindowHandle)
            && (ColorFormat  == RHS.ColorFormat)
            && (DepthFormat  == RHS.DepthFormat)
            && (Width        == RHS.Width)
            && (Height       == RHS.Height);
    }

    bool operator!=(const FRHIViewportInitializer& RHS) const
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
// FRHIViewport

class FRHIViewport 
    : public FRHIResource
{
protected:
    explicit FRHIViewport(const FRHIViewportInitializer& Initializer)
        : FRHIResource()
        , Format(Initializer.ColorFormat)
        , Width(Initializer.Width)
        , Height(Initializer.Height)
    { }

    ~FRHIViewport() = default;

public:
    virtual bool Resize(uint32 InWidth, uint32 InHeight) { return true; }

    virtual FRHITexture2D* GetBackBuffer() const { return nullptr; };

    EFormat GetColorFormat() const { return Format; }

    uint32  GetWidth()       const { return Width;  }
    uint32  GetHeight()      const { return Height; }

protected:
    EFormat Format;
    uint16  Width;
    uint16  Height;
};

#if defined(PLATFORM_COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(PLATFORM_COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
