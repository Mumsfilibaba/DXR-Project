#include "Core/PlatformInterface/IPlatformThread.h"

CONST_INIT FThreadLocalSlot IPlatformThread::TLSSlot;

IPlatformThread* IPlatformThread::GetThread()
{
    void* LocalThread = TLSSlot.GetValue();
    return reinterpret_cast<IPlatformThread*>(LocalThread);
}
