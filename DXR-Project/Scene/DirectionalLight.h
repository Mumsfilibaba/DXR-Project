#pragma once
#include "Light.h"

struct DirectionalLightProperties
{
	XMFLOAT3 Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	Float32 Padding;
	XMFLOAT3 Direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
};

/*
* DirectionalLight
*/
class DirectionalLight : public Light
{
	CORE_OBJECT(DirectionalLight, Light);

public:
	DirectionalLight();
	~DirectionalLight();

	virtual bool Initialize(class D3D12Device* Device) override;

	virtual void BuildBuffer(class D3D12CommandList* CommandList) override;

	void SetDirection(const XMFLOAT3& InPosition);
	void SetDirection(Float32 X, Float32 Y, Float32 Z);

	FORCEINLINE const XMFLOAT3& GetDirection() const
	{
		return Direction;
	}

private:
	XMFLOAT3 Direction;
};