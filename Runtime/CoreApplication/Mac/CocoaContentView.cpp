#include "CocoaContentView.h"
#include "MacApplication.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CCocoaWindowView

@implementation CCocoaWindowView

- (instancetype) init:(CMacApplication*) InApplication
{
    self = [super init];
    if (self)
    {
        Application = InApplication;
    }
    
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
    Application->DeferEvent(Event);
}

- (void) keyUp:(NSEvent*) Event
{
    Application->DeferEvent(Event);
}

- (void) mouseDown:(NSEvent*) Event
{
    Application->DeferEvent(Event);
}

- (void) mouseDragged:(NSEvent*) Event
{
    Application->DeferEvent(Event);
}

- (void) mouseUp:(NSEvent*) Event
{
    Application->DeferEvent(Event);
}

- (void) mouseMoved:(NSEvent*) Event
{
    Application->DeferEvent(Event);
}

- (void) rightMouseDown:(NSEvent*) Event
{
    Application->DeferEvent(Event);
}

- (void) rightMouseDragged:(NSEvent*) Event
{
    Application->DeferEvent(Event);
}

- (void) rightMouseUp:(NSEvent*) Event
{
    Application->DeferEvent(Event);
}

- (void) otherMouseDown:(NSEvent*) Event
{
    Application->DeferEvent(Event);
}

- (void) otherMouseDragged:(NSEvent*) Event
{
    Application->DeferEvent(Event);
}

- (void) otherMouseUp:(NSEvent*) Event
{
    Application->DeferEvent(Event);
}

- (void) scrollWheel:(NSEvent*) Event
{
    Application->DeferEvent(Event);
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
    
    Application->DeferEvent(Characters);
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
    Application->DeferEvent(Event);
}

- (void) mouseEntered:(NSEvent*) Event
{
    Application->DeferEvent(Event);
}

@end
