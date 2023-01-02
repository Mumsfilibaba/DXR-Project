#pragma once
#include "Core/Delegates/Delegate.h"
#include "Core/Containers/SharedRef.h"
#include "Core/Time/Stopwatch.h"

#include "RHI/RHIResources.h"
#include "RHI/RHIShader.h"

#include "Application/InputHandler.h"
#include "Application/IViewportRenderer.h"

class FViewportRenderer final
    : public IViewportRenderer
{
public:
    FViewportRenderer()  = default;
    ~FViewportRenderer() = default;

     /** @brief - Initialize the context */
    virtual bool InitContext(InterfaceContext Context) override final;

     /** @brief - Start the update of the UI, after the call to this function, calls to UI window's tick are valid */
    virtual void BeginTick() override final;

     /** @brief - End the update of the UI, after the call to this function, calls to UI window's tick are NOT valid  */
    virtual void EndTick() override final;

     /** @brief - Render all the UI for this frame */
    virtual void Render(class FRHICommandList& Commandlist) override final;

private:

};
