#pragma once

#if defined(PLATFORM_MACOS) 
#if defined(__OBJC__)

#include <Appkit/Appkit.h>
#include <MetalKit/MetalKit.h>

class CMacApplication;

@interface CCocoaContentView : MTKView<NSTextInputClient>
{
    CMacApplication* Application;
}

- (id) init:(CMacApplication*) InApplication;
@end

#else

class CCocoaContentView;

#endif
#endif
