#if defined(PLATFORM_MACOS) && defined(__OBJC__)
#include "MacOutputConsole.h"
#include "CocoaConsoleWindow.h"
#include "ScopedAutoreleasePool.h"

#include "Core/Application/Platform/PlatformApplicationMisc.h"

CMacOutputConsole::CMacOutputConsole()
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

CMacOutputConsole::~CMacOutputConsole()
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

void CMacOutputConsole::Print(const std::string& Message )
{
    if (Window)
    {
        //TODO: Make sure this is the mainthread
        SCOPED_AUTORELEASE_POOL();

        NSString* String = [NSString stringWithUTF8String:Message.c_str()];

        [Window appendStringAndScroll:String];
        
        PlatformApplicationMisc::PumpMessages( true );
    }
}

void CMacOutputConsole::PrintLine(const std::string& Message )
{
    if (Window)
    {
        //TODO: Make sure this is the mainthread
        SCOPED_AUTORELEASE_POOL();

        NSString* String      = [NSString stringWithUTF8String:Message.c_str()];
        NSString* FinalString = [String stringByAppendingString:@"\n"];
        
        [Window appendStringAndScroll:FinalString];
        
        PlatformApplicationMisc::PumpMessages( true );
    }
}

void CMacOutputConsole::Clear()
{
    if (Window)
    {
        [Window clearWindow];
    }
}

void CMacOutputConsole::ClearLastLine()
{
    //TODO: Implement
}

void CMacOutputConsole::SetTitle(const std::string& InTitle)
{
    if (Window)
    {
        SCOPED_AUTORELEASE_POOL();
        
        NSString* Title = [NSString stringWithUTF8String:InTitle.c_str()];
        [Window setTitle:Title];
    }
}

void CMacOutputConsole::SetColor(EConsoleColor Color)
{
    if (Window)
    {
        [Window setColor:Color];
    }
}

#endif