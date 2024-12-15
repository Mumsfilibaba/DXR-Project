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

- (void)windowWillClose:(NSNotification*)Notification
{
    @autoreleasepool
    {
        // Remove the window's delegate to prevent further messages
        [self setDelegate:nil];
    }

    if (GMacApplication)
    {
        TSharedRef<FMacWindow> Window = GMacApplication->FindWindowFromNSWindow(self);
        GMacApplication->CloseWindow(Window);
    }
}

- (NSSize)windowWillResize:(NSWindow*)Sender toSize:(NSSize)FrameSize
{
    if (GMacApplication)
    {
        TSharedRef<FMacWindow> Window = GMacApplication->FindWindowFromNSWindow(self);
        GMacApplication->OnWindowWillResize(Window);
    }
    
    return FrameSize;
}

- (void)windowDidResize:(NSNotification*)Notification
{
    if (GMacApplication)
    {
        TSharedRef<FMacWindow> Window = GMacApplication->FindWindowFromNSWindow(self);
        GMacApplication->DeferEvent(Notification);
    }
}

- (void)windowDidMove:(NSNotification*)Notification
{
    if (GMacApplication)
    {
        GMacApplication->DeferEvent(Notification);
    }
}

- (void)windowDidMiniaturize:(NSNotification*)Notification
{
    if (GMacApplication)
    {
        GMacApplication->DeferEvent(Notification);
    }
}

- (void)windowDidDeminiaturize:(NSNotification*)Notification
{
    if (GMacApplication)
    {
        GMacApplication->DeferEvent(Notification);
    }
}

- (void)windowDidEnterFullScreen:(NSNotification*)Notification
{
    if (GMacApplication)
    {
        GMacApplication->DeferEvent(Notification);
    }
}

- (void)windowDidExitFullScreen:(NSNotification*)Notification
{
    if (GMacApplication)
    {
        GMacApplication->DeferEvent(Notification);
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

        if (GMacApplication)
        {
            GMacApplication->DeferEvent(Notification);
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
        
        if (GMacApplication)
        {
            GMacApplication->DeferEvent(Notification);
        }
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

- (BOOL)preservesContentDuringLiveResize
{
    // Return NO to improve performance during live resizing
    return YES;
}

// Indicate that the view does not have any marked text (used for input methods)
- (BOOL)hasMarkedText
{
    return NO;
}

// Accept the first mouse event even if the view is not the key view. This method is called by
// the system to determine whether the view should receive a mouse-down event (or other mouse
// events) even if the window is not the key window or the view is not the first responder.
// This allows the view to handle the mouse event immediately without requiring the window to
// become active first. This enhances the user experience by making the application more responsive,
// eliminating the need for the user to click twice (once to activate the window, and once to
// perform the action).
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
// to the marked text. This simplifies the handling of input methods and ensures compatibility
// without extra complexity.
- (NSArray*)validAttributesForMarkedText
{
    return @[];
}

- (void)keyDown:(NSEvent*)Event
{
    // Intentionally left empty for now
}

- (void)keyUp:(NSEvent*)Event
{
    // Intentionally left empty for now
}

- (void)mouseDown:(NSEvent*)Event
{
    // Intentionally left empty for now
}

- (void)mouseDragged:(NSEvent*)Event
{
    // Intentionally left empty for now
}

- (void)mouseUp:(NSEvent*)Event
{
    // Intentionally left empty for now
}

- (void)rightMouseDown:(NSEvent*)Event
{
    // Intentionally left empty for now
}

- (void)rightMouseDragged:(NSEvent*)Event
{
    // Intentionally left empty for now
}

- (void)rightMouseUp:(NSEvent*)Event
{
    // Intentionally left empty for now
}

- (void)otherMouseDown:(NSEvent*)Event
{
    // Intentionally left empty for now
}

- (void)otherMouseDragged:(NSEvent*)Event
{
    // Intentionally left empty for now
}

- (void)otherMouseUp:(NSEvent*)Event
{
    // Intentionally left empty for now
}

- (void)scrollWheel:(NSEvent*)Event
{
    // Intentionally left empty for now
}

@end
