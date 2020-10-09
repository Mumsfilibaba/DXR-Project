#pragma once
#include "RenderingCore/Texture.h"

#include "D3D12Resource.h"

/*
* D3D12Texture1D
*/

class D3D12Texture1D : public Texture1D, public D3D12Resource
{
public:
	D3D12Texture1D(D3D12Device* InDevice);
	~D3D12Texture1D();

	virtual bool Initialize(const Texture1DInitializer& InInitializer) override;
};

/*
* D3D12Texture2D
*/

class D3D12Texture2D : public Texture2D, public D3D12Resource
{
public:
	D3D12Texture2D(D3D12Device* InDevice);
	~D3D12Texture2D();

	virtual bool Initialize(const Texture2DInitializer& InInitializer) override;
};

/*
* D3D12Texture2DArray
*/

class D3D12Texture2DArray : public Texture2DArray, public D3D12Resource
{
public:
	D3D12Texture2DArray(D3D12Device* InDevice);
	~D3D12Texture2DArray();

	virtual bool Initialize(const Texture2DArrayInitializer& InInitializer) override;
};

/*
* D3D12TextureCube
*/

class D3D12TextureCube : public TextureCube, public D3D12Resource
{
public:
	D3D12TextureCube(D3D12Device* InDevice);
	~D3D12TextureCube();

	virtual bool Initialize(const TextureCubeInitializer& InInitializer) override;
};

/*
* D3D12Texture3D
*/

class D3D12Texture3D : public Texture3D, public D3D12Resource
{
public:
	D3D12Texture3D(D3D12Device* InDevice);
	~D3D12Texture3D();

	virtual bool Initialize(const Texture3DInitializer& InInitializer) override;
};