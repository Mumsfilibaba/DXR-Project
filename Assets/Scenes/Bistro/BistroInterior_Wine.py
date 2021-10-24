# BistroInterior_Wine.fbx

# Absorption coefficients (or extinction coefficient in absence of scattering)
# Taken from https://cseweb.ucsd.edu/~ravir/dilution.pdf and adapted to work with scene units in meters
volume_absorption = {
    'white_wine': float3(12.28758, 16.51818, 20.30273),
    'red_wine': float3(117.13133, 251.91133, 294.33867),
    'beer': float3(11.78552, 25.45862, 58.37241),
    'bottle_wine': float3(102.68063, 168.015, 246.80438)
}

glass = scene.material("TransparentGlass")
glass.indexOfRefraction = 1.55
glass.specularTransmission = 1
glass.doubleSided = True
glass.nestedPriority = 2

bottle_wine = scene.material("TransparentGlassWine")
bottle_wine.indexOfRefraction = 1.55
bottle_wine.specularTransmission = 1
bottle_wine.doubleSided = True
bottle_wine.nestedPriority = 2
bottle_wine.volumeAbsorption = volume_absorption['bottle_wine']

water = scene.material("Water")
water.indexOfRefraction = 1.33
water.specularTransmission = 1
water.doubleSided = True
water.nestedPriority = 1

ice = scene.material("Ice")
ice.indexOfRefraction = 1.31
ice.specularTransmission = 1
ice.doubleSided = True
ice.nestedPriority = 1

white_wine = scene.material("White_Wine")
white_wine.indexOfRefraction = 1.33
white_wine.specularTransmission = 1
white_wine.doubleSided = True
white_wine.nestedPriority = 1
white_wine.volumeAbsorption = volume_absorption['white_wine']

red_wine = scene.material("Red_Wine")
red_wine.indexOfRefraction = 1.33
red_wine.specularTransmission = 1
red_wine.doubleSided = True
red_wine.nestedPriority = 1
red_wine.volumeAbsorption = volume_absorption['red_wine']

beer = scene.material("Beer")
beer.indexOfRefraction = 1.33
beer.specularTransmission = 1
beer.doubleSided = True
beer.nestedPriority = 1
beer.volumeAbsorption = volume_absorption['beer']
