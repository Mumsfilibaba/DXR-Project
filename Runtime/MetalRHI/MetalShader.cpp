#include "MetalRHI/MetalShader.h"
#include "Core/Memory/Memory.h"

static NSString* ResolveMetalFunctionName(id<MTLLibrary> Library)
{
    NSArray<NSString*>* FunctionNames = [Library functionNames];
    if (FunctionNames.count == 0)
    {
        return nil;
    }

    if ([FunctionNames containsObject:@"Main"])
    {
        return @"Main";
    }

    return FunctionNames.firstObject;
}

FMetalShader::FMetalShader(FMetalDevice* InDevice, EShaderVisibility::Type InVisibility)
    : FMetalDeviceChild(InDevice)
    , Library(nil)
    , FunctionName(nil)
    , Visibility(InVisibility)
    , Function(nil)
    , ThreadGroupSizeX(0)
    , ThreadGroupSizeY(0)
    , ThreadGroupSizeZ(0)
    , ShaderConstantsSize(0)
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
    TArrayView<const uint8> Source;
    if (!ParseMSLShaderByteCode(InCode, Bindings, Source))
    {
        LOG_ERROR("Shader bytecode is not a valid MSL blob");
        return false;
    }

    if (InCode.Size() >= static_cast<int32>(sizeof(FMSLShaderHeader)))
    {
        FMSLShaderHeader Header;
        Memory::Memcpy(&Header, InCode.Data(), sizeof(FMSLShaderHeader));

        if (Header.Magic == FMSLShaderHeader::ExpectedMagic && Header.Version == FMSLShaderHeader::ExpectedVersion)
        {
            ThreadGroupSizeX    = Header.ThreadGroupSizeX;
            ThreadGroupSizeY    = Header.ThreadGroupSizeY;
            ThreadGroupSizeZ    = Header.ThreadGroupSizeZ;
            ShaderConstantsSize = Header.ShaderConstantsSize;
        }
    }

    @autoreleasepool
    {
        // Shader bytecode is not null-terminated, so construct the source with its explicit length.
        const CHAR* CodeString = reinterpret_cast<const CHAR*>(Source.Data());
        const int32 CodeLength = Source.Size();
        
        const String SourceString(CodeString, CodeLength);
        
        NSString* Source = SourceString.GetNSString();
        CHECK(Source != nil);
        [Source retain];
        
        id<MTLDevice> Device = GetDevice()->GetMTLDevice();
        CHECK(Device != nil);
        
        NSError* Error = nil;
        Library = [Device newLibraryWithSource:Source options:nil error:&Error];
        if (!Library)
        {
            const String ErrorString([Error localizedDescription]);
            LOG_ERROR("Failed to compile shader. Error: %s", *ErrorString);
            return false;
        }
        
        FunctionName = [ResolveMetalFunctionName(Library) retain];
        if (!FunctionName)
        {
            LOG_ERROR("Compiled Library does not contain an entry-point");
            return false;
        }

        Function = [Library newFunctionWithName:FunctionName];
        if (!Function)
        {
            const String NameString(FunctionName);
            LOG_ERROR("Failed to retrieve function '%s' from Library", *NameString);
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

FMetalMeshShaderRHI::FMetalMeshShaderRHI(FMetalDevice* InDevice)
    : FRHIMeshShader()
    , FMetalShader(InDevice, EShaderVisibility::Mesh)
{
}

FMetalMeshShaderRHI::~FMetalMeshShaderRHI() = default;

FMetalAmplificationShaderRHI::FMetalAmplificationShaderRHI(FMetalDevice* InDevice)
    : FRHIAmplificationShader()
    , FMetalShader(InDevice, EShaderVisibility::Amplification)
{
}

FMetalAmplificationShaderRHI::~FMetalAmplificationShaderRHI() = default;

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

bool FMetalRayTracingShader::Initialize(const TArray<uint8>& InCode)
{
    if (!FMetalShader::Initialize(InCode))
    {
        return false;
    }

    Identifier = String(FunctionName);
    return true;
}

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

FMetalRayIntersectionShaderRHI::FMetalRayIntersectionShaderRHI(FMetalDevice* InDevice)
    : FRHIRayIntersectionShader()
    , FMetalRayTracingShader(InDevice)
{
}

FMetalRayIntersectionShaderRHI::~FMetalRayIntersectionShaderRHI() = default;

FMetalRayCallableShaderRHI::FMetalRayCallableShaderRHI(FMetalDevice* InDevice)
    : FRHIRayCallableShader()
    , FMetalRayTracingShader(InDevice)
{
}

FMetalRayCallableShaderRHI::~FMetalRayCallableShaderRHI() = default;

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

void* FMetalMeshShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(GetMTLFunction());
}

void* FMetalMeshShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FMetalShader*>(this);
}

void* FMetalAmplificationShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(GetMTLFunction());
}

void* FMetalAmplificationShaderRHI::GetRHIBaseInterface()
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

void* FMetalRayIntersectionShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(GetMTLFunction());
}

void* FMetalRayIntersectionShaderRHI::GetRHIBaseInterface()
{
    return static_cast<FMetalRayTracingShader*>(this);
}

void* FMetalRayCallableShaderRHI::GetRHINativeHandle()
{
    return reinterpret_cast<void*>(GetMTLFunction());
}

void* FMetalRayCallableShaderRHI::GetRHIBaseInterface()
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
