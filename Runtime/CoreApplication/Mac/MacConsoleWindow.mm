#if defined(PLATFORM_MACOS) && defined(__OBJC__)
#include "MacConsoleWindow.h"
#include "CocoaConsoleWindow.h"
#include "ScopedAutoreleasePool.h"

#include "CoreApplication/Platform/PlatformApplicationMisc.h"

CMacConsoleWindow::CMacConsoleWindow()
    : Window( nullptr )
{
    // TODO: This needs to happen on the mainthread, make sure that there is some mechanism for this 
    SCOPED_AUTORELEASE_POOL();
    
    // TODO: Control with console vars? 
    const CGFloat Width  = 640.0f;
    const CGFloat Height = 360.0f;
    
    Window = [[CCocoaConsoleWindow alloc] init: Width Height:Height];
    [Window setColor:EConsoleColor::White];
    
    Assert(Window != nullptr);

    // TODO: Pump events here
    PlatformApplicationMisc::PumpMessages( true );
}

CMacConsoleWindow::~CMacConsoleWindow()
{
    if ( Window )
    {
        // TODO: Ensure mainthread
        SCOPED_AUTORELEASE_POOL();
        
        PlatformApplicationMisc::PumpMessages( true );
        
        [Window release];
        Window = nullptr;
    }
}

void CMacConsoleWindow::Print(const CString& Message )
{
    // TODO: Quick hack for now
    if (![NSThread isMainThread])
    {
        return;
    }
    
    if (Window)
    {
        //TODO: Make sure this is the mainthread
        SCOPED_AUTORELEASE_POOL();

        NSString* String = [NSString stringWithUTF8String:Message.CStr()];

        [Window appendStringAndScroll:String];
        
        PlatformApplicationMisc::PumpMessages( true );
    }
}

void CMacConsoleWindow::PrintLine(const CString& Message )
{
    // TODO: Quick hack for now
    if (![NSThread isMainThread])
    {
        return;
    }
    
    if (Window)
    {
        //TODO: Make sure this is the mainthread
        SCOPED_AUTORELEASE_POOL();

        NSString* String      = [NSString stringWithUTF8String:Message.CStr()];
        NSString* FinalString = [String stringByAppendingString:@"\n"];
        
        [Window appendStringAndScroll:FinalString];
        
        PlatformApplicationMisc::PumpMessages( true );
    }
}

void CMacConsoleWindow::Clear()
{
    // TODO: Quick hack for now
    if (![NSThread isMainThread])
    {
        return;
    }
    
    if (Window)
    {
        [Window clearWindow];
    }
}

void CMacConsoleWindow::SetTitle(const CString& InTitle)
{
    // TODO: Quick hack for now
    if (![NSThread isMainThread])
    {
        return;
    }
    
    if (Window)
    {
        SCOPED_AUTORELEASE_POOL();
        
        NSString* Title = [NSString stringWithUTF8String:InTitle.CStr()];
        [Window setTitle:Title];
    }
}

void CMacConsoleWindow::SetColor(EConsoleColor Color)
{
    // TODO: Quick hack for now
    if (![NSThread isMainThread])
    {
        return;
    }
    
    if (Window)
    {
        [Window setColor:Color];
    }
}

#endif
