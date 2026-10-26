#pragma once

// Ported from UnleashedRecomp-Android (os/android/vulkan_driver_android.h, 0.6.1).
//
// Loads a custom Vulkan ICD (Mesa Turnip) via libadrenotools instead of the vendor driver.
// Returns a vkGetInstanceProcAddr function pointer (cast to PFN_vkGetInstanceProcAddr by the
// caller) if the selected custom driver was found and loaded successfully, or nullptr if it
// is not used or loading failed - callers then fall back to volkInitialize().
// extern "C" linkage so it can be forward-declared and called from within plume's "plume"
// namespace (plume_vulkan.cpp) without matching a C++ namespace across that boundary.
extern "C" void* AndroidGetCustomVulkanLoader();

// Called by Video only after both the Vulkan device and a usable swapchain exist. This is
// deliberately separate from AndroidGetCustomVulkanLoader(): dlopen/vkGetInstanceProcAddr
// succeeding does not prove that the selected driver can finish Vulkan startup.
void AndroidMarkVulkanStartupSuccessful();
