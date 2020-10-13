#pragma once
#include "RenderingCore/Texture.h"

#include "D3D12Resource.h"

/*
* D3D12Texture1D
*/

class D3D12Texture1D : public Texture1D, public D3D12Resource
{
public:
	inline D3D12Texture1D(D3D12Device* InDevice, EFormat InFormat, Uint32 InUsage, Uint32 InWidth, Uint32 InMipLevels, const ClearValue& InOptimizedClearValue);
		: Texture1D(InFormat, InUsage, InWidth, InMipLevels, InOptimizedClearValue)
		, D3D12Resource(InDevice)
	{
	}

	~D3D12Texture1D() = default;
};

/*
* D3D12Texture2D
*/

class D3D12Texture2D : public Texture2D, public D3D12Resource
{
public:
	inline D3D12Texture2D(D3D12Device* InDevice, EFormat InFormat, Uint32 InUsage, Uint32 InWidth, Uint32 InHeight, Uint32 InMipLevels, Uint32 InSampleCount, const ClearValue& InOptimizedClearValue);
		: Texture2D(InFormat, InUsage, InWidth, InHeight, InMipLevels, InSampleCount, InOptimizedClearValue)
		, D3D12Resource(InDevice)
	{
	}
	
	~D3D12Texture2D() = default;
};

/*
* D3D12Texture2DArray
*/

class D3D12Texture2DArray : public Texture2DArray, public D3D12Resource
{
public:
	inline D3D12Texture2DArray(D3D12Device* InDevice, EFormat InFormat, Uint32 InUsage, Uint32 InWidth, Uint32 InHeight, Uint32 InMipLevels, Uint32 InArrayCount, Uint32 InSampleCount, const ClearValue& InOptimizedClearValue)
		: Texture2DArray(InFormat, InUsage, InWidth, InHeight, InMipLevels, InArrayCount, InSampleCount, InOptimizedClearValue)
		, D3D12Resource(InDevice)
	{
	}

	~D3D12Texture2DArray() = default;
};

/*
* D3D12TextureCube
*/

class D3D12TextureCube : public TextureCube, public D3D12Resource
{
public:
	inline D3D12TextureCube(D3D12Device* InDevice, EFormat InFormat, Uint32 InUsage, Uint32 InSize, Uint32 InMipLevels, Uint32 InSampleCount, const ClearValue& InOptimizedClearValue);
		: TextureCube(InFormat, InUsage, InSize, InMipLevels, InSampleCount, InOptimizedClearValue)
		, D3D12Resource(InDevice)
	{
	}
	
	~D3D12TextureCube() = default;
};

/*
* D3D12Texture3D
*/

class D3D12Texture3D : public Texture3D, public D3D12Resource
{
public:
	inline D3D12Texture3D(D3D12Device* InDevice, EFormat InFormat, Uint32 InUsage, Uint32 InWidth, Uint32 InHeight, Uint32 InDepth, Uint32 InMipLevels, const ClearValue& InOptimizedClearValue)
		: Texture3D(InFormat, InUsage, InWidth, InHeight, InDepth, InMipLevels, InOptimizedClearValue)
		, D3D12Resource(InDevice)
	{
	}

	~D3D12Texture3D() = default;
};