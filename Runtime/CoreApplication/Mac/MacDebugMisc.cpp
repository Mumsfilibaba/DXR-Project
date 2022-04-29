#include "MacDebugMisc.h"

#include <Foundation/Foundation.h>

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CMacDebugMisc

void CMacDebugMisc::OutputDebugString(const String& Message)
{ 
    NSLog(@"%s\n", Message.CStr());
}
