#include "MetalShader.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMetalShader

CMetalShader::CMetalShader(CMetalDeviceContext* InDevice, const TArray<uint8>& InCode)
    : CMetalObject(InDevice)
    , Library(nil)
    , Function(nil)
{
    @autoreleasepool
    {
        NSError* Error = nil;
        
        String SourceString(reinterpret_cast<const char*>(InCode.Data()), InCode.Size());
        
        NSString* Source = SourceString.GetNSString();
        Check(Source != nil);
        
        Library = [GetDeviceContext()->GetMTLDevice() newLibraryWithSource:Source options:nil error:&Error];
        Check(Library != nil);
        
        // Retrieve the entrypoint
        const auto Length = NMath::Max(SourceString.Find('\n') - 3, 0);
        NSString* EntryPoint = String(SourceString.CStr() + 3, Length).GetNSString();
        
        Function = [Library newFunctionWithName:EntryPoint];
        Check(Function != nil);
    }
}

CMetalShader::~CMetalShader()
{
    NSSafeRelease(Library);
    NSSafeRelease(Function);
}
