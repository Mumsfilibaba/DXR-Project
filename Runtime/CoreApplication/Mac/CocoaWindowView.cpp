#include "CocoaWindowView.h"
#include "MacApplication.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceLogger.h"

static TAutoConsoleVariable<bool> CVarIsRetinaAware(
    "MacOS.IsRetinaAware",
    "If set to true the process is set to be using the full retina framebuffer for window surfaces, therwise not",
    false,
    EConsoleVariableFlags::Default);

@implementation FCocoaWindowView

- (instancetype)initWithFrame:(NSRect)Frame
{
    self = [super initWithFrame:Frame];
    if (self)
    {
        ScaleX            = 0.0f;
        ScaleY            = 0.0f;
        FrameBufferWidth  = 0.0f;
        FrameBufferHeight = 0.0f;
        return self;
    }
    
    return nil;
}

- (BOOL) canBecomeKeyView
{
    return YES;
}

- (BOOL) acceptsFirstResponder
{
    return YES;
}

- (BOOL) hasMarkedText
{
    return NO;
}

- (BOOL) wantsUpdateLayer
{
    return YES;
}

- (BOOL) acceptsFirstMouse:(NSEvent*) Event
{
    return YES;
}

- (NSArray*) validAttributesForMarkedText
{
    return @[];
}

- (void) viewWillMoveToWindow:(NSWindow*) window
{
    SCOPED_AUTORELEASE_POOL();
    
    NSTrackingArea* trackingArea = [[NSTrackingArea alloc] initWithRect:self.bounds options: (NSTrackingMouseEnteredAndExited | NSTrackingActiveAlways) owner:self userInfo:nil];
    [self addTrackingArea:trackingArea];
}

- (void) keyDown:(NSEvent*) Event
{
    SCOPED_AUTORELEASE_POOL();
    
    // Interpret key Event and make sure we get a KeyTyped Event
    [self interpretKeyEvents:[NSArray arrayWithObject: Event]];
    
    if (MacApplication)
    {
        MacApplication->DeferEvent(Event);
    }
}

- (void) keyUp:(NSEvent*) Event
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Event);
    }
}

- (void) mouseDown:(NSEvent*) Event
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Event);
    }
}

- (void) mouseDragged:(NSEvent*) Event
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Event);
    }
}

- (void) mouseUp:(NSEvent*) Event
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Event);
    }
}

- (void) mouseMoved:(NSEvent*) Event
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Event);
    }
}

- (void) rightMouseDown:(NSEvent*) Event
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Event);
    }
}

- (void) rightMouseDragged:(NSEvent*) Event
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Event);
    }
}

- (void) rightMouseUp:(NSEvent*) Event
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Event);
    }
}

- (void) otherMouseDown:(NSEvent*) Event
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Event);
    }
}

- (void) otherMouseDragged:(NSEvent*) Event
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Event);
    }
}

- (void) otherMouseUp:(NSEvent*) Event
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Event);
    }
}

- (void) scrollWheel:(NSEvent*) Event
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Event);
    }
}

- (void) insertText:(id) FString replacementRange:(NSRange) ReplacementRange
{
    SCOPED_AUTORELEASE_POOL();
    
    // Get characters
    NSString* Characters = nullptr;
    if ([FString isKindOfClass:[NSAttributedString class]])
    {
        NSAttributedString* AttributedString = (NSAttributedString*)FString;
        Characters = AttributedString.string;
    }
    else
    {
        Characters = (NSString*)FString;
    }
    
    if (MacApplication)
    {
        MacApplication->DeferEvent(Characters);
    }
}

/* Necessary methods for NSTextInputClient */
- (void) doCommandBySelector:(SEL)selector
{
}

- (nullable NSAttributedString*) attributedSubstringForProposedRange:(NSRange)Range actualRange:(nullable NSRangePointer)ActualRange
{
    return nil;
}

- (NSUInteger) characterIndexForPoint:(NSPoint)Point
{
    return 0;
}

- (NSRect) firstRectForCharacterRange:(NSRange)Range actualRange:(nullable NSRangePointer)ActualRange
{
    const NSRect Frame = self.frame;
    return NSMakeRect(Frame.origin.x, Frame.origin.y, 0.0f, 0.0f);
}

- (NSRange)markedRange
{
    return NSMakeRange(NSNotFound, 0);
}

- (NSRange) selectedRange
{
    return NSMakeRange(NSNotFound, 0);
}

- (void)setMarkedText:(nonnull id)FString selectedRange:(NSRange)SelectedRange replacementRange:(NSRange)ReplacementRange
{
}

- (void)unmarkText
{
}

- (void)viewDidChangeBackingProperties
{
    const NSRect ContentRect     = self.frame;
    const NSRect FrameBufferRect = [self convertRectToBacking:ContentRect];
    
    // Check the scale of the view
    const CGFloat NewScaleX = FrameBufferRect.size.width / ContentRect.size.width;
    const CGFloat NewScaleY = FrameBufferRect.size.height / ContentRect.size.height;
    if (NewScaleX != ScaleX || NewScaleY != ScaleY)
    {
        if (CVarIsRetinaAware.GetValue() && self.layer)
        {
            if (self.window)
            {
                CGFloat BackingScaleFactor = self.window.backingScaleFactor;
                self.layer.contentsScale = BackingScaleFactor;
            }
            else
            {
                LOG_ERROR("Window is not valid");
                DEBUG_BREAK();
            }
        }
    }

    if (FrameBufferRect.size.width != FrameBufferWidth || FrameBufferRect.size.height != FrameBufferHeight)
    {
        FrameBufferWidth  = FrameBufferRect.size.width;
        FrameBufferHeight = FrameBufferRect.size.height;
        LOG_INFO("viewDidChangeBackingProperties FrameBufferSize: w=%.4f, h=%.4f", FrameBufferWidth, FrameBufferHeight);
    }
}

- (void) mouseExited:(NSEvent*) Event
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Event);
    }
}

- (void) mouseEntered:(NSEvent*) Event
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Event);
    }
}

@end
