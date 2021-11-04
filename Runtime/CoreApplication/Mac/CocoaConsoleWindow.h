#pragma once 

#if defined(PLATFORM_MACOS) && defined(__OBJC__)
#include "Core/Application/Generic/GenericOutputConsole.h"

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