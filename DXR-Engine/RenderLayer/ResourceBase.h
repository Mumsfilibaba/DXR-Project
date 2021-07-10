#pragma once
#include "RenderingCore.h"

#include "Core/RefCountedObject.h"

class Resource : public RefCountedObject
{
public:
    virtual void* GetNativeResource() const
    {
        return nullptr;
    }

    virtual bool IsValid() const
    {
        return false;
    }

    virtual void SetName( const std::string& InName )
    {
        Name = InName;
    }

    const std::string& GetName() const
    {
        return Name;
    }

private:
    std::string Name;
};