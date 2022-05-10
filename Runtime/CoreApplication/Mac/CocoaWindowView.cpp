#include "CocoaWindowView.h"
#include "MacApplication.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CCocoaWindowView

@implementation CCocoaWindowView

- (instancetype)initWithFrame:(NSRect)Frame
{
    self = [super initWithFrame:Frame];
    return self;
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
    NSTrackingArea* trackingArea = [[NSTrackingArea alloc] initWithRect:self.bounds options: (NSTrackingMouseEnteredAndExited | NSTrackingActiveAlways) owner:self userInfo:nil];
    [self addTrackingArea:trackingArea];
}

- (void) keyDown:(NSEvent*) Event
{
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

- (void) insertText:(id) String replacementRange:(NSRange) ReplacementRange
{
    SCOPED_AUTORELEASE_POOL();
    
    // Get characters
    NSString* Characters = nullptr;
    if ([String isKindOfClass:[NSAttributedString class]])
    {
        NSAttributedString* AttributedString = (NSAttributedString*)String;
        Characters = AttributedString.string;
    }
    else
    {
        Characters = (NSString*)String;
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

- (void)setMarkedText:(nonnull id)String selectedRange:(NSRange)SelectedRange replacementRange:(NSRange)ReplacementRange
{
}

- (void)unmarkText
{
}

- (void) viewDidChangeBackingProperties
{
    CGFloat BackingScaleFactor = self.window.backingScaleFactor;
    self.layer.contentsScale = BackingScaleFactor;
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
