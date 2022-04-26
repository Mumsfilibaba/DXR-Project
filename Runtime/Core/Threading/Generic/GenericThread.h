#pragma once
#include "Core/Containers/SharedRef.h"
#include "Core/Containers/String.h"

#if defined(COMPILER_MSVC)
    #pragma warning(push)
    #pragma warning(disable : 4100) // Disable unreferenced variable
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wunused-parameter"
#endif

typedef void(*ThreadFunction)();

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// CGenericThread

class CGenericThread : public CRefCounted
{
protected:

    CGenericThread()  = default;
    ~CGenericThread() = default;

public:

    // TODO: Enable member-functions and lambdas (Use TFunction to solve this)

    /**
     * @brief: Create a new thread 
     * 
     * @param InFunction: Entry-point for the new thread
     * @return: An newly created thread interface
     */
    static TSharedRef<CGenericThread> Make(ThreadFunction InFunction) { return dbg_new CGenericThread(); }
   
    /**
      * Create a new thread with a name
      *
      * @param InFunction: Entry-point for the new thread
      * @param InName: Name of the new thread
      * @return: An newly created thread interface
      */
    static TSharedRef<CGenericThread> Make(ThreadFunction InFunction, const String& InName) { return dbg_new CGenericThread(); }

    /** @return: Start thread-execution and returns true if the thread started successfully */
    virtual bool Start() { return true; }

    /** @brief: Wait until thread has finished running */
    virtual void WaitUntilFinished() { }

    /**
     * @brief: Set name of thread. Needs to be called before start on some platforms for the changes to take effect 
     * 
     * @param InName: New name of the thread
     */
    virtual void SetName(const String& InName) { }

    /**
     * @brief: Retrieve platform specific handle
     *
     * @return: Returns a platform specific handle or zero if no platform handle is defined
     */
    virtual void* GetOSHandle() { return nullptr; }
};

#if defined(COMPILER_MSVC)
    #pragma warning(pop)
#elif defined(COMPILER_CLANG)
    #pragma clang diagnostic pop
#endif
