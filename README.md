# DXR Project
This engine is a small hobby rendering/game engine created for testing different rendering techniques. The name may suggest that this is only for DXR, and that is how it started. However, it has evolved into more than that. Now it has more focus on abstracting D3D12 and testing game-engine architecture techniques. Maybe I change the name in the future, perhaps not.

# Features
**Current:**
* FXAA
* Deferred Rendering
* Normal mapping
* Parallax Occlusion Mapping
* Physically Based Rendering with Image-Based Lightning
* Shadow Mapping (Both Variance Shadow Mapping and "Traditional" is supported, and is selectable at compile time) _**(Variance is currently broken)**_
* Dynamic lights (Point-Lights, Directional-Lights, both with shadow support)
* RayTraced reflections (Perfect Specular Only) _**(RayTracing is currently broken)**_
* SSAO

**Planed:**
* Vulkan backend
* Tiled Deferred Rendering
* Screen Space Reflections
* Cascade Shadow-maps

# Screenshots
RayTracing **Off**
![Screen3](Screenshots/screen3.png? "Screen3")
RayTracing **Off**
![Screen0](Screenshots/screen0.png? "Screen0")
RayTracing **Off**
![Screen1](Screenshots/screen1.png? "Screen1")
RayTracing **Off**
![Screen2](Screenshots/screen2.png? "Screen2")
