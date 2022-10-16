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
        NSError* Error = nil;
        
        FString SourceString(reinterpret_cast<const CHAR*>(InCode.GetData()), InCode.GetSize());
        
        NSString* Source = SourceString.GetNSString();
        CHECK(Source != nil);
        
        Library = [GetDeviceContext()->GetMTLDevice() newLibraryWithSource:Source options:nil error:&Error];
        CHECK(Library != nil);
        
        // Retrieve the entrypoint
        const auto Length = NMath::Max(SourceString.FindChar('\n') - 3, 0);
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
