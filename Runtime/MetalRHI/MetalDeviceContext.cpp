#include "MetalDeviceContext.h"

FMetalDeviceContext::FMetalDeviceContext(id<MTLDevice> InDevice)
    : Device(InDevice)
    , CommandQueue([InDevice newCommandQueue])
{
    CHECK(CommandQueue != nullptr);
}

FMetalDeviceContext::~FMetalDeviceContext()
{
    NSSafeRelease(Device);
    NSSafeRelease(CommandQueue);
}

FMetalDeviceContext* FMetalDeviceContext::CreateContext()
{
    SCOPED_AUTORELEASE_POOL();
    
    NSArray<id<MTLDevice>>* AvailableDevices = MTLCopyAllDevices();
        
    id<MTLDevice> SelectedDevice;
    for (id<MTLDevice> Device in AvailableDevices)
    {
        if (!Device.isRemovable && !Device.isLowPower)
        {
            SelectedDevice = Device;
        }
    }
    
    if (!SelectedDevice)
    {
        SelectedDevice = MTLCreateSystemDefaultDevice();
    }
    
    const FString DeviceName = SelectedDevice.name;
    METAL_INFO("Selected Device=%s", DeviceName.GetCString());

    const bool bSupportRayTracing           = SelectedDevice.supportsRaytracing;
    const bool bSupportRayTracingFromRender = SelectedDevice.supportsRaytracingFromRender;
    METAL_INFO("bSupportRayTracing=%s, bSupportRayTracingFromRender=%s", bSupportRayTracing ? "true" : "false", bSupportRayTracingFromRender ? "true" : "false");
    
    NSRelease(AvailableDevices);
    return new FMetalDeviceContext(SelectedDevice);
}
