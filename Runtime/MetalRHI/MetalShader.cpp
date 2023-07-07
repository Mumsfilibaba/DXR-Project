#include "MetalShader.h"

FMetalShader::FMetalShader(FMetalDeviceContext* InDevice, EShaderVisibility InVisibility, const TArray<uint8>& InCode)
    : FMetalObject(InDevice)
    , Library(nil)
    , FunctionName(nil)
    , Visbility(InVisibility)
    , Function(nil)
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
            LOG_ERROR("Failed to compile shader. Error: %s", ErrorString.GetCString());
            DEBUG_BREAK();
        }
        
        // Retrieve the entrypoint
        const auto Length = FMath::Max(SourceString.FindChar('\n') - 3, 0);
        NSString* EntryPoint = FString(SourceString.GetCString() + 3, Length).GetNSString();
        FunctionName = [EntryPoint retain];
        
        Function = [Library newFunctionWithName:EntryPoint];
        CHECK(Function != nil);
    }
}

FMetalShader::~FMetalShader()
{
    NSSafeRelease(Library);
    NSSafeRelease(FunctionName);
    NSSafeRelease(Function);
}
