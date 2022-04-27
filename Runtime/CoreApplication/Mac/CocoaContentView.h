#pragma once
#include <Appkit/Appkit.h>
#include <MetalKit/MetalKit.h>

class CMacApplication;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CCocoaContentView

@interface CCocoaContentView : MTKView<NSTextInputClient>
{
    CMacApplication* Application;
}

- (id)init : (CMacApplication*)InApplication;

@end
