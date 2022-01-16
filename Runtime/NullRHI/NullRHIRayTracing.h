#pragma once
#include "RHI/RHIRayTracing.h"

#include "NullRHIViews.h"

#if defined(COMPILER_MSVC)
#pragma warning(push)
#pragma warning(disable : 4100) // Disable unreferenced variable

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#endif

class CNullRHIRayTracingGeometry : public CRHIRayTracingGeometry
{
public:
    CNullRHIRayTracingGeometry(uint32 InFlags)
        : CRHIRayTracingGeometry(InFlags)
    {
    }

    ~CNullRHIRayTracingGeometry() = default;

    virtual void SetName(const CString& InName) override
    {
        CRHIResource::SetName(InName);
    }

    virtual bool IsValid() const override
    {
        return true;
    }
};

class CNullRHIRayTracingScene : public CRHIRayTracingScene
{
public:
    CNullRHIRayTracingScene(uint32 InFlags)
        : CRHIRayTracingScene(InFlags)
        , View(dbg_new CNullRHIShaderResourceView())
    {
    }

    ~CNullRHIRayTracingScene() = default;

    virtual void SetName(const CString& InName) override final
    {
        CRHIResource::SetName(InName);
    }

    virtual bool IsValid() const override final
    {
        return true;
    }

    virtual CRHIShaderResourceView* GetShaderResourceView() const override final
    {
        return View.Get();
    }

private:
    TSharedRef<CNullRHIShaderResourceView> View;
};

#if defined(COMPILER_MSVC)
#pragma warning(pop)

#elif defined(COMPILER_CLANG)
#pragma clang diagnostic pop
#endif
