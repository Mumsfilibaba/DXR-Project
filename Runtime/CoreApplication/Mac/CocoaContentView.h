#pragma once

#if PLATFORM_MACOS
#include <Appkit/Appkit.h>
#include <MetalKit/MetalKit.h>

class CMacApplication;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Content view with a backing Metal-view attached to it

@interface CCocoaContentView : MTKView<NSTextInputClient>
{
    CMacApplication* Application;
}

- (id)init : (CMacApplication*)InApplication;

@end

#endif
