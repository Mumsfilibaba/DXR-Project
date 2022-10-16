#include "MetalDeviceContext.h"


FMetalDeviceContext::FMetalDeviceContext(FMetalInterface* InCoreInterface, id<MTLDevice> InDevice)
    : CoreInterface(InCoreInterface)
    , Device(InDevice)
    , CommandQueue([InDevice newCommandQueue])
{
    CHECK(CommandQueue != nullptr);
}

FMetalDeviceContext::~FMetalDeviceContext()
{
    NSSafeRelease(Device);
    NSSafeRelease(CommandQueue);
}

FMetalDeviceContext* FMetalDeviceContext::CreateContext(FMetalInterface* InCoreInterface)
{
    SCOPED_AUTORELEASE_POOL();
    
    METAL_ERROR_COND(InCoreInterface != nullptr);
    
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
    return dbg_new FMetalDeviceContext(InCoreInterface, SelectedDevice);
}
