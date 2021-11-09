#pragma once 

#if PLATFORM_MACOS && defined(__OBJC__)
#include "CoreApplication/Interface/PlatformConsoleWindow.h"

#include <stdarg.h>

#include <AppKit/AppKit.h>

@interface CCocoaConsoleWindow : NSWindow<NSWindowDelegate>
{
    NSTextView* TextView;
    NSScrollView* ScrollView;
    NSDictionary* ConsoleColor;
}

// Instance
- (id)init:(CGFloat)Width Height : (CGFloat)Height;

-(void)appendStringAndScroll:(NSString*)String;
-(void)clearWindow;

-(void)setColor:(EConsoleColor)Color;

-(NSUInteger)getLineCount;

// Static
+(NSString*)convertStringWithArgs:(const char*)Format Args : (va_list)Args;

@end

#else

class CCocoaConsoleWindow;

#endif
