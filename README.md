### DXR Project

A hobby rendering and game engine created for testing and experimenting with various rendering techniques. While the project began as a DirectX Raytracing (DXR) experimentation engine, it has evolved into a versatile platform with both **D3D12** and **Vulkan** backends. The focus has shifted toward abstracting graphics APIs, exploring game engine architecture, and experimenting with advanced rendering techniques.

---

### Features

#### **Current:**
- **Graphics Backends:**
  - Vulkan
  - D3D12
- **Rendering Techniques:**
  - Tiled Deferred Rendering
  - Normal Mapping
  - Parallax Occlusion Mapping
  - Physically Based Rendering (PBR) with Image-Based Lighting (IBL)
  - Shadow Mapping:
    - Cascaded Shadow Maps for Directional Lights
    - Omni-Directional Shadows for Point Lights
- **Post-Processing Effects:**
  - Screen Space Ambient Occlusion (SSAO)
  - Temporal Anti-Aliasing (TAA)
  - Fast Approximate Anti-Aliasing (FXAA)
- **Dynamic Lighting:**
  - Point Lights
  - Directional Lights
- **Custom Data Structures:**
  - `TArray`: Custom array container
  - `TString`: Custom string container
  - `TBitArray`: Custom bit array container

#### **Planned:**
- Screen Space Reflections (SSR)
- Ray-Traced Reflections
- Spotlights
- Local Environment Probes

# Screenshots
**(Master - Branch)** **RT Off**
![Screen2](Screenshots/screen2.png? "Screen2")
**(Master - Branch)** **RT Off**
![Screen1](Screenshots/screen1.png? "Screen1")
**(Cascaded Shadow Maps - Branch)** Cascaded Shadow Maps **RT Off**
![Screen0](Screenshots/screen0.png? "Screen0")
**(Ray Traced Reflections - Branch)** Ray Traced Reflections **RT On**
![ReflectionsScreen](Screenshots/ReflectionsScreen.png? "ReflectionsScreen") 
**(Master - Branch)** Profiler **RT Off**
![Profiler](Screenshots/Profiler.png? "Profiler") 
**(Master - Branch)** Texture Debugger **RT Off**
![TextureDebugger](Screenshots/TextureDebugger.png? "TextureDebugger") 
**(Master - Branch)** In-Game Console **RT Off**
![Console](Screenshots/Console.png? "Console") 



