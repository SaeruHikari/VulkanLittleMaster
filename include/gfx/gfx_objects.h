#pragma once
#include "os/window.h"
#include "gfx/volk.h"
#include <vector>

class LittleGFXAdapter
{
    friend class LittleGFXWindow;
    friend class LittleGFXInstance;
    friend class LittleGFXDevice;

protected:
    std::vector<const char*> deviceExtensions;
    std::vector<const char*> deviceLayers;
    VkPhysicalDevice vkPhysicalDevice;
    int64_t gfxQueueIndex;
    uint32_t queueFamiliesCount;
    LittleGFXInstance* gfxInstance;
    VkPhysicalDeviceProperties2 vkPhysDeviceProps;

protected:
    void queryProperties();
    void selectExtensionsAndLayers();
    void selectQueueIndices();
};

class LittleGFXInstance
{
    friend class LittleGFXWindow;
    friend class LittleGFXAdapter;
    friend class LittleGFXDevice;

public:
    bool Initialize(bool enableDebugLayer);
    bool Destroy();

    uint32_t GetAdapterCount() const { return adapters.size(); }
    LittleGFXAdapter* GetAdapter(uint32_t idx) { return &adapters[idx]; }

protected:
    VkInstance vkInstance;
    std::vector<LittleGFXAdapter> adapters;
    std::vector<const char*> instanceLayers;
    std::vector<const char*> instanceExtensions;

protected:
    void selectExtensionsAndLayers(bool enableDebugLayer);
    void fetchAllAdapters();
};

class LittleGFXDevice
{
    friend class LittleGFXWindow;

public:
    bool Initialize(LittleGFXAdapter* adapter);
    bool Destroy();

protected:
    LittleGFXAdapter* gfxAdapter;
    VkDevice vkDevice;
    VolkDeviceTable volkTable;
};

class LittleGFXQueue
{
protected:
    VkQueue vkQueue;
};

class LittleGFXWindow : public LittleWindow
{
    friend class LittleGFXInstance;

public:
    bool Initialize(const wchar_t* title, LittleGFXDevice* device, bool enableVsync);
    bool Destroy();

protected:
    VkSurfaceKHR vkSurface;
    VkSwapchainKHR vkSwapchain;
    LittleGFXDevice* gfxDevice;

protected:
    void createSurface(LittleGFXInstance* inst);
    void createSwapchainKHR(LittleGFXDevice* device, bool enableVsync);
};

static const char* validation_layer_name = "VK_LAYER_KHRONOS_validation";
static const char* wanted_instance_exts[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif defined(_MACOS)
    VK_MVK_MACOS_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
    VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
#endif
    //这个扩展允许我们使用DeviceProperties2来查询更多信息
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
};
static const char* wanted_device_exts[] = {
    "VK_KHR_portability_subset", //如果使用MoltenVK这种移植性兼容层，打开此扩展
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};