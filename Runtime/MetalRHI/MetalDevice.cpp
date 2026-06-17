#include "MetalRHI/MetalDevice.h"
#include "MetalRHI/MetalDeviceChild.h"

FMetalDeviceChild::~FMetalDeviceChild()
{
    Device = nullptr;
}

FMetalDevice::FMetalDevice()
    : Device(nil)
    , CommandQueue(nil)
{
}

FMetalDevice::~FMetalDevice()
{
    [Device release];
    [CommandQueue release];
}

bool FMetalDevice::Initialize()
{
    SCOPED_AUTORELEASE_POOL();

    NSArray<id<MTLDevice>>* AvailableDevices = MTLCopyAllDevices();

    id<MTLDevice> SelectedDevice = nil;
    for (id<MTLDevice> CandidateDevice in AvailableDevices)
    {
        if (!CandidateDevice.isRemovable && !CandidateDevice.isLowPower)
        {
            SelectedDevice = CandidateDevice;
        }
    }

    if (!SelectedDevice)
    {
        SelectedDevice = MTLCreateSystemDefaultDevice();
    }

    [AvailableDevices release];

    if (!SelectedDevice)
    {
        METAL_ERROR("Failed to select a Metal device");
        return false;
    }

    const String DeviceName = SelectedDevice.name;
    METAL_INFO("Selected Device=%s", *DeviceName);

    const bool bSupportRayTracing           = SelectedDevice.supportsRaytracing;
    const bool bSupportRayTracingFromRender = SelectedDevice.supportsRaytracingFromRender;
    METAL_INFO("bSupportRayTracing=%s, bSupportRayTracingFromRender=%s", bSupportRayTracing ? "true" : "false", bSupportRayTracingFromRender ? "true" : "false");

    Device       = SelectedDevice;
    CommandQueue = [Device newCommandQueue];
    if (!CommandQueue)
    {
        METAL_ERROR("Failed to create MTLCommandQueue");
        return false;
    }

    return true;
}
