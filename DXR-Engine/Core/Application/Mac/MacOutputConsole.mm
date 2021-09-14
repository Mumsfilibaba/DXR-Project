#if defined(PLATFORM_MACOS)
#include "MacOutputConsole.h"
#include "CocoaConsoleWindow.h"
#include "ScopedAutoreleasePool.h"

CMacOutputConsole::CMacOutputConsole()
    : Window( nullptr )
{
    // TODO: This needs to happen on the mainthread, make sure that there is some mechanism for this 
    SCOPED_AUTORELEASE_POOL();
    
    // TODO: Control with console vars? 
    const CGFloat Width  = 1280.0f;
    const CGFloat Height = 720.0f;
    
    Window = [[CCocoaConsoleWindow alloc] init: Width Height:Height];
    [Window setColor:EConsoleColor::White];
    
    Assert(Window != nullptr);

    // TODO: Pump events here
    //MacApplication::PeekEvents();
}

CMacOutputConsole::~CMacOutputConsole()
{
    if ( Window )
    {
        // TODO: Ensure mainthread
        SCOPED_AUTORELEASE_POOL();
        
        //MacApplication::PeekEvents();
        
        [Window release];
        Window = nullptr;
    }
}

void CMacOutputConsole::Print(const std::string& Message )
{
    if (Window)
    {
        NSString* String      = [NSString stringWithUTF8String:Message.c_str()];
        NSString* FinalString = [String stringByAppendingString:@"\n"];

        //TODO: Make sure this is the mainthread
        SCOPED_AUTORELEASE_POOL();
        
        [Window appendStringAndScroll:FinalString];
        [FinalString release];
        
        // TODO: Pump messages
        //MacApplication::PeekEvents();
    }
}

void CMacOutputConsole::Clear()
{
    if (Window)
    {
        [Window clearWindow];
    }
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
