#ifndef DEBUG_COLOR_HLSLI
#define DEBUG_COLOR_HLSLI

// Hash a 32-bit instance ID into a stable, well-spread RGB color so neighbouring IDs look distinct.
float3 InstanceIDToColor(uint InstanceID)
{
    uint Hash = InstanceID + 1u;
    Hash ^= Hash >> 17;
    Hash *= 0xed5ad4bbu;
    Hash ^= Hash >> 11;
    Hash *= 0xac4c1b51u;
    Hash ^= Hash >> 15;
    Hash *= 0x31848babu;
    Hash ^= Hash >> 14;

    const float R = float((Hash >> 0)  & 0xffu) / 255.0f;
    const float G = float((Hash >> 8)  & 0xffu) / 255.0f;
    const float B = float((Hash >> 16) & 0xffu) / 255.0f;

    // Bias away from near-black so every instance stays visible against the black miss background.
    return lerp(float3(0.15f, 0.15f, 0.15f), float3(1.0f, 1.0f, 1.0f), float3(R, G, B));
}

#endif
