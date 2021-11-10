#if PLATFORM_MACOS && defined(__OBJC__)
#include "MacConsoleWindow.h"
#include "CocoaConsoleWindow.h"
#include "ScopedAutoreleasePool.h"

#include "Core/Threading/Mac/MacRunLoop.h"

#include "CoreApplication/Platform/PlatformApplicationMisc.h"

CMacConsoleWindow::CMacConsoleWindow()
    : Window( nullptr )
{
    // Will probably never be initialized on another thread, but this ensures that this is the case
    MakeMainThreadCall(^
    {
        SCOPED_AUTORELEASE_POOL();
        
        // TODO: Control with console vars? 
        const CGFloat Width  = 640.0f;
        const CGFloat Height = 360.0f;
        
        Window = [[CCocoaConsoleWindow alloc] init: Width Height:Height];
        [Window setColor:EConsoleColor::White];
        
        Assert(Window != nullptr);

        PlatformApplicationMisc::PumpMessages( true );
    }, true);
}

CMacConsoleWindow::~CMacConsoleWindow()
{
    if ( Window )
    {
        MakeMainThreadCall(^
        {
            SCOPED_AUTORELEASE_POOL();
        
            PlatformApplicationMisc::PumpMessages( true );
        
            [Window release];
            Window = nullptr;
        }, true);
    }
}

void CMacConsoleWindow::Print(const CString& Message )
{  
    if (Window)
    {
        MakeMainThreadCall(^
        {
            SCOPED_AUTORELEASE_POOL();

            NSString* String = [NSString stringWithUTF8String:Message.CStr()];

            [Window appendStringAndScroll:String];
            
            PlatformApplicationMisc::PumpMessages( true );
        }, true);
    }
}

void CMacConsoleWindow::PrintLine(const CString& Message )
{
    if (Window)
    {
        MakeMainThreadCall(^
        {
            SCOPED_AUTORELEASE_POOL();

            NSString* String      = [NSString stringWithUTF8String:Message.CStr()];
            NSString* FinalString = [String stringByAppendingString:@"\n"];
        
            [Window appendStringAndScroll:FinalString];
        
            PlatformApplicationMisc::PumpMessages( true );
        }, true);
    }
}

void CMacConsoleWindow::Clear()
{
    if (Window)
    {
        MakeMainThreadCall(^
        {
            [Window clearWindow];
        }, true);
    }
}

void CMacConsoleWindow::SetTitle(const CString& InTitle)
{
    if (Window)
    {
        MakeMainThreadCall(^
        {
            SCOPED_AUTORELEASE_POOL();
            
            NSString* Title = [NSString stringWithUTF8String:InTitle.CStr()];
            [Window setTitle:Title];
        }, true);
    }
}

void CMacConsoleWindow::SetColor(EConsoleColor Color)
{
    if (Window)
    {
        MakeMainThreadCall(^
        {
            [Window setColor:Color];
        }, true);
    }
}

#endif
