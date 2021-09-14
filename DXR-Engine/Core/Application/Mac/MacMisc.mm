#include "MacMisc.h"

#include <Foundation/Foundation.h>

void CMacMisc::OutputDebugString( const std::string& Message )
{ 
    NSLog( @"%s\n", Message.c_str() );
}
