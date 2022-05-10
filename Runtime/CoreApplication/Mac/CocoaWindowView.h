#pragma once
#include <Appkit/Appkit.h>
#include <MetalKit/MetalKit.h>

class CMacApplication;

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CCocoaWindowView

@interface CCocoaWindowView : NSView<NSTextInputClient>
{
    CMacApplication* Application;
}

- (instancetype)init : (CMacApplication*)InApplication NS_DESIGNATED_INITIALIZER;

@end
