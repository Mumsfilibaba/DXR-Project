#if PLATFORM_MACOS
#include "CocoaContentView.h"
#include "MacApplication.h"
#include "ScopedAutoreleasePool.h"

@implementation CCocoaContentView

- (id) init:(CMacApplication*) InApplication
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

- (BOOL) acceptsFirstMouse:(NSEvent* ) Event
{
    return YES;
}

- (NSArray*) validAttributesForMarkedText
{
    return [NSArray array];
}

- (void) viewWillMoveToWindow:(NSWindow*) window
{
    NSTrackingArea* trackingArea = [[NSTrackingArea alloc] initWithRect:[self bounds] options: (NSTrackingMouseEnteredAndExited | NSTrackingActiveAlways) owner:self userInfo:nil];
    [self addTrackingArea:trackingArea];
}

- (void) keyDown:(NSEvent*) Event
{
    // Interpret key Event and make sure we get a KeyTyped Event
    [self interpretKeyEvents:[NSArray arrayWithObject: Event]];
    Application->HandleEvent(Event);
}

- (void) keyUp:(NSEvent*) Event
{
    Application->HandleEvent(Event);
}

- (void) mouseDown:(NSEvent*) Event
{
    Application->HandleEvent(Event);
}

- (void) mouseDragged:(NSEvent*) Event
{
    Application->HandleEvent(Event);
}

- (void) mouseUp:(NSEvent*) Event
{
    Application->HandleEvent(Event);
}

- (void) mouseMoved:(NSEvent*) Event
{
    Application->HandleEvent(Event);
}

- (void) rightMouseDown:(NSEvent*) Event
{
    Application->HandleEvent(Event);
}

- (void) rightMouseDragged:(NSEvent*) Event
{
    Application->HandleEvent(Event);
}

- (void) rightMouseUp:(NSEvent*) Event
{
    Application->HandleEvent(Event);
}

- (void) otherMouseDown:(NSEvent*) Event
{
    Application->HandleEvent(Event);
}

- (void) otherMouseDragged:(NSEvent*) Event
{
    Application->HandleEvent(Event);
}

- (void) otherMouseUp:(NSEvent*) Event
{
    Application->HandleEvent(Event);
}

- (void) scrollWheel:(NSEvent*) Event
{
    Application->HandleEvent(Event);
}

- (void) insertText:(id) String replacementRange:(NSRange) ReplacementRange
{
    SCOPED_AUTORELEASE_POOL();
    
    // Get characters
    NSString* Characters = nullptr;
    if ([String isKindOfClass:[NSAttributedString class]])
    {
        NSAttributedString* AttributedString = (NSAttributedString*)String;
        Characters = [AttributedString string];
    }
    else
    {
        Characters = (NSString*)String;
    }
    
    Application->HandleKeyTypedEvent( Characters );
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
    const NSRect Frame = [self frame];
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
    CGFloat BackingScaleFactor = [[self window] backingScaleFactor];
    [[self layer] setContentsScale:BackingScaleFactor];
}

- (void) mouseExited:(NSEvent* ) Event
{
    Application->HandleEvent(Event);
}

- (void) mouseEntered:(NSEvent* )Event
{
    Application->HandleEvent(Event);
}

@end

#endif