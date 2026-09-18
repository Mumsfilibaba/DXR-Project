#pragma once

bool RHIBoot_Null_Test();

#if PLATFORM_MACOS
bool RHIBoot_Metal_Test();
bool RHIBoot_Vulkan_Test();
#elif PLATFORM_WINDOWS
bool RHIBoot_D3D12_Test();
bool RHIBoot_Vulkan_Test();
#endif
