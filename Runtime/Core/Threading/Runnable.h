#pragma once
#include "Core/Core.h"

struct FRunnable
{
    virtual ~FRunnable() = default;

    /** @return Called before run and returns true if the thread should continue to run. */
    virtual bool Start() { return true; }

    /** @return Main function of the thread, returns zero if successful. */
    virtual int32 Run() = 0;

    /** @brief Stop the thread from executing. This should ensure that the Run function returns. */
    virtual void Stop() { }

    /** @brief Called by before the native thread finish executes and allows for cleanup of any resources */
    virtual void Destroy() { }
};