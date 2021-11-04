#if defined(PLATFORM_MACOS) && defined(__OBJC__)
#include "CocoaConsoleWindow.h"
#include "ScopedAutoreleasePool.h"

@implementation CCocoaConsoleWindow

- (id) init:(CGFloat) Width Height:(CGFloat) Height
{
    SCOPED_AUTORELEASE_POOL();
    
    NSRect     ContentRect = NSMakeRect(0.0f, 0.0f, Width, Height);
    NSUInteger StyleMask   = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;
    
    self = [super initWithContentRect:ContentRect styleMask:StyleMask backing:NSBackingStoreBuffered defer:NO];
    if (self)
    {
        NSRect ContentFrame = [[self contentView] frame];
        ScrollView = [[NSScrollView alloc] initWithFrame:ContentFrame];
        [ScrollView setBorderType:NSNoBorder];
        [ScrollView setHasVerticalScroller:YES];
        [ScrollView setHasHorizontalScroller:NO];
        [ScrollView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
        
        TextView = [[NSTextView alloc] initWithFrame:ContentFrame];
        [TextView setEditable:NO];
        [TextView setMinSize:NSMakeSize(0.0f, Height)];
        [TextView setMaxSize:NSMakeSize(FLT_MAX, FLT_MAX)];
        [TextView setVerticallyResizable:YES];
        [TextView setHorizontallyResizable:NO];
        [TextView setAutoresizingMask:NSViewWidthSizable];
        
        NSTextContainer* Container = [TextView textContainer];
        [Container setContainerSize:NSMakeSize( Width, FLT_MAX )];
        [Container setWidthTracksTextView:YES];
        
        [ScrollView setDocumentView:TextView];
        
        [self setTitle:@"Output Console"];
        [self setContentView:ScrollView];
        [self setDelegate:self];
        [self setInitialFirstResponder:TextView];
        [self setOpaque:YES];
        
        [self makeKeyAndOrderFront:self];
    }
    
    return self;
}

- (void) dealloc
{
    SCOPED_AUTORELEASE_POOL();
    
    [TextView release];
    [ScrollView release];
    [ConsoleColor release];
    
    [super dealloc];
}

- (void) appendStringAndScroll:(NSString*) String
{
    SCOPED_AUTORELEASE_POOL();
    
    NSAttributedString* AttributedString = [[NSAttributedString alloc] initWithString:String attributes:ConsoleColor];
        
    NSTextStorage* Storage = [TextView textStorage];
    [Storage beginEditing];
    
    // Remove lines
    NSUInteger LineCount  = [self getLineCount];
    NSString*  TextString = [TextView string];
    if (LineCount >= 196)
    {
        NSUInteger LineIndex;
        NSUInteger NumberOfLines = 0;
        NSUInteger StringLength  = [TextString length];
        for (LineIndex = 0; LineIndex < StringLength; NumberOfLines++)
        {
            LineIndex = NSMaxRange([TextString lineRangeForRange:NSMakeRange(LineIndex, 0)]);
            if (NumberOfLines >= 1)
            {
                break;
            }
        }
        
        NSRange Range = NSMakeRange(0, LineIndex);
        [Storage deleteCharactersInRange:Range];
    }
    
    // Add the new String
    [Storage appendAttributedString:AttributedString];
    [Storage setFont:[NSFont fontWithName:@"Courier" size:12.0f]];
    
    [Storage endEditing];
    
    [TextView scrollToEndOfDocument:TextView];
    
    [AttributedString release];
}

- (void) clearWindow
{
    SCOPED_AUTORELEASE_POOL();
    
    [TextView setString:@""];
}

- (void) setColor:(EConsoleColor) Color
{
    SCOPED_AUTORELEASE_POOL();
    
    if (ConsoleColor)
    {
        [ConsoleColor release];
    }
    
    NSMutableArray* Colors     = [[NSMutableArray alloc] init];
    NSMutableArray* Attributes = [[NSMutableArray alloc] init];
    [Attributes addObject:NSForegroundColorAttributeName];
    [Attributes addObject:NSBackgroundColorAttributeName];

    // Add foreground Color
    if (Color == EConsoleColor::White)
    {
        [Colors addObject:[NSColor colorWithSRGBRed:1.0f green:1.0f blue:1.0f alpha:1.0f]];
    }
    else if (Color == EConsoleColor::Red)
    {
        [Colors addObject:[NSColor colorWithSRGBRed:1.0f green:0.0f blue:0.0f alpha:1.0f]];
    }
    else if (Color == EConsoleColor::Green)
    {
        [Colors addObject:[NSColor colorWithSRGBRed:0.0f green:1.0f blue:0.0f alpha:1.0f]];
    }
    else if (Color == EConsoleColor::Yellow)
    {
        [Colors addObject:[NSColor colorWithSRGBRed:1.0f green:1.0f blue:0.0f alpha:1.0f]];
    }
    
    // Add background Color
    [Colors addObject:[NSColor colorWithSRGBRed:0.1f green:0.1f blue:0.1f alpha:0.1f]];
    
    ConsoleColor = [[NSDictionary alloc] initWithObjects:Colors forKeys:Attributes];

    [Colors release];
    [Attributes release];
}

- (NSUInteger) getLineCount
{
    NSString* String = [TextView string];
    
    NSUInteger NumberOfLines = 0;
    NSUInteger StringLength  = [String length];
    for (NSUInteger LineIndex = 0; LineIndex < StringLength; NumberOfLines++)
    {
        LineIndex = NSMaxRange([String lineRangeForRange:NSMakeRange(LineIndex, 0)]);
    }
    
    return NumberOfLines;
}

- (BOOL) windowShouldClose:(NSWindow* ) Sender
{
    SCOPED_AUTORELEASE_POOL();
    
    [Sender release];
    return YES;
}

- (BOOL) acceptsFirstResponder
{
    return NO;
}

+ (NSString*) convertStringWithArgs:(const char*) Format Args:(va_list)Args
{
    NSString* TempFormat = [NSString stringWithUTF8String:Format];
    NSString* Result     = [[NSString alloc] initWithFormat:TempFormat arguments:Args];
    return Result;
}

@end

#endif
