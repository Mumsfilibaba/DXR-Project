#include "MetalShader.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// FMetalShader

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
        
        FString SourceString(reinterpret_cast<const char*>(InCode.Data()), InCode.Size());
        
        NSString* Source = SourceString.GetNSString();
        Check(Source != nil);
        
        Library = [GetDeviceContext()->GetMTLDevice() newLibraryWithSource:Source options:nil error:&Error];
        Check(Library != nil);
        
        // Retrieve the entrypoint
        const auto Length = NMath::Max(SourceString.Find('\n') - 3, 0);
        NSString* EntryPoint = FString(SourceString.CStr() + 3, Length).GetNSString();
        FunctionName = [EntryPoint retain];
        
        Function = [Library newFunctionWithName:EntryPoint];
        Check(Function != nil);
    }
}

FMetalShader::~FMetalShader()
{
    NSSafeRelease(Library);
    NSSafeRelease(FunctionName);
    NSSafeRelease(Function);
}
