#pragma once
#include "Resource.h"

/*
* Viewport
*/

class Viewport : public PipelineResource
{
public:
	virtual class Texture2D* GetSurface() const
	{
		return nullptr;
	}

	virtual class RenderTargetView* GetRenderTargetView() const = 0;

	virtual class GenericWindow* GetWindow() const = 0;

	virtual UInt32 GetWidth() const
	{
		return 0;
	}

	virtual UInt32 GetHeight() const
	{
		return 0;
	}
};