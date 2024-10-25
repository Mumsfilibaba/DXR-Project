#include "Core/Mac/Mac.h"
#include "Core/Misc/ConsoleManager.h"
#include "Core/Misc/OutputDeviceLogger.h"
#include "CoreApplication/Mac/CocoaWindow.h"
#include "CoreApplication/Mac/MacApplication.h"
#include "CoreApplication/Mac/MacWindow.h"

static TAutoConsoleVariable<bool> CVarIsRetinaAware(
    "MacOS.IsRetinaAware",
    "If set to true, the process uses the full Retina framebuffer for window surfaces; otherwise, it does not.",
    false,
    EConsoleVariableFlags::Default);

@implementation FCocoaWindow

- (instancetype)initWithContentRect:(NSRect)ContentRect styleMask:(NSWindowStyleMask)StyleMask backing:(NSBackingStoreType)BackingStoreType defer:(BOOL)Flag
{
    self = [super initWithContentRect:ContentRect styleMask:StyleMask backing:BackingStoreType defer:Flag];
    if (self)
    {
        CachedWidth  = ContentRect.size.width;
        CachedHeight = ContentRect.size.height;

        // Disable window snapshot restoration to prevent macOS from automatically restoring the
        // window's state upon relaunch. This ensures that our custom window initialization is not
        // overridden by restored state, avoids displaying outdated or invalid content.
        [super disableSnapshotRestoration];
    }
    
    return self;
}

// Allow the window to become the key window (i.e., receive keyboard input)
- (BOOL)canBecomeKeyWindow
{
    return YES;
}

// Allow the window to become the main window. The main window is the principal
// window of an application that is the focus for user actions.
- (BOOL)canBecomeMainWindow
{
    return YES;
}

- (BOOL)acceptsMouseMovedEvents
{
    return YES;
}

// Allow the window to accept first responder status. The first responder is the first
// object in the responder chain that can respond to events like key presses or mouse
// clicks. By accepting first responder status, the window can handle events before they
// are passed to other objects.
- (BOOL)acceptsFirstResponder
{
    return YES;
}

- (void)windowWillClose:(NSNotification*)Notification
{
    @autoreleasepool
    {
        // Remove the window's delegate to prevent further messages
        [self setDelegate:nil];

        if (MacApplication)
        {
            TSharedRef<FMacWindow> Window = MacApplication->GetWindowFromNSWindow(self);
            MacApplication->CloseWindow(Window);
        }
    }
}

- (void)windowDidResize:(NSNotification*)Notification
{
    const NSRect ContentRect = [self contentRectForFrameRect:self.frame];
    const CGFloat Width  = ContentRect.size.width;
    const CGFloat Height = ContentRect.size.height;

    // Prevent sending multiple events for the same size
    if (Width != CachedWidth || Height != CachedHeight)
    {
        if (MacApplication)
        {
            MacApplication->DeferEvent(Notification);
        }

        CachedWidth  = Width;
        CachedHeight = Height;
    }
}

- (void)windowDidMove:(NSNotification*)Notification
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Notification);
    }
}

- (void)windowDidMiniaturize:(NSNotification*)Notification
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Notification);
    }
}

- (void)windowDidDeminiaturize:(NSNotification*)Notification
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Notification);
    }
}

- (void)windowDidEnterFullScreen:(NSNotification*)Notification
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Notification);
    }
}

- (void)windowDidExitFullScreen:(NSNotification*)Notification
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Notification);
    }
}

// This method is called when the window becomes the main window of the application.
// A window becomes the main window in response to specific user actions:
//  - Clicking on the Window: When the user clicks on the window's title bar or content
//    area, bringing it to the foreground.
//  - Application Activation: When the user switches back to the application from another
//    app (e.g., using Command+Tab or clicking the app icon in the Dock), the frontmost
//    window becomes the main window.
//  - Programmatic Activation: When the application programmatically makes the window
//    the main window using `[NSWindow makeMainWindow]`.
// The main window is the principal focus for user actions, such as menu commands and non-keyboard events.
// In this method, we check if the application is visible and, if so, bring the window to the front.

- (void)windowDidBecomeMain:(NSNotification*)Notification
{
    @autoreleasepool
    {
        if ([NSApp isHidden] == NO)
        {
            // Order the window to the front of its level to ensure it is visible to the user
            [self orderFront:nil];
        }

        if (MacApplication)
        {
            MacApplication->DeferEvent(Notification);
        }
    }
}

// This method is called when the window loses its status as the main window of the application.
// A window resigns main window status in response to specific user actions:
//  - Activating Another Window: When the user clicks on a different window within the same application, making it the new main window.
//  - Switching to Another Application: When the user switches focus to a different application, the current window resigns main status.
//  - Programmatic Changes: When the application programmatically changes the main window or resigns it.
- (void)windowDidResignMain:(NSNotification*)Notification
{
    @autoreleasepool
    {
        // Ensure the window remains movable by its title bar. When 'movable' is set to YES, the
        // user can move the window by clicking and dragging its title bar. This is standard behavior
        // for windows, allowing users to reposition them on the screen as needed.
        [self setMovable:YES];
        
        // Prevent the window from being moved by clicking and dragging its background (the content area).
        // When 'movableByWindowBackground' is set to NO, the user cannot move the window by clicking and
        // dragging anywhere within the window's content area. Disabling this behavior when the window is
        // not the main window helps prevent accidental window movements. It ensures that interactions within
        // the window's content (such as clicking buttons or selecting text) do not inadvertently move the window.
        [self setMovableByWindowBackground:NO];
    }
}

@end

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

// Allow the view to become the key view and receive keyboard events
- (BOOL)canBecomeKeyView
{
    return YES;
}

// Allow the view to accept first responder status. By accepting first responder status,
// this view can handle events like key presses before they are passed to other objects.
- (BOOL)acceptsFirstResponder
{
    return YES;
}

// Indicate that the view does not have any marked text (used for input methods)
- (BOOL)hasMarkedText
{
    return NO;
}

// Indicate that the view wants to use a layer for drawing. This method is part of the NSView
// drawing system in macOS. By overriding 'wantsUpdateLayer' and returning YES, we inform
// AppKit that our view prefers to use a layer-backed drawing model. For example use a CAMetalView
// to use metal in order to perform rendering.
- (BOOL)wantsUpdateLayer
{
    return YES;
}

// Accept the first mouse event even if the view is not the key view. This method is called by
// the system to determine whether the view should receive a mouse-down event (or other mouse
// events) even if the window is not the key window or the view is not the first responder.
// This allows the view to handle the mouse event immediately without requiring the window to
// become active first. This enhances the user experience by making the application more responsive,
// eliminating the need for the user to click twice (once to activate the window, and once to
// perform the action). This is particularly important in applications where immediate response
// to user input is crucial.
- (BOOL)acceptsFirstMouse:(NSEvent*)Event
{
    return YES;
}

// Provide an empty array of valid attributes for marked text (input methods). This method is
// part of the NSTextInputClient protocol, which is used for handling complex text input, such
// as input from languages like Chinese, Japanese, and Korean. The method returns an array of
// attribute names (as NSStrings) that the view supports for marked text. Marked text is text
// that is being composed but not yet finalized (committed) by the user. By returning an empty
// array, we indicate that we do not support any special text attributes for marked text. This
// means that the input system will not apply any attributes like underlines, colors, or fonts
// to the marked text. Our application does not need to display marked text differently. This
// simplifies the handling of input methods and ensures compatibility without extra complexity.
- (NSArray*)validAttributesForMarkedText
{
    return @[];
}

// Prepare the view when it's about to move to a new window. This method is called whenever the
// view is about to be added to a window or moved to a different window. It's an opportunity to
// perform setup or cleanup related to the window change. In this method, we set up an
// NSTrackingArea to monitor mouse movements within the view. An NSTrackingArea allows the view
// to receive mouse-entered and mouse-exited events. By creating a tracking area that covers the
// entire bounds of the view, we ensure that we are notified whenever the mouse enters or leaves
// the view's area. The options NSTrackingMouseEnteredAndExited and NSTrackingActiveAlways specify
// that we want to receive these events regardless of the application's active status.
- (void)viewWillMoveToWindow:(NSWindow*)window
{
    SCOPED_AUTORELEASE_POOL();

    for (NSTrackingArea* Area in self.trackingAreas)
    {
        [self removeTrackingArea:Area];
    }

    // NSTrackingAreaOptions
    //  - NSTrackingMouseEnteredAndExited   // Receive mouseEntered: and mouseExited: events
    //  - NSTrackingActiveAlways            // Track events even when the application is not active
    const NSTrackingAreaOptions Options = NSTrackingMouseEnteredAndExited | NSTrackingActiveAlways;
    
    NSTrackingArea* TrackingArea = [[NSTrackingArea alloc] initWithRect:self.bounds options:Options owner:self userInfo:nil];
    [self addTrackingArea:TrackingArea];
}

- (void)keyDown:(NSEvent*)Event
{
    SCOPED_AUTORELEASE_POOL();

    // Interpret key events for proper text input handling
    [self interpretKeyEvents:@[Event]];

    if (MacApplication)
    {
        MacApplication->DeferEvent(Event);
    }
}

- (void)keyUp:(NSEvent*)Event
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Event);
    }
}

// Handle modifier key changes (e.g., Shift, Control)
- (void)flagsChanged:(NSEvent*)Event
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Event);
    }
}

- (void)mouseDown:(NSEvent*)Event
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Event);
    }
}

- (void)mouseDragged:(NSEvent*)Event
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Event);
    }
}

- (void)mouseUp:(NSEvent*)Event
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Event);
    }
}

- (void)mouseMoved:(NSEvent*)Event
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Event);
    }
}

- (void)rightMouseDown:(NSEvent*)Event
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Event);
    }
}

- (void)rightMouseDragged:(NSEvent*)Event
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Event);
    }
}

- (void)rightMouseUp:(NSEvent*)Event
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Event);
    }
}

// Handle other mouse-button events (e.g., middle mouse button)
- (void)otherMouseDown:(NSEvent*)Event
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Event);
    }
}

- (void)otherMouseDragged:(NSEvent*)Event
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Event);
    }
}

- (void)otherMouseUp:(NSEvent*)Event
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Event);
    }
}

- (void)scrollWheel:(NSEvent*)Event
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Event);
    }
}

// Handle text insertion (part of NSTextInputClient protocol)
- (void)insertText:(id)FString replacementRange:(NSRange)ReplacementRange
{
    SCOPED_AUTORELEASE_POOL();

    // Extract the string content from the input, which may be attributed
    NSString* Characters = nil;
    if ([FString isKindOfClass:[NSAttributedString class]])
    {
        Characters = [(NSAttributedString*)FString string];
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

// Implement required methods for NSTextInputClient protocol (empty implementations)

// This method is called by the input system when it wants the client to execute a command.
// The selector parameter specifies the command to perform (e.g., deleteBackward:, moveLeft:).
// Since we handle keyboard input differently or do not support these text commands,
// we leave this method empty to indicate that no action is taken.
- (void)doCommandBySelector:(SEL)selector
{
    // Command handling is not required; method intentionally left empty
}

// This method returns an attributed substring specified by the given range.
// The input system uses this to retrieve the text content for various purposes,
// such as spell-checking or candidate window positioning.
// Since our view does not manage text content (we have no text storage),
// we return nil to indicate that no text is available.
- (nullable NSAttributedString*)attributedSubstringForProposedRange:(NSRange)Range actualRange:(nullable NSRangePointer)ActualRange
{
    // No attributed substring available; return nil to indicate absence of text
    return nil;
}

// This method maps a point in the view's coordinate system to a character index in the text content.
// The input system might use this to determine the insertion point based on mouse clicks.
// Since our view does not contain text, we return 0 as a default value.
- (NSUInteger)characterIndexForPoint:(NSPoint)Point
{
    // No text content; return 0 as a default character index
    return 0;
}

// This method provides the screen coordinates for a given character range.
// It's used by the input system to position auxiliary windows (like the candidate list in an input method editor).
// Since we have no text, we return a zero-sized rectangle at the view's origin.
- (NSRect)firstRectForCharacterRange:(NSRange)Range actualRange:(nullable NSRangePointer)ActualRange
{
    // Return a zero-sized rectangle at the origin of the view as we have no text
    const NSRect Frame = self.frame;
    return NSMakeRect(Frame.origin.x, Frame.origin.y, 0.0f, 0.0f);
}

// This method returns the range of currently marked text (text that is being composed but not yet confirmed).
// Marked text is used in complex text input (like composing characters in East Asian languages).
// Since we do not support marked text, we return NSNotFound to indicate its absence.
- (NSRange)markedRange
{
    return NSMakeRange(NSNotFound, 0);
}

// This method returns the range of the current selection within the text content.
// The input system uses this to understand where the selection is for editing purposes.
// Since we have no selectable text, we return NSNotFound to indicate that there is no selection.
- (NSRange)selectedRange
{
    return NSMakeRange(NSNotFound, 0);
}

// This method is called by the input system to set marked text during composition.
// Marked text represents partially composed input, such as characters not yet confirmed by the user.
// Since we do not support text input in this manner, we do not implement any functionality here.
- (void)setMarkedText:(nonnull id)FString selectedRange:(NSRange)SelectedRange replacementRange:(NSRange)ReplacementRange
{
}

// This method is called to unmark the currently marked text, finalizing the composition.
// Since we do not handle marked text, we leave this method empty.
- (void)unmarkText
{
}

// Handle changes to the view's backing properties (e.g., when moving between Retina and non-Retina displays)
- (void)viewDidChangeBackingProperties
{
    const NSRect ContentRect     = self.frame;
    const NSRect FrameBufferRect = [self convertRectToBacking:ContentRect];

    // Calculate the new scale factors based on the framebuffer size
    const CGFloat NewScaleX = FrameBufferRect.size.width / ContentRect.size.width;
    const CGFloat NewScaleY = FrameBufferRect.size.height / ContentRect.size.height;

    // Check if the scale factors have changed
    if (NewScaleX != ScaleX || NewScaleY != ScaleY)
    {
        // If the application is Retina-aware and the view has a layer, update the layer's content scale
        if (CVarIsRetinaAware.GetValue() && self.layer)
        {
            if (self.window)
            {
                CGFloat BackingScaleFactor = self.window.backingScaleFactor;
                self.layer.contentsScale = BackingScaleFactor;
            }
            else
            {
                // Log an error if the window is not valid and trigger a debug breakpoint
                LOG_ERROR("Window is not valid");
                DEBUG_BREAK();
            }
        }
    }

    // Update the framebuffer dimensions if they have changed
    if (FrameBufferRect.size.width != FrameBufferWidth || FrameBufferRect.size.height != FrameBufferHeight)
    {
        FrameBufferWidth  = FrameBufferRect.size.width;
        FrameBufferHeight = FrameBufferRect.size.height;

        // Optional: Log the new framebuffer size for debugging purposes
        #if 0
            LOG_INFO("viewDidChangeBackingProperties FrameBufferSize: w=%.4f, h=%.4f", FrameBufferWidth, FrameBufferHeight);
        #endif
    }
}

// Handle mouse-exited events
- (void)mouseExited:(NSEvent*)Event
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Event);
    }
}

// Handle mouse-entered events
- (void)mouseEntered:(NSEvent*)Event
{
    if (MacApplication)
    {
        MacApplication->DeferEvent(Event);
    }
}

@end
