#pragma once
#include "InterfaceImage.h"
#include "IInterfaceWindow.h"

#include "Core/Modules/IEngineModule.h"
#include "Core/Containers/SharedRef.h"

class IInterfaceRenderer : public CDefaultEngineModule, public CRefCounted
{
public:

    virtual ~IInterfaceRenderer() = default;

    /* Start the update of the UI, after the call to this function, calls to UI window's tick are valid */
    virtual void BeginTick() = 0;

    /* End the update of the UI, after the call to this function, calls to UI window's tick are NOT valid  */
    virtual void EndTick() = 0;

    /* Render all the UI for this frame */
    virtual void Render( class CRHICommandList& Commandlist ) = 0;

    /* Retrieve the context handle */
    virtual InterfaceContext GetContext() const = 0;
};