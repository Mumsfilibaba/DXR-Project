#pragma once

#if PLATFORM_MACOS
#include <Appkit/Appkit.h>
#include <MetalKit/MetalKit.h>

class CMacApplication;

@interface CCocoaContentView : MTKView<NSTextInputClient>
{
    CMacApplication* Application;
}

- (id)init : (CMacApplication*)InApplication;

@end

#endif
