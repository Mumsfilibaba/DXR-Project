# DXR Project
This engine is a small hobby rendering/game engine created for testing different rendering techniques. The name may suggest that this is only for DXR, and that is how it started. However, it has evolved into more than that. Now it has more focus on abstracting D3D12 and testing game-engine architecture techniques. Maybe I change the name in the future, perhaps not.

# Contributing
Feel free to make a pull request but please ensure that code follow the code-standard which you can find [here](CodeStandard.md).

# Features
**Current:**
* Support for both Vulkan (Windows and macOS) and D3D12 (Windows)
* Multiplatform support (Windows and macOS)
* Tiled Deferred Rendering
* Normal mapping
* Parallax Occlusion Mapping
* Physically Based Rendering with Image-Based Lightning
* Shadow Mapping
    * Cascaded Shadow-Maps for Directional-Lights
    * Omni-Directional shadows for Point-Lights 
* Dynamic lights (Point-Lights, Directional-Lights)
* SSAO
* TAA
* FXAA
* Support for dynamic block-compression of textures (BCH6)
* Custom containers
    * TArray
    * TString
    * TBitArray
* Task System (For easy multi-threading)
* Module System (To make the engine more modular)

**Planed:**
* Screen Space Reflections
* Re-enable Ray-Traced reflections
* Spotlights
* Local Environment Probes

# Screenshots
![Screen2](Screenshots/screen2.png? "Screen2")
![Screen1](Screenshots/screen1.png? "Screen1")
![Screen0](Screenshots/screen0.png? "Screen0")
![ReflectionsScreen](Screenshots/ReflectionsScreen.png? "ReflectionsScreen") 
![Profiler](Screenshots/Profiler.png? "Profiler") 
![TextureDebugger](Screenshots/TextureDebugger.png? "TextureDebugger") 
![Console](Screenshots/Console.png? "Console") 



