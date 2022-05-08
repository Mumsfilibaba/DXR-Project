#include "MetalDeviceContext.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalDeviceContext

CMetalDeviceContext::CMetalDeviceContext(CMetalCoreInterface* InCoreInterface, id<MTLDevice> InDevice)
    : CoreInterface(InCoreInterface)
    , Device(InDevice)
    , CommandQueue([InDevice newCommandQueue])
{
    Check(CommandQueue != nullptr);
}

CMetalDeviceContext::~CMetalDeviceContext()
{
    NSSafeRelease(Device);
    NSSafeRelease(CommandQueue);
}

CMetalDeviceContext* CMetalDeviceContext::CreateContext(CMetalCoreInterface* InCoreInterface)
{
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
    
    const String DeviceName = SelectedDevice.name;
    METAL_INFO("Selected Device=%s", DeviceName.CStr());

    const bool bSupportRayTracing           = SelectedDevice.supportsRaytracing;
    const bool bSupportRayTracingFromRender = SelectedDevice.supportsRaytracingFromRender;
    METAL_INFO("bSupportRayTracing=%s, bSupportRayTracingFromRender=%s", bSupportRayTracing ? "true" : "false", bSupportRayTracingFromRender ? "true" : "false");
    
	return dbg_new CMetalDeviceContext(InCoreInterface, SelectedDevice);
}
