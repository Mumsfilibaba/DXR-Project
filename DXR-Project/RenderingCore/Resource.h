#pragma once
#include "RenderingCore.h"

#include "Core/RefCountedObject.h"

#include "Containers/TArray.h"

class Texture;
class Buffer;
class RayTracingGeometry;
class RayTracingScene;

/*
* PipelineResource
*/

class PipelineResource : public RefCountedObject
{
public: 
	PipelineResource()			= default;
	virtual ~PipelineResource()	= default;

	virtual void SetName(const std::string& Name)
	{
	}

	virtual VoidPtr GetNativeResource() const
	{
		return nullptr;
	}
};

/*
* Resource
*/

class Resource : public PipelineResource
{
public: 
	Resource()			= default;
	virtual ~Resource()	= default;

	// Casting Functions
	virtual Texture* AsTexture()
	{
		return nullptr;
	}

	virtual const Texture* AsTexture() const
	{
		return nullptr;
	}

	virtual Buffer* AsBuffer()
	{
		return nullptr;
	}

	virtual const Buffer* AsBuffer() const
	{
		return nullptr;
	}

	virtual RayTracingGeometry* AsRayTracingGeometry()
	{
		return nullptr;
	}

	virtual const RayTracingGeometry* AsRayTracingGeometry() const
	{
		return nullptr;
	}

	virtual RayTracingScene* AsRayTracingScene()
	{
		return nullptr;
	}

	virtual const RayTracingScene* AsRayTracingScene() const
	{
		return nullptr;
	}
};