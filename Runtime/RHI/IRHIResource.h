#pragma once
#include "RHICore.h"

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// IRHIResource

class IRHIResource
{
protected:

    IRHIResource() = default;
    virtual ~IRHIResource() = default;

public:

    /**
     * @brief: Add a reference by increasing the reference-count
     *
     * @return: Returns the the Reference-Count
     */
    virtual int32 AddRef() = 0;

    /**
     * @brief: Release a reference by decreasing the reference-count
     *
     * @return: Returns the reference count
     */
    virtual int32 Release() = 0;

};