#include "gfx/gfx_objects.h"
#include <vector>
#include <string_view>
#include <iostream>

void LittleGFXAdapter::queryProperties()
{
    vkPhysDeviceProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    vkGetPhysicalDeviceProperties2(vkPhysicalDevice, &vkPhysDeviceProps);
    std::cout << vkPhysDeviceProps.properties.deviceName << std::endl;
}

void LittleGFXAdapter::selectExtensionsAndLayers()
{
    uint32_t ext_count = 0;
    vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, NULL, &ext_count, NULL);
    std::vector<VkExtensionProperties> allExtentions(ext_count);
    vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, NULL, &ext_count, allExtentions.data());
    for (auto ext : wanted_device_exts)
    {
        for (auto usable_ext : allExtentions)
        {
            if (std::string_view(ext) == std::string_view(usable_ext.extensionName))
            {
                deviceExtensions.emplace_back(ext);
            }
        }
    }
}

void LittleGFXAdapter::selectQueueIndices()
{
    vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamiliesCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueProps(queueFamiliesCount);
    vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &queueFamiliesCount, queueProps.data());
    uint32_t queueIdx = 0;
    for (auto&& queueProp : queueProps)
    {
        // select graphics index
        if (queueProp.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            gfxQueueIndex = queueIdx;
        }
        queueIdx++;
    }
}

bool LittleGFXInstance::Initialize(bool enableDebugLayer)
{
    // volk需要初始化，这个初始化过程其实就是在LoadLibrary("vulkan-1.dll")
    static VkResult volkInit = volkInitialize();
    if (volkInit != VK_SUCCESS)
    {
        assert(0 && "Volk Initialize Failed!");
        return false;
    }
    selectExtensionsAndLayers(enableDebugLayer);
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "LittleMaster";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_1;
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    // 填写我们上文筛选出的可以打开的层以及扩展
    createInfo.enabledLayerCount = (uint32_t)instanceLayers.size();
    createInfo.ppEnabledLayerNames = instanceLayers.data();
    createInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();
    // 创建VkInstance
    if (vkCreateInstance(&createInfo, nullptr, &vkInstance) != VK_SUCCESS)
    {
        assert(0 && "Vulkan: failed to create instance!");
    }
    // 使用volk的动态加载方法，直接加载Instance中的Vulkan函数地址
    volkLoadInstance(vkInstance);
    // 直接获取所有的Adapter/PhysicalDevice供以后使用
    fetchAllAdapters();
    return true;
}

bool LittleGFXInstance::Destroy()
{
    vkDestroyInstance(vkInstance, VK_NULL_HANDLE);
    return true;
}

void LittleGFXInstance::selectExtensionsAndLayers(bool enableDebugLayer)
{
    // 查询Extension支持并打开它们
    {
        // 这是C API中很常用的一种两段式query法
        uint32_t ext_count = 0;
        // 首先传入NULL Data和一个计数指针，API会返回一个数量
        vkEnumerateInstanceExtensionProperties(NULL, &ext_count, NULL);
        // 随后应用程序可以根据返回的数量来开辟合适的空间
        std::vector<VkExtensionProperties> allExtentions(ext_count);
        // 最后再把空间传回API，获得对应的返回数据
        vkEnumerateInstanceExtensionProperties(NULL, &ext_count, allExtentions.data());
        for (auto ext : wanted_instance_exts)
        {
            for (auto usable_ext : allExtentions)
            {
                if (std::string_view(ext) == std::string_view(usable_ext.extensionName))
                {
                    instanceExtensions.emplace_back(ext);
                }
            }
        }
    }
    // 查询Validation Layer支持并打开它
    if (enableDebugLayer)
    {
        uint32_t layer_count = 0;
        vkEnumerateInstanceLayerProperties(&layer_count, NULL);
        std::vector<VkLayerProperties> allLayers(layer_count);
        vkEnumerateInstanceLayerProperties(&layer_count, allLayers.data());
        for (auto usable_layer : allLayers)
        {
            if (std::string_view(validation_layer_name) == std::string_view(usable_layer.layerName))
            {
                instanceLayers.emplace_back(validation_layer_name);
            }
        }
    }
}

void LittleGFXInstance::fetchAllAdapters()
{
    uint32_t adapter_count = 0;
    vkEnumeratePhysicalDevices(vkInstance, &adapter_count, nullptr);
    adapters.resize(adapter_count);
    std::vector<VkPhysicalDevice> allVkAdapters(adapter_count);
    vkEnumeratePhysicalDevices(vkInstance, &adapter_count, allVkAdapters.data());
    for (uint32_t i = 0; i < allVkAdapters.size(); i++)
    {
        adapters[i].gfxInstance = this;
        adapters[i].vkPhysicalDevice = allVkAdapters[i];
        adapters[i].queryProperties();
        adapters[i].selectExtensionsAndLayers();
        adapters[i].selectQueueIndices();
    }
}

// 队列优先级。概念上是分配不同Queue执行调度优先级的参数。
// 全部给1.f，忽略此参数。
const float queuePriorities[] = {
    1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, //
    1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, //
    1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, //
    1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, //
};
bool LittleGFXDevice::Initialize(LittleGFXAdapter* adapter)
{
    gfxAdapter = adapter;
    // 要申请的graphics queue
    VkDeviceQueueCreateInfo queueInfo = {};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueCount = 1;
    queueInfo.queueFamilyIndex = adapter->gfxQueueIndex;
    queueInfo.pQueuePriorities = queuePriorities;
    VkPhysicalDeviceFeatures deviceFeatures{};
    VkDeviceCreateInfo deviceInfo = {};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.pEnabledFeatures = &deviceFeatures;
    // 打开需要的扩展和层
    deviceInfo.enabledExtensionCount = adapter->deviceExtensions.size();
    deviceInfo.ppEnabledExtensionNames = adapter->deviceExtensions.data();
    deviceInfo.enabledLayerCount = adapter->deviceLayers.size();
    deviceInfo.ppEnabledLayerNames = adapter->deviceLayers.data();
    if (vkCreateDevice(adapter->vkPhysicalDevice, &deviceInfo, nullptr, &vkDevice) != VK_SUCCESS)
    {
        assert(0 && "failed to create logical device!");
        return false;
    }
    // 使用volk从device中读出相关的API函数地址
    // 这些API被放进volkTable中，因为转发层数很少所以性能有一定提升
    volkLoadDeviceTable(&volkTable, vkDevice);
    return true;
}

bool LittleGFXDevice::Destroy()
{
    vkDestroyDevice(vkDevice, nullptr);
    return true;
}

bool LittleGFXWindow::Initialize(const wchar_t* title, LittleGFXDevice* device, bool enableVsync)
{
    auto succeed = LittleWindow::Initialize(title);
    gfxDevice = device;
    createSurface(device->gfxAdapter->gfxInstance);
    createSwapchainKHR(device, enableVsync);
    return succeed;
}

bool LittleGFXWindow::Destroy()
{
    auto succeed = LittleWindow::Destroy();
    gfxDevice->volkTable.vkDestroySwapchainKHR(gfxDevice->vkDevice, vkSwapchain, nullptr);
    vkDestroySurfaceKHR(gfxDevice->gfxAdapter->gfxInstance->vkInstance, vkSurface, nullptr);
    return succeed;
}

void LittleGFXWindow::createSurface(LittleGFXInstance* inst)
{
#if defined(_WIN32) || defined(_WIN64)
    VkWin32SurfaceCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.hinstance = GetModuleHandle(NULL);
    create_info.hwnd = hWnd;
    if (vkCreateWin32SurfaceKHR(inst->vkInstance, &create_info, nullptr, &vkSurface) != VK_SUCCESS)
    {
        assert(0 && "Create VKWin32 Surface Failed!");
    }
    return;
#endif
    assert(0 && "Platform not supported!");
}

/*
VkPresentModeKHR preferredModeList[] = {
    VK_PRESENT_MODE_IMMEDIATE_KHR,    // normal
    VK_PRESENT_MODE_MAILBOX_KHR,      // low latency
    VK_PRESENT_MODE_FIFO_RELAXED_KHR, // minimize stuttering
    VK_PRESENT_MODE_FIFO_KHR          // low power consumption
};
*/
#define clamp(x, min, max) (x) < (min) ? (min) : ((x) > (max) ? (max) : (x))
void LittleGFXWindow::createSwapchainKHR(LittleGFXDevice* device, bool enableVsync)
{
    // 获取surface支持的格式信息
    VkSurfaceCapabilitiesKHR caps = { 0 };
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->gfxAdapter->vkPhysicalDevice, vkSurface, &caps);
    // 创建
    uint32_t presentQueueFamilyIndex = device->gfxAdapter->gfxQueueIndex;
    VkExtent2D extent{
        clamp(width, caps.minImageExtent.width, caps.maxImageExtent.width),
        clamp(height, caps.minImageExtent.height, caps.maxImageExtent.height)
    };
    VkSwapchainCreateInfoKHR swapchainInfo = {};
    swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.pNext = NULL;
    swapchainInfo.flags = 0;
    swapchainInfo.surface = vkSurface;
    swapchainInfo.minImageCount = enableVsync ? 3 : 2;
    swapchainInfo.presentMode = enableVsync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_IMMEDIATE_KHR;
    // 因为OGL标准，此format和色彩空间一定是被现在的显卡支持的
    swapchainInfo.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    swapchainInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchainInfo.imageExtent = extent;
    swapchainInfo.imageArrayLayers = 1;
    swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    // 反转缓冲区呈现交换链是一种GPU行为，同样是被Queue执行的。这里指定可以执行Present操作的Queue。
    swapchainInfo.queueFamilyIndexCount = 1;
    swapchainInfo.pQueueFamilyIndices = &presentQueueFamilyIndex;
    swapchainInfo.clipped = VK_TRUE;
    // 在这里指定一个老的交换链可以加速创建
    swapchainInfo.oldSwapchain = VK_NULL_HANDLE;
    // 可以在呈现时指定某种变换，比如把图片逆时针旋转90度
    swapchainInfo.preTransform = caps.currentTransform;
    // 是否使用Alpha通道和其它的窗口混合，这里可以实现很多奇特的效果，但是我们不需要。所以设定为OPAQUE（不透明）模式
    swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    VkResult res = device->volkTable.vkCreateSwapchainKHR(
        device->vkDevice, &swapchainInfo, nullptr, &vkSwapchain);
    if (VK_SUCCESS != res)
    {
        assert(0 && "fatal: vkCreateSwapchainKHR failed!");
    }
}