#pragma once

class FCameraComponent;
class FWorld;
class FActor;

FActor* SpawnProjectileSphere(FWorld* World, FCameraComponent* Camera);

/** Release the cached projectile mesh before RHI teardown (owns GPU buffers). */
void ReleaseProjectileMeshCache();
