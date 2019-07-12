#include "stdafx.h"
#include "vulkanloader.hpp"
#include <stdio.h>
#ifdef __linux__
#include <dlfcn.h>
#endif

// Unfortunately what follows is a bit of a mess
// so I'll explain it as we go for other people
// who need to work in this file

#ifdef VK_USE_PLATFORM_WIN32_KHR
#define LoadFuncAddress GetProcAddress
#else
#define LoadFuncAddress dlsym
#endif

// This generates a list of variable defintions
// for example
// VK_GLOBAL_FUNC( vkGetInstanceProcAddr )
// will expand to
// PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
#define VK_GLOBAL_FUNC(func) PFN_##func func;
#include "vulkan.inl"
#undef VK_GLOBAL_FUNC

// Differences in OS for handling
// references to dynamically loaded
// libraries
#ifdef VK_USE_PLATFORM_WIN32_KHR
static HMODULE s_vulkan_module = nullptr;
#else
static void* s_vulkan_module = nullptr;
#endif

namespace Vulkan
{
	// Loads Vulkan
	// as well as it's global and module functions
	// Do to the nature of Vulkan we have to
	// define different cats of function pointers
	// depending on how we get them.
	bool LoadVulkan()
	{
		// Load the dynamic library here
		// I believe both linux and mac use dlopen
		// Mac will require you load moltenvk
		#ifdef VK_USE_PLATFORM_WIN32_KHR
		s_vulkan_module = LoadLibrary("vulkan-1.dll");
		#endif

		#ifdef VK_USE_PLATFORM_XLIB_KHR
		s_vulkan_module = dlopen("libvulkan.so", RTLD_NOW);
		#endif

		#ifdef VK_USE_PLATFORM_MACOS_MVK
		fprintf(stderr, "Loader: MacOS not supported yet");
		return false;
		#endif

		if (!s_vulkan_module)
		{
			fprintf(stderr, "Couldn't load Vulkan");
			return false;
		}

		// This is the global function loader, IE
		// functions which need to be loaded by the OS
		// We only need this for the next loading function
		// we need vkGetInstanceProcAddr
		#define VK_GLOBAL_FUNC(func) \
		if( !(func = (PFN_##func)LoadFuncAddress(s_vulkan_module, #func)) ) \
		{ \
			fprintf(stderr, "Error loading Vulkan global function ##func"); \
			return false; \
		} \
		fprintf(stderr, "Loaded: %s (global)\n", #func); \

		// The above code defines the code uses to load
		// and OS-level function and we include vulkan.inl
		// to expand the code above with our list of function
		// names
		#include "vulkan.inl"
		#undef VK_GLOBAL_FUNC

		return true;
	}

	// Free the library here
	void ReleaseVulkan()
	{
		if (s_vulkan_module)
		{
			#ifdef VK_USE_PLATFORM_WIN32_KHR
			FreeLibrary(s_vulkan_module);
			#else
			dlclose(s_vulkan_module);
			#endif
		}
	}
}