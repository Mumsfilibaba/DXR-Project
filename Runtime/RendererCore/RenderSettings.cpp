#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "RHI/RHI.h"
#include "RendererCore/RenderSettings.h"

RENDERERCORE_API bool GRayTracingEnabled = false;
static FAutoConsoleVariableRef CVarRayTracingEnabled(
    "Renderer.Feature.RayTracing",
    "Enables ray tracing. Only takes effect when the hardware reports ray tracing support. Turning it off releases every ray tracing "
    "resource (acceleration structures, shaders, pipelines, textures and buffers), turning it on creates them again.",
    GRayTracingEnabled,
    EConsoleVariableFlags::Default);

RENDERERCORE_API uint32 RenderSettings::RenderWidth  = 1920;
RENDERERCORE_API uint32 RenderSettings::RenderHeight = 1080;
RENDERERCORE_API bool   RenderSettings::bNeedsResize = false;

bool RenderSettings::IsRayTracingEnabled()
{
    return RHI::bSupportsRayTracing && GRayTracingEnabled;
}

void RenderSettings::ChangeRenderResolution(uint32 Width, uint32 Height)
{
    if ((RenderWidth != Width || RenderHeight != Height) && Width > 0 && Height > 0)
    {
        LOG_INFO("Changed render-resolution. From: w=%d h=%d, To: w=%d h=%d", RenderWidth, RenderHeight, Width, Height);

        RenderWidth  = Width;
        RenderHeight = Height;
        bNeedsResize = true;
    }
}

void RenderSettings::OnDidChangeRenderResolution(uint32 Width, uint32 Height)
{
	if (RenderWidth == Width && RenderHeight == Height && Width > 0 && Height > 0)
	{
		bNeedsResize = false;
	}
}
