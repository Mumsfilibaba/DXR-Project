#if defined(PLATFORM_MACOS)
#include "MacDebugMisc.h"

#include <Foundation/Foundation.h>

void CMacDebugMisc::OutputDebugString( const std::string& Message )
{ 
    NSLog( @"%s\n", Message.c_str() );
}

#endif