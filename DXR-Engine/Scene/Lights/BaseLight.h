#pragma once
#include "Core/CoreObject.h"

class BaseLight : public CoreObject
{
    CORE_OBJECT(BaseLight, CoreObject);

public:
    BaseLight()
        : CoreObject()
    {
        CORE_OBJECT_INIT();
    }

    virtual ~BaseLight() = default;
};