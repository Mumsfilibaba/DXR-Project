#include "MetalShader.h"

FMetalShader::FMetalShader(FMetalDeviceContext* InDevice, EShaderVisibility InVisibility)
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
        
        id<MTLDevice> Device = GetDeviceContext()->GetMTLDevice();
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
