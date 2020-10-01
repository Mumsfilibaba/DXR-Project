#include "Resource.h"

/*
* Buffer
*/

class Buffer : public Resource
{
public:
	Buffer() = default;
	~Buffer() = default;

	// Casting functions
	virtual Texture* AsTexture() override
	{
		return nullptr;
	}

	virtual const Texture* AsTexture() const override
	{
		return nullptr;
	}

	virtual Buffer* AsBuffer() override
	{
		return this;
	}

	virtual const Buffer* AsBuffer() const override
	{
		return this;
	}
};