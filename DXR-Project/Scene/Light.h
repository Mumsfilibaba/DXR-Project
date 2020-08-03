#pragma once

class Light
{
public:
	Light();
	~Light();

	void SetIntensity(Float32 InIntensity);

	void SetColor(const XMFLOAT3& InColor);
	void SetColor(Float32 R, Float32 G, Float32 B);

	FORCEINLINE Float32 GetIntensity() const
	{
		return Intensity;
	}

	FORCEINLINE const XMFLOAT3& GetColor() const
	{
		return Color;
	}

private:
	XMFLOAT3 Color;
	Float32 Intensity = 1.0f;
};