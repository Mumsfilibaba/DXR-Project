#pragma once
#include "Core/Core.h"
#include "Core/Containers/SharedRef.h"

class ENGINE_API FResource : public FRefCounted
{
public:
    virtual ~FResource() = default;
};