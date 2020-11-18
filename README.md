# Personal Engine
This engine is a small hobby rendering/game engine created for testing different rendering techniques. The name may suggest that this is only for DXR, and that is how it started. However, it has evolved into more than that. Now it has more focus on abstracting D3D12 and testing game-engine architecture techniques. Maybe I change the name in the future, perhaps not.

# Features
**Current:**
* FXAA
* Deferred Rendering
* Normal mapping
* Parallax Occlusion Mapping
* Physically Based Rendering with Image-Based Lightning
* Shadow Mapping (Both Variance Shadow Mapping and "Traditional" is supported, and is selectable at compile time)
* Dynamic lights (Point-Lights, Directional-Lights, both with shadow support)
* Ray Traced reflections (Currently disabled)

**Planed:**
* Vulkan backend
* Better API abstraction
* Tiled Deferred Rendering
* SSAO
* Screen Space Reflections
* Cascade Shadow-maps
