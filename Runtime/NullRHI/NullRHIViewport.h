#pragma once
#include "Core/Containers/ArrayView.h"

#include "RHI/RHIViewport.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

class CNullRHIViewport : public CRHIViewport
{
public:
    CNullRHIViewport( EFormat InFormat, uint32 InWidth, uint32 InHeight )
        : CRHIViewport( InFormat, InWidth, InHeight )
        , BackBuffer( dbg_new TNullRHITexture<CNullRHITexture2D>( InFormat, Width, Height, 1, 1, 0, SClearValue() ) )
        , BackBufferView( dbg_new CNullRHIRenderTargetView() )
    {
    }

    ~CNullRHIViewport() = default;

    virtual bool Resize( uint32 InWidth, uint32 InHeight ) override final
    {
        Width = InWidth;
        Height = InHeight;
        return true;
    }

    virtual bool Present( bool VerticalSync ) override final
    {
        return true;
    }

    virtual void SetName( const CString& InName ) override final
    {
        CRHIResource::SetName( InName );
    }

    virtual CRHIRenderTargetView* GetRenderTargetView() const override final
    {
        return BackBufferView.Get();
    }

    virtual CRHITexture2D* GetBackBuffer() const override final
    {
        return BackBuffer.Get();
    }

    virtual bool IsValid() const
    {
        return true;
    }

private:
    TSharedRef<CNullRHITexture2D>        BackBuffer;
    TSharedRef<CNullRHIRenderTargetView> BackBufferView;
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif
