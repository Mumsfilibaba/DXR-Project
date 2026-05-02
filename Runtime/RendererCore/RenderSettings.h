#pragma once
#include "Core/Core.h"

struct RenderSettings
{
public:
    static RENDERERCORE_API void ChangeRenderResolution(uint32 Width, uint32 Height);
    static RENDERERCORE_API void OnDidChangeRenderResolution(uint32 Width, uint32 Height);

    static FORCEINLINE uint32 GetRenderWidth()  { return RenderWidth; }
    static FORCEINLINE uint32 GetRenderHeight() { return RenderHeight; }
    static FORCEINLINE bool   NeedsResize()     { return bNeedsResize; }

private:
    static RENDERERCORE_API uint32 RenderWidth;
    static RENDERERCORE_API uint32 RenderHeight;
    static RENDERERCORE_API bool   bNeedsResize;
};
