#include "Core/Misc/OutputDeviceLogger.h"
#include "RendererCore/RenderSettings.h"

RENDERERCORE_API uint32  RenderSettings::RenderWidth      = 1920;
RENDERERCORE_API uint32  RenderSettings::RenderHeight     = 1080;
RENDERERCORE_API bool    RenderSettings::bNeedsResize     = false;
RENDERERCORE_API EFormat RenderSettings::BackBufferFormat = EFormat::B8G8R8A8_Unorm;

void RenderSettings::ChangeRenderResolution(uint32 Width, uint32 Height)
{
    if ((RenderWidth != Width || RenderHeight != Height) && Width > 0 && Height > 0)
    {
        LOG_INFO("Requested changed render-resolution. From: w=%d h=%d, To: w=%d h=%d", RenderWidth, RenderHeight, Width, Height);

        RenderWidth  = Width;
        RenderHeight = Height;
        bNeedsResize = true;
    }
}

void RenderSettings::OnDidChangeRenderResolution(uint32 Width, uint32 Height)
{
    if ((RenderWidth == Width && RenderHeight == Height) && Width > 0 && Height > 0)
    {
        bNeedsResize = false;
    }
}
