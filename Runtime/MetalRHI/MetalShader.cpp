#include "MetalRHI/MetalShader.h"

FMetalShader::FMetalShader(FMetalDevice* InDevice, EShaderVisibility::Type InVisibility)
    : FMetalDeviceChild(InDevice)
    , Library(nil)
    , FunctionName(nil)
    , Visibility(InVisibility)
    , Function(nil)
{
}

FMetalShader::~FMetalShader()
{
    [Library release];
    [FunctionName release];
    [Function release];
}

bool FMetalShader::Initialize(const TArray<uint8>& InCode)
{
    @autoreleasepool
    {
        // NOTE: That there are no null-terminator in the shader code, therefore, when creating this string we need to use the known size
        const CHAR* CodeString = reinterpret_cast<const CHAR*>(InCode.Data());
        const int32 CodeLength = InCode.Size();
        
        const FString SourceString(CodeString, CodeLength);
        
        NSString* Source = SourceString.GetNSString();
        CHECK(Source != nil);
        [Source retain];
        
        id<MTLDevice> Device = GetDevice()->GetMTLDevice();
        CHECK(Device != nil);
        
        NSError* Error = nil;
        Library = [Device newLibraryWithSource:Source options:nil error:&Error];
        if (!Library)
        {
            const FString ErrorString([Error localizedDescription]);
            LOG_ERROR("Failed to compile shader. Error: %s", *ErrorString);
            return false;
        }
        
        // Retrieve the entrypoint (All SPIR-V shaders have a static entrypoint)
        NSString* EntryPoint = FString("Spirv_Main").GetNSString();
        FunctionName = [EntryPoint retain];
        
        // Retrieve the function
        Function = [Library newFunctionWithName:EntryPoint];
        if (!Function)
        {
            LOG_ERROR("Failed to retrieve function from Library");
            return false;
        }
    }
    
    return true;
}

FMetalVertexShaderRHI::FMetalVertexShaderRHI(FMetalDevice* InDevice)
    : FRHIVertexShader()
    , FMetalShader(InDevice, EShaderVisibility::Vertex)
{
}

FMetalVertexShaderRHI::~FMetalVertexShaderRHI() = default;

FMetalPixelShaderRHI::FMetalPixelShaderRHI(FMetalDevice* InDevice)
    : FRHIPixelShader()
    , FMetalShader(InDevice, EShaderVisibility::Pixel)
{
}

FMetalPixelShaderRHI::~FMetalPixelShaderRHI() = default;

FMetalComputeShaderRHI::FMetalComputeShaderRHI(FMetalDevice* InDevice)
    : FRHIComputeShader()
    , FMetalShader(InDevice, EShaderVisibility::Compute)
{
}

FMetalComputeShaderRHI::~FMetalComputeShaderRHI() = default;

FMetalRayTracingShader::FMetalRayTracingShader(FMetalDevice* InDevice)
    : FMetalShader(InDevice, EShaderVisibility::Compute)
{
}

FMetalRayTracingShader::~FMetalRayTracingShader() = default;

FMetalRayGenShaderRHI::FMetalRayGenShaderRHI(FMetalDevice* InDevice)
    : FRHIRayGenShader()
    , FMetalRayTracingShader(InDevice)
{
}

FMetalRayGenShaderRHI::~FMetalRayGenShaderRHI() = default;

FMetalRayAnyHitShaderRHI::FMetalRayAnyHitShaderRHI(FMetalDevice* InDevice)
    : FRHIRayAnyHitShader()
    , FMetalRayTracingShader(InDevice)
{
}

FMetalRayAnyHitShaderRHI::~FMetalRayAnyHitShaderRHI() = default;

FMetalRayClosestHitShaderRHI::FMetalRayClosestHitShaderRHI(FMetalDevice* InDevice)
    : FRHIRayClosestHitShader()
    , FMetalRayTracingShader(InDevice)
{
}

FMetalRayClosestHitShaderRHI::~FMetalRayClosestHitShaderRHI() = default;

FMetalRayMissShaderRHI::FMetalRayMissShaderRHI(FMetalDevice* InDevice)
    : FRHIRayMissShader()
    , FMetalRayTracingShader(InDevice)
{
}

FMetalRayMissShaderRHI::~FMetalRayMissShaderRHI() = default;

void* FMetalVertexShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(GetMTLFunction());
}

void* FMetalVertexShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FMetalShader*>(this);
}

void* FMetalPixelShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(GetMTLFunction());
}

void* FMetalPixelShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FMetalShader*>(this);
}

void* FMetalRayGenShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(GetMTLFunction());
}

void* FMetalRayGenShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FMetalRayTracingShader*>(this);
}

void* FMetalRayAnyHitShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(GetMTLFunction());
}

void* FMetalRayAnyHitShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FMetalRayTracingShader*>(this);
}

void* FMetalRayClosestHitShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(GetMTLFunction());
}

void* FMetalRayClosestHitShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FMetalRayTracingShader*>(this);
}

void* FMetalRayMissShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(GetMTLFunction());
}

void* FMetalRayMissShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FMetalRayTracingShader*>(this);
}

void* FMetalComputeShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(GetMTLFunction());
}

void* FMetalComputeShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FMetalShader*>(this);
}
