#include "SpotLight.h"

/*
* SpotLight
*/

SpotLight::SpotLight()
	: Light()
	, ConeAngle(45.0f)
{
	CORE_OBJECT_INIT();
}

SpotLight::~SpotLight()
{
}

void SpotLight::SetConeAngle(Float InConeAngle)
{
	ConeAngle = InConeAngle;
}
