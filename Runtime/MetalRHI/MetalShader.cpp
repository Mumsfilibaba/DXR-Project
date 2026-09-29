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

FMetalShader::FMetalShader(FMetalDevice* InDevice)
    : FMetalDeviceChild(InDevice)
    , Library(nil)
    , FunctionName(nil)
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

FMetalRayTracingShader::FMetalRayTracingShader(FMetalDevice* InDevice)
    : FMetalShader(InDevice)
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
