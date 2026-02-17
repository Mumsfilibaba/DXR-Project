#pragma once
#include "Core/Core.h"
#include "RHI/RHITypes.h"

class RenderSettings
{
public:
    static RENDERERCORE_API void ChangeRenderResolution(uint32 Width, uint32 Height);
    static RENDERERCORE_API void OnDidChangeRenderResolution(uint32 Width, uint32 Height);
    
    static FORCEINLINE uint32  GetRenderWidth()      { return RenderWidth; }
    static FORCEINLINE uint32  GetRenderHeight()     { return RenderHeight; }
    static FORCEINLINE bool    NeedsResize()         { return bNeedsResize; }
    static FORCEINLINE EFormat GetBackBufferFormat() { return BackBufferFormat; }

private:
    static RENDERERCORE_API uint32  RenderWidth;
    static RENDERERCORE_API uint32  RenderHeight;
    static RENDERERCORE_API bool    bNeedsResize;
    static RENDERERCORE_API EFormat BackBufferFormat;
};
