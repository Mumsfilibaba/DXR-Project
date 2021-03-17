#pragma once
#include "Application/Generic/GenericCursor.h"

#include "Windows.h"

class WindowsApplication;

class WindowsCursor : public GenericCursor
{
public:
    WindowsCursor(WindowsApplication* InApplication);
    ~WindowsCursor();

    virtual bool Init(const CursorCreateInfo& InCreateInfo) override final;

    virtual void* GetNativeHandle() const override final
    {
        return reinterpret_cast<void*>(hCursor);
    }

    HCURSOR GetCursor() const { return hCursor; }

private:
    WindowsApplication* Application;
    HCURSOR hCursor;
    LPCSTR  CursorName;
};