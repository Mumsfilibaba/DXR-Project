#pragma once
#include "RenderingCore/Texture.h"

#include "D3D12Resource.h"

/*
* D3D12Texture
*/

class D3D12Texture : public D3D12Resource
{
public:
	D3D12Texture(D3D12Device* InDevice)
		: D3D12Resource(InDevice)
	{
	}

	FORCEINLINE DXGI_FORMAT GetNativeFormat() const
	{
		return Desc.Format;
	}
};

/*
* D3D12Texture1D
*/

class D3D12Texture1D : public Texture1D, public D3D12Texture
{
public:
	D3D12Texture1D(D3D12Device* InDevice, EFormat InFormat, UInt32 InUsage, UInt32 InWidth, UInt32 InMipLevels, const ClearValue& InOptimizedClearValue)
		: Texture1D(InFormat, InUsage, InWidth, InMipLevels, InOptimizedClearValue)
		, D3D12Texture(InDevice)
	{
	}

	virtual void SetName(const std::string& Name) override final
	{
		D3D12Resource::SetName(Name);
	}
};

/*
* D3D12Texture1DArray
*/

class D3D12Texture1DArray : public Texture1DArray, public D3D12Texture
{
public:
	D3D12Texture1DArray(D3D12Device* InDevice, EFormat InFormat, UInt32 InUsage, UInt32 InWidth, UInt32 InMipLevels, UInt16 InArrayCount, const ClearValue& InOptimizedClearValue)
		: Texture1DArray(InFormat, InUsage, InWidth, InMipLevels, InArrayCount, InOptimizedClearValue)
		, D3D12Texture(InDevice)
	{
	}

	virtual void SetName(const std::string& Name) override final
	{
		D3D12Resource::SetName(Name);
	}
};

/*
* D3D12Texture2D
*/

class D3D12Texture2D : public Texture2D, public D3D12Texture
{
public:
	D3D12Texture2D(D3D12Device* InDevice, EFormat InFormat, UInt32 InUsage, UInt32 InWidth, UInt32 InHeight, UInt32 InMipLevels, UInt32 InSampleCount, const ClearValue& InOptimizedClearValue)
		: Texture2D(InFormat, InUsage, InWidth, InHeight, InMipLevels, InSampleCount, InOptimizedClearValue)
		, D3D12Texture(InDevice)
	{
	}

	virtual void SetName(const std::string& Name) override final
	{
		D3D12Resource::SetName(Name);
	}
};

/*
* D3D12BackBufferTexture2D
*/

class D3D12BackBufferTexture2D : public D3D12Texture2D
{
public:
	D3D12BackBufferTexture2D(D3D12Device* InDevice, TComPtr<ID3D12Resource>& Resource)
		: D3D12Texture2D(InDevice, EFormat::Format_Unknown, 0, 0, 0, 0, 0, ClearValue(ColorClearValue(0.0f, 0.0f, 0.0f, 0.0f)))
	{
		NativeResource = Resource;
		Desc = Resource->GetDesc();

		Width	= UInt32(Desc.Width);
		Height	= Desc.Height;
		SampleCount = Desc.SampleDesc.Count;
		MipLevels	= Desc.MipLevels;
	}
};

/*
* D3D12Texture2DArray
*/

class D3D12Texture2DArray : public Texture2DArray, public D3D12Texture
{
public:
	D3D12Texture2DArray(D3D12Device* InDevice, EFormat InFormat, UInt32 InUsage, UInt32 InWidth, UInt32 InHeight, UInt32 InMipLevels, UInt16 InArrayCount, UInt32 InSampleCount, const ClearValue& InOptimizedClearValue)
		: Texture2DArray(InFormat, InUsage, InWidth, InHeight, InMipLevels, InArrayCount, InSampleCount, InOptimizedClearValue)
		, D3D12Texture(InDevice)
	{
	}

	virtual void SetName(const std::string& Name) override final
	{
		D3D12Resource::SetName(Name);
	}
};

/*
* D3D12TextureCube
*/

class D3D12TextureCube : public TextureCube, public D3D12Texture
{
public:
	D3D12TextureCube(D3D12Device* InDevice, EFormat InFormat, UInt32 InUsage, UInt32 InSize, UInt32 InMipLevels, UInt32 InSampleCount, const ClearValue& InOptimizedClearValue)
		: TextureCube(InFormat, InUsage, InSize, InMipLevels, InSampleCount, InOptimizedClearValue)
		, D3D12Texture(InDevice)
	{
	}

	virtual void SetName(const std::string& Name) override final
	{
		D3D12Resource::SetName(Name);
	}
};

/*
* D3D12TextureCubeArray
*/

class D3D12TextureCubeArray : public TextureCubeArray, public D3D12Texture
{
public:
	D3D12TextureCubeArray(D3D12Device* InDevice, EFormat InFormat, UInt32 InUsage, UInt32 InSize, UInt32 InMipLevels, UInt16 InArrayCount, UInt32 InSampleCount, const ClearValue& InOptimizedClearValue)
		: TextureCubeArray(InFormat, InUsage, InSize, InMipLevels, InArrayCount, InSampleCount, InOptimizedClearValue)
		, D3D12Texture(InDevice)
	{
	}

	virtual void SetName(const std::string& Name) override final
	{
		D3D12Resource::SetName(Name);
	}
};

/*
* D3D12Texture3D
*/

class D3D12Texture3D : public Texture3D, public D3D12Texture
{
public:
	D3D12Texture3D(D3D12Device* InDevice, EFormat InFormat, UInt32 InUsage, UInt32 InWidth, UInt32 InHeight, UInt16 InDepth, UInt32 InMipLevels, const ClearValue& InOptimizedClearValue)
		: Texture3D(InFormat, InUsage, InWidth, InHeight, InDepth, InMipLevels, InOptimizedClearValue)
		, D3D12Texture(InDevice)
	{
	}

	virtual void SetName(const std::string& Name) override final
	{
		D3D12Resource::SetName(Name);
	}
};

/*
* Cast a texture to correct type
*/

inline D3D12Texture* D3D12TextureCast(Texture* Texture)
{
	VALIDATE(Texture != nullptr);

	if (Texture->AsTexture1D() != nullptr)
	{
		return static_cast<D3D12Texture1D*>(Texture);
	}
	else if (Texture->AsTexture1DArray() != nullptr)
	{
		return static_cast<D3D12Texture1DArray*>(Texture);
	}
	else if (Texture->AsTexture2D() != nullptr)
	{
		return static_cast<D3D12Texture2D*>(Texture);
	}
	else if (Texture->AsTexture2DArray() != nullptr)
	{
		return static_cast<D3D12Texture2DArray*>(Texture);
	}
	else if (Texture->AsTextureCube() != nullptr)
	{
		return static_cast<D3D12TextureCube*>(Texture);
	}
	else if (Texture->AsTextureCubeArray() != nullptr)
	{
		return static_cast<D3D12TextureCubeArray*>(Texture);
	}
	else if (Texture->AsTexture3D() != nullptr)
	{
		return static_cast<D3D12Texture3D*>(Texture);
	}
	else
	{
		return nullptr;
	}
}

inline const D3D12Texture* D3D12TextureCast(const Texture* Texture)
{
	VALIDATE(Texture != nullptr);

	if (Texture->AsTexture1D() != nullptr)
	{
		return static_cast<const D3D12Texture1D*>(Texture);
	}
	else if (Texture->AsTexture1DArray() != nullptr)
	{
		return static_cast<const D3D12Texture1DArray*>(Texture);
	}
	else if (Texture->AsTexture2D() != nullptr)
	{
		return static_cast<const D3D12Texture2D*>(Texture);
	}
	else if (Texture->AsTexture2DArray() != nullptr)
	{
		return static_cast<const D3D12Texture2DArray*>(Texture);
	}
	else if (Texture->AsTextureCube() != nullptr)
	{
		return static_cast<const D3D12TextureCube*>(Texture);
	}
	else if (Texture->AsTextureCubeArray() != nullptr)
	{
		return static_cast<const D3D12TextureCubeArray*>(Texture);
	}
	else if (Texture->AsTexture3D() != nullptr)
	{
		return static_cast<const D3D12Texture3D*>(Texture);
	}
	else
	{
		return nullptr;
	}
}